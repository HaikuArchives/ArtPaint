/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <StatusBar.h>
#include <new>
#include <Window.h>

#include "AddOns.h"
#include "EnhanceEdges.h"
#include "ManipulatorInformer.h"
#include "Selection.h"

#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = "Enhance Edges";
	char menu_help_string[255] = "Enhances the edges in the image.";
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = SHARPEN_FILTER_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap*,ManipulatorInformer *i)
{
	delete i;	// This should be deleted if it is not needed
	return new EnhanceEdgesManipulator();
}



EnhanceEdgesManipulator::EnhanceEdgesManipulator()
		: Manipulator()
{
}


EnhanceEdgesManipulator::~EnhanceEdgesManipulator()
{
}


BBitmap* EnhanceEdgesManipulator::ManipulateBitmap(BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	/*
		This function will first copy the original and then calculate the effect from the copy
		to the original. Edges will be enhanced by using convolution with the following kernel.

			-1	-1	-1
			-1	 9	-1
			-1	-1	-1

		In effect what this does is to add the laplacian of the image back to itself. Care must
		be take so that the resulting values fall between 0 and 255. This function does not change the
		alpha-channel of the original.

		This effect will be calculated using multiple threads. Instead of passing the data to the
		threads we can record it to non-static attributes in this class.
	*/

	BBitmap *duplicate;
	try {
		// Make the duplicate 1 larger than the original. This allows the
		// convolution to work properly.
		duplicate = DuplicateBitmap(original,-1);
	}
	catch (std::bad_alloc e) {
		// Here we could clean up if there was need for that.
		return NULL;	// Returning NULL means that the image did not change.
	}


	// Record the info to member variables so that the threads
	// know what to do.
	source_bitmap = duplicate;
	target_bitmap = original;
	the_selection = selection;
	progress_bar = status_bar;

	system_info info;
	get_system_info(&info);

	thread_id *threads = new thread_id[info.cpu_count];

	// Start the threads. If the image is small enough
	// one thread might be enough.
	for (int32 i=0;i<info.cpu_count;i++) {
		threads[i] = spawn_thread(thread_entry,"enhance_edges_thread",B_NORMAL_PRIORITY,this);
		resume_thread(threads[i]);
		send_data(threads[i],i,NULL,0);
	}

	// Wait for them threads to finish.
	for (int32 i=0;i<info.cpu_count;i++) {
		int32 return_value;
		wait_for_thread(threads[i],&return_value);
	}

	delete[] threads;

	// Get rid of the duplicate bitmap.
	delete duplicate;
	duplicate = NULL;

	// Return the original (to which the processing was done).
	return original;
}

char* EnhanceEdgesManipulator::ReturnName()
{
	return "Enhance Edges";
}



int32 EnhanceEdgesManipulator::thread_entry(void *data)
{
	int32 thread_number;
	thread_number = receive_data(NULL,NULL,0);

	EnhanceEdgesManipulator *this_pointer = (EnhanceEdgesManipulator*)data;

	return this_pointer->thread_function(thread_number);
}


int32 EnhanceEdgesManipulator::thread_function(int32 thread_number)
{
	BWindow *progress_bar_window = NULL;
	if (progress_bar != NULL)
		progress_bar_window = progress_bar->Window();

	uint32 *source = (uint32*)source_bitmap->Bits();
	uint32 *target = (uint32*)target_bitmap->Bits();
	int32 source_bpr = source_bitmap->BytesPerRow()/4;
	int32 target_bpr = target_bitmap->BytesPerRow()/4;

	// This union must be used to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	if (the_selection->IsEmpty()) {
		// Here handle the whole image.
		int32 left = target_bitmap->Bounds().left;
		int32 right = target_bitmap->Bounds().right;
		int32 top = target_bitmap->Bounds().top;
		int32 bottom = target_bitmap->Bounds().bottom;

		system_info info;
		get_system_info(&info);

		float height = bottom - top;
		top = height/info.cpu_count*thread_number;
		bottom = min_c(bottom,top + (height+1)/info.cpu_count);

		int32 update_interval = 10;
		float update_amount = 100.0/(bottom-top)*update_interval/(float)info.cpu_count;
		float missed_update = 0;

		target += top*target_bpr;

		// Loop through all pixels in original.
		for (int32 y=top;y<=bottom;++y) {
			for (int32 x=left;x<=right;++x) {
				float red = 0;
				float green = 0;
				float blue = 0;

				// First the top row
				color.word = *(source + 1+x-1 + (1+y-1)*source_bpr);
				red -= color.bytes[2];
				green -= color.bytes[1];
				blue -= color.bytes[0];

				color.word = *(source + 1+x + (1+y-1)*source_bpr);
				red -= color.bytes[2];
				green -= color.bytes[1];
				blue -= color.bytes[0];

				color.word = *(source + 1+x+1 + (1+y-1)*source_bpr);
				red -= color.bytes[2];
				green -= color.bytes[1];
				blue -= color.bytes[0];

				// Then the middle row
				color.word = *(source + 1+x-1 + (1+y)*source_bpr);
				red -= color.bytes[2];
				green -= color.bytes[1];
				blue -= color.bytes[0];

				color.word = *(source + 1+x + (1+y)*source_bpr);
				red += 9*color.bytes[2];
				green += 9*color.bytes[1];
				blue += 9*color.bytes[0];

				color.word = *(source + 1+x+1 + (1+y)*source_bpr);
				red -= color.bytes[2];
				green -= color.bytes[1];
				blue -= color.bytes[0];

				// Then the bottom row
				color.word = *(source + 1+x-1 + (1+y+1)*source_bpr);
				red -= color.bytes[2];
				green -= color.bytes[1];
				blue -= color.bytes[0];

				color.word = *(source + 1+x + (1+y+1)*source_bpr);
				red -= color.bytes[2];
				green -= color.bytes[1];
				blue -= color.bytes[0];

				color.word = *(source + 1+x+1 + (1+y+1)*source_bpr);
				red -= color.bytes[2];
				green -= color.bytes[1];
				blue -= color.bytes[0];


				// Then limit the values between 0 and 255.
				red = min_c(255,max_c(0,red));
				green = min_c(255,max_c(0,green));
				blue = min_c(255,max_c(0,blue));

				color.word = *target;
				color.bytes[0] = (uint8)blue;
				color.bytes[1] = (uint8)green;
				color.bytes[2] = (uint8)red;

				*target++ = color.word;
			}

			// Update the status-bar
			if ( ((y % update_interval) == 0) && (progress_bar_window != NULL) && (progress_bar_window->LockWithTimeout(0) == B_OK) ) {
				progress_bar->Update(update_amount+missed_update);
				progress_bar_window->Unlock();
				missed_update = 0;
			}
			else if ((y % update_interval) == 0) {
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

		float height = bottom - top;
		top += height/info.cpu_count*thread_number;
		bottom = min_c(bottom,top + (height+1)/info.cpu_count);

		int32 update_interval = 10;
		float update_amount = 100.0/(bottom-top)*update_interval/(float)info.cpu_count;

		// Loop through all pixels in original.
		for (int32 y=top;y<=bottom;++y) {
			for (int32 x=left;x<=right;++x) {
				if (the_selection->ContainsPoint(x,y)) {
					float red = 0;
					float green = 0;
					float blue = 0;

					// First the top row
					color.word = *(source + 1+x-1 + (1+y-1)*source_bpr);
					red -= color.bytes[2];
					green -= color.bytes[1];
					blue -= color.bytes[0];

					color.word = *(source + 1+x + (1+y-1)*source_bpr);
					red -= color.bytes[2];
					green -= color.bytes[1];
					blue -= color.bytes[0];

					color.word = *(source + 1+x+1 + (1+y-1)*source_bpr);
					red -= color.bytes[2];
					green -= color.bytes[1];
					blue -= color.bytes[0];

					// Then the middle row
					color.word = *(source + 1+x-1 + (1+y)*source_bpr);
					red -= color.bytes[2];
					green -= color.bytes[1];
					blue -= color.bytes[0];

					color.word = *(source + 1+x + (1+y)*source_bpr);
					red += 9*color.bytes[2];
					green += 9*color.bytes[1];
					blue += 9*color.bytes[0];

					color.word = *(source + 1+x+1 + (1+y)*source_bpr);
					red -= color.bytes[2];
					green -= color.bytes[1];
					blue -= color.bytes[0];

					// Then the bottom row
					color.word = *(source + 1+x-1 + (1+y+1)*source_bpr);
					red -= color.bytes[2];
					green -= color.bytes[1];
					blue -= color.bytes[0];

					color.word = *(source + 1+x + (1+y+1)*source_bpr);
					red -= color.bytes[2];
					green -= color.bytes[1];
					blue -= color.bytes[0];

					color.word = *(source + 1+x+1 + (1+y+1)*source_bpr);
					red -= color.bytes[2];
					green -= color.bytes[1];
					blue -= color.bytes[0];


					// Then limit the values between 0 and 255.
					red = min_c(255,max_c(0,red));
					green = min_c(255,max_c(0,green));
					blue = min_c(255,max_c(0,blue));

					color.word = *(target + x + y*target_bpr);
					color.bytes[0] = (uint8)blue;
					color.bytes[1] = (uint8)green;
					color.bytes[2] = (uint8)red;

					*(target + x + y*target_bpr) = color.word;
				}
			}

			// Update the status-bar
			if ( ((y % update_interval) == 0) && (progress_bar_window != NULL) && (progress_bar_window->LockWithTimeout(0) == B_OK) ) {
				progress_bar->Update(update_amount);
				progress_bar_window->Unlock();
			}
		}
	}

	return B_OK;
}
