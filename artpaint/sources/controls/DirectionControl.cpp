/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <math.h>
#include <StringView.h>



#include "DirectionControl.h"

#define PI M_PI

DirectionControl::DirectionControl(BRect frame,char *name,char *label,BMessage *message)
	:	BControl(frame,name,label,message,B_FOLLOW_LEFT|B_FOLLOW_TOP,B_WILL_DRAW)
{
	ResizeTo(CONTROLLER_DIMENSION,CONTROLLER_DIMENSION);
	SetValue(0);
	old_value = 0;

	last_angle = angle = 0;
	new_angle = 0;

	BPoint point_list[3];
	point_list[0] = BPoint(CONTROLLER_DIMENSION/2-4,5);
	point_list[1] = BPoint(CONTROLLER_DIMENSION/2,1);
	point_list[2] = BPoint(CONTROLLER_DIMENSION/2+4,5);


	arrow_head = new HSPolygon(point_list,3);

	point_list[0] = BPoint(CONTROLLER_DIMENSION/2,1);
	point_list[1] = BPoint(CONTROLLER_DIMENSION/2,CONTROLLER_DIMENSION-2);

	line = new HSPolygon(point_list,2);
}

DirectionControl::~DirectionControl()
{
	delete arrow_head;
	delete line;
}


void DirectionControl::Draw(BRect area)
{
	// Here draw the arrows at a proper angle.
	if (new_angle != last_angle) {
		if (area.IsValid() == FALSE) {
			// Erase the previous drawing.
			BPolygon *a_poly = arrow_head->GetBPolygon();
			SetHighColor(ViewColor());
			FillPolygon(a_poly);
			delete a_poly;
		}

		angle += new_angle-last_angle;
		arrow_head->Rotate(BPoint(CONTROLLER_DIMENSION/2,CONTROLLER_DIMENSION/2),new_angle-last_angle);
		line->Rotate(BPoint(CONTROLLER_DIMENSION/2,CONTROLLER_DIMENSION/2),new_angle-last_angle);
		last_angle = new_angle;
	}
	SetHighColor(0,0,0,255);
	SetDrawingMode(B_OP_COPY);
	BPolygon *a_poly = arrow_head->GetBPolygon();
	FillPolygon(a_poly);
	delete a_poly;
	a_poly = line->GetBPolygon();
	StrokePolygon(a_poly);
	delete a_poly;
	if (area.IsValid()) {
		StrokeArc(Bounds(),225,180);
		SetHighColor(255,255,255,255);
		StrokeArc(Bounds(),45,180);
	}
}


void DirectionControl::MouseDown(BPoint point)
{
	uint32 buttons;
	arrow_head->Rotate(BPoint(CONTROLLER_DIMENSION/2,CONTROLLER_DIMENSION/2),-angle);
	line->Rotate(BPoint(CONTROLLER_DIMENSION/2,CONTROLLER_DIMENSION/2),-angle);
	GetMouse(&point,&buttons);
	last_angle = 0;
	angle = 0;

	BPoint center;
	center.x = (Bounds().left+Bounds().right)/2.0;
	center.y = (Bounds().top+Bounds().bottom)/2.0;
	Invalidate();
	while (buttons) {
		GetMouse(&point,&buttons);
		new_angle = atan2(point.x-center.x,center.y-point.y)*180/PI;
		if (new_angle != last_angle) {
			BRect area = BRect(0,0,-1,-1);
			Draw(area);
		}
		snooze(20 * 1000);
	}
	SetValue(int32(new_angle));
	Invoke();
}


void DirectionControl::setValue(float ang)
{
	new_angle = ang;
//	Draw();
}

DirectionControlBox::DirectionControlBox(BRect frame,char *name,char *label,BMessage *message)
	:	BBox(frame,name)
{
	SetBorder(B_PLAIN_BORDER);

	d_control = new DirectionControl(BRect(EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE),name,label,message);
	AddChild(d_control);

	ResizeTo(frame.Width(),d_control->Frame().Height()+2*EXTRA_EDGE);

	BStringView *string_view = new BStringView(BRect(d_control->Frame().right+EXTRA_EDGE,Bounds().top,Bounds().right-1,Bounds().bottom),name,label);
	AddChild(string_view);
	font_height fHeight;
	string_view->GetFontHeight(&fHeight);
	string_view->ResizeTo(string_view->Bounds().Width(),fHeight.ascent+fHeight.descent+fHeight.leading);
	string_view->MoveBy(0,(Bounds().Height()- string_view->Bounds().Height())/2.0);
}


DirectionControlBox::~DirectionControlBox()
{

}


void DirectionControlBox::setValue(float angle)
{
	d_control->setValue(angle);
}
