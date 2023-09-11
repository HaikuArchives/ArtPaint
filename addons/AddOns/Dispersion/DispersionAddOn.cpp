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
#include <Window.h>
#include <stdlib.h>
#include <string.h>


#include "AddOns.h"
#include "BitmapDrawer.h"
#include "DispersionAddOn.h"
#include "ManipulatorInformer.h"
#include "Selection.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddOns_Dispersion"


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = B_TRANSLATE_MARK("Dispersion");
	char menu_help_string[255] = B_TRANSLATE_MARK("Randomly moves pixels a bit.");
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = EFFECT_FILTER_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator*
instantiate_add_on(BBitmap* bm, ManipulatorInformer* i)
{
	delete i;
	return new DispersionManipulator(bm);
}


DispersionManipulator::DispersionManipulator(BBitmap*)
	:
	Manipulator(),
	selection(NULL)
{
}


BBitmap*
DispersionManipulator::ManipulateBitmap(BBitmap* original, BStatusBar* status_bar)
{
	BWindow* status_bar_window = NULL;
	if (status_bar != NULL)
		status_bar_window = status_bar->Window();

	BBitmap* spare_buffer = DuplicateBitmap(original);
	BitmapDrawer* target = new BitmapDrawer(original);

	int32 width = original->Bounds().Width() + 1;
	int32 height = original->Bounds().Height() + 1;

	uint32* spare_bits = (uint32*)spare_buffer->Bits();
	uint32 moved_pixel;

	int32 dx, dy;

	for (int32 y = 0; y < height; y++) {
		for (int32 x = 0; x < width; x++) {
			moved_pixel = *spare_bits++;
			dx = (rand() % MAX_DISPERSION_X) * (rand() % 2 == 0 ? -1 : 1);
			dy = (rand() % MAX_DISPERSION_Y) * (rand() % 2 == 0 ? -1 : 1);
			target->SetPixel(BPoint(x + dx, y + dy), moved_pixel, selection);
		}
		if (((y % 20) == 0) && (status_bar_window != NULL) && (status_bar != NULL)
			&& (status_bar_window->LockWithTimeout(0) == B_OK)) {
			status_bar->Update(100.0 / height * 20);
			status_bar_window->Unlock();
		}
	}

	// we should also delete the spare-bitmap
	delete spare_buffer;
	delete target;

	return original;
}


const char*
DispersionManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Randomly moves pixels a bit.");
}


const char*
DispersionManipulator::ReturnName()
{
	return B_TRANSLATE("Dispersion");
}
