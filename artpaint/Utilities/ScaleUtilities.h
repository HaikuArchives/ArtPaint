/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef _SCALE_UTILITIES_H
#define	_SCALE_UTILITIES_H

#include "PixelOperations.h"

#include <Bitmap.h>
#include <Catalog.h>
#include <Point.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ScaleUtilities"


#define SCALE_METHOD_CHANGED	'ScMc'


enum interpolation_type {
	NEAREST_NEIGHBOR,
	BILINEAR,
	BICUBIC,
	BICUBIC_CATMULL_ROM,
	BICUBIC_BSPLINE,
	MITCHELL
};


inline const char* interpolation_type_to_string(interpolation_type method)
{
	switch(method) {
		case NEAREST_NEIGHBOR:
			return B_TRANSLATE("Nearest neighbor");
		case BILINEAR:
			return B_TRANSLATE("Bilinear");
		case BICUBIC:
			return B_TRANSLATE("Bicubic");
		case BICUBIC_CATMULL_ROM:
			return B_TRANSLATE("Bicubic (Catmull-Rom)");
		case BICUBIC_BSPLINE:
			return B_TRANSLATE("Bicubic (B-spline)");
		case MITCHELL:
			return B_TRANSLATE("Mitchell-Netravali");
	}

	return "";
}


class ScaleUtilities {
public:
	static void		MoveGrabbers(BPoint point, BPoint& previous,
						float& left, float& top, float& right,
						float& bottom, float aspect_ratio,
						bool& move_left, bool& move_top,
						bool& move_right, bool& move_bottom,
						bool& move_all, bool first_click, bool lock_aspect);
	static void		ScaleHorizontally(float width, float height, BPoint offset,
						BBitmap* source, BBitmap* target,
						float ratio, interpolation_type method);
	static void		ScaleVertically(float width, float height, BPoint offset,
						BBitmap* source, BBitmap* target,
						float ratio, interpolation_type method);
	static void		ScaleHorizontallyGray(float width, float height, BPoint offset,
						BBitmap* source, BBitmap* target, float ratio);
	static void		ScaleVerticallyGray(float width, float height, BPoint offset,
						BBitmap* source, BBitmap* target, float ratio);
};


#endif	// _SCALE_UTILITIES_H
