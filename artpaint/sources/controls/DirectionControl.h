/* 

	Filename:	DirectionControl.h
	Contents:	DirectionControl-class declarations	
	Author:		Heikki Suhonen
	
*/



#ifndef DIRECTION_CONTROL_H
#define DIRECTION_CONTROL_H

#include "HSPolygon.h"
#include "Controls.h"

#define CONTROLLER_DIMENSION	32

class DirectionControl : public BControl {
float		angle;
float		last_angle;
float		new_angle;
HSPolygon	*arrow_head;
HSPolygon	*line;
int32 		old_value;

public:
		DirectionControl(BRect,char*,char*,BMessage*);
		~DirectionControl();
		
void	Draw(BRect area);
void	MouseDown(BPoint);
void	setValue(float);
};


class DirectionControlBox : public BBox {

		DirectionControl	*d_control;
public:
		DirectionControlBox(BRect,char*,char*,BMessage*);
		~DirectionControlBox();	

void	setValue(float);
};

#endif