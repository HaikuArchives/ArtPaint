/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "AddOns.h"
#include "AHEManipulator.h"
#include "ImageProcessingLibrary.h"


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = "Adaptive Histogram Equalization";
	char menu_help_string[255] = "Improves the contrast of the active layer by equalizing its histogram locally.";
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = COLOR_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new AHEManipulator(bm);
}


AHEManipulator::AHEManipulator(BBitmap*)
		: Manipulator()
{
}


AHEManipulator::~AHEManipulator()
{
}


BBitmap* AHEManipulator::ManipulateBitmap(BBitmap *original, Selection *selection, BStatusBar*)
{
	// This manipulator assumes a grayscale image
	ImageProcessingLibrary iplib;

	iplib.grayscale_clahe(original,16,5);

	if (selection->IsEmpty() == true) {
	}
	else {
	}

	return original;
}
