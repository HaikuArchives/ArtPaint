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
#include <Node.h>
#include <Slider.h>
#include <StatusBar.h>
#include <Window.h>
#include <string.h>

#include "AddOns.h"
#include "Brightness.h"
#include "ManipulatorInformer.h"
#include "Selection.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddOns_Brightness"


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = B_TRANSLATE_MARK("Brightness" B_UTF8_ELLIPSIS);
	char menu_help_string[255] = B_TRANSLATE_MARK("Adjusts the brightness.");
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = COLOR_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator*
instantiate_add_on(BBitmap* bm, ManipulatorInformer* i)
{
	delete i;
	return new BrightnessManipulator(bm);
}


BrightnessManipulator::BrightnessManipulator(BBitmap* bm)
	:
	WindowGUIManipulator(),
	selection(NULL)
{
	preview_bitmap = NULL;
	config_view = NULL;
	copy_of_the_preview_bitmap = NULL;

	previous_settings.brightness = settings.brightness + 1;

	SetPreviewBitmap(bm);
}


BrightnessManipulator::~BrightnessManipulator()
{
	delete copy_of_the_preview_bitmap;
	delete config_view;
}


BBitmap*
BrightnessManipulator::ManipulateBitmap(
	ManipulatorSettings* set, BBitmap* original, BStatusBar* status_bar)
{
	BrightnessManipulatorSettings* new_settings = dynamic_cast<BrightnessManipulatorSettings*>(set);

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	if (original == preview_bitmap) {
		if ((*new_settings == previous_settings) && (last_calculated_resolution <= 1))
			return original;

		source_bitmap = copy_of_the_preview_bitmap;
		target_bitmap = original;
	} else {
		source_bitmap = original;
		target_bitmap = new BBitmap(original->Bounds(), B_RGB32, FALSE);
	}

	current_resolution = 1;
	current_settings = *new_settings;
	progress_bar = status_bar;

	start_threads();

	return target_bitmap;
}


int32
BrightnessManipulator::PreviewBitmap(bool full_quality, BRegion* updated_region)
{
	progress_bar = NULL;
	if (settings == previous_settings) {
		if ((last_calculated_resolution != highest_available_quality)
			&& (last_calculated_resolution > 0)) {
			last_calculated_resolution
				= max_c(highest_available_quality, floor(last_calculated_resolution / 2.0));
		} else
			last_calculated_resolution = 0;
	} else
		last_calculated_resolution = lowest_available_quality;

	if (full_quality)
		last_calculated_resolution = min_c(1, last_calculated_resolution);

	previous_settings = settings;

	if (last_calculated_resolution > 0) {
		current_resolution = last_calculated_resolution;
		updated_region->Set(preview_bitmap->Bounds());

		target_bitmap = preview_bitmap;
		source_bitmap = copy_of_the_preview_bitmap;
		current_settings = settings;

		start_threads();
	}

	return last_calculated_resolution;
}


void
BrightnessManipulator::start_threads()
{
	number_of_threads = GetSystemCpuCount();

	thread_id* threads = new thread_id[number_of_threads];

	for (int32 i = 0; i < number_of_threads; i++) {
		threads[i] = spawn_thread(thread_entry, "brightness_thread", B_NORMAL_PRIORITY, this);
		resume_thread(threads[i]);
		send_data(threads[i], i, NULL, 0);
	}

	for (int32 i = 0; i < number_of_threads; i++) {
		int32 return_value;
		wait_for_thread(threads[i], &return_value);
	}

	delete[] threads;
}


int32
BrightnessManipulator::thread_entry(void* data)
{
	int32 thread_number;
	thread_number = receive_data(NULL, NULL, 0);

	BrightnessManipulator* this_pointer = (BrightnessManipulator*)data;

	return this_pointer->thread_function(thread_number);
}


int32
BrightnessManipulator::thread_function(int32 thread_number)
{
	// This function interpolates the image with a degenerate version,
	// which in this case is black. The black image is not actually
	// used, but the idea a*X + (1-a)*0 = a*X is used instead. This function
	// does not touch the alpha-channel.

	int32 step = current_resolution;
	uint32 brightness = settings.brightness;

	BWindow* progress_bar_window = NULL;
	if (progress_bar != NULL)
		progress_bar_window = progress_bar->Window();

	uint32* source_bits = (uint32*)source_bitmap->Bits();
	uint32* target_bits = (uint32*)target_bitmap->Bits();
	int32 source_bpr = source_bitmap->BytesPerRow() / 4;
	int32 target_bpr = target_bitmap->BytesPerRow() / 4;

	// This union must be used to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	float coeff = current_settings.brightness / 100.0;

	if (selection->IsEmpty()) {
		// Here handle the whole image.
		int32 left = target_bitmap->Bounds().left;
		int32 right = target_bitmap->Bounds().right;
		int32 top = target_bitmap->Bounds().top;
		int32 bottom = target_bitmap->Bounds().bottom;

		float height = bottom - top;
		top = height / number_of_threads * thread_number;
		top = ceil(top / (float)step);
		top *= step;
		bottom = min_c(bottom, top + (height + 1) / number_of_threads);
		int32 update_interval = 10;
		float update_amount = 100.0 / (bottom - top) * update_interval / (float)number_of_threads;
		float missed_update = 0;

		// Loop through all pixels in original.
		uint32 sum;
		brightness *= 3;
		uint8 brighness_array[256];
		for (int32 i = 0; i < 256; i++)
			brighness_array[i] = min_c(255, i * coeff);

		for (int32 y = top; y <= bottom; y += step) {
			int32 y_times_source_bpr = y * source_bpr;
			int32 y_times_target_bpr = y * target_bpr;
			for (int32 x = left; x <= right; x += step) {
				color.word = *(source_bits + x + y_times_source_bpr);
				color.bytes[0] = brighness_array[color.bytes[0]];
				color.bytes[1] = brighness_array[color.bytes[1]];
				color.bytes[2] = brighness_array[color.bytes[2]];
				*(target_bits + x + y_times_target_bpr) = color.word;
			}

			// Update the status-bar
			if (((y % update_interval) == 0) && (progress_bar_window != NULL)
				&& (progress_bar_window->LockWithTimeout(0) == B_OK)) {
				progress_bar->Update(update_amount + missed_update);
				progress_bar_window->Unlock();
				missed_update = 0;
			} else if ((y % update_interval) == 0)
				missed_update += update_amount;
		}
	} else {
		// Here handle only those pixels for which selection->ContainsPoint(x,y) is true.
		BRect rect = selection->GetBoundingRect();

		int32 left = rect.left;
		int32 right = rect.right;
		int32 top = rect.top;
		int32 bottom = rect.bottom;

		float height = bottom - top;
		top += height / number_of_threads * thread_number;
		top *= step;
		top /= step;

		bottom = min_c(bottom, top + (height + 1) / number_of_threads);

		int32 update_interval = 10;
		float update_amount = 100.0 / (bottom - top) * update_interval / (float)number_of_threads;
		uint8 brighness_array[256];
		for (int32 i = 0; i < 256; i++)
			brighness_array[i] = min_c(255, i * coeff);

		// Loop through all pixels in original.
		for (int32 y = top; y <= bottom; y += step) {
			int32 y_times_source_bpr = y * source_bpr;
			int32 y_times_target_bpr = y * target_bpr;
			for (int32 x = left; x <= right; x += step) {
				if (selection->ContainsPoint(x, y)) {
					color.word = *(source_bits + x + y_times_source_bpr);
					color.bytes[0] = brighness_array[color.bytes[0]];
					color.bytes[1] = brighness_array[color.bytes[1]];
					color.bytes[2] = brighness_array[color.bytes[2]];
					*(target_bits + x + y_times_target_bpr) = color.word;
				}
			}

			// Update the status-bar
			if (((y % update_interval) == 0) && (progress_bar_window != NULL)
				&& (progress_bar_window->LockWithTimeout(0) == B_OK)) {
				progress_bar->Update(update_amount);
				progress_bar_window->Unlock();
			}
		}
	}

	return B_OK;
}


void
BrightnessManipulator::MouseDown(BPoint point, uint32, BView*, bool first_click)
{
	// This function does nothing in BrightnessManipulator.
}


void
BrightnessManipulator::SetPreviewBitmap(BBitmap* bm)
{
	if (preview_bitmap != bm) {
		delete copy_of_the_preview_bitmap;
		if (bm != NULL) {
			preview_bitmap = bm;
			copy_of_the_preview_bitmap = DuplicateBitmap(bm, 0);
		} else {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
		}
	}

	if (preview_bitmap != NULL) {
		// Let's select a resolution that can handle all the pixels at least
		// 10 times in a second while assuming that one pixel calculation takes
		// about 50 CPU cycles.
		double speed = GetSystemClockSpeed() / (10 * 50);
		BRect bounds = preview_bitmap->Bounds();
		float num_pixels = (bounds.Width() + 1) * (bounds.Height() + 1);
		lowest_available_quality = 1;
		while ((num_pixels / lowest_available_quality / lowest_available_quality) > speed)
			lowest_available_quality *= 2;

		lowest_available_quality = min_c(lowest_available_quality, 16);
		highest_available_quality = max_c(lowest_available_quality / 2, 1);
	} else {
		lowest_available_quality = 1;
		highest_available_quality = 1;
	}
	last_calculated_resolution = lowest_available_quality;
}


void
BrightnessManipulator::Reset()
{
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with a loop.
		uint32* source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32* target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target, source, bits_length);
	}
}


BView*
BrightnessManipulator::MakeConfigurationView(const BMessenger& target)
{
	if (config_view == NULL) {
		config_view = new BrightnessManipulatorView(this, target);
		config_view->ChangeSettings(&settings);
	}

	return config_view;
}


ManipulatorSettings*
BrightnessManipulator::ReturnSettings()
{
	return new BrightnessManipulatorSettings(settings);
}


void
BrightnessManipulator::ChangeSettings(ManipulatorSettings* s)
{
	BrightnessManipulatorSettings* new_settings;
	new_settings = dynamic_cast<BrightnessManipulatorSettings*>(s);
	if (new_settings != NULL)
		settings = *new_settings;
}


const char*
BrightnessManipulator::ReturnName()
{
	return B_TRANSLATE("Brightness");
}


const char*
BrightnessManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Adjusts the brightness.");
}


BrightnessManipulatorView::BrightnessManipulatorView(
	BrightnessManipulator* manip, const BMessenger& t)
	:
	WindowGUIManipulatorView()
{
	target = t;
	manipulator = manip;
	started_adjusting = FALSE;

	brightness_slider = new BSlider("brightness_slider", B_TRANSLATE("Brightness:"),
		new BMessage(BRIGHTNESS_ADJUSTING_FINISHED), 0, 255, B_HORIZONTAL, B_TRIANGLE_THUMB);
	brightness_slider->SetModificationMessage(new BMessage(BRIGHTNESS_ADJUSTED));
	brightness_slider->SetLimitLabels(B_TRANSLATE("Low"), B_TRANSLATE("High"));
	brightness_slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	brightness_slider->SetHashMarkCount(11);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(brightness_slider)
		.SetInsets(B_USE_SMALL_INSETS)
		.End();
}


BrightnessManipulatorView::~BrightnessManipulatorView()
{
}


void
BrightnessManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();
	brightness_slider->SetTarget(BMessenger(this));
}


void
BrightnessManipulatorView::AllAttached()
{
	brightness_slider->SetValue(settings.brightness);
}


void
BrightnessManipulatorView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case BRIGHTNESS_ADJUSTED:
		{
			settings.brightness = brightness_slider->Value();
			manipulator->ChangeSettings(&settings);
			if (!started_adjusting) {
				target.SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
				started_adjusting = TRUE;
			}
		} break;
		case BRIGHTNESS_ADJUSTING_FINISHED:
		{
			started_adjusting = FALSE;
			settings.brightness = brightness_slider->Value();
			manipulator->ChangeSettings(&settings);
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		} break;
		default:
			WindowGUIManipulatorView::MessageReceived(message);
	}
}


void
BrightnessManipulatorView::ChangeSettings(ManipulatorSettings* set)
{
	BrightnessManipulatorSettings* new_settings = dynamic_cast<BrightnessManipulatorSettings*>(set);

	if (set != NULL) {
		settings = *new_settings;

		BWindow* window = Window();
		if (window != NULL) {
			window->Lock();
			brightness_slider->SetValue(settings.brightness);
			window->Unlock();
		}
	}
}
