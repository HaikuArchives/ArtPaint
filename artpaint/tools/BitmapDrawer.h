/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef BITMAP_DRAWER_H
#define BITMAP_DRAWER_H

#include <Rect.h>
#include <Bitmap.h>

#include "PixelOperations.h"


class Selection;


class BitmapDrawer {
	BRect 		bitmap_bounds;

	uint32*		bitmap_bits;
	int32		bitmap_bpr;
	int32		bitmap_data_length;

	float		MinimumCrossingPoint(BPoint&, BPoint&, int32);
	float		MaximumCrossingPoint(BPoint&, BPoint&, int32);

public:
				BitmapDrawer(BBitmap*);

	status_t	DrawHairLine(BPoint, BPoint, uint32,
					bool anti_alias = TRUE, Selection* sel = NULL,
					uint32 (*composite_func)(uint32, uint32) = src_over_fixed);
	status_t	DrawLine(BPoint, BPoint, uint32, float,
					bool anti_alias = TRUE, Selection* sel = NULL,
					uint32 (*composite_func)(uint32, uint32) = src_over_fixed);
	status_t	DrawCircle(BPoint, float, uint32, bool fill = TRUE,
					bool anti_alias = TRUE, Selection* sel = NULL,
					uint32 (*composite_func)(uint32, uint32) = src_over_fixed);
	status_t	DrawEllipse(BRect, uint32, bool fill = TRUE,
					bool anti_alias = TRUE, Selection* sel = NULL,
					uint32 (*composite_func)(uint32, uint32) = src_over_fixed);
	status_t	DrawBitmap(BBitmap*, BRect, BRect, bool use_alpha = TRUE);

	status_t	DrawConvexPolygon(BPoint*, int32, uint32, bool fill = TRUE,
					bool anti_alias = TRUE);
	status_t	DrawRectanglePolygon(BPoint*, uint32, bool fill = TRUE,
					bool anti_alias = TRUE, Selection* sel = NULL,
					uint32 (*composite_func)(uint32, uint32) = src_over_fixed);

	status_t	FillAntiAliasedRectangle(BPoint*, uint32,
					Selection* sel = NULL,
					uint32 (*composite_func)(uint32, uint32) = src_over_fixed);
	status_t	FillRectangle(BPoint*, uint32,
					Selection* sel = NULL,
					uint32 (*composite_func)(uint32, uint32) = src_over_fixed);

	status_t	SetPixel(BPoint location, uint32 color,
					Selection* sel = NULL,
					uint32 (*composite_func)(uint32, uint32) = src_over_fixed);
	status_t	SetPixel(int32 x, int32 y, uint32 color,
					Selection* sel = NULL,
					uint32 (*composite_func)(uint32, uint32) = src_over_fixed);

	uint32		GetPixel(BPoint location);
	uint32		GetPixel(int32 x, int32 y);

	void 		SetMirroredPixels(BPoint center, uint32 x, uint32 y, uint32 color,
					Selection* sel = NULL,
					uint32 (*composite_func)(uint32, uint32) = src_over_fixed);
	void		FillColumn(BPoint center, uint32 x, uint32 miny, uint32 maxy,
					uint32 color, Selection* sel = NULL,
					uint32 (*composite_func)(uint32, uint32) = src_over_fixed);
	void 		FillRow(BPoint center, uint32 minx, uint32 maxx, uint32 y,
					uint32 color, Selection* sel = NULL,
					uint32 (*composite_func)(uint32, uint32) = src_over_fixed);
};


inline float round_float(float c)
{
	return (((c - floor(c)) > 0.5) ? ceil(c) : floor(c));
}


#endif // BITMAP_DRAWER_H
