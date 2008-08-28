/* 

	Filename:	CMYControl.h
	Contents:	CMYControl-class declaration	
	Author:		Heikki Suhonen
	
*/

#ifndef CMY_CONTROL_H
#define CMY_CONTROL_H

#include "VisualColorControl.h"

class CMYControl : public VisualColorControl {
void	CalcRamps();

int32	value_at_1();
int32	value_at_2();
int32	value_at_3();
			
public:
		CMYControl(BPoint position, rgb_color c);

void	MouseDown(BPoint point);
};
#endif