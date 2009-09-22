/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <Message.h>
#include <StatusBar.h>
#include <stdlib.h>
#include <Window.h>

#include "AddOns.h"
#include "OilAddOn.h"
#include "BitmapDrawer.h"
#include "ManipulatorInformer.h"
#include "Selection.h"

#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = "Oil";
	char menu_help_string[255] = "Makes an \"oil\" effect on the active layer.";
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = EFFECT_FILTER_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new OilManipulator(bm);
}


OilManipulator::OilManipulator(BBitmap*)
		: Manipulator()
{
}


OilManipulator::~OilManipulator()
{

}

BBitmap* OilManipulator::ManipulateBitmap(BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	BWindow *status_bar_window = NULL;
	if (status_bar != NULL)
		status_bar_window = status_bar->Window();

	BRect a_rect = original->Bounds();
	BBitmap *spare_buffer = DuplicateBitmap(original);

	BitmapDrawer *target = new BitmapDrawer(original);
	BitmapDrawer *source = new BitmapDrawer(spare_buffer);

	int32 width = original->Bounds().Width() + 1;
	int32 height = original->Bounds().Height() + 1;

	float status_bar_update_step = 100.0 / (width*height) * 1000.0;

	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta",0.0);

	// We must select all pixels between 0 and width*height in a
	// pseudo-random order.
	int32 *offsets = new int32[width*height];
	for (int32 i=0;i<width*height;i++)
		offsets[i] = i;

	/*
		We copy each pixel with the following pattern:

						 O
						OOO
			X	->		 OXOO
						  O

	*/

	int32 size_of_area = width*height-1;
	int32 width_times_height = width*height;
	uint32 moved_pixel;
	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	int32 random_array_size = 32;
	int32 *random_array = new int32[random_array_size];
	for (int32 i=0;i<random_array_size;i++) {
		random_array[i] = random()%10 * (random()%2 == 0?-1:1);
	}

	if ((selection == NULL) || (selection->IsEmpty() == TRUE)) {
//		for (int32 i=0;i<width_times_height;i++) {
		while (size_of_area > 0) {
			// Select one pixel at random
			int32 new_offset = rand() % size_of_area;
			int32 spare = offsets[new_offset];
			offsets[new_offset] = offsets[size_of_area];
			size_of_area--;
			int32 x = spare % width;
			int32 y = spare / width;
			color.word = source->GetPixel(BPoint(x,y));
			// randomize the color a bit
			color.bytes[0] = min_c(255,max_c(0,(int32)color.bytes[0] + random_array[size_of_area%random_array_size]));
			color.bytes[1] = min_c(255,max_c(0,(int32)color.bytes[1] + random_array[size_of_area%random_array_size]));
			color.bytes[2] = min_c(255,max_c(0,(int32)color.bytes[2] + random_array[size_of_area%random_array_size]));

			moved_pixel = color.word;
			target->SetPixel(BPoint(x-2,y-1),moved_pixel);
			target->SetPixel(BPoint(x-1,y-1),moved_pixel);
			target->SetPixel(BPoint(x,y-1),moved_pixel);
			target->SetPixel(BPoint(x-1,y-2),moved_pixel);
			target->SetPixel(BPoint(x-1,y),moved_pixel);
			target->SetPixel(BPoint(x,y),moved_pixel);
			target->SetPixel(BPoint(x+1,y),moved_pixel);
			target->SetPixel(BPoint(x+2,y),moved_pixel);
			target->SetPixel(BPoint(x,y+1),moved_pixel);

			if (((size_of_area % 1000) == 0) && (status_bar != NULL) && (status_bar_window != NULL) && (status_bar_window->LockWithTimeout(0) == B_OK)) {
				status_bar->Update(status_bar_update_step);
				status_bar_window->Unlock();
			}
		}
	}
	else {
		while (size_of_area > 0) {
			// Select one pixel at random
			int32 new_offset = rand() % size_of_area;
			int32 spare = offsets[new_offset];
			offsets[new_offset] = offsets[size_of_area];
			size_of_area--;
			int32 x = spare % width;
			int32 y = spare / width;
			color.word = source->GetPixel(BPoint(x,y));
			// randomize the color a bit
			color.bytes[0] = min_c(255,max_c(0,(int32)color.bytes[0] + random_array[size_of_area%random_array_size]));
			color.bytes[1] = min_c(255,max_c(0,(int32)color.bytes[1] + random_array[size_of_area%random_array_size]));
			color.bytes[2] = min_c(255,max_c(0,(int32)color.bytes[2] + random_array[size_of_area%random_array_size]));

			moved_pixel = color.word;

			target->SetPixel(BPoint(x-2,y-1),moved_pixel,selection);
			target->SetPixel(BPoint(x-1,y-1),moved_pixel,selection);
			target->SetPixel(BPoint(x,y-1),moved_pixel,selection);
			target->SetPixel(BPoint(x-1,y-2),moved_pixel,selection);
			target->SetPixel(BPoint(x-1,y),moved_pixel,selection);
			target->SetPixel(BPoint(x,y),moved_pixel,selection);
			target->SetPixel(BPoint(x+1,y),moved_pixel,selection);
			target->SetPixel(BPoint(x+2,y),moved_pixel,selection);
			target->SetPixel(BPoint(x,y+1),moved_pixel,selection);
			if (((size_of_area % 1000) == 0) && (status_bar != NULL) && (status_bar_window != NULL) && (status_bar_window->LockWithTimeout(0) == B_OK)) {
				status_bar->Update(status_bar_update_step);
				status_bar_window->Unlock();
			}
		}
	}
	// we should also delete the spare-bitmap
	delete spare_buffer;
	delete target;
	delete source;
	delete[] offsets;

	return original;
}
