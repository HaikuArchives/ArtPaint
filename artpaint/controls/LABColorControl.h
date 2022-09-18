/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef LAB_COLOR_CONTROL_H
#define LAB_COLOR_CONTROL_H


#include "MultichannelColorControl.h"


class LABColorControl : public MultichannelColorControl {
public:
 		LABColorControl(rgb_color c);

virtual	void		SetValue(rgb_color c);
virtual void		SetValue(float one, float two,
 							float three, float four);

 		rgb_color	ValueAsColor();
virtual	void		SetSliderColors(rgb_color c);

private:
inline	void		lab2rgb(float l, float a, float bb,
							float& r, float& g, float& b);
inline	void		rgb2lab(float r, float g, float b,
							float& l, float& a, float& bb);
};


inline void
LABColorControl::lab2rgb(float l, float a, float bb,
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
LABColorControl::rgb2lab(float r, float g, float b,
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
		x = pow(x, 1./3.);
	else
		x = 7.787 * x  + (16. / 116.);
	if (y > 0.008856)
		y = pow(y, 1./3.);
	else
		y = 7.787 * y  + (16. / 116.);
	if (z > 0.008856)
		z = pow(z, 1./3.);
	else
		z = 7.787 * z  + (16. / 116.);

	l = (116. * y) - 16;
	a = 500. * (x - y);
	bb = 200. * (y - z);
}


#endif
