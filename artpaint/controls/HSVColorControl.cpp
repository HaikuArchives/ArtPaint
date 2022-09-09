/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "HSVColorControl.h"


#include "UtilityClasses.h"


#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ColorSliders"


HSVColorControl::HSVColorControl(rgb_color c)
 	: MultichannelColorControl(c, "H", "S", "V", "A")
{
	slider1->SetMinMax(0, 359);
	slider1->SetToolTip(B_TRANSLATE_COMMENT("Hue",
		"For HSV color slider"));
	slider1->SetResolution(0);
	slider2->SetMinMax(0, 1);
	slider2->SetToolTip(B_TRANSLATE_COMMENT("Saturation",
		"For HSV color slider"));
	slider2->SetResolution(2);
	slider3->SetMinMax(0, 1);
	slider3->SetToolTip(B_TRANSLATE_COMMENT("Value",
		"For HSV color slider - 'value' as 'color value', also called 'lightness'"));
 	slider3->SetResolution(2);
 	slider4->SetResolution(0);
 	slider4->SetToolTip(B_TRANSLATE_COMMENT("Alpha",
 		"For color sliders"));
}


void
HSVColorControl::SetValue(rgb_color c)
{
	float h, s, v;

	rgb2hsv((float)c.red, (float)c.green, (float)c.blue,
		h, s, v);

 	slider1->SetValue(h);
 	slider2->SetValue(s);
 	slider3->SetValue(v);
 	slider4->SetValue(c.alpha);
	SetSliderColors(c);
}


void
HSVColorControl::SetSliderColors(rgb_color c)
{
 	BList colorList;

	float h, s, v;

	rgb2hsv((float)c.red, (float)c.green, (float)c.blue,
		h, s, v);

	float r, g, b;

	for (int i = 0; i < 6; ++i) {
		hsv2rgb((float)(i * 60), s, v, r, g, b);
		union color_conversion hue;
		hue.bytes[2] = (uint8)r;
		hue.bytes[1] = (uint8)g;
		hue.bytes[0] = (uint8)b;
		hue.bytes[3] = 255;

		uint32* hue_word = new uint32;
		*hue_word = hue.word;
		colorList.AddItem(hue_word);
	}

 	uint32* hue_word = new uint32;
 	*hue_word = *((uint32 *)colorList.ItemAt(0));
 	colorList.AddItem(hue_word);

 	slider1->Slider()->SetColors(&colorList);

 	while (colorList.CountItems() > 0) {
 		uint32* hue_word = (uint32*)(colorList.RemoveItem(0));
 		delete hue_word;
 	}

	rgb_color color2s, color2e;
 	rgb_color color3s, color3e;
	rgb_color color4s, color4e;

	hsv2rgb(h, 0, v, r, g, b);
	color2s = {(uint8)r, (uint8)g, (uint8)b, 255};
	hsv2rgb(h, s, 0, r, g, b);
	color3s = {(uint8)r, (uint8)g, (uint8)b, 255};
	color4s = {c.red, c.green, c.blue, 0};
	hsv2rgb(h, 1.0, v, r, g, b);
	color2e = {(uint8)r, (uint8)g, (uint8)b, 255};
	hsv2rgb(h, s, 1.0, r, g, b);
	color3e = {(uint8)r, (uint8)g, (uint8)b, 255};
	color4e = {c.red, c.green, c.blue, 255};

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
HSVColorControl::SetValue(float one, float two,
 	float three, float four)
{
 	float r, g, b;

	hsv2rgb(one, two, three, r, g, b);

 	value.bytes[0] = (uint8)b;
 	value.bytes[1] = (uint8)g;
 	value.bytes[2] = (uint8)r;
 	value.bytes[3] = (uint8)four;
 	SetSliderColors(BGRAColorToRGB(value.word));
}
