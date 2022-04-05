/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "BitmapUtilities.h"

#include <stdio.h>

#include <Screen.h>



status_t
BitmapUtilities::FixMissingAlpha(BBitmap *bitmap)
{
	uint32 *bits = (uint32*)bitmap->Bits();
	int32 bits_length = bitmap->BitsLength()/4;

	if (bitmap->ColorSpace() != B_RGB32)
		return B_BAD_TYPE;


	union {
		uint8 bytes[4];
		uint32 word;
	} c;

	for (int32 i=0;i<bits_length;i++) {
		c.word = *bits++;
		if (c.bytes[3] != 0) {
			return B_OK;
		}
	}

	bits = (uint32*)bitmap->Bits();
	c.word = 0x00000000;
	c.bytes[3] = 255;
	for (int32 i=0;i<bits_length;i++) {
		 *bits = *bits | c.word;
		bits++;
	}

	return B_OK;
}



BBitmap*
BitmapUtilities::ConvertColorSpace(BBitmap *inBitmap, color_space wantSpace)
{
	if (inBitmap->ColorSpace() == wantSpace) {
		return inBitmap;
	}
	else if (wantSpace == B_RGBA32) {
		switch (inBitmap->ColorSpace()) {
			case B_RGB32:
				return inBitmap;

			case B_CMAP8:
			{
				BBitmap *out_map = new BBitmap(inBitmap->Bounds(),wantSpace);
				uint32 *out_bits = (uint32*)out_map->Bits();
				int32 out_bpr = out_map->BytesPerRow()/4;

				uint8 *in_bits = (uint8*)inBitmap->Bits();
				int32 in_bpr = inBitmap->BytesPerRow();

				union {
					uint8 bytes[4];
					uint32 word;
				} c;

				c.bytes[3] = 0xff;

				BScreen screen;
				const rgb_color *color_list = screen.ColorMap()->color_list;

				for (int32 y=0;y<out_map->Bounds().IntegerHeight();y++) {
					for (int32 x=0;x<out_map->Bounds().IntegerWidth();x++) {
						rgb_color color = color_list[*(in_bits + x + y*in_bpr)];
						c.bytes[0] = color.blue;
						c.bytes[1] = color.green;
						c.bytes[2] = color.red;

						*(out_bits + x + y*out_bpr) = c.word;
					}
				}

				delete inBitmap;

				return out_map;
			}

			default:
				return NULL;
		}
	}
	else {
		return NULL;
	}
}
