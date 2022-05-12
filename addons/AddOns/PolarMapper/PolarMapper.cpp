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
#include <Node.h>
#include <StatusBar.h>
#include <stdlib.h>
#include <string.h>
#include <Window.h>

#include "AddOns.h"
#include "PolarMapper.h"
#include "ManipulatorInformer.h"
#include "Selection.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddOns_PolarMapper"


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = B_TRANSLATE_MARK("Polar mapper");
	char menu_help_string[255] = B_TRANSLATE_MARK("Maps the image to its polar coordinate representation.");
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = EFFECT_FILTER_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new PolarMapper(bm);
}




PolarMapper::PolarMapper(BBitmap*)
		: Manipulator()
{
}


PolarMapper::~PolarMapper()
{

}

BBitmap* PolarMapper::ManipulateBitmap(BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	BWindow *status_bar_window = NULL;
	if (status_bar != NULL)
		status_bar_window = status_bar->Window();

	BBitmap *spare_buffer = new BBitmap(original->Bounds(),B_RGBA32);

	// first map it
	int32 width = original->Bounds().Width() + 1;
	int32 height = original->Bounds().Height() + 1;

	uint32 *spare_bits = (uint32*)spare_buffer->Bits();
	uint32 *original_bits = (uint32*)original->Bits();
	int32 original_bpr = original->BytesPerRow()/4;

	float max_radius = sqrt(width*width/4.0 + height*height/4.0);

	int32 mid_x = width/2;
	int32 mid_y = height/2;

	for (int32 y=0;y<height;y++) {
		for (int32 x=0;x<width;x++) {
			float angle = (float)(height-1-y)/(float)height * M_PI * 2;
			float radius = (float)x/(float)width*max_radius;

			int32 new_x = radius * cos(angle) + mid_x;
			int32 new_y = radius * sin(angle) + mid_y;

			union {
				uint8 bytes[4];
				uint32 word;
			} new_color;

			if ((new_x>=0)&&(new_x<width)&&(new_y>=0)&&(new_y<height)) {
				new_color.word = *(original_bits + new_x + (height-new_y-1) * original_bpr);
			}
			else {
				new_color.word = 0xFFFFFFFF;
				new_color.bytes[3] = 0x00;
			}
			*spare_bits++ = new_color.word;
		}
	}


	spare_bits = (uint32*)spare_buffer->Bits();
	original_bits = (uint32*)original->Bits();
	// then copy it
	if ((selection == NULL) || (selection->IsEmpty() == TRUE)) {
		for (int32 y=0;y<height;y++) {
			for (int32 x=0;x<width;x++) {
				*original_bits++ = *spare_bits++;
			}
			if (((y % 20) == 0) && (status_bar_window != NULL) && (status_bar != NULL) && (status_bar_window->LockWithTimeout(0) == B_OK)) {
				status_bar->Update(100.0/height*20);
				status_bar_window->Unlock();
			}
		}
	}
	else {
		for (int32 y=0;y<height;y++) {
			for (int32 x=0;x<width;x++) {
				if (selection->ContainsPoint(x,y))
					*original_bits = *spare_bits;
				original_bits++;
				spare_bits++;
			}
			if (((y % 20) == 0) && (status_bar_window != NULL) && (status_bar != NULL) && (status_bar_window->LockWithTimeout(0) == B_OK)) {
				status_bar->Update(100.0/height*20);
				status_bar_window->Unlock();
			}
		}
	}
	// we should also delete the spare-bitmap
	delete spare_buffer;

	return original;
}


const char* PolarMapper::ReturnHelpString()
{
	return B_TRANSLATE("Maps the image to its polar coordinate representation.");
}


const char*	PolarMapper::ReturnName()
{
	return B_TRANSLATE("Polar mapper");
}
