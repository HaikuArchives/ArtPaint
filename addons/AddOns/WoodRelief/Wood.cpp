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
#include "Wood.h"
#include "PixelOperations.h"
#include "PerlinNoiseGenerator.h"

extern "C" __declspec(dllexport) char name[255] = "PerlinWood";
extern "C" __declspec(dllexport) char menu_help_string[255] = "Woods the image using perlin noise function.";
extern "C" __declspec(dllexport) int32 add_on_api_version = ADD_ON_API_VERSION;
extern "C" __declspec(dllexport) add_on_types add_on_type = EFFECT_FILTER_ADD_ON;


Manipulator* instantiate_add_on(BBitmap*,ManipulatorInformer *i)
{
	delete i;
	return new WoodManipulator();
}



WoodManipulator::WoodManipulator()
		: Manipulator()
{
}


WoodManipulator::~WoodManipulator()
{
}


BBitmap* WoodManipulator::ManipulateBitmap(BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	BStopWatch watch("PerlinWood");

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

char* WoodManipulator::ReturnName()
{
	return "PerlinWood";
}



int32 WoodManipulator::thread_entry(void *data)
{
	int32 thread_number;
	thread_number = receive_data(NULL,NULL,0);

	WoodManipulator *this_pointer = (WoodManipulator*)data;

	return this_pointer->thread_function(thread_number);
}


int32 WoodManipulator::thread_function(int32 thread_number)
{
	BWindow *progress_bar_window = NULL;
	if (progress_bar != NULL)
		progress_bar_window = progress_bar->Window();

	register uint32 *source = (uint32*)source_bitmap->Bits();
	int32 source_bpr = source_bitmap->BytesPerRow()/4;

	register uint32 *spare_bits = (uint32*)spare_copy_bitmap->Bits();
	int32 spare_bpr = spare_copy_bitmap->BytesPerRow()/4;

	PerlinNoiseGenerator generator(0.7,7);

	// This union must be used to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} color,color1,color2,color3;

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

		spare_bits += (int32)left + 1 + ((int32)top+1)*spare_bpr;

		// Loop through all pixels in original.
		float one_per_width = 1.0/8;
		float one_per_height = 1.0/256;
		float one_per_depth = 1.0/1024;
		source += (int32)top*source_bpr;

		for (float y=top;y<=bottom;++y) {
			int32 y_times_bpr = y*source_bpr;
			for (float x=left;x<=right;++x) {

				color1.word = *(spare_bits - spare_bpr - 1);
				color2.word = *(spare_bits + spare_bpr + 1);
				color3.word = *source;

				float difference = 	.144*(color1.bytes[0] - color2.bytes[0]) +
									.587*(color1.bytes[1] - color2.bytes[1]) +
									.299*(color1.bytes[2] - color2.bytes[2]);

//				if (difference != 0)
//					difference /= abs(difference);

				difference /= 255.0;

// This creates a woodlike texture when one_per_width is 1/8 and one_per_height is 1/256,
// and generator is initialized with 0.7,8.
				float z = 	.144*color3.bytes[0] +
							.597*color3.bytes[1] +
							.299*color3.bytes[2];

				float noise = generator.PerlinNoise3D(x*one_per_width,y*one_per_height,z*one_per_depth);

				float coeff = 0.5+(1+noise)*.25;
				color3.bytes[0] = min_c(255,max_c(0,30*coeff+difference*200));
				color3.bytes[1] = min_c(255,max_c(0,140*coeff+difference*200));
				color3.bytes[2] = min_c(255,max_c(0,200*coeff+difference*200));

				*source++ = color3.word;

				spare_bits++;
			}
			spare_bits += 2;
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

		spare_bits += (int32)left + 1 + ((int32)top+1)*spare_bpr;

		// Loop through all pixels in original.
		float one_per_width = 1.0/8;
		float one_per_height = 1.0/256;
		float one_per_depth = 1.0/1024;
		source += (int32)left + (int32)top*source_bpr;

		// Loop through all pixels in original.
		for (int32 y=top;y<=bottom;++y) {
			for (int32 x=left;x<=right;++x) {
				if (the_selection->ContainsPoint(x,y)) {

					color1.word = *(spare_bits - spare_bpr - 1);
					color2.word = *(spare_bits + spare_bpr + 1);
					color3.word = *source;

					float difference = 	.144*(color1.bytes[0] - color2.bytes[0]) +
										.587*(color1.bytes[1] - color2.bytes[1]) +
										.299*(color1.bytes[2] - color2.bytes[2]);
					difference /= 255.0;

// 	This creates a woodlike texture when one_per_width is 1/8 and one_per_height is 1/256,
// 	and generator is initialized with 0.7,8.
					float z = 	.144*color3.bytes[0] +
								.597*color3.bytes[1] +
								.299*color3.bytes[2];

					float noise = generator.PerlinNoise3D(x*one_per_width,y*one_per_height,z*one_per_depth);

					float coeff = 0.5+(1+noise)*.25;
					color3.bytes[0] = min_c(255,max_c(0,30*coeff+difference*200));
					color3.bytes[1] = min_c(255,max_c(0,140*coeff+difference*200));
					color3.bytes[2] = min_c(255,max_c(0,200*coeff+difference*200));

					*source++ = color3.word;

					spare_bits++;
				}
				else {
					++source; ++spare_bits;
				}
			}
			spare_bits += (spare_bpr - (int32)(right-left)-1);
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
