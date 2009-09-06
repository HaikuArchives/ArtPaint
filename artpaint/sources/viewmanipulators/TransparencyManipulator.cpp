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


#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Window.h>


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
	delete copy_of_the_preview_bitmap;
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
TransparencyManipulator::MakeConfigurationView(const BMessenger& target)
{
	config_view = new (std::nothrow) TransparencyManipulatorView(this, target);
	if (config_view)
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


TransparencyManipulatorView::TransparencyManipulatorView(
		TransparencyManipulator* manipulator, const BMessenger& target)
	: WindowGUIManipulatorView()
	, fTarget(target)
	, fTracking(false)
	, fManipulator(manipulator)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	fTransparency = new ControlSlider("transparency",
		StringServer::ReturnString(TRANSPARENCY_STRING),
		new BMessage(MOUSE_TRACKING_FINISHED), 0, 255, B_TRIANGLE_THUMB);
	fTransparency->SetValue(0);
	fTransparency->SetLimitLabels(StringServer::ReturnString(TRANSPARENT_STRING),
		StringServer::ReturnString(OPAQUE_STRING));
	fTransparency->SetModificationMessage(new BMessage(TRANSPARENCY_CHANGED));

	AddChild(fTransparency);
}


void
TransparencyManipulatorView::AttachedToWindow()
{
	fTransparency->SetTarget(BMessenger(this));
	WindowGUIManipulatorView::AttachedToWindow();
}


void
TransparencyManipulatorView::AllAttached()
{
	fTransparency->SetValue(int32(settings.transparency));
}


void
TransparencyManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case TRANSPARENCY_CHANGED: {
			fManipulator->SetTransparency(fTransparency->Value());
			if (!fTracking) {
				fTracking = true;
				fTarget.SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
			}
		}	break;

		case MOUSE_TRACKING_FINISHED: {
			fTracking = false;
			fManipulator->SetTransparency(fTransparency->Value());
			fTarget.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
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
		fTransparency->SetValue(int32(new_set->transparency));
		Window()->Unlock();
	}
}
