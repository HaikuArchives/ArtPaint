/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef UTILITY_CLASSES_H
#define UTILITY_CLASSES_H

#include <View.h>


#include <stack>

#define B_UNUSED(x) (void)x;

class BitmapView : public BView {
public:
						BitmapView(BBitmap* bitmap, BRect frame);
						BitmapView(BBitmap* bitmap, BPoint leftTop);
	virtual				~BitmapView();

	virtual	void		AttachedToWindow();
	virtual	void		Draw(BRect updateRect);

			BBitmap*	Bitmap() const;
			void		SetBitmap(BBitmap* bitmap);

private:
			BBitmap*	fBitmap;
};


class PointStack : public std::stack<BPoint> {
public:
						PointStack() {}
						~PointStack() {}

			bool		IsEmpty() const { return empty(); }
			void		Push(const BPoint& point) { push(point); }
			BPoint		Pop() { BPoint point = top(); pop(); return point; }
};


BRect FitRectToScreen(BRect source);
BRect CenterRectOnScreen(BRect source);
BRect MakeRectFromPoints(const BPoint& point1, const BPoint& point2);
float SnapToAngle(const float snap_angle, const float src_angle,
	const float max_angle = 90.);


inline uint32
RGBColorToBGRA(rgb_color c)
{
	union {
		char bytes[4];
		uint32 word;
	} color;

	color.bytes[0] = c.blue;
	color.bytes[1] = c.green;
	color.bytes[2] = c.red;
	color.bytes[3] = c.alpha;

	return color.word;
}


inline rgb_color
BGRAColorToRGB(uint32 bgra_color)
{
	rgb_color c;
#if __POWERPC__
	c.red = (bgra_color >> 8) & 0xFF;
	c.green = (bgra_color >> 16) & 0xFF;
	c.blue = (bgra_color >> 24) & 0xFF;
	c.alpha = (bgra_color) & 0xFF;
#else
	c.red = (bgra_color >> 16) & 0xFF;
	c.green = (bgra_color >> 8) & 0xFF;
	c.blue = (bgra_color >> 0) & 0xFF;
	c.alpha = (bgra_color >> 24) & 0xFF;
#endif
	return c;
}

#endif
