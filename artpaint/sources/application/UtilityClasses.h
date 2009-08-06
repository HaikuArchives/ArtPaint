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

#include <Box.h>
#include <View.h>
#include <Window.h>


#include <stack>


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


BRect		FitRectToScreen(BRect);



inline uint32 RGBColorToBGRA(rgb_color c)
{
	union {
		char bytes[4];
		uint32 word;
	} color;

	color.bytes[0] = c.blue;
	color.bytes[1] = c.green;
	color.bytes[2] = c.red;
	color.bytes[3] = c.alpha;
//	return ((c.blue << 24) & 0xFF000000) | ((c.green << 16) & 0x00FF0000) | ((c.red << 8) & 0x0000FF00) | (c.alpha & 0xFF);
	return color.word;
}



#if __POWERPC__
inline rgb_color BGRAColorToRGB(uint32 bgra_color)
{
	rgb_color c;
	c.red = (bgra_color >> 8) & 0xFF;
	c.green = (bgra_color >> 16) & 0xFF;
	c.blue = (bgra_color >> 24) & 0xFF;
	c.alpha = (bgra_color) & 0xFF;

	return c;
}
#else
inline rgb_color BGRAColorToRGB(uint32 bgra_color)
{
	rgb_color c;
	c.red = (bgra_color >> 16) & 0xFF;
	c.green = (bgra_color >> 8) & 0xFF;
	c.blue = (bgra_color >> 0) & 0xFF;
	c.alpha = (bgra_color >> 24) & 0xFF;

	return c;
}
#endif


// This function makes a rect out of two points after sorting the points
BRect make_rect_from_points(BPoint&,BPoint&);

#endif
