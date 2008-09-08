/*

	Filename:	Manipulator.cpp
	Contents:	Manipulator-class definitions
	Author:		Heikki Suhonen

*/


#include "Manipulator.h"
#include <new.h>
#include <stdlib.h>

BBitmap* Manipulator::DuplicateBitmap(BBitmap *original,int32 inset,bool accept_views)
{
	BRect bounds = original->Bounds();
	BRect new_bounds = bounds;
	new_bounds.InsetBy(inset,inset);

	BBitmap *copy = new BBitmap(new_bounds,B_RGB32,accept_views);
	if (copy->IsValid() == FALSE)
		throw bad_alloc();

	uint32 *target_bits = (uint32*)copy->Bits();
	uint32 *source_bits = (uint32*)original->Bits();
	uint32 bits_length = original->BitsLength()/4;
	uint32 source_bpr = original->BytesPerRow()/4;

	if (inset == 0) {
		// Just copy the bitmap straight away.
		for (int32 i=0;i<bits_length;i++)
			*target_bits++ = *source_bits++;
	}
	else if (inset > 0) {
		// Just leave some of the edge away from the copy
		int32 width = new_bounds.IntegerWidth();
		int32 height = new_bounds.IntegerHeight();
		for (int32 y=0;y<=height;y++) {
			for (int32 x=0;x<=width;x++) {
				*target_bits++ = *(source_bits + inset+x + (inset+y)*source_bpr);
			}
		}
	}
	else if (inset < 0) {
		// This is the hardest case where we have to duplicate the edges.
		// First duplicate the top row
		int32 width = bounds.IntegerWidth();
		int32 height = bounds.IntegerHeight();
		for (int32 y=0;y<abs(inset);y++) {
			// Duplicate the start of the row
			for (int32 addx=0;addx<abs(inset);addx++) {
				*target_bits++ = *source_bits;
			}
			// Duplicate the actual row
			for (int32 x=0;x<=width;x++) {
				*target_bits++ = *source_bits++;
			}
			// Duplicate the end of the row
			source_bits--;
			for (int32 addx=0;addx<abs(inset);addx++) {
				*target_bits++ = *source_bits;
			}
			source_bits = source_bits + 1 - source_bpr;
		}
		// Then duplicate the actual bitmap
		for (int32 y=0;y<=height;y++) {
			// Duplicate the start of the row
			for (int32 addx=0;addx<abs(inset);addx++) {
				*target_bits++ = *source_bits;
			}
			// Duplicate the actual row
			for (int32 x=0;x<=width;x++) {
				*target_bits++ = *source_bits++;
			}
			// Duplicate the end of the row
			source_bits--;
			for (int32 addx=0;addx<abs(inset);addx++) {
				*target_bits++ = *source_bits;
			}
			source_bits++;
		}
		// Then duplicate the last row
		source_bits = source_bits - source_bpr;
		for (int32 y=0;y<abs(inset);y++) {
			// Duplicate the start of the row
			for (int32 addx=0;addx<abs(inset);addx++) {
				*target_bits++ = *source_bits;
			}
			// Duplicate the actual row
			for (int32 x=0;x<=width;x++) {
				*target_bits++ = *source_bits++;
			}
			// Duplicate the end of the row
			source_bits--;
			for (int32 addx=0;addx<abs(inset);addx++) {
				*target_bits++ = *source_bits;
			}
			source_bits = source_bits + 1 - source_bpr;
		}
	}

	return copy;
}


