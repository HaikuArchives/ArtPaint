/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef COLOR_UTILITIES_H
#define COLOR_UTILITIES_H

#include <math.h>


inline void
lab2rgb(float l, float a, float bb,
	float& r, float& g, float& b)
{
	double y = (l + 16.) / 116;
	double x = (a / 500.) + y;
	double z = y - (bb / 200.);

	if (pow(y, 3) > 0.008856)
		y = pow(y, 3);
	else
		y = (y - (16. / 116.)) / 7.787;
	if (pow(x, 3) > 0.008856)
		x = pow(x, 3);
	else
		x = (x - (16. / 116.)) / 7.787;
	if (pow(z, 3) > 0.008856)
		z = pow(z, 3);
	else
		z = (z - (16. / 116.)) / 7.787;

	// observer = 2 deg, illuminant = D65
	x *= 95.047;
	y *= 100.0;
	z *= 108.883;

	x /= 100.;
	y /= 100.;
	z /= 100.;

	double dr = x * 3.2406 + y * -1.5372 + z * -0.4986;
	double dg = x * -0.9689 + y * 1.8758 + z * 0.0415;
	double db = x * 0.0557 + y * -0.2040 + z * 1.0570;

	float power = 1. / 2.4;
	if (dr > 0.0031308)
		dr = 1.055 * pow(dr, power) - 0.055;
	else
		dr *= 12.92;
	if (dg > 0.0031308)
		dg = 1.055 * pow(dg, power) - 0.055;
	else
		dg *= 12.92;
	if (db > 0.0031308)
		db = 1.055 * pow(db, power) - 0.055;
	else
		db *= 12.92;

	dr *= 255.;
	dg *= 255.;
	db *= 255.;

	r = max_c(0, min_c(255, dr));
	g = max_c(0, min_c(255, dg));
	b = max_c(0, min_c(255, db));
}


inline void
rgb2lab(float r, float g, float b,
	float& l, float& a, float& bb)
{
	r /= 255.;
	g /= 255.;
	b /= 255.;

	if (r > 0.04045)
		r = pow(((r + 0.055) / 1.055), 2.4);
	else
		r /= 12.92;

	if (g > 0.04045)
		g = pow(((g + 0.055) / 1.055), 2.4);
	else
		g /= 12.92;

	if (b > 0.04045)
		b = pow(((b + 0.055) / 1.055), 2.4);
	else
		b /= 12.92;

	r *= 100.;
	g *= 100.;
	b *= 100.;

	float x = r * 0.4125 + g * 0.3576 + b * 0.1804;
	float y = r * 0.2127 + g * 0.7152 + b * 0.0722;
	float z = r * 0.0193 + g * 0.1192 + b * 0.9503;

	// observer = 2 deg, illuminant = D65
	x /= 95.047;
	y /= 100.0;
	z /= 108.883;

	if (x > 0.008856)
		x = pow(x, 1. / 3.);
	else
		x = 7.787 * x  + (16. / 116.);
	if (y > 0.008856)
		y = pow(y, 1./3.);
	else
		y = 7.787 * y  + (16. / 116.);
	if (z > 0.008856)
		z = pow(z, 1. / 3.);
	else
		z = 7.787 * z  + (16. / 116.);

	l = (116. * y) - 16;
	a = 500. * (x - y);
	bb = 200. * (y - z);
}


inline void
yuv2rgb(float y, float u, float v,
	float& r, float& g, float& b)
{
	u -= 128;
	v -= 128;

 	uint32 R = (int32)(298 * y + 409 * u + 128) >> 8;
	uint32 G = (int32)(298 * y - 100 * u - 208 * v + 128) >> 8;
	uint32 B = (int32)(298 * y + 516 * u + 128) >> 8;

	r = (float)R;
	g = (float)G;
	b = (float)B;
}


inline void
rgb2yuv(float r, float g, float b,
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
}


inline void
cmyk2rgb(float c, float m, float y, float k,
	float& r, float& g, float& b)
{
	c /= 100.;
	m /= 100.;
	y /= 100.;
	k /= 100.;

	b = (1.0 - y) * (1.0 - k) * 255.;
 	g = (1.0 - m) * (1.0 - k) * 255.;
 	r = (1.0 - c) * (1.0 - k) * 255.;
}


inline void
rgb2cmyk(float r, float g, float b,
	float& c, float& m, float& y, float& k)
{
	r /= 255.;
	g /= 255.;
	b /= 255.;

	k = 1.0 - max_c(r, max_c(g, b));
	float inv_k = 1.0 - k + 0.00001;  // guard against divide-by-zero
	c = ((1.0 - r - k) / inv_k) * 100.;
 	m = ((1.0 - g - k) / inv_k) * 100.;
 	y = ((1.0 - b - k) / inv_k) * 100.;
 	k *= 100.;
}


inline void
hsv2rgb(float h, float s, float v,
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
rgb2hsv(float r, float g, float b,
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


inline float hue2rgb(float v1, float v2, float h)
{
	if (h < 0)
		h += 1;
	if (h > 1)
		h -= 1;

	if (h * 6 < 1)
		return v1 + (v2 - v1) * 6 * h;
	if (h * 2 < 1)
		return v2;
	if (h * 3 < 2)
		return v1 + (v2 - v1) * ((2. / 3.) - h) * 6;

	return v1;
}


inline void
hsl2rgb(float h, float s, float l,
	float& r, float &g, float &b)
{
	if (s <= 0) {
		r = l;
		g = l;
		b = l;
	} else {
		h /= 360.;

		float v1, v2;

		if (l < 0.5)
			v2 = l * (1. + s);
		else
			v2 = (l + s) - (s * l);

		v1 = 2 * l - v2;

		r = hue2rgb(v1, v2, h + (1. / 3.));
		g = hue2rgb(v1, v2, h);
		b = hue2rgb(v1, v2, h - (1. / 3.));
	}

	r *= 255;
	g *= 255;
	b *= 255;
}



inline void
rgb2hsl(float r, float g, float b,
	float& h, float& s, float& l)
{
	r /= 255.;
	g /= 255.;
	b /= 255.;

	float min = min_c(b, min_c(r, g));
	float max = max_c(b, max_c(r, g));

	l = (max + min) / 2.;
	float delta = max - min;
	if (delta == 0) {
		h = 0;
		s = 0;
	} else {
		if (l < 0.5)
			s = delta / (max + min);
		else
			s = delta / (2 - max - min);

		float r_dist = (((max - r) / 6.) + (delta / 2.)) / delta;
		float g_dist = (((max - g) / 6.) + (delta / 2.)) / delta;
		float b_dist = (((max - b) / 6.) + (delta / 2.)) / delta;

		if (r == max)
			h = b_dist - g_dist;
		else if (g == max)
			h = (1. / 3.) + r_dist - b_dist;
		else
			h = (2. / 3.) + g_dist - r_dist;
	}

	h = h * 360;
	if (h < 0.0)
		h += 360.0;
}


#endif // COLOR_UTILITIES_H
