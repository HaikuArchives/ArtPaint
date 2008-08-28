/* 

	Filename:	YUVControl.h
	Contents:	YUVControl-class declaration	
	Author:		Heikki Suhonen
	
*/

#ifndef YUV_CONTROL_H
#define YUV_CONTROL_H

#include "VisualColorControl.h"

class YUVControl : public VisualColorControl {
float	y_value;
float	u_value;
float	v_value;


void	CalcRamps();

int32	value_at_1();
int32	value_at_2();
int32	value_at_3();
float	max_value_at_1() { return 255; }
float	max_value_at_2() { return 111.435; }
float	max_value_at_3() { return 156.825; }

float	min_value_at_1() { return 0; }
float	min_value_at_2() { return -111.18; }
float	min_value_at_3() { return -156.825; }

			
public:
		YUVControl(BPoint position, rgb_color c);

void	MouseDown(BPoint point);
void	SetValue(int32 val);
void	SetValue(rgb_color c);
};
#endif