/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef _BITMAP_UTILITIES_H
#define	_BITMAP_UTILITIES_H

#include <Bitmap.h>

class BitmapUtilities {
public:
	static	status_t	FixMissingAlpha(BBitmap *bitmap);
	static	BBitmap*	ConvertColorSpace(BBitmap *inBitmap, color_space wantSpace);
};



#endif	// _BITMAP_UTILITIES_H
