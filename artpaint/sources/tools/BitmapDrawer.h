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

class Selection;

class BitmapDrawer {
	BRect 	bitmap_bounds;

	uint32	*bitmap_bits;
	int32	bitmap_bpr;
	int32	bitmap_data_length;


		float	MinimumCrossingPoint(BPoint&,BPoint&,int32);
		float	MaximumCrossingPoint(BPoint&,BPoint&,int32);
inline	uint32	MixColors(uint32 c1,uint32 c2,float mix);

public:
			BitmapDrawer(BBitmap*);

status_t	DrawHairLine(BPoint,BPoint,uint32,bool anti_alias=TRUE,Selection *sel=NULL);
status_t	DrawHairLine(BPoint,BPoint,uint32,float,bool anti_alias=TRUE,Selection *sel=NULL);
status_t	DrawLine(BPoint,BPoint,uint32,float,bool anti_alias=TRUE,Selection *sel=NULL);
status_t	DrawCircle(BPoint,float,uint32,bool fill=TRUE,bool anti_alias=TRUE,Selection *sel=NULL);
status_t	DrawEllipse(BRect,uint32,bool fill=TRUE,bool anti_alias=TRUE,Selection *sel=NULL);
status_t	DrawBitmap(BBitmap*,BRect,BRect,bool use_alpha=TRUE);

status_t	DrawConvexPolygon(BPoint*,int32,uint32,bool fill=TRUE,bool anti_alias=TRUE);
status_t	DrawRectanglePolygon(BPoint*,uint32,bool fill=TRUE,bool anti_alias=TRUE,Selection *sel=NULL);

status_t	FillAntiAliasedRectangle(BPoint*,uint32,Selection*);
status_t	FillRectangle(BPoint*,uint32,Selection*);

// These BPoint versions check that the point is within bitmap's bounds.
status_t	SetPixel(BPoint,uint32);
status_t	SetPixel(BPoint,uint32,Selection*);
void		SetPixel(int32,int32,uint32,Selection*);
uint32		GetPixel(BPoint);

// These versions do not check that the point is within bitmap's bounds.
inline	void		SetPixel(int32,int32,uint32);
inline	uint32		GetPixel(int32,int32);

};

inline void BitmapDrawer::SetPixel(int32 x, int32 y, uint32 color)
{
	*(bitmap_bits + x + y*bitmap_bpr) = color;
}

inline uint32 BitmapDrawer::GetPixel(int32 x,int32 y)
{
	return *(bitmap_bits + x + y*bitmap_bpr);
}

inline uint32 BitmapDrawer::MixColors(uint32 c1, uint32 c2, float mix)
{
	// Mixes two colors. c1 will get the weight mix and c2 1.0-mix.
	float inv_mix = 1.0-mix;

	return 	(((uint32)(((c1 >> 24) & 0xFF) * mix)<<24) + ((uint32)(((c2 >> 24) & 0xFF) * inv_mix)<<24)) |
			(((uint32)(((c1 >> 16) & 0xFF) * mix)<<16) + ((uint32)(((c2 >> 16) & 0xFF) * inv_mix)<<16)) |
			(((uint32)(((c1 >> 8) & 0xFF) * mix)<<8) + ((uint32)(((c2 >> 8) & 0xFF) * inv_mix)<<8)) |
			(((uint32)(((c1) & 0xFF) * mix)) + ((uint32)(((c2) & 0xFF) * inv_mix)));


}

inline float round_float(float c)
{
	return (((c - floor(c)) > 0.5) ? ceil(c) : floor(c));
}



#endif
