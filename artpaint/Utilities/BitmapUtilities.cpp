/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#include "BitmapUtilities.h"


#include "PixelOperations.h"


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


void
BitmapUtilities::CompositeBitmapOnSource(BBitmap* toBuffer, BBitmap* srcBuffer, BBitmap* fromBuffer,
	BRect updated_rect)
{
	updated_rect = updated_rect & toBuffer->Bounds();

	int32 bpr = toBuffer->BytesPerRow() / 4;
	int32 width = updated_rect.IntegerWidth()+1;
	int32 height = updated_rect.IntegerHeight()+1;

	int32 start_x, start_y;
	start_x = (int32)updated_rect.left;
	start_y = (int32)updated_rect.top;

	uint32* bits = (uint32*)toBuffer->Bits();
	bits += bpr*start_y + start_x;

	uint32* src_bits = (uint32*)srcBuffer->Bits();
	uint32* from_bits = (uint32*)fromBuffer->Bits();
	src_bits += bpr*start_y + start_x;
	from_bits += bpr*start_y + start_x;

	for (int y=0;y<height;y++) {
		int32 ypos = y*bpr;
		for (int x=0;x<width;x++) {
			*bits++ = src_over_fixed(*(src_bits + x + ypos),
				*(from_bits + x + ypos));
		}
		bits += bpr - width;
	}
}


void
BitmapUtilities::ClearBitmap(BBitmap* bitmap, uint32 color, BRect* area)
{
	uint32 width = bitmap->Bounds().IntegerWidth()+1;
	uint32 height = bitmap->Bounds().IntegerHeight()+1;
	uint32 bpr = bitmap->BytesPerRow() / 4;

	int32 start_x = 0;
	int32 start_y = 0;

	if (area) {
		width = area->IntegerWidth()+1;
		height = area->IntegerHeight()+1;
		start_x = (int32)area->left;
		start_y = (int32)area->top;
	}

	uint32* bits = (uint32*)bitmap->Bits();
	bits += start_x + bpr * start_y;

	for (int y = 0;y < height;++y) {
		for (int x = 0;x < width;++x) {
			*bits++ = color;
		}
		bits += bpr - width;
	}
}


void
BitmapUtilities::CheckerBitmap(BBitmap* bitmap,
	uint32 color1, uint32 color2, uint32 grid_size,
	BRect* area)
{
	uint32 width = bitmap->Bounds().IntegerWidth()+1;
	uint32 height = bitmap->Bounds().IntegerHeight()+1;
	uint32 bpr = bitmap->BytesPerRow() / 4;

	int32 start_x = 0;
	int32 start_y = 0;

	if (area) {
		*area = *area & bitmap->Bounds();
		width = area->IntegerWidth()+1;
		height = area->IntegerHeight()+1;
		if (width > bitmap->Bounds().IntegerWidth()+1)
			return;
		if (height > bitmap->Bounds().IntegerHeight()+1)
			return;
		start_x = (int32)area->left;
		start_y = (int32)area->top;
	}

	uint32 grid_color[2] = { color1, color2 };
	uint32 cur_color = grid_color[0];

	uint32* bits = (uint32*)bitmap->Bits();
	bits += start_x + bpr * start_y;
	uint32 row_size = bpr - width;

	for (int y = start_y;y < height+start_y;++y) {
		int rowMod2 = (y / grid_size) % 2;
		for (int x = start_x;x < width+start_x;++x) {
			int col = x / grid_size;
			if (rowMod2 == col % 2)
				cur_color = grid_color[1];
			else
				cur_color = grid_color[0];

			*bits++ = cur_color;
		}
		bits += row_size;
	}
}
