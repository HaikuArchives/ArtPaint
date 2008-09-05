/* 

	Filename:	BitmapUtilities.h
	Contents:	BitmapUtilities-class declaration		
	Author:		Heikki Suhonen (Heikki.Suhonen@Helsinki.FI)
	
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
