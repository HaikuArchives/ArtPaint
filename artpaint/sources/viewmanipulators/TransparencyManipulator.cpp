/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "AddOns.h"
#include "Controls.h"
#include "Image.h"
#include "Layer.h"
#include "MessageConstants.h"
#include "PixelOperations.h"
#include "TransparencyManipulator.h"
#include "StringServer.h"


TransparencyManipulator::TransparencyManipulator(BBitmap *bm)
	: WindowGUIManipulator()
	, ImageAdapter()
	, preview_bitmap(NULL)
	, copy_of_the_preview_bitmap(NULL)
	, preview_layer(NULL)
	, transparency(0)
	, last_calculated_resolution(0)
	, lowest_available_quality(0)
	, highest_available_quality(0)
	, previous_transparency_change(0.0)
	, original_transparency_coefficient(0.0)
	, settings(NULL)
	, config_view(NULL)
{
	if (bm) {
		preview_bitmap = bm;
		copy_of_the_preview_bitmap = DuplicateBitmap(preview_bitmap);
	}
	settings = new TransparencyManipulatorSettings();
}


TransparencyManipulator::~TransparencyManipulator()
{
//	delete copy_of_the_preview_bitmap;
	delete settings;
	if (config_view) {
		config_view->RemoveSelf();
		delete config_view;
	}
}


BBitmap*
TransparencyManipulator::ManipulateBitmap(ManipulatorSettings* set,
	BBitmap* original, Selection* selection, BStatusBar* status_bar)
{
	return NULL;
}


int32
TransparencyManipulator::PreviewBitmap(Selection* selection, bool full_quality,
	BRegion* updated_region)
{
	// First decide the resolution of the bitmap
	if ((previous_transparency_change == settings->transparency)
		&& (full_quality == FALSE)) {
		if (last_calculated_resolution <= highest_available_quality) {
			last_calculated_resolution = 0;
			if (full_quality) {
				updated_region->Set(preview_bitmap->Bounds());
				return 1;
			} else {
				return 0;
			}
		} else {
			last_calculated_resolution = last_calculated_resolution / 2;
		}
	} else if (full_quality == TRUE) {
		last_calculated_resolution = 1;
	} else {
		last_calculated_resolution = lowest_available_quality;
	}

	if (image == NULL)
		return 0;

	updated_region->Include(image->ReturnActiveBitmap()->Bounds());

	if (preview_layer)
		preview_layer->SetTransparency(settings->transparency / 255.0);

	return 1;
}


void
TransparencyManipulator::SetPreviewBitmap(BBitmap *bm)
{
	if (image) {
		original_transparency_coefficient =
			image->ReturnActiveLayer()->GetTransparency();
		settings->transparency = original_transparency_coefficient * 255;

		if (config_view)
			config_view->ChangeSettings(settings);

		preview_layer = image->ReturnActiveLayer();
	}
}


void
TransparencyManipulator::SetTransparency(float change)
{
	previous_transparency_change = settings->transparency;
	settings->transparency = change;
}


void
TransparencyManipulator::Reset(Selection*)
{
	if (image && image->ContainsLayer(preview_layer)) {
		preview_layer->SetTransparency(original_transparency_coefficient);
	}
}


BView*
TransparencyManipulator::MakeConfigurationView(BMessenger *target)
{
	config_view = new TransparencyManipulatorView(BRect(0,0,0,0), this, target);
	config_view->ChangeSettings(settings);
	return config_view;
}


ManipulatorSettings*
TransparencyManipulator::ReturnSettings()
{
	return new TransparencyManipulatorSettings(settings);
}


const char*
TransparencyManipulator::ReturnHelpString()
{
	return StringServer::ReturnString(DO_CHANGE_TRANSPARENCY_HELP_STRING);

}


const char*
TransparencyManipulator::ReturnName()
{
	return StringServer::ReturnString(CHANGE_TRANSPARENCY_STRING);
}


// #prgma mark -- TransparencyManipulatorView


TransparencyManipulatorView::TransparencyManipulatorView(BRect rect,
		TransparencyManipulator *manip, BMessenger *t)
	: WindowGUIManipulatorView(rect)
	, target(new BMessenger(*t))
	, manipulator(manip)
	, transparency_control(NULL)
	, started_manipulating(false)
{
	transparency_control = new ControlSlider(BRect(0, 0, 200, 40),
		"transparency_control", StringServer::ReturnString(TRANSPARENCY_STRING),
		new BMessage(MOUSE_TRACKING_FINISHED), 0, 255, B_TRIANGLE_THUMB);
	transparency_control->SetValue(0);
	transparency_control->SetLimitLabels(StringServer::ReturnString(TRANSPARENT_STRING),
		StringServer::ReturnString(OPAQUE_STRING));
	transparency_control->ResizeToPreferred();
	transparency_control->MoveTo(10,10);
	transparency_control->SetModificationMessage(new BMessage(TRANSPARENCY_CHANGED));

	ResizeTo(transparency_control->Frame().Width() + 20,
		transparency_control->Frame().Height() + 20);
	AddChild(transparency_control);
}


TransparencyManipulatorView::~TransparencyManipulatorView()
{
	delete target;
}


void
TransparencyManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();
	transparency_control->SetTarget(BMessenger(this));
}


void
TransparencyManipulatorView::AllAttached()
{
	transparency_control->SetValue(int32(settings.transparency));
}


void
TransparencyManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case TRANSPARENCY_CHANGED: {
			manipulator->SetTransparency(transparency_control->Value());
			if (started_manipulating == FALSE) {
				started_manipulating = TRUE;
				target->SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
			}
		}	break;

		case MOUSE_TRACKING_FINISHED: {
			started_manipulating = FALSE;
			manipulator->SetTransparency(transparency_control->Value());
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		}	break;

		default: {
			WindowGUIManipulatorView::MessageReceived(message);
		}	break;
	}
}


void
TransparencyManipulatorView::ChangeSettings(TransparencyManipulatorSettings *new_set)
{
	settings = *new_set;

	if (Window() && Window()->Lock()) {
		transparency_control->SetValue(int32(new_set->transparency));
		Window()->Unlock();
	}
}
