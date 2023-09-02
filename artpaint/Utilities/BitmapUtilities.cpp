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


#include "HSPolygon.h"
#include "PixelOperations.h"
#include "UtilityClasses.h"


#include <Screen.h>


status_t
BitmapUtilities::FixMissingAlpha(BBitmap *bitmap)
{
	uint32 *bits = (uint32*)bitmap->Bits();
	int32 bits_length = bitmap->BitsLength()/4;

	if (bitmap->ColorSpace() != B_RGB32)
		return B_BAD_TYPE;

	union color_conversion c;

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
	} else if (wantSpace == B_RGBA32) {
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

				union color_conversion c;

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
	} else if (wantSpace == B_GRAY8) {
		switch (inBitmap->ColorSpace()) {
			case B_GRAY8:
				return inBitmap;

			case B_RGBA32:
			{
				BBitmap* out_map = new BBitmap(inBitmap->Bounds(), wantSpace);
				uint8* out_bits = (uint8*)out_map->Bits();
				int32 out_bpr = out_map->BytesPerRow();

				uint32 *in_bits = (uint32*)inBitmap->Bits();
				int32 in_bpr = inBitmap->BytesPerRow()/4;

				union color_conversion c;

				out_map->LockBits();
				uint32 pos = 0;
				for (int32 y = 0; y <= out_map->Bounds().IntegerHeight(); y++) {
					for (int32 x = 0; x <= out_map->Bounds().IntegerWidth(); x++) {
						c.word = *(in_bits + x + y * in_bpr);
						float alpha = (float)c.bytes[3] / 255.;
						float value = 0.21 * ((float)c.bytes[2] / 255.) +
							0.72 * ((float)c.bytes[1] / 255.) +
							0.07 * ((float)c.bytes[0] / 255.);

						*(out_bits + x + y * out_bpr) = (uint8)(value * alpha * 255);
					}

				}
				out_map->UnlockBits();

				return out_map;
			}

			default:
				return NULL;
		}
	} else {
		return NULL;
	}
}


BBitmap*
BitmapUtilities::ConvertToMask(BBitmap *inBitmap, uint8 color)
{
	BBitmap* out_map = new BBitmap(inBitmap->Bounds(), B_GRAY8);
	uint8* out_bits = (uint8*)out_map->Bits();
	int32 out_bpr = out_map->BytesPerRow();

	uint32 *in_bits = (uint32*)inBitmap->Bits();
	int32 in_bpr = inBitmap->BytesPerRow()/4;

	union color_conversion c;

	out_map->LockBits();
	uint32 pos = 0;
	for (int32 y = 0; y < out_map->Bounds().IntegerHeight() + 1; y++) {
		for (int32 x = 0; x < out_map->Bounds().IntegerWidth() + 1; x++) {
			c.word = *(in_bits + x + y * in_bpr);
			// revert this change when the rest of ArtPaint understands
			// selections with alpha
			float alpha = 0;
			if (c.bytes[3] > 0x00)
				alpha = 1;

			*(out_bits + x + y * out_bpr) = (uint8)((float)color * alpha);
		}

	}
	out_map->UnlockBits();

	return out_map;
}


void
BitmapUtilities::CompositeBitmapOnSource(BBitmap* toBuffer, BBitmap* srcBuffer, BBitmap* fromBuffer,
	BRect updated_rect, uint32 (*composite_func)(uint32, uint32), uint32 color)
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
	src_bits += bpr * start_y + start_x;
	from_bits += bpr * start_y + start_x;

	for (int y = 0; y < height; y++) {
		int32 ypos = y*bpr;
		for (int x = 0; x < width; x++) {
			union color_conversion from_color, mix_color;
			from_color.word = *(from_bits + x + ypos);
			mix_color.word = color;
			for (int i = 0; i < 4; ++i)
				from_color.bytes[i] = (uint8)((from_color.bytes[i] *
					mix_color.bytes[i]) / 255);

			*bits++ = (*composite_func)(*(src_bits + x + ypos),
				from_color.word);
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
		BRect tmpArea = *area & bitmap->Bounds();

		start_x = tmpArea.left;
		start_y = tmpArea.top;

		if (start_x > width || start_y > height)
			return;

		width = min_c(width, tmpArea.IntegerWidth()+1);
		height = min_c(height, tmpArea.IntegerHeight()+1);
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


uint32
BitmapUtilities::GetPixel(BBitmap* bitmap, int32 x, int32 y)
{
	uint32 bpr;
	uint32 value;

	if (bitmap->Bounds().Contains(BPoint(x, y)) == false)
		return 0;

	if (bitmap->ColorSpace() == B_GRAY8) {
		bpr = bitmap->BytesPerRow();
		uint8* bits = (uint8*)bitmap->Bits();

		return (uint32)*(bits + x + y * bpr);

	} else if (bitmap->ColorSpace() == B_RGBA32) {
		bpr = bitmap->BytesPerRow() / 4;
		uint32* bits = (uint32*)bitmap->Bits();

		return (uint32)*(bits + x + y * bpr);
	}

	return 0;
}


uint32
BitmapUtilities::GetPixel(BBitmap* bitmap, BPoint location)
{
	return GetPixel(bitmap, (int32)location.x, (int32)location.y);
}


void
BitmapUtilities::RasterToPolygonsMoore(BBitmap* bitmap, BRect bounds,
	BList* polygons)
{
	// This function uses the selection_map to make
	// a new set of polygons that make up the selection.
	// The selection_map is searched for the edges of selections
	// and then those edges are traced to form the polygons
	// using the Moore's neighborhood walk algorithm
	int32 leftb = (int32)max_c(bounds.left - 1, 0);
	int32 rightb = (int32)min_c(bounds.right + 1, bitmap->Bounds().IntegerWidth());
	int32 topb = (int32)max_c(bounds.top - 1, 0);
	int32 bottomb = (int32)min_c(bounds.bottom + 1, bitmap->Bounds().IntegerHeight());

	BPoint neighbors[8] = {
		BPoint( -1,  0),
		BPoint( -1, -1),
		BPoint(  0, -1),
		BPoint(  1, -1),
		BPoint(  1,  0),
		BPoint(  1,  1),
		BPoint(  0,  1),
		BPoint( -1,  1)
	};

	int next[8] = {7, 7, 1, 1, 3, 3, 5, 5};

	PointContainer* included_points = new PointContainer();

	bool inside = false;
	bool border = true;
	BPoint pos;

	for (int32 y = topb; y <= bottomb; y++) {
		for (int32 x = leftb; x <= rightb; x++) {
			// We start a new polygon if point at x,y is at the edge and
			// has not been included in previous polygons.

			// if black pixel found and previous is white, then trace
			// clockwise with moore
			// if black pixel found and following pixel is white then trace
			// counterclockwise with moore as it should be a hole

			pos = BPoint(x, y);

			bool containsPoint = (BitmapUtilities::GetPixel(bitmap, (int32)x, (int32)y) != 0x00);

			if (included_points->HasPoint(x, y) && inside == false)
				inside = true;
			else if (containsPoint == true && inside == true)
				continue;
			else if (containsPoint == false && inside == true) {
				inside = false;
				if ((x - 1) >= 0 && included_points->HasPoint(x - 1, y))
					border = true;
				else
					border = !border;
			} else if (containsPoint == true && inside == false) {
				included_points->InsertPoint(x - 0.5, y - 0.5);

				int32 max_point_count = 1024;
				// set B to be empty
				BPoint* point_list = new BPoint[max_point_count];
				int32 point_count = 0;

				// insert s in B - s = (x, y)
				point_list[point_count++] = BPoint(x - 0.5, y - 0.5);

				int checkLocation = 1;
				int newCheckLocation = 0;
				BPoint startPos = BPoint(pos);
				int end_counter = 0;
				int point_counter = 0;

				while(true) {
					BPoint check = pos + neighbors[checkLocation - 1];
					int newCheckLocation = next[checkLocation - 1];

					bool containsPoint = (GetPixel(bitmap,
						(int32)check.x, (int32)check.y) != 0x00);
					if (containsPoint) {
						int prevCheckLocation = checkLocation;
						checkLocation = newCheckLocation;
						pos = check;
						point_counter = 0;

						if (prevCheckLocation == 5) {
							if (included_points->HasPoint(check.x - 0.5, check.y - 0.5) == false) {
								included_points->InsertPoint(check.x - 0.5, check.y - 0.5);
								point_list[point_count++] = BPoint(check.x - 0.5, check.y - 0.5);
							}
							if (included_points->HasPoint(check.x + 0.5, check.y - 0.5) == false) {
								included_points->InsertPoint(check.x + 0.5, check.y - 0.5);
								point_list[point_count++] = BPoint(check.x + 0.5, check.y - 0.5);
							}
						} else if (prevCheckLocation == 7) {
							if (included_points->HasPoint(check.x + 0.5, check.y - 0.5) == false) {
								point_list[point_count++] = BPoint(check.x + 0.5, check.y - 0.5);
								included_points->InsertPoint(check.x + 0.5, check.y - 0.5);
							}
							if (included_points->HasPoint(check.x + 0.5, check.y + 0.5) == false) {
								point_list[point_count++] = BPoint(check.x + 0.5, check.y + 0.5);
								included_points->InsertPoint(check.x + 0.5, check.y + 0.5);
							}
						} else if (prevCheckLocation == 3) {
							if (included_points->HasPoint(check.x - 0.5, check.y + 0.5) == false) {
								point_list[point_count++] = BPoint(check.x - 0.5, check.y + 0.5);
								included_points->InsertPoint(check.x - 0.5, check.y + 0.5);
							}
							if (included_points->HasPoint(check.x - 0.5, check.y - 0.5) == false) {
								point_list[point_count++] = BPoint(check.x - 0.5, check.y - 0.5);
								included_points->InsertPoint(check.x - 0.5, check.y - 0.5);
							}
						} else if (prevCheckLocation == 1) {
							if (included_points->HasPoint(check.x + 0.5, check.y + 0.5) == false) {
								point_list[point_count++] = BPoint(check.x + 0.5, check.y + 0.5);
								included_points->InsertPoint(check.x + 0.5, check.y + 0.5);
							}
							if (included_points->HasPoint(check.x - 0.5, check.y + 0.5) == false) {
								point_list[point_count++] = BPoint(check.x - 0.5, check.y + 0.5);
								included_points->InsertPoint(check.x - 0.5, check.y + 0.5);
							}
						} else if (prevCheckLocation == 6) {
							if (included_points->HasPoint(check.x - 0.5, check.y - 1.5) == false) {
								point_list[point_count++] = BPoint(check.x - 0.5, check.y - 1.5);
								included_points->InsertPoint(check.x - 0.5, check.y - 1.5);
							}
							if (included_points->HasPoint(check.x - 0.5, check.y - 0.5) == false) {
								point_list[point_count++] = BPoint(check.x - 0.5, check.y - 0.5);
								included_points->InsertPoint(check.x - 0.5, check.y - 0.5);
							}
							if (included_points->HasPoint(check.x + 0.5, check.y - 0.5) == false) {
								point_list[point_count++] = BPoint(check.x + 0.5, check.y - 0.5);
								included_points->InsertPoint(check.x + 0.5, check.y - 0.5);
							}
						} else if (prevCheckLocation == 8) {
							if (included_points->HasPoint(check.x + 1.5, check.y - 0.5) == false) {
								point_list[point_count++] = BPoint(check.x + 1.5, check.y - 0.5);
								included_points->InsertPoint(check.x + 1.5, check.y - 0.5);
							}
							if (included_points->HasPoint(check.x + 0.5, check.y - 0.5) == false) {
								point_list[point_count++] = BPoint(check.x + 0.5, check.y - 0.5);
								included_points->InsertPoint(check.x + 0.5, check.y - 0.5);
							}
							if (included_points->HasPoint(check.x + 0.5, check.y + 0.5) == false) {
								point_list[point_count++] = BPoint(check.x + 0.5, check.y + 0.5);
								included_points->InsertPoint(check.x + 0.5, check.y + 0.5);
							}
						} else if (prevCheckLocation == 2) {
							if (included_points->HasPoint(check.x + 0.5, check.y + 1.5) == false) {
								point_list[point_count++] = BPoint(check.x + 0.5, check.y + 1.5);
								included_points->InsertPoint(check.x + 0.5, check.y + 1.5);
							}
							if (included_points->HasPoint(check.x + 0.5, check.y + 0.5) == false) {
								point_list[point_count++] = BPoint(check.x + 0.5, check.y + 0.5);
								included_points->InsertPoint(check.x + 0.5, check.y + 0.5);
							}
							if (included_points->HasPoint(check.x - 0.5, check.y + 0.5) == false) {
								point_list[point_count++] = BPoint(check.x - 0.5, check.y + 0.5);
								included_points->InsertPoint(check.x - 0.5, check.y + 0.5);
							}
						} else if (prevCheckLocation == 4) {
							if (included_points->HasPoint(check.x - 1.5, check.y + 0.5) == false) {
								point_list[point_count++] = BPoint(check.x - 1.5, check.y + 0.5);
								included_points->InsertPoint(check.x - 1.5, check.y + 0.5);
							}
							if (included_points->HasPoint(check.x - 0.5, check.y + 0.5) == false) {
								point_list[point_count++] = BPoint(check.x - 0.5, check.y + 0.5);
								included_points->InsertPoint(check.x - 0.5, check.y + 0.5);
							}
							if (included_points->HasPoint(check.x - 0.5, check.y - 0.5) == false) {
								point_list[point_count++] = BPoint(check.x - 0.5, check.y - 0.5);
								included_points->InsertPoint(check.x - 0.5, check.y - 0.5);
							}
						}

						if (check == startPos) {
							++end_counter;

							if (newCheckLocation == 1 ||
								end_counter >= 3) {
								inside = true;
								break;
							}
						}
					} else {
						checkLocation = 1 + (checkLocation % 8);
						if (point_counter > 8) {
							point_counter = 0;
							break;
						} else
							++point_counter;
					}

					if (point_count + 5 >= max_point_count) {
						max_point_count *= 2;
						BPoint* new_points = new BPoint[max_point_count];
						for (int32 i = 0; i < point_count; i++)
							new_points[i] = point_list[i];

						delete[] point_list;
						point_list = new_points;
					}
				}

				if (point_count > 1) {
					HSPolygon* new_polygon = NULL;
					if (border == true)
						new_polygon = new HSPolygon(point_list, point_count,
							HS_POLYGON_CLOCKWISE);
					else
						new_polygon = new HSPolygon(point_list, point_count,
							HS_POLYGON_COUNTERCLOCKWISE);

					// algorithm makes the polygon 0.5 pixels
					// too large in both dimensions, so scale it
					// down by 1 pixel
					BPoint centroid = BPoint(0,0);
					for (int32 i=0;i<point_count;i++) {
						centroid += point_list[i];
					}
					centroid.x /= point_count;
					centroid.y /= point_count;
					BRect bounds = new_polygon->BoundingBox();

					float xscale = (bounds.Width() - 1) / (bounds.Width());
					float yscale = (bounds.Height() - 1) / (bounds.Height());

					new_polygon->ScaleBy(centroid, xscale, yscale);

					if (new_polygon != NULL) {
						polygons->AddItem(new_polygon);
					}
				}

				delete[] point_list;
			}
		}
	}

	delete included_points;
}
