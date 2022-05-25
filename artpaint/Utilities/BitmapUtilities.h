/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef _BITMAP_UTILITIES_H
#define	_BITMAP_UTILITIES_H


#include <Bitmap.h>


class BitmapUtilities {
public:
	static	status_t	FixMissingAlpha(BBitmap *bitmap);
	static	BBitmap*	ConvertColorSpace(BBitmap *inBitmap,
							color_space wantSpace);
	static  void		CompositeBitmapOnSource(BBitmap* toBuffer,
							BBitmap* srcBuffer, BBitmap* fromBuffer,
							BRect updated_rect);
	static  void		ClearBitmap(BBitmap* bitmap, uint32 color,
							BRect* area = NULL);
};


#endif	// _BITMAP_UTILITIES_H
