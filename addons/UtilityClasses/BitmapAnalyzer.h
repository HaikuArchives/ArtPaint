/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef BITMAP_ANALYZER_H
#define	BITMAP_ANALYZER_H

/*
	This class has functions that can be used to report information
	about a bitmap. Currently it contains the following functions:
		float	GradientMagnitude(BPoint p)
		float	GradientMagnitude(int32 x, int32 y)

		BPoint	GradientDirection(BPoint p)
		BPoint	GradientDirection(int32 x, int32 y)
*/

class BitmapAnalyzer {
		bool	buffered;

		uint32	*bits;
		int32	bpr;
		int32	height;

		BRect	bounds;
public:
		BitmapAnalyzer(BBitmap*,bool heavy_access=false);



inline	float	GradientMagnitude(BPoint);
inline	float	GradientMagnitude(int32,int32);

inline	BPoint	GradientDirection(BPoint);
inline	BPoint	GradientDirection(int32,int32);
};


BitmapAnalyzer::BitmapAnalyzer(BBitmap *bitmap,bool heavy_access)
{
	buffered = false;

	bits = (uint32*)bitmap->Bits();
	bpr = bitmap->BytesPerRow() / 4;
	height = bitmap->Bounds().IntegerHeight();

	bounds = bitmap->Bounds();
}



float BitmapAnalyzer::GradientMagnitude(BPoint point)
{
	if (!bounds.Contains(point))
		return 0;
	else
		return GradientMagnitude(point.x,point.y);
}





float BitmapAnalyzer::GradientMagnitude(int32 x,int32 y)
{
	// The eight surrounding pixel-values (luminance)
	float lt,t,rt;
	float l,r;
	float lb,b,rb;

	if (!buffered) {
		int32 left_x = x-1;
		int32 right_x = x+1;
		int32 top_y = y-1;
		int32 bottom_y = y+1;
		if (x<=0)
			left_x=0;
		if (x>=(bpr-1))
			right_x = bpr;

		if (y<=0)
			top_y = y;
		if (y >= height)
			bottom_y = height;

		union {
			uint8	bytes[4];
			uint32	word;
		} c;

		c.word = *(bits + left_x + top_y*bpr);
		lt = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];

		c.word = *(bits + x + top_y*bpr);
		t = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];

		c.word = *(bits + right_x + top_y*bpr);
		rt = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];

		c.word = *(bits + left_x + y*bpr);
		l = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];

		c.word = *(bits + right_x + y*bpr);
		r = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];

		c.word = *(bits + left_x + bottom_y*bpr);
		lb = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];

		c.word = *(bits + x + bottom_y*bpr);
		b = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];

		c.word = *(bits + right_x + bottom_y*bpr);
		rb = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];
	}
	else {

	}

	float y_magnitude = lt-lb + 2*(t-b) + rt-rb;
	float x_magnitude = lb-rb + 2*(l-r) + lt-rt;

	float length = sqrt(pow(x_magnitude,2)+pow(y_magnitude,2));

	return length;
}


BPoint BitmapAnalyzer::GradientDirection(BPoint point)
{
	BPoint dir(0,0);
	if (bounds.Contains(point))
		return dir;
	else
		return GradientDirection(point.x,point.y);
}


BPoint BitmapAnalyzer::GradientDirection(int32 x,int32 y)
{
	// The eight surrounding pixel-values (luminance)
	float lt,t,rt;
	float l,r;
	float lb,b,rb;

	if (!buffered) {
		int32 left_x = x-1;
		int32 right_x = x+1;
		int32 top_y = y-1;
		int32 bottom_y = y+1;
		if (x<=0)
			left_x=0;
		if (x>=(bpr-1))
			right_x = bpr;

		if (y<=0)
			top_y = y;
		if (y >= height)
			bottom_y = height;

		union {
			uint8	bytes[4];
			uint32	word;
		} c;

		c.word = *(bits + left_x + top_y*bpr);
		lt = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];

		c.word = *(bits + x + top_y*bpr);
		t = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];

		c.word = *(bits + right_x + top_y*bpr);
		rt = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];

		c.word = *(bits + left_x + y*bpr);
		l = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];

		c.word = *(bits + right_x + y*bpr);
		r = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];

		c.word = *(bits + left_x + bottom_y*bpr);
		lb = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];

		c.word = *(bits + x + bottom_y*bpr);
		b = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];

		c.word = *(bits + right_x + bottom_y*bpr);
		rb = 0.11*c.bytes[0] + 0.59*c.bytes[1] + 0.30*c.bytes[2];
	}
	else {

	}

	float y_magnitude = lt-lb + 2*(t-b) + rt-rb;
	float x_magnitude = lb-rb + 2*(l-r) + lt-rt;

	float length = sqrt(pow(x_magnitude,2)+pow(y_magnitude,2));
	BPoint direction(x_magnitude/length,y_magnitude/length);

	return direction;
}

#endif
