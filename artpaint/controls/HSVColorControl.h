/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef HSV_COLOR_CONTROL_H
#define HSV_COLOR_CONTROL_H

#include "MultichannelColorControl.h"


class HSVColorControl : public MultichannelColorControl {
public:
						HSVColorControl(rgb_color c);

	virtual	void		SetValue(rgb_color c);
			void		SetValue(float one, float two, float three, float four);
			rgb_color	ValueAsColor();
	virtual	void		SetSliderColors(rgb_color c);

};


#endif // HSV_COLOR_CONTROL_H
