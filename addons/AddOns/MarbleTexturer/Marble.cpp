/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <StatusBar.h>
#include <StopWatch.h>
#include <Window.h>

#include "AddOns.h"
#include "Marble.h"
#include "PixelOperations.h"
#include "PerlinNoiseGenerator.h"
#include "SplineGenerator.h"

extern "C" __declspec(dllexport) char name[255] = "Marble";
extern "C" __declspec(dllexport) char menu_help_string[255] = "Puts a marble-like texture over the image.";
extern "C" __declspec(dllexport) int32 add_on_api_version = ADD_ON_API_VERSION;
extern "C" __declspec(dllexport) add_on_types add_on_type = EFFECT_FILTER_ADD_ON;


Manipulator* instantiate_add_on(BBitmap*,ManipulatorInformer *i)
{
	return new MarbleManipulator(i);
}



MarbleManipulator::MarbleManipulator(ManipulatorInformer *i)
		: Manipulator()
{
	informer = i;
}


MarbleManipulator::~MarbleManipulator()
{
	delete informer;
}


BBitmap* MarbleManipulator::ManipulateBitmap(BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	BStopWatch watch("PerlinMarble");

	source_bitmap = original;
	target_bitmap = original;
	the_selection = selection;
	progress_bar = status_bar;

	system_info info;
	get_system_info(&info);

	thread_id *threads = new thread_id[info.cpu_count];

	spare_copy_bitmap = DuplicateBitmap(original,-1);

	for (int32 i=0;i<info.cpu_count;i++) {
		threads[i] = spawn_thread(thread_entry,"perlin_fade_thread",B_NORMAL_PRIORITY,this);
		resume_thread(threads[i]);
		send_data(threads[i],i,NULL,0);
	}

	for (int32 i=0;i<info.cpu_count;i++) {
		int32 return_value;
		wait_for_thread(threads[i],&return_value);
	}

	delete[] threads;

	delete spare_copy_bitmap;

	// Return the original.
	return original;
}

char* MarbleManipulator::ReturnName()
{
	return "Marble";
}



int32 MarbleManipulator::thread_entry(void *data)
{
	int32 thread_number;
	thread_number = receive_data(NULL,NULL,0);

	MarbleManipulator *this_pointer = (MarbleManipulator*)data;

	return this_pointer->thread_function(thread_number);
}


int32 MarbleManipulator::thread_function(int32 thread_number)
{
	BWindow *progress_bar_window = NULL;
	if (progress_bar != NULL)
		progress_bar_window = progress_bar->Window();

	register uint32 *source = (uint32*)source_bitmap->Bits();
	int32 source_bpr = source_bitmap->BytesPerRow()/4;

	PerlinNoiseGenerator generator(0.5,7);

	// This union must be used to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	if (the_selection->IsEmpty()) {
		// Here handle the whole image.
		float left = target_bitmap->Bounds().left;
		float right = target_bitmap->Bounds().right;
		float top = target_bitmap->Bounds().top;
		float bottom = target_bitmap->Bounds().bottom;


		system_info info;
		get_system_info(&info);

		int32 height = (bottom - top+1)/info.cpu_count;
		top = min_c(bottom,top+thread_number*height);
		bottom = min_c(bottom,top + height-1);

		int32 update_interval = 10;
		float update_amount = 100.0/(bottom-top)*update_interval/(float)info.cpu_count;
		float missed_update = 0;

		// Loop through all pixels in original.
		float one_per_width = 1.0/128;
		float one_per_height = 1.0/128;
		float one_per_depth = 1.0/1024;
		source += (int32)top*source_bpr;

		for (float y=top;y<=bottom;++y) {
			int32 y_times_bpr = y*source_bpr;
			for (float x=left;x<=right;++x) {
				color.word = *source;
				float noise = generator.PerlinNoise2D(x*one_per_width,y*one_per_height);

				if (noise > 0) {
					rgb_color c = informer->GetForegroundColor();
					color.bytes[0] = c.blue;
					color.bytes[1] = c.green;
					color.bytes[2] = c.red;
					color.bytes[3] = c.alpha;
					*source = mix_2_pixels_fixed(*source,color.word,32768*(1.0-noise));
				}
				++source;
			}
			// Update the status-bar
			if ( (((int32)y % update_interval) == 0) && (progress_bar_window != NULL) && (progress_bar_window->LockWithTimeout(0) == B_OK) ) {
				progress_bar->Update(update_amount+missed_update);
				progress_bar_window->Unlock();
				missed_update = 0;
			}
			else if (((int32)y % update_interval) == 0) {
				missed_update += update_amount;
			}
		}

	}
	else {
		// Here handle only those pixels for which selection->ContainsPoint(x,y) is true.
		BRect rect = the_selection->GetBoundingRect();

		int32 left = rect.left;
		int32 right = rect.right;
		int32 top = rect.top;
		int32 bottom = rect.bottom;

		system_info info;
		get_system_info(&info);

		int32 height = (bottom - top+1)/info.cpu_count;
		top = min_c(bottom,top+thread_number*height);
		bottom = min_c(bottom,top + height-1);

		int32 update_interval = 10;
		float update_amount = 100.0/(bottom-top)*update_interval/(float)info.cpu_count;
		float missed_update = 0;


		// Loop through all pixels in original.
		float one_per_width = 1.0/8;
		float one_per_height = 1.0/256;
		float one_per_depth = 1.0/1024;
		source += (int32)left + (int32)top*source_bpr;

		// Loop through all pixels in original.
		for (int32 y=top;y<=bottom;++y) {
			for (int32 x=left;x<=right;++x) {
				if (the_selection->ContainsPoint(x,y)) {

				}
				else {
					++source;
				}
			}
			source += (source_bpr - (int32)(right-left)-1);

			// Update the status-bar
			if ( (((int32)y % update_interval) == 0) && (progress_bar_window != NULL) && (progress_bar_window->LockWithTimeout(0) == B_OK) ) {
				progress_bar->Update(update_amount+missed_update);
				progress_bar_window->Unlock();
				missed_update = 0;
			}
			else if (((int32)y % update_interval) == 0) {
				missed_update += update_amount;
			}
		}
	}

	return B_OK;
}



float MarbleManipulator::marble_amount(float x)
{
	x = fabs(x);
	// Select the four value and proper u
	float v1 = 0;
	float v2 = 127;
	float v3 = -128;
	float v4 = 0;

	float u;

	if (x<0.33) {
		v2 = v1;
		v3 = v2;
		v4 = v3;
		u = (0.33-x)/0.33;
	}
	else if (x < 0.66) {
		u = (0.66-x)/0.66;
	}
	else {
		v1 = v2;
		v2 = v3;
		v3 = v4;
		u = (1-x)/0.33;
	}

	return SplineGenerator::CardinalSpline(v1,v2,v3,v4,u);
}
