/*

	Filename:	AddOns.h
	Contents:	Definitions for stuff that is needed by add-ons.
	Author:		Heikki Suhonen

*/


#ifndef ADD_ONS_H
#define	ADD_ONS_H

#include <Bitmap.h>
#include "ManipulatorInformer.h"
#include "Manipulator.h"

enum add_on_types {
	BLUR_FILTER_ADD_ON,
	SHARPEN_FILTER_ADD_ON,
	EFFECT_FILTER_ADD_ON,
	DISTORT_ADD_ON,
	COLOR_ADD_ON,
	ANALYZER_ADD_ON,
	GENERIC_ADD_ON
};


extern "C" __declspec(dllexport) Manipulator* instantiate_add_on(BBitmap*,ManipulatorInformer*);

#endif
