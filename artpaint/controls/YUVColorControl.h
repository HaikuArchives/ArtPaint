/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef YUV_COLOR_CONTROL_H
#define YUV_COLOR_CONTROL_H


#include "MultichannelColorControl.h"


class YUVColorControl : public MultichannelColorControl {
public:
 		YUVColorControl(rgb_color c);

virtual	void		SetValue(rgb_color c);
 			rgb_color	ValueAsColor();

private:
virtual	void		_SetColor(float one, float two,
 								float three, float four);

inline		void		yuv2rgb(float y, float u, float v,
							float& r, float& g, float& b);
inline		void		rgb2yuv(float r, float g, float b,
							float& y, float& u, float& v);
};


inline void
YUVColorControl::yuv2rgb(float y, float u, float v,
	float& r, float& g, float& b)
{
	u -= 128;
	v -= 128;

	/*
	b = min_c(255, max_c(0.0, y + 1.14 * v));
	g = min_c(255, max_c(0.0, y - 0.3455 * u - 0.7169 * v));
 	r = min_c(255, max_c(0.0, y + 2.028 * u));
	*/

	//b = min_c(255, max_c(0.0, y + 1.14 * v));
	//g = min_c(255, max_c(0.0, y - 0.395 * u - 0.581 * v));
 	//r = min_c(255, max_c(0.0, y + 2.032 * u));

 	uint32 R = (int32)(298*y + 409*u + 128) >> 8;
	uint32 G = (int32)(298*y - 100*u - 208*v + 128) >> 8;
	uint32 B = (int32)(298*y + 516*u + 128) >> 8;

	r = (float)R;
	g = (float)G;
	b = (float)B;
}


inline void
YUVColorControl::rgb2yuv(float r, float g, float b,
	float& y, float& u, float& v)
{

	y = 0.299 * r + 0.587 * g + 0.114 * b;
	u = -0.147 * r - 0.289 * g + 0.436 * b + 128;
	v = 0.615 * r - 0.515 * g - 0.1 * b + 128;

	uint32 Y = ((int32)(66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
	uint32 U = ((int32)(-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
	uint32 V = ((int32)(112* r - 94*g - 18* b + 128) >> 8) + 128;

	y = (float)Y;
	u = (float)U;
	v = (float)V;

	/*y = 0.299 * r + 0.587 * g + 0.114 * b; // + 16;
 	u = -0.168736 * r - 0.331264 * g + 0.5 * b + 128;
 	v = 0.5 * r - 0.418688 * g - 0.081312 * b + 128;
	*/

	/*y = 0.299 * r + 0.587 * g + 0.114 * b; // + 16;
 	u = 0.492 * (b - y) + 128;
 	v = 0.877 * (r - y) + 128;
	*/
	/*y = 0.257 * r + 0.504 * g + 0.098 * b; // + 16;
 	u = -0.147 * r - 0.289 * g + 0.437 * b + 128;
 	v = 0.615 * r - 0.515 * g - 0.1 * b + 128;
	*/
}


#endif
