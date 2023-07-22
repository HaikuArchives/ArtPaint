/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef LAB_COLOR_CONTROL_H
#define LAB_COLOR_CONTROL_H


#include "MultichannelColorControl.h"


class LABColorControl : public MultichannelColorControl {
public:
 		LABColorControl(rgb_color c);

virtual	void		SetValue(rgb_color c);
virtual void		SetValue(float one, float two,
 							float three, float four);

 		rgb_color	ValueAsColor();
virtual	void		SetSliderColors(rgb_color c);
};

#endif
