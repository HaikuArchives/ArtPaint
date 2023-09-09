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
#include <StatusBar.h>
#include <StopWatch.h>
#include <Window.h>

#include "AddOns.h"
#include "ManipulatorInformer.h"
#include "Marble.h"
#include "PerlinNoiseGenerator.h"
#include "PixelOperations.h"
#include "Selection.h"
#include "SplineGenerator.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddOns_Marble"


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = B_TRANSLATE_MARK("Marble");
	char menu_help_string[255] = B_TRANSLATE_MARK("Puts a marble-like texture over the image.");
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = EFFECT_FILTER_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator*
instantiate_add_on(BBitmap*, ManipulatorInformer* i)
{
	return new MarbleManipulator(i);
}


MarbleManipulator::MarbleManipulator(ManipulatorInformer* i)
	:
	Manipulator(),
	selection(NULL)
{
	informer = i;
	processor_count = GetSystemCpuCount();
}


MarbleManipulator::~MarbleManipulator()
{
	delete informer;
}


BBitmap*
MarbleManipulator::ManipulateBitmap(BBitmap* original, BStatusBar* status_bar)
{
	BStopWatch watch("PerlinMarble");

	source_bitmap = original;
	target_bitmap = original;
	progress_bar = status_bar;

	thread_id* threads = new thread_id[processor_count];

	spare_copy_bitmap = DuplicateBitmap(original, -1);

	for (int32 i = 0; i < processor_count; i++) {
		threads[i] = spawn_thread(thread_entry, "perlin_fade_thread", B_NORMAL_PRIORITY, this);
		resume_thread(threads[i]);
		send_data(threads[i], i, NULL, 0);
	}

	for (int32 i = 0; i < processor_count; i++) {
		int32 return_value;
		wait_for_thread(threads[i], &return_value);
	}

	delete[] threads;

	delete spare_copy_bitmap;

	// Return the original.
	return original;
}


const char*
MarbleManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Puts a marble-like texture over the image.");
}


const char*
MarbleManipulator::ReturnName()
{
	return B_TRANSLATE("Marble");
}


int32
MarbleManipulator::thread_entry(void* data)
{
	int32 thread_number;
	thread_number = receive_data(NULL, NULL, 0);

	MarbleManipulator* this_pointer = (MarbleManipulator*)data;

	return this_pointer->thread_function(thread_number);
}


int32
MarbleManipulator::thread_function(int32 thread_number)
{
	BWindow* progress_bar_window = NULL;
	if (progress_bar != NULL)
		progress_bar_window = progress_bar->Window();

	uint32* source = (uint32*)source_bitmap->Bits();
	int32 source_bpr = source_bitmap->BytesPerRow() / 4;

	PerlinNoiseGenerator generator(0.5, 7);

	// This union must be used to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	if (selection->IsEmpty()) {
		// Here handle the whole image.
		float left = target_bitmap->Bounds().left;
		float right = target_bitmap->Bounds().right;
		float top = target_bitmap->Bounds().top;
		float bottom = target_bitmap->Bounds().bottom;

		int32 height = (bottom - top + 1) / processor_count;
		top = min_c(bottom, top + thread_number * height);
		bottom = min_c(bottom, top + height - 1);

		int32 update_interval = 10;
		float update_amount = 100.0 / (bottom - top) * update_interval / (float)processor_count;
		float missed_update = 0;

		// Loop through all pixels in original.
		float one_per_width = 1.0 / 128;
		float one_per_height = 1.0 / 128;
		float one_per_depth = 1.0 / 1024;
		source += (int32)top * source_bpr;

		for (float y = top; y <= bottom; ++y) {
			for (float x = left; x <= right; ++x) {
				color.word = *source;
				float noise = generator.PerlinNoise2D(x * one_per_width, y * one_per_height);

				if (noise > 0) {
					rgb_color c = informer->GetForegroundColor();
					color.bytes[0] = c.blue;
					color.bytes[1] = c.green;
					color.bytes[2] = c.red;
					color.bytes[3] = c.alpha;
					*source = mix_2_pixels_fixed(*source, color.word, 32768 * (1.0 - noise));
				}
				++source;
			}
			// Update the status-bar
			if ((((int32)y % update_interval) == 0) && (progress_bar_window != NULL)
				&& (progress_bar_window->LockWithTimeout(0) == B_OK)) {
				progress_bar->Update(update_amount + missed_update);
				progress_bar_window->Unlock();
				missed_update = 0;
			} else if (((int32)y % update_interval) == 0)
				missed_update += update_amount;
		}
	} else {
		// Here handle only those pixels for which selection->ContainsPoint(x,y) is true.
		BRect rect = selection->GetBoundingRect();

		int32 left = rect.left;
		int32 right = rect.right;
		int32 top = rect.top;
		int32 bottom = rect.bottom;

		int32 height = (bottom - top + 1) / processor_count;
		top = min_c(bottom, top + thread_number * height);
		bottom = min_c(bottom, top + height - 1);

		int32 update_interval = 10;
		float update_amount = 100.0 / (bottom - top) * update_interval / (float)processor_count;
		float missed_update = 0;

		// Loop through all pixels in original.
		float one_per_width = 1.0 / 128;
		float one_per_height = 1.0 / 128;
		float one_per_depth = 1.0 / 1024;

		// Loop through all pixels in original.
		for (int32 y = top; y <= bottom; ++y) {
			for (int32 x = left; x <= right; ++x) {
				if (selection->ContainsPoint(x, y)) {
					color.word = *(source + x + y * source_bpr);
					float noise = generator.PerlinNoise2D(x * one_per_width, y * one_per_height);

					if (noise > 0) {
						rgb_color c = informer->GetForegroundColor();
						color.bytes[0] = c.blue;
						color.bytes[1] = c.green;
						color.bytes[2] = c.red;
						color.bytes[3] = c.alpha;
						*(source + x + y * source_bpr) = mix_2_pixels_fixed(
							*(source + x + y * source_bpr), color.word, 32768 * (1.0 - noise));
					}
				}
			}

			// Update the status-bar
			if ((((int32)y % update_interval) == 0) && (progress_bar_window != NULL)
				&& (progress_bar_window->LockWithTimeout(0) == B_OK)) {
				progress_bar->Update(update_amount + missed_update);
				progress_bar_window->Unlock();
				missed_update = 0;
			} else if (((int32)y % update_interval) == 0)
				missed_update += update_amount;
		}
	}

	return B_OK;
}


float
MarbleManipulator::marble_amount(float x)
{
	x = fabs(x);
	// Select the four value and proper u
	float v1 = 0;
	float v2 = 127;
	float v3 = -128;
	float v4 = 0;

	float u;

	if (x < 0.33) {
		v2 = v1;
		v3 = v2;
		v4 = v3;
		u = (0.33 - x) / 0.33;
	} else if (x < 0.66)
		u = (0.66 - x) / 0.66;
	else {
		v1 = v2;
		v2 = v3;
		v3 = v4;
		u = (1 - x) / 0.33;
	}

	return SplineGenerator::CardinalSpline(v1, v2, v3, v4, u);
}
