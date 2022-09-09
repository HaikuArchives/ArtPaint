/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef CMY_COLOR_CONTROL_H
#define CMY_COLOR_CONTROL_H


#include "ColorFloatSlider.h"
#include "MultichannelColorControl.h"


using ArtPaint::Interface::ColorFloatSlider;


class CMYColorControl : public MultichannelColorControl {
public:
		CMYColorControl(rgb_color c);

		void		MessageReceived(BMessage* message);
virtual	void		SetValue(rgb_color c);
virtual	void		SetValue(float one, float two,
						float three, float four, float five);
		rgb_color	ValueAsColor();
virtual	void		SetSliderColors(rgb_color c);

private:
inline	void		cmyk2rgb(float c, float m, float y, float k,
							float& r, float& g, float& b);
inline	void		rgb2cmyk(float r, float g, float b,
							float& c, float& m, float& y, float& k);

		ColorFloatSlider* slider5;
};


inline void
CMYColorControl::cmyk2rgb(float c, float m, float y, float k,
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
CMYColorControl::rgb2cmyk(float r, float g, float b,
	float& c, float& m, float& y, float& k)
{
	r /= 255.;
	g /= 255.;
	b /= 255.;

	k = 1.0 - max_c(r, max_c(g, b));
	float inv_k = 1.0 - k; // + 0.00001;
	c = ((1.0 - r - k) / inv_k) * 100.;
 	m = ((1.0 - g - k) / inv_k) * 100.;
 	y = ((1.0 - b - k) / inv_k) * 100.;
 	k *= 100.;
}


#endif
