/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef YIQ_CONTROL_H
#define YIQ_CONTROL_H

#include "VisualColorControl.h"

class YIQControl : public VisualColorControl {
float	y_value;
float	i_value;
float	q_value;


void	CalcRamps();

int32	value_at_1();
int32	value_at_2();
int32	value_at_3();
float	max_value_at_1() { return 255; }
float	max_value_at_2() { return 151.98; }
float	max_value_at_3() { return 133.365; }

float	min_value_at_1() { return 0; }
float	min_value_at_2() { return -151.98; }
float	min_value_at_3() { return -133.365; }


public:
		YIQControl(BPoint position, rgb_color c);

void	MouseDown(BPoint point);

void	SetValue(int32 val);
void	SetValue(rgb_color c);
};
#endif
