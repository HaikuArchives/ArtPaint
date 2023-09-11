/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#include "MultichannelColorControl.h"


#include "UtilityClasses.h"


#include <GridLayout.h>
#include <LayoutBuilder.h>


using ArtPaint::Interface::ColorFloatSlider;


MultichannelColorControl::MultichannelColorControl(
	rgb_color c, BString label1, BString label2, BString label3, BString label4)
	:
	BControl("multi color control", "multi-color-control", NULL, B_WILL_DRAW)
{
	BFont font;
	font_height height;

	font.GetHeight(&height);
	float barHeight = (height.ascent - height.descent) / 1.5;

	BMessage* message1 = new BMessage(SLIDER1_CHANGED);
	slider1 = new ColorFloatSlider(label1, "0", message1, 0, 255, false);
	slider1->Slider()->SetBarThickness(barHeight);

	BMessage* message2 = new BMessage(SLIDER2_CHANGED);
	slider2 = new ColorFloatSlider(label2, "0", message2, 0, 255, false);
	slider2->Slider()->SetBarThickness(barHeight);

	BMessage* message3 = new BMessage(SLIDER3_CHANGED);
	slider3 = new ColorFloatSlider(label3, "0", message3, 0, 255, false);
	slider3->Slider()->SetBarThickness(barHeight);

	BMessage* message4 = new BMessage(SLIDER4_CHANGED);
	slider4 = new ColorFloatSlider(label4, "0", message4, 0, 255, false);
	slider4->Slider()->SetBarThickness(barHeight);

	BGridLayout* sliderGrid = BLayoutBuilder::Grid<>(this, 0, 0)
		.Add(slider1, 0, 0, 0, 0)
		.Add(slider1->LabelLayoutItem(), 0, 0)
		.Add(slider1->TextViewLayoutItem(), 1, 0)
		.Add(slider1->Slider(), 2, 0)
		.Add(slider2, 0, 1, 0, 0)
		.Add(slider2->LabelLayoutItem(), 0, 1)
		.Add(slider2->TextViewLayoutItem(), 1, 1)
		.Add(slider2->Slider(), 2, 1)
		.Add(slider3, 0, 2, 0, 0)
		.Add(slider3->LabelLayoutItem(), 0, 2)
		.Add(slider3->TextViewLayoutItem(), 1, 2)
		.Add(slider3->Slider(), 2, 2)
		.Add(slider4, 0, 3, 0, 0)
		.Add(slider4->LabelLayoutItem(), 0, 3)
		.Add(slider4->TextViewLayoutItem(), 1, 3)
		.Add(slider4->Slider(), 2, 3)
		.SetInsets(0, 0, 0, 0);

	sliderGrid->SetColumnWeight(0, 0.1);
	sliderGrid->SetColumnWeight(1, 0.1);
	sliderGrid->SetColumnWeight(2, 0.8);
	sliderGrid->SetMaxColumnWidth(0, font.StringWidth("XXX"));
	sliderGrid->SetMaxColumnWidth(1, font.StringWidth("XXXXX"));

	SetValue(c);
}


MultichannelColorControl::~MultichannelColorControl()
{
	slider1->RemoveSelf();
	slider2->RemoveSelf();
	slider3->RemoveSelf();
	slider4->RemoveSelf();

	if (slider1 != NULL)
		delete slider1;
	if (slider2 != NULL)
		delete slider2;
	if (slider3 != NULL)
		delete slider3;
	if (slider4 != NULL)
		delete slider4;
}


void
MultichannelColorControl::AttachedToWindow()
{
	slider1->SetTarget(this);
	slider2->SetTarget(this);
	slider3->SetTarget(this);
	slider4->SetTarget(this);
}


void
MultichannelColorControl::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case SLIDER1_CHANGED:
		case SLIDER2_CHANGED:
		case SLIDER3_CHANGED:
		case SLIDER4_CHANGED:
		{
			uint32 buttons;
			BPoint point;
			GetMouse(&point, &buttons);

			SetValue(slider1->Value(), slider2->Value(), slider3->Value(), slider4->Value());

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
MultichannelColorControl::SetValue(uint32 val)
{
	value.word = val;
	SetValue(BGRAColorToRGB(value.word));
}


void
MultichannelColorControl::SetValue(rgb_color c)
{
	slider1->SetValue(c.red);
	slider2->SetValue(c.green);
	slider3->SetValue(c.blue);
	slider4->SetValue(c.alpha);
	value.word = RGBColorToBGRA(c);
}


void
MultichannelColorControl::SetValue(float one, float two, float three, float four)
{
	value.bytes[0] = (uint8)one;
	value.bytes[1] = (uint8)two;
	value.bytes[2] = (uint8)three;
	value.bytes[3] = (uint8)four;
}


rgb_color
MultichannelColorControl::ValueAsColor()
{
	return BGRAColorToRGB(value.word);
}


uint32
MultichannelColorControl::Value()
{
	return value.word;
}
