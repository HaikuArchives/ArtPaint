/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef RGB_COLOR_CONTROL_H
#define RGB_COLOR_CONTROL_H


#include "MultichannelColorControl.h"


class RGBColorControl : public MultichannelColorControl {
public:
 		RGBColorControl(rgb_color c);

virtual	void		SetValue(rgb_color c);
virtual	void		SetValue(float one, float two,
 							float three, float four);
 			rgb_color	ValueAsColor();
virtual	void		SetSliderColors(rgb_color c);

private:
virtual	void		_SetColor(float one, float two,
 								float three, float four);

};


#endif
