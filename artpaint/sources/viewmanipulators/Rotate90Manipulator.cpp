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

#include "MessageConstants.h"
#include "Rotate90Manipulator.h"
#include "ImageView.h"
#include "StringServer.h"

Rotate90ClockwiseManipulator::Rotate90ClockwiseManipulator()
	:	Manipulator()
{
}

BBitmap* Rotate90ClockwiseManipulator::ManipulateBitmap(BBitmap *original,Selection*,BStatusBar *status_bar)
{
	// create a new bitmap that is as wide as the original is high
	BRect new_bounds(0,0,original->Bounds().Height(),original->Bounds().Width());
	BBitmap *new_bitmap = new BBitmap(new_bounds,B_RGBA32);


	// the first row in original becomes the last column in the new one
	uint32 *original_bits = (uint32*)original->Bits();
	uint32 *new_bits = (uint32*)new_bitmap->Bits();
	int32 new_bpr = new_bitmap->BytesPerRow()/4;


	int32 height = original->Bounds().IntegerHeight();
	int32 width = original->Bounds().IntegerWidth();

	for (int32 y=0;y<=height;y++) {
		for (int32 x=0;x<=width;x++) {
			*(new_bits + x*new_bpr + (new_bpr-y-1)) = *original_bits++;
		}
	}

	return new_bitmap;
}

const char* Rotate90ClockwiseManipulator::ReturnName()
{
	return StringServer::ReturnString(ROTATE_CW_STRING);
}




Rotate90CounterclockwiseManipulator::Rotate90CounterclockwiseManipulator()
	:	Manipulator()
{
}


BBitmap* Rotate90CounterclockwiseManipulator::ManipulateBitmap(BBitmap *original,Selection*,BStatusBar *status_bar)
{
	// create a new bitmap that is as wide as the original is high
	BRect new_bounds(0,0,original->Bounds().Height(),original->Bounds().Width());
	BBitmap *new_bitmap = new BBitmap(new_bounds,B_RGBA32);


	// the last row in original becomes the last column in the new one
	uint32 *original_bits = (uint32*)original->Bits();
	uint32 *new_bits = (uint32*)new_bitmap->Bits();
	int32 new_bpr = new_bitmap->BytesPerRow()/4;

	int32 height = original->Bounds().IntegerHeight();
	int32 width = original->Bounds().IntegerWidth();

	for (int32 y=0;y<=height;y++) {
		for (int32 x=0;x<=width;x++) {
			*(new_bits + (width-x)*new_bpr + y) = *original_bits++;
		}
	}

	return new_bitmap;
}

const char* Rotate90CounterclockwiseManipulator::ReturnName()
{
	return StringServer::ReturnString(ROTATE_CCW_STRING);
}
