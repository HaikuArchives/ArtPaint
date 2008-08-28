/* 

	Filename:	PopUpSlider.cpp
	Contents:	PopUpSlider-class definitions	
	Author:		Heikki Suhonen
	
*/


#include <InterfaceDefs.h>
#include <Slider.h>


#include "PopUpSlider.h"


PopUpSlider::PopUpSlider(BRect rect)
	: BWindow(rect,"pop_up_slider_window",B_BORDERED_WINDOW_LOOK,B_FLOATING_APP_WINDOW_FEEL,0)
{
	the_slider = NULL;
}


PopUpSlider* PopUpSlider::Instantiate(BPoint screen_point,BMessenger *messenger,BMessage *message,int32 range_min,int32 range_max)
{
	BSlider *slider = new BSlider(BRect(0,0,150,0),"pop_up_slider",NULL,message,range_min,range_max,B_TRIANGLE_THUMB);
	slider->ResizeToPreferred();	
	slider->SetTarget(*messenger);
	slider->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			
	BRect rect = slider->Bounds();
	rect.OffsetTo(screen_point);
	rect.OffsetBy(-rect.Width()/2,-rect.Height());
	
	PopUpSlider *pop_up_slider = new PopUpSlider(rect);
	pop_up_slider->the_slider = slider;
	pop_up_slider->AddChild(slider);

	return pop_up_slider;		
}


void PopUpSlider::Go()
{
	// This function should return as soon as possible
	Show();
	PostMessage(B_MOUSE_DOWN,the_slider);	
	PostMessage(B_QUIT_REQUESTED,this);
}