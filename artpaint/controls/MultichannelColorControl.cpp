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


#define SLIDER_CHANGED		'slCh'


MultichannelColorControl::MultichannelColorControl(rgb_color c,
 	BString label1, BString label2, BString label3, BString label4)
 	: BControl("multi color control", "multi-color-control", NULL,
 		B_WILL_DRAW)
{
 	BMessage *message = new BMessage(SLIDER_CHANGED);
 	slider1 = new ColorFloatSlider(label1,
 		"0", message, 0, 255, false);

 	slider2 = new ColorFloatSlider(label2,
 		"0", message, 0, 255, false);

 	slider3 = new ColorFloatSlider(label3,
 		"0", message, 0, 255, false);

 	slider4 = new ColorFloatSlider(label4,
 		"0", message, 0, 255, false);

 	BGridLayout* sliderGrid = BLayoutBuilder::Grid<>(this,
 		B_USE_SMALL_SPACING, 0)
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
 			.Add(slider4->Slider(), 2, 3);
 	sliderGrid->SetColumnWeight(0, 0.2);
	sliderGrid->SetColumnWeight(1, 0.1);
	sliderGrid->SetColumnWeight(2, 0.7);

	SetValue(c);
}


MultichannelColorControl::~MultichannelColorControl()
{
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
MultichannelColorControl::Draw(BRect rect)
{
 	BControl::Draw(rect);
}


void
MultichannelColorControl::MessageReceived(BMessage* message)
{
 	switch(message->what) {
 		case SLIDER_CHANGED: {
 			uint32 buttons;
 			BPoint point;
 			GetMouse(&point, &buttons);

 			SetValue(slider1->Value(),
 				slider2->Value(),
 				slider3->Value(),
 				slider4->Value());

 			if (buttons != 0 && Message() != NULL)
 				if (Message()->HasInt32("buttons"))
 					Message()->ReplaceInt32("buttons", buttons);

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
MultichannelColorControl::SetValue(float one, float two,
	float three, float four)
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
