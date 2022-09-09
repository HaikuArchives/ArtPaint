/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "RGBColorControl.h"


#include "UtilityClasses.h"


#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ColorSliders"


RGBColorControl::RGBColorControl(rgb_color c)
 	: MultichannelColorControl(c, "R", "G", "B", "A")
{
	slider1->SetResolution(0);
	slider1->SetToolTip(B_TRANSLATE_COMMENT("Red",
		"For RGB color sliders"));
	slider2->SetResolution(0);
	slider2->SetToolTip(B_TRANSLATE_COMMENT("Green",
		"For RGB color sliders"));
	slider3->SetResolution(0);
	slider3->SetToolTip(B_TRANSLATE_COMMENT("Blue",
		"For RGB color sliders"));
	slider4->SetResolution(0);
	slider4->SetToolTip(B_TRANSLATE_COMMENT("Alpha",
		"For color sliders"));
}


void
RGBColorControl::SetValue(rgb_color c)
{
	MultichannelColorControl::SetValue(c);
 	SetSliderColors(c);
}


void
RGBColorControl::SetValue(float one, float two,
 	float three, float four)
{
 	value.bytes[0] = (uint8)three;
 	value.bytes[1] = (uint8)two;
 	value.bytes[2] = (uint8)one;
 	value.bytes[3] = (uint8)four;
 	SetSliderColors(BGRAColorToRGB(value.word));
}


void
RGBColorControl::SetSliderColors(rgb_color c)
{
	rgb_color color1s, color1e;
 	rgb_color color2s, color2e;
 	rgb_color color3s, color3e;
	rgb_color color4s, color4e;

	color1s = {0, c.green, c.blue, 255};
	color2s = {c.red, 0, c.blue, 255};
	color3s = {c.red, c.green, 0, 255};
	color4s = {c.red, c.green, c.blue, 0};
	color1e = {255, c.green, c.blue, 255};
	color2e = {c.red, 255, c.blue, 255};
	color3e = {c.red, c.green, 255, 255};
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
RGBColorControl::_SetColor(float one, float two,
 	float three, float four)
{
 	value.bytes[0] = (uint8)three;
 	value.bytes[1] = (uint8)two;
 	value.bytes[2] = (uint8)one;
 	value.bytes[3] = (uint8)four;
 	SetValue(BGRAColorToRGB(value.word));
}
