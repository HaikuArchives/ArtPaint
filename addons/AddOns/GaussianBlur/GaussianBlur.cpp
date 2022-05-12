/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <StatusBar.h>
#include <stdio.h>
#include <StopWatch.h>
#include <Slider.h>
#include <Window.h>
#include <string.h>

#include "AddOns.h"
#include "ManipulatorInformer.h"
#include "GaussianBlur.h"
#include "ImageProcessingLibrary.h"
#include "Selection.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddOns_GaussianBlur"


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = B_TRANSLATE_MARK("Gaussian blur" B_UTF8_ELLIPSIS);
	char menu_help_string[255] = B_TRANSLATE_MARK("Adds a gaussian blur to the image.");
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = COLOR_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new GaussianBlurManipulator(bm);
}



GaussianBlurManipulator::GaussianBlurManipulator(BBitmap *bm)
		: WindowGUIManipulator()
{
	preview_bitmap = NULL;
	config_view = NULL;
	copy_of_the_preview_bitmap = NULL;
	selection_bitmap = NULL;

	previous_settings.blur = settings.blur + 1;

	processor_count = GetSystemCpuCount();

	ipLibrary = new ImageProcessingLibrary();

	SetPreviewBitmap(bm);
}


GaussianBlurManipulator::~GaussianBlurManipulator()
{
	delete copy_of_the_preview_bitmap;
	delete config_view;

	delete selection_bitmap;

	delete ipLibrary;
}


BBitmap* GaussianBlurManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	GaussianBlurManipulatorSettings *new_settings = dynamic_cast<GaussianBlurManipulatorSettings*>(set);

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	if (original == preview_bitmap) {
		if ((*new_settings == previous_settings) && (last_calculated_resolution <= 1))
			return original;

		source_bitmap = copy_of_the_preview_bitmap;
		target_bitmap = original;
	}
	else {
		source_bitmap = original;
		target_bitmap = new BBitmap(original->Bounds(),B_RGB32,FALSE);
	}


	current_resolution = 1;
	current_selection = selection;
	current_settings = *new_settings;
	progress_bar = status_bar;

	return target_bitmap;
}

int32 GaussianBlurManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
{
	progress_bar = NULL;
	current_selection = selection;
	if (settings == previous_settings ) {
		if ((last_calculated_resolution != highest_available_quality) && (last_calculated_resolution > 0))
			last_calculated_resolution = max_c(highest_available_quality,floor(last_calculated_resolution/2.0));
		else
			last_calculated_resolution = 0;
	}
	else
		last_calculated_resolution = lowest_available_quality;

	if (full_quality) {
		last_calculated_resolution = min_c(1,last_calculated_resolution);
	}
	previous_settings = settings;

	if (last_calculated_resolution > 0) {
		if (selection->IsEmpty()) {
			updated_region->Set(preview_bitmap->Bounds());
			Reset(selection);
			ipLibrary->gaussian_blur(preview_bitmap,settings.blur,processor_count);
		}
		else {
			BRect selection_bounds = selection->GetBoundingRect();

			if (selection_bitmap != NULL) {
				if ((selection_bitmap->Bounds().Width() != selection_bounds.Width()) ||
					(selection_bitmap->Bounds().Height() != selection_bounds.Height())) {
					delete selection_bitmap;
					selection_bitmap = NULL;
				}
			}

			if (selection_bitmap == NULL) {
				selection_bitmap = new BBitmap(selection_bounds,B_RGBA32);
			}

			uint32 *s_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
			uint32 s_bpr = copy_of_the_preview_bitmap->BytesPerRow()/4;

			uint32 *d_bits = (uint32*)selection_bitmap->Bits();

			for (int32 y=selection_bounds.top;y<=selection_bounds.bottom;y++) {
				for (int32 x=selection_bounds.left;x<=selection_bounds.right;x++) {
					*d_bits++ = *(s_bits + y*s_bpr + x);
				}
			}
			ipLibrary->gaussian_blur(selection_bitmap,settings.blur);

			s_bits = (uint32*)preview_bitmap->Bits();
			s_bpr = preview_bitmap->BytesPerRow()/4;

			d_bits = (uint32*)selection_bitmap->Bits();

			for (int32 y=selection_bounds.top;y<=selection_bounds.bottom;y++) {
				for (int32 x=selection_bounds.left;x<=selection_bounds.right;x++) {
					if (selection->ContainsPoint(x,y))
						*(s_bits + y*s_bpr + x) = *d_bits++;
					else
						d_bits++;
				}
			}

			updated_region->Set(selection->GetBoundingRect());
		}
	}

	return 1;
}

void GaussianBlurManipulator::MouseDown(BPoint point,uint32,BView*,bool first_click)
{
	// This function does nothing in GaussianBlurManipulator.
}


void GaussianBlurManipulator::SetPreviewBitmap(BBitmap *bm)
{
	if (preview_bitmap != bm) {
		delete copy_of_the_preview_bitmap;
		if (bm != NULL) {
			preview_bitmap = bm;
			copy_of_the_preview_bitmap = DuplicateBitmap(bm,0);
		}
		else {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
		}
	}

	if (preview_bitmap != NULL) {
		// Let's select a resolution that can handle all the pixels at least
		// 10 times in a second while assuming that one pixel calculation takes
		// about 50 CPU cycles.
		double speed = GetSystemClockSpeed() / (10*50);
		BRect bounds = preview_bitmap->Bounds();
		float num_pixels = (bounds.Width()+1) * (bounds.Height() + 1);
		lowest_available_quality = 1;

		while ((num_pixels/lowest_available_quality/lowest_available_quality) > speed)
			lowest_available_quality *= 2;

		lowest_available_quality = min_c(lowest_available_quality,16);
		highest_available_quality = max_c(lowest_available_quality/2,1);
	} else {
		lowest_available_quality = 1;
		highest_available_quality = 1;
	}
	last_calculated_resolution = lowest_available_quality;
}


void GaussianBlurManipulator::Reset(Selection*)
{
	printf("Reset\n");
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with a loop.
		uint32 *source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target,source,bits_length);
	}
}

BView* GaussianBlurManipulator::MakeConfigurationView(const BMessenger& target)
{
	if (config_view == NULL) {
		config_view = new GaussianBlurManipulatorView(this,target);
		config_view->ChangeSettings(&settings);
	}

	return config_view;
}


ManipulatorSettings* GaussianBlurManipulator::ReturnSettings()
{
	return new GaussianBlurManipulatorSettings(settings);
}

void GaussianBlurManipulator::ChangeSettings(ManipulatorSettings *s)
{
	GaussianBlurManipulatorSettings *new_settings;
	new_settings = dynamic_cast<GaussianBlurManipulatorSettings*>(s);
	if (new_settings != NULL) {
		settings = *new_settings;
	}
}

const char* GaussianBlurManipulator::ReturnName()
{
	return B_TRANSLATE("Gaussian blur");
}

const char* GaussianBlurManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Adds a gaussian blur.");
}




// -------------------------------------
GaussianBlurManipulatorView::GaussianBlurManipulatorView(GaussianBlurManipulator *manip,
		const BMessenger& t)
	: WindowGUIManipulatorView()
{
	target = t;
	manipulator = manip;
	started_adjusting = FALSE;

	blur_slider = new BSlider("blur_slider", B_TRANSLATE("Blur size:"),
		new BMessage(BLUR_ADJUSTING_FINISHED), 0, 2000, B_HORIZONTAL,
		B_TRIANGLE_THUMB);
	blur_slider->SetLimitLabels(B_TRANSLATE("Small"), B_TRANSLATE("Large"));
	blur_slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	blur_slider->SetHashMarkCount(11);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(blur_slider)
		.SetInsets(B_USE_SMALL_INSETS)
		.End();
}

GaussianBlurManipulatorView::~GaussianBlurManipulatorView()
{
}

void GaussianBlurManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();
	blur_slider->SetTarget(BMessenger(this));
}

void GaussianBlurManipulatorView::AllAttached()
{
	blur_slider->SetValue(settings.blur * 10.0-1);
}


void GaussianBlurManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case BLUR_ADJUSTING_FINISHED:
			started_adjusting = FALSE;
			settings.blur = (blur_slider->Value()+1) / 40.0;
			manipulator->ChangeSettings(&settings);
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}


void GaussianBlurManipulatorView::ChangeSettings(ManipulatorSettings *set)
{
	GaussianBlurManipulatorSettings *new_settings = dynamic_cast<GaussianBlurManipulatorSettings*>(set);

	if (set != NULL) {
		settings = *new_settings;

		BWindow *window = Window();
		if (window != NULL) {
			window->Lock();
			blur_slider->SetValue(settings.blur * 40.0-1);
			window->Unlock();
		}
	}
}
