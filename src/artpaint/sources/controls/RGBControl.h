/* 

	Filename:	RGBControl.h
	Contents:	RGBControl-class declaration	
	Author:		Heikki Suhonen
	
*/

#ifndef RGB_CONTROL_H
#define RGB_CONTROL_H

#include "VisualColorControl.h"

class RGBControl : public VisualColorControl {
void	CalcRamps();

int32	value_at_1();
int32	value_at_2();
int32	value_at_3();
			
public:
		RGBControl(BPoint position, rgb_color c);

void	MouseDown(BPoint point);
};
#endif