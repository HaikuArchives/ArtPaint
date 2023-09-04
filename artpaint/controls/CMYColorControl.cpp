/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "CMYColorControl.h"


#include "ColorUtilities.h"
#include "UtilityClasses.h"


#include <Catalog.h>
#include <GridLayout.h>


using ArtPaint::Interface::ColorFloatSlider;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ColorSliders"


CMYColorControl::CMYColorControl(rgb_color c)
	: MultichannelColorControl(c, "C", "M", "Y", "K")
{
	BGridLayout* mainLayout = (BGridLayout*)GetLayout();

	BMessage* message = new BMessage(SLIDER5_CHANGED);
	slider5 = new ColorFloatSlider("A", "0", message, 0, 255, false);
	slider5->Slider()->SetBarThickness(slider1->Slider()->BarThickness());

	mainLayout->AddView(slider5, 0, 4, 0, 0);
	mainLayout->AddItem(slider5->LabelLayoutItem(), 0, 4);
	mainLayout->AddItem(slider5->TextViewLayoutItem(), 1, 4);
	mainLayout->AddView(slider5->Slider(), 2, 4);

	slider1->SetResolution(0);
	slider1->SetMinMax(0, 100);
	slider1->SetToolTip(B_TRANSLATE_COMMENT("Cyan", "For CMYK color slider"));
	slider2->SetResolution(0);
	slider2->SetMinMax(0, 100);
	slider2->SetToolTip(B_TRANSLATE_COMMENT("Magenta", "For CMYK color slider"));
	slider3->SetResolution(0);
	slider3->SetMinMax(0, 100);
	slider3->SetToolTip(B_TRANSLATE_COMMENT("Yellow", "For CMYK color slider"));
	slider4->SetResolution(0);
	slider4->SetMinMax(0, 100);
	slider4->SetToolTip(B_TRANSLATE_COMMENT("Black", "For CMYK color slider"));

	slider5->SetResolution(0);
	slider5->SetToolTip(B_TRANSLATE_COMMENT("Alpha", "For color sliders"));
}


CMYColorControl::~CMYColorControl()
{
	slider5->RemoveSelf();

	if (slider5 != NULL)
		delete slider5;
}


void
CMYColorControl::AttachedToWindow()
{
	slider5->SetTarget(this);

	MultichannelColorControl::AttachedToWindow();
}


void
CMYColorControl::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case SLIDER1_CHANGED:
		case SLIDER2_CHANGED:
		case SLIDER3_CHANGED:
		case SLIDER4_CHANGED:
		case SLIDER5_CHANGED:
		{
			uint32 buttons;
			BPoint point;
			GetMouse(&point, &buttons);

			SetValue(slider1->Value(),
				slider2->Value(),
				slider3->Value(),
				slider4->Value(),
				slider5->Value());

			if (buttons != 0 && Message() != NULL) {
				if (Message()->HasInt32("buttons"))
					Message()->ReplaceInt32("buttons", buttons);
			}
			Invoke();
		} break;
		default:
			BControl::MessageReceived(message);
	}
}


void
CMYColorControl::SetValue(rgb_color c)
{
	float cc, m, y, k;
	rgb2cmyk((float)c.red, (float)c.green, (float)c.blue, cc, m, y, k);

	slider1->SetValue(cc);
	slider2->SetValue(m);
	slider3->SetValue(y);
	slider4->SetValue(k);
	slider5->SetValue(c.alpha);

	SetSliderColors(c);
}


void
CMYColorControl::SetSliderColors(rgb_color c)
{
	rgb_color color1s, color1e;
	rgb_color color2s, color2e;
	rgb_color color3s, color3e;
	rgb_color color4s, color4e;
	rgb_color color5s, color5e;

	float cc, m, y, k;
	rgb2cmyk((float)c.red, (float)c.green, (float)c.blue, cc, m, y, k);

	float r, g, b;
	cmyk2rgb(0, m, y, k, r, g, b);
	color1s = {(uint8)r, (uint8)g, (uint8)b, 255};
	cmyk2rgb(cc, 0, y, k, r, g, b);
	color2s = {(uint8)r, (uint8)g, (uint8)b, 255};
	cmyk2rgb(cc, m, 0, k, r, g, b);
	color3s = {(uint8)r, (uint8)g, (uint8)b, 255};
	cmyk2rgb(cc, m, y, 0, r, g, b);
	color4s = {(uint8)r, (uint8)g, (uint8)b, 255};
	color5s = {c.red, c.green, c.blue, 0};

	cmyk2rgb(100., m, y, k, r, g, b);
	color1e = {(uint8)r, (uint8)g, (uint8)b, 255};
	cmyk2rgb(cc, 100., y, k, r, g, b);
	color2e = {(uint8)r, (uint8)g, (uint8)b, 255};
	cmyk2rgb(cc, m, 100., k, r, g, b);
	color3e = {(uint8)r, (uint8)g, (uint8)b, 255};
	cmyk2rgb(cc, m, y, 100., r, g, b);
	color4e = {(uint8)r, (uint8)g, (uint8)b, 255};
	color5e = {c.red, c.green, c.blue, 255};

	slider1->Slider()->SetColors(color1s, color1e);
	slider2->Slider()->SetColors(color2s, color2e);
	slider3->Slider()->SetColors(color3s, color3e);
	slider4->Slider()->SetColors(color4s, color4e);
	slider5->Slider()->SetColors(color5s, color5e);

	slider1->Slider()->Invalidate();
	slider2->Slider()->Invalidate();
	slider3->Slider()->Invalidate();
	slider4->Slider()->Invalidate();
	slider5->Slider()->Invalidate();
	Draw(Bounds());
}


void
CMYColorControl::SetValue(float one, float two, float three, float four, float five)
{
	float r, g, b;
	cmyk2rgb(one, two, three, four, r, g, b);

	value.bytes[0] = (uint8)b;
	value.bytes[1] = (uint8)g;
	value.bytes[2] = (uint8)r;
	value.bytes[3] = (uint8)five;
	SetSliderColors(BGRAColorToRGB(value.word));
}
