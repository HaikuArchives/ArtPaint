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


#include <String.h>
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


inline bool
HexStringToBGRA(BString hexColor, uint32& bgra_color)
{
	bool valid_color = FALSE;

	union {
		char bytes[4];
		uint32 word;
	} color;

	hexColor.ReplaceAll("#", "");
	hexColor.ToUpper();

	if (hexColor.Length() == 3 || hexColor.Length() == 4) {
		BString byteStr;
		uint8 byteVal;

		for (int i = 0; i < 3; ++i) {
			char digit = hexColor.ByteAt(i);
			byteStr.SetToFormat("%c%c", digit, digit);
			byteStr.ScanWithFormat("%X", &byteVal);
			color.bytes[2 - i] = byteVal;
		}

		if (hexColor.Length() == 4) {
			char digit = hexColor.ByteAt(3);
			byteStr.SetToFormat("%c%c", digit, digit);
			byteStr.ScanWithFormat("%X", &byteVal);
			color.bytes[3] = byteVal;
		} else
			color.bytes[3] = 0xFF;

		valid_color = TRUE;
	} else if (hexColor.Length() == 6 || hexColor.Length() == 8) {
		BString byteStr;
		uint8 byteVal;

		for (int i = 0; i < 3; ++i) {
			int j = i * 2;
			char digit1 = hexColor.ByteAt(j);
			char digit2 = hexColor.ByteAt(j + 1);
			byteStr.SetToFormat("%c%c", digit1, digit2);
			byteStr.ScanWithFormat("%x", &byteVal);
			color.bytes[2 - i] = byteVal;
		}
		if (hexColor.Length() == 8) {
			char digit1 = hexColor.ByteAt(6);
			char digit2 = hexColor.ByteAt(7);

			byteStr.SetToFormat("%c%c", digit1, digit2);
			byteStr.ScanWithFormat("%X", &byteVal);
			color.bytes[3] = byteVal;
		} else
			color.bytes[3] = 0xFF;
		valid_color = TRUE;
	}

	bgra_color = color.word;

	return valid_color;
}


#endif
