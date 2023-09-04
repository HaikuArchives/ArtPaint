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
					float angle = 0,
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


	status_t    _DrawShearedEllipse(BPoint center, float width, float height, uint32 color,
                    bool fill, bool anti_alias, float shear_dx, float shear_dy, Selection* sel,
                    uint32 (*composite_func)(uint32, uint32));
    void        _SetShearedPixel(int32 x, int32 y, int32 dx, int32 dy,
                    float shear_dx, float shear_dy, uint32 color,
                    Selection* sel, uint32 (*composite_func)(uint32, uint32));
    void        _FillShearedColumn(int32 x, int32 y, int32 dx, int32 dy0, int32 dy1,
                    float shear_dx, float shear_dy, uint32 color,
                    Selection* sel, uint32 (*composite_func)(uint32, uint32));
    void        _FillShearedRow(int32 x, int32 y, int32 dx0, int32 dx1, int32 dy,
                    float shear_dx, float shear_dy, uint32 color,
                    Selection* sel, uint32 (*composite_func)(uint32, uint32));
};


inline float round_float(float c)
{
	return (((c - floor(c)) > 0.5) ? ceil(c) : floor(c));
}


#endif // BITMAP_DRAWER_H
