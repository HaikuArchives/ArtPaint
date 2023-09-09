/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef MULTICHANNEL_COLOR_CONTROL_H
#define MULTICHANNEL_COLOR_CONTROL_H

#include "ColorFloatSlider.h"
#include "PixelOperations.h"

#include <Control.h>
#include <String.h>


#define SLIDER1_CHANGED		'slCh'
#define SLIDER2_CHANGED		's2Ch'
#define SLIDER3_CHANGED		's3Ch'
#define SLIDER4_CHANGED		's4Ch'


using ArtPaint::Interface::ColorFloatSlider;


class MultichannelColorControl : public BControl {
public:
						MultichannelColorControl(rgb_color c,
								BString label1, BString label2,
								BString label3, BString label4);
	virtual				~MultichannelColorControl();

			void		AttachedToWindow();
			void 		MessageReceived(BMessage* message);
			void		SetValue(uint32 val);
	virtual void		SetValue(rgb_color c);
	virtual void		SetValue(float one, float two, float three, float four);
	virtual	void		SetSliderColors(rgb_color c) {};
			rgb_color	ValueAsColor();
			uint32		Value();

protected:
			union color_conversion value;

			ColorFloatSlider* slider1;
			ColorFloatSlider* slider2;
			ColorFloatSlider* slider3;
			ColorFloatSlider* slider4;
};


#endif // MULTICHANNEL_COLOR_CONTROL_H
