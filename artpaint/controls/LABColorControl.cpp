/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "LABColorControl.h"


#include "UtilityClasses.h"


#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ColorSliders"


LABColorControl::LABColorControl(rgb_color c)
 	: MultichannelColorControl(c, "L", "a", "b", "A")
{
 	slider1->SetMinMax(0, 100);
	slider1->SetToolTip(B_TRANSLATE("Lightness"));
	slider1->SetResolution(1);
	slider2->SetMinMax(-128, 127);
	slider2->SetToolTip(B_TRANSLATE("Channel a"));
	slider2->SetResolution(1);
	slider3->SetMinMax(-128, 127);
	slider3->SetToolTip(B_TRANSLATE("Channel b"));
	slider3->SetResolution(1);
	slider4->SetResolution(1);
	slider4->SetToolTip(B_TRANSLATE("Alpha"));
}


void
LABColorControl::SetValue(rgb_color c)
{
 	float l, a, bb;

 	rgb2lab((float)c.red, (float)c.green, (float)c.blue, l, a, bb);

 	slider1->SetValue(l);
 	slider2->SetValue(a);
 	slider3->SetValue(bb);
 	slider4->SetValue(c.alpha);
 	SetSliderColors(c);
}


void
LABColorControl::SetSliderColors(rgb_color c)
{
 	rgb_color color1s, color1e;
 	rgb_color color2s, color2e;
 	rgb_color color3s, color3e;
	rgb_color color4s, color4e;

	float l, a, bb;

 	rgb2lab((float)c.red, (float)c.green, (float)c.blue, l, a, bb);

	float r, g, b;
	lab2rgb(0.0, a, bb, r, g, b);
	color1s = {(uint8)r, (uint8)g, (uint8)b, 255};
	lab2rgb(l, -128., bb, r, g, b);
	color2s = {(uint8)r, (uint8)g, (uint8)b, 255};
	lab2rgb(l, a, -128., r, g, b);
	color3s = {(uint8)r, (uint8)g, (uint8)b, 255};
	color4s = {c.red, c.green, c.blue, 0};
	lab2rgb(100., a, bb, r, g, b);
	color1e = {(uint8)r, (uint8)g, (uint8)b, 255};
	lab2rgb(l, 127., bb, r, g, b);
	color2e = {(uint8)r, (uint8)g, (uint8)b, 255};
	lab2rgb(l, a, 127., r, g, b);
	color3e = {(uint8)r, (uint8)g, (uint8)b, 255};
	color4e = {c.red, c.green, c.blue, 255};

	slider1->Slider()->SetColors(color1s, color1e);
	slider2->Slider()->SetColors(color2s, color2e);
	slider3->Slider()->SetColors(color3s, color3e);
	slider4->Slider()->SetColors(color4s, color4e);

	slider1->Slider()->Invalidate();
	slider2->Slider()->Invalidate();
	slider3->Slider()->Invalidate();
	slider4->Slider()->Invalidate();
	Draw(Bounds());
}


void
LABColorControl::SetValue(float one, float two,
 	float three, float four)
{
 	float r, g, b;

 	lab2rgb(one, two, three, r, g, b);

 	value.bytes[0] = (uint8)b;
 	value.bytes[1] = (uint8)g;
 	value.bytes[2] = (uint8)r;
 	value.bytes[3] = (uint8)four;
 	SetSliderColors(BGRAColorToRGB(value.word));
}
