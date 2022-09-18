/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef HSV_COLOR_CONTROL_H
#define HSV_COLOR_CONTROL_H


#include "MultichannelColorControl.h"


class HSVColorControl : public MultichannelColorControl {
public:
 		HSVColorControl(rgb_color c);

virtual	void		SetValue(rgb_color c);
		void		SetValue(float one, float two,
						float three, float four);
 		rgb_color	ValueAsColor();
virtual	void		SetSliderColors(rgb_color c);

private:
inline	void		hsv2rgb(float h, float s, float v,
							float& r, float& g, float& b);
inline	void		rgb2hsv(float r, float g, float b,
							float& h, float& s, float& v);
};


inline void
HSVColorControl::hsv2rgb(float h, float s, float v,
	float& r, float &g, float &b)
{
 	if (s <= 0) {
 		r = v;
 		g = v;
 		b = v;
 	}

	h /= 60.;
 	uint8 i = (uint8)floor(h);
 	float ff = h - i;
 	float p, q, t;
 	p = v * (1.0 - s);
 	q = v * (1.0 - (s * ff));
 	t = v * (1.0 - (s * (1.0 - ff)));

 	switch (i) {
 		case 0: {
 			r = v;
 			g = t;
 			b = p;
 		} break;
 		case 1: {
 			r = q;
 			g = v;
 			b = p;
 		} break;
 		case 2: {
 			r = p;
 			g = v;
 			b = t;
 		} break;
 		case 3: {
 			r = p;
 			g = q;
 			b = v;
 		} break;
 		case 4: {
 			r = t;
 			g = p;
 			b = v;
 		} break;
 		default: {
 			r = v;
 			g = p;
 			b = q;
 		} break;
 	}

 	r *= 255;
 	g *= 255;
 	b *= 255;
}


inline void
HSVColorControl::rgb2hsv(float r, float g, float b,
	float& h, float &s, float &v)
{
	r /= 255.;
	g /= 255.;
	b /= 255.;

 	float min = min_c(b, min_c(r, g));
 	float max = max_c(b, max_c(r, g));

 	v = max;
 	float delta = max - min;
 	if (delta < 0.00001) {
 		h = 0;
 		s = 0;
 	} else {
 		if (max > 0.0) {
 			s = delta / max;
			float r_dist = (max - r) / delta;
			float g_dist = (max - g) / delta;
			float b_dist = (max - b) / delta;

			if (r == max)
				h = b_dist - g_dist;
			else if (g == max)
				h = 2 + r_dist - b_dist;
			else
				h = 4 + g_dist - r_dist;
 		} else {
 			h = 0;
 			s = 0;
 		}
 	}

 	h = h * 60;
 	if (h < 0.0)
 		h += 360.0;
}


 #endif
