/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef HSV_CONTROL_H
#define HSV_CONTROL_H

#include "VisualColorControl.h"

class HSVControl : public VisualColorControl {
float	h_value;
float	s_value;
float	v_value;


void	CalcRamps();

int32	value_at_1();
int32	value_at_2();
int32	value_at_3();
float	max_value_at_1() { return 360; }
float	max_value_at_2() { return 182; }
float	max_value_at_3() { return 255; }

float	min_value_at_1() { return 0; }
float	min_value_at_2() { return 0; }
float	min_value_at_3() { return 0; }

inline	void	hsv_to_rgb(float h,float s,float v,float *r,float *g,float *b);
inline	void	rgb_to_hsv(float r,float g,float b,float *h,float *s,float *v);

public:
		HSVControl(BPoint position, rgb_color c);

void	MouseDown(BPoint point);

void	SetValue(int32 val);
void	SetValue(rgb_color c);
};


inline void HSVControl::hsv_to_rgb(float h,float s,float v,float *r,float *g,float *b)
{
	s /= max_value_at_2();
	v /= max_value_at_3();
	if (s == 0) {
		*r = *b = *g = v;
	}
	else {
		if (h == 360)
			h = 0;

		h /= 60.0;

		int32 i = (int32)floor(h);
		float f = h - i;
		float p = v*(1-s);
		float q = v*(1-s*f);
		float t = v*(1-s*(1-f));

		switch (i) {
			case 0:
				*r = v; *g = t; *b = p;
				break;
			case 1:
				*r = q; *g = v; *b = p;
				break;
			case 2:
				*r = p; *g = v; *b = t;
				break;
			case 3:
				*r = p; *g = q; *b = v;
				break;
			case 4:
				*r = t; *g = p; *b = v;
				break;
			case 5:
				*r = v; *g = p; *b = q;
				break;
		}
	}
	*r *= 255;
	*g *= 255;
	*b *= 255;
}


inline void HSVControl::rgb_to_hsv(float r,float g,float b,float *h,float *s,float *v)
{
	float max_value = max_c(r,max_c(g,b));
	float min_value = min_c(r,min_c(g,b));

	float diff = max_value - min_value;
	*v = max_value;

	if (max_value != 0)
		*s = diff / max_value;
	else
		*s = 0;

	*s *= (max_value_at_2()-min_value_at_2());

	if (*s == 0)
		*h = 0;	// undefined
	else {
		float r_dist = (max_value-r)/diff;
		float g_dist = (max_value-g)/diff;
		float b_dist = (max_value-b)/diff;

		if (r == max_value)
			*h = b_dist - g_dist;
		else if (g == max_value)
			*h = 2+r_dist-b_dist;
		else if (b == max_value)
			*h = 4+g_dist-r_dist;

		*h *= 60.0;
		if (*h < 0)
			*h += 360.0;
	}
}


#endif
