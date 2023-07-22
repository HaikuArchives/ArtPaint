/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef YUV_COLOR_CONTROL_H
#define YUV_COLOR_CONTROL_H


#include "MultichannelColorControl.h"


class YUVColorControl : public MultichannelColorControl {
public:
 		YUVColorControl(rgb_color c);

virtual	void		SetValue(rgb_color c);
 			rgb_color	ValueAsColor();

private:
virtual	void		_SetColor(float one, float two,
 								float three, float four);
};


#endif
