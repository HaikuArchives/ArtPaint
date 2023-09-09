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


#define SLIDER5_CHANGED		's5Ch'


using ArtPaint::Interface::ColorFloatSlider;


class CMYColorControl : public MultichannelColorControl {
public:
						CMYColorControl(rgb_color c);
						~CMYColorControl();

			void		AttachedToWindow();
			void		MessageReceived(BMessage* message);
	virtual	void		SetValue(rgb_color c);
	virtual	void		SetValue(float one, float two, float three, float four, float five);
			rgb_color	ValueAsColor();
	virtual	void		SetSliderColors(rgb_color c);

private:
			ColorFloatSlider* slider5;
};


#endif // CMY_COLOR_CONTROL_H
