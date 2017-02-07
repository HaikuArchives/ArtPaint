/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <Picture.h>
#include <StringView.h>

#include "VisualColorControl.h"
#include "UtilityClasses.h"
#include "Patterns.h"

VisualColorControl::VisualColorControl(BPoint position, rgb_color c,
		const char *s1, const char *s2, const char *s3, const char *s4)
	:	BControl(BRect(position,position),"rgb control","RGB-Control",NULL,B_FOLLOW_TOP|B_FOLLOW_LEFT,B_WILL_DRAW)
{
//	value = ( 	((c.blue<<24) & 0xFF000000) | ((c.green<<16) & 0x00FF0000)
//				| ((c.red<<8) &0x0000FF00) | (c.alpha & 0x000000FF) );

	value.bytes[0] = c.blue;
	value.bytes[1] = c.green;
	value.bytes[2] = c.red;
	value.bytes[3] = c.alpha;

	// create the ramp-bitmaps
	ramp1 = new BBitmap(BRect(0,0,255,0),B_RGB_32_BIT);
	ramp2 = new BBitmap(BRect(0,0,255,0),B_RGB_32_BIT);
	ramp3 = new BBitmap(BRect(0,0,255,0),B_RGB_32_BIT);
	ramp4 = new BBitmap(BRect(0,0,255,0),B_RGB_32_BIT);

	// TODO: check this
	label1 = s1;
	label2 = s2;
	label3 = s3;
	label4 = s4;

	previous_value_at_1 = -100000;
	previous_value_at_2 = -100000;
	previous_value_at_3 = -100000;
	previous_value_at_4 = -100000;


	// resize ourselves to proper size i.e. 30 pixels in height per ramp
	// and 150 pixels in width for a ramp + 40 pixels in width for color-plate
	ResizeTo(8+RAMP_WIDTH + PLATE_WIDTH,4*COLOR_HEIGHT);
}

void VisualColorControl::AttachedToWindow()
{
	BPoint *points = new BPoint[3];
	points[0] = BPoint(0,0);
	points[1] = BPoint(-4,-4);
	points[2] = BPoint(4,-4);

	// calculate the initial ramps
	CalcRamps();

	// create the arrow-pictures
	BeginPicture(new BPicture());
		SetHighColor(100,100,255);
		FillPolygon(points,3);
		SetHighColor(0,0,0);
		StrokePolygon(points,3);
	down_arrow = EndPicture();

	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());

	BStringView *sv = new BStringView(BRect(0,COLOR_HEIGHT/2,1,COLOR_HEIGHT/2),"label view",label1);
	AddChild(sv);
	float sv_width = sv->StringWidth(label1);
	sv_width = max_c(sv_width,sv->StringWidth(label2));
	sv_width = max_c(sv_width,sv->StringWidth(label3));
	font_height fHeight;
	sv->GetFontHeight(&fHeight);
	sv->ResizeTo(sv_width,fHeight.ascent+fHeight.descent);
	sv->MoveBy(0,-(fHeight.ascent+fHeight.descent)/2.0);
	BRect sv_frame = sv->Frame();
	sv_frame.OffsetBy(0,COLOR_HEIGHT);
	sv->SetAlignment(B_ALIGN_CENTER);
	sv = new BStringView(sv_frame,"label view",label2);
	AddChild(sv);
	sv->SetAlignment(B_ALIGN_CENTER);
	sv_frame.OffsetBy(0,COLOR_HEIGHT);
	sv = new BStringView(sv_frame,"label view",label3);
	AddChild(sv);
	sv->SetAlignment(B_ALIGN_CENTER);
	sv_frame.OffsetBy(0,COLOR_HEIGHT);
	sv = new BStringView(sv_frame,"label view",label4);
	AddChild(sv);
	sv->SetAlignment(B_ALIGN_CENTER);

	ramp_left_edge = sv->Bounds().IntegerWidth()+2;

	ResizeBy(ramp_left_edge,0);

	delete[] points;
}

VisualColorControl::~VisualColorControl()
{
	// here delete the bitmaps
	delete ramp1;
	delete ramp2;
	delete ramp3;
	delete ramp4;
}


void VisualColorControl::Draw(BRect)
{
	// ramp height is COLOR_HEIGHT - 12 pixels that are required for controls
	int32 ramp_height = COLOR_HEIGHT - 12;

	// here draw the ramps
	BRect ramp_rect = BRect(ramp_left_edge+4,6,ramp_left_edge+RAMP_WIDTH+4,6+ramp_height-1);

	DrawBitmap(ramp1,ramp1->Bounds(),ramp_rect);
	ramp_rect.OffsetBy(0,COLOR_HEIGHT);
	DrawBitmap(ramp2,ramp2->Bounds(),ramp_rect);
	ramp_rect.OffsetBy(0,COLOR_HEIGHT);
	DrawBitmap(ramp3,ramp3->Bounds(),ramp_rect);
	ramp_rect.OffsetBy(0,COLOR_HEIGHT);
	DrawBitmap(ramp4,ramp4->Bounds(),ramp_rect);

	// draw the color-plate
	BRect a_rect = BRect(Bounds().Width()-PLATE_WIDTH+2,2,Bounds().Width()-2,Bounds().Height()-2);
	SetHighColor(0,0,0);
	StrokeRect(a_rect);


	rgb_color low = ValueAsColor();
	rgb_color high = low;

	float coeff = high.alpha / 255.0;
	low.red = (uint8)(coeff*low.red);
	low.green = (uint8)(coeff*low.green);
	low.blue = (uint8)(coeff*low.blue);
	low.alpha = 255;

	high.red = (uint8)(coeff*high.red + (1-coeff)*255);
	high.green = (uint8)(coeff*high.green + (1-coeff)*255);
	high.blue = (uint8)(coeff*high.blue + (1-coeff)*255);
	high.alpha = 255;

	SetHighColor(high);
	SetLowColor(low);
	a_rect.InsetBy(1,1);
	FillRect(a_rect,HS_2X2_BLOCKS);

	// clear the previous arrows and draw new arrows
	SetHighColor(ViewColor());
	BRect bg_rect = BRect(ramp_left_edge + 0,0,ramp_left_edge + RAMP_WIDTH+8,5);
	if (previous_value_at_1 != value_at_1())
		FillRect(bg_rect);

	bg_rect.OffsetBy(0,COLOR_HEIGHT);

	if (previous_value_at_2 != value_at_2())
		FillRect(bg_rect);

	bg_rect.OffsetBy(0,COLOR_HEIGHT);

	if (previous_value_at_3 != value_at_3())
		FillRect(bg_rect);

	bg_rect.OffsetBy(0,COLOR_HEIGHT);

	if (previous_value_at_4 != value_at_4())
		FillRect(bg_rect);

	DrawPicture(down_arrow,BPoint(ramp_left_edge + (float)((value_at_1()-min_value_at_1()))/(max_value_at_1()-min_value_at_1())*RAMP_WIDTH+4,5));
	DrawPicture(down_arrow,BPoint(ramp_left_edge + (float)((value_at_2()-min_value_at_2()))/(max_value_at_2()-min_value_at_2())*RAMP_WIDTH+4,COLOR_HEIGHT+5));
	DrawPicture(down_arrow,BPoint(ramp_left_edge + (float)((value_at_3()-min_value_at_3()))/(max_value_at_3()-min_value_at_3())*RAMP_WIDTH+4,2*COLOR_HEIGHT+5));
	DrawPicture(down_arrow,BPoint(ramp_left_edge + (float)((value_at_4()-min_value_at_4()))/(max_value_at_4()-min_value_at_4())*RAMP_WIDTH+4,3*COLOR_HEIGHT+5));
	Sync();

	previous_value_at_1 = value_at_1();
	previous_value_at_2 = value_at_2();
	previous_value_at_3 = value_at_3();
	previous_value_at_4 = value_at_4();


//	DrawPicture(down_arrow,BPoint(ramp_left_edge + (float)(value_at_2())/255.0*RAMP_WIDTH+4,COLOR_HEIGHT+5));
//	DrawPicture(down_arrow,BPoint(ramp_left_edge + (float)(value_at_3())/255.0*RAMP_WIDTH+4,2*COLOR_HEIGHT+5));
}


void VisualColorControl::MessageReceived(BMessage *message)
{
	rgb_color *new_color;
	ssize_t color_size;
	switch (message->what) {
		case B_PASTE:
			if (message->FindData("RGBColor",B_RGB_COLOR_TYPE,(const void**)&new_color,&color_size) == B_OK) {
				SetValue(*new_color);
			}
			break;

		default:
			BControl::MessageReceived(message);
			break;
	}
}

void VisualColorControl::SetValue(int32 val)
{
	value.word = val;
	CalcRamps();
	Draw(Bounds());
}

void VisualColorControl::SetValue(rgb_color c)
{
//	value = ( 	((c.blue<<24) & 0xFF000000) | ((c.green<<16) & 0x00FF0000)
//				| ((c.red<<8) &0x0000FF00) | (c.alpha & 0x000000FF) );

	value.bytes[0] = c.blue;
	value.bytes[1] = c.green;
	value.bytes[2] = c.red;
	value.bytes[3] = c.alpha;

	CalcRamps();
	Draw(Bounds());
}

rgb_color VisualColorControl::ValueAsColor()
{
	rgb_color c;
//	c.red = (value >> 8) & 0xFF;
//	c.blue = (value >> 24) & 0xFF;
//	c.green = (value >>16) & 0xFF;
//	c.alpha = (value) & 0xFF;
	c.red = value.bytes[2];
	c.green = value.bytes[1];
	c.blue = value.bytes[0];
	c.alpha = value.bytes[3];

	return c;
}
