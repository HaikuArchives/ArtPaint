/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <ClassInfo.h>
#include <Slider.h>
#include <stdio.h>


#include "MagnificationView.h"
#include "MessageConstants.h"
#include "StringServer.h"
#include "PopUpSlider.h"
#include "ImageView.h"
#include "Cursors.h"

MagnificationView::MagnificationView(BRect rect)
	: BView(rect,"magnification_view",B_FOLLOW_TOP|B_FOLLOW_LEFT,B_WILL_DRAW)
{
	char string[256];
	sprintf(string,"%s: %.1f%%",StringServer::ReturnString(MAG_STRING),1600.0);

	BRect string_rect;
	string_rect.left = 0;
	string_rect.right = StringWidth(string);
	ResizeTo(string_rect.Width()+2*(rect.Height()+4),rect.Height());
//	string_rect.OffsetBy(rect.Height()+4,0);
	rect = Bounds();

	BRect button_rect;
	button_rect.left = 0;
	button_rect.top = 0;
	button_rect.bottom = rect.Height()-0;
	button_rect.right = rect.Height();
	button_rect.OffsetTo(rect.Width()-2*button_rect.Width(),0);
	minus_button = new BButton(button_rect,"minus_button","-",new BMessage(HS_ZOOM_IMAGE_OUT));
	minus_button->ResizeTo(button_rect.Width(),button_rect.Height());
	AddChild(minus_button);

	button_rect.OffsetBy(button_rect.Width(),0);
	plus_button = new BButton(button_rect,"plus_button","+",new BMessage(HS_ZOOM_IMAGE_IN));
	plus_button->ResizeTo(button_rect.Width(),button_rect.Height());
	AddChild(plus_button);

	string_rect.top = button_rect.top;
	string_rect.bottom = button_rect.bottom;
	string_rect.InsetBy(-2,0);
	string_rect.OffsetTo(0,0);
	string_box = new BBox(string_rect,"string_box");
	string_box->SetBorder(B_PLAIN_BORDER);
	AddChild(string_box);

	string_rect.InsetBy(2,1);
	string_rect.OffsetTo(1,1);
	string_view = new MagStringView(string_rect,"string_view",string);
	string_box->AddChild(string_view);

	string_view->AddFilter(new BMessageFilter(B_MOUSE_DOWN,filter1));
}

void MagnificationView::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
}

void MagnificationView::Draw(BRect area)
{
	BView::Draw(area);
}


void MagnificationView::SetMagnificationLevel(float level)
{
	char string[256];
	// Convert to percentage.
	sprintf(string,"%s: %.1f%%",StringServer::ReturnString(MAG_STRING),100.0*level);
	string_view->SetText(string);
}


void MagnificationView::SetTarget(BMessenger &messenger)
{
	minus_button->SetTarget(messenger);
	plus_button->SetTarget(messenger);
}


// ----------
MagStringView::MagStringView(BRect rect,char *name, char *label)
	: BStringView(rect,name,label)
{
}

void MagStringView::MouseMoved(BPoint,uint32 transit,const BMessage*)
{
	if (transit == B_ENTERED_VIEW) {
		be_app->SetCursor(HS_MINUS_PLUS_HAND_CURSOR);
	}
	else if (transit == B_EXITED_VIEW) {
		be_app->SetCursor(B_HAND_CURSOR);
	}
}


//---------
filter_result filter1(BMessage *message,BHandler **handlers,BMessageFilter *filter)
{
	BWindow *window = cast_as(filter->Looper(),BWindow);
	BView *image_view = NULL;
	BView *string_view = cast_as(handlers[0],BView);

	if (window != NULL) {
		image_view = window->FindView("image_view");
	}

	int32 value = (int32)((sqrt(((ImageView*)image_view)->getMagScale())*16/15.9-0.1)*100);

	BPoint point;
	message->FindPoint("point",&point);
	BPoint screen_point = string_view->ConvertToScreen(point);

	PopUpSlider *slider = PopUpSlider::Instantiate(screen_point,new BMessenger(image_view,window),new BMessage(HS_SET_MAGNIFYING_SCALE),10,1600);
	slider->ReturnSlider()->SetModificationMessage(new BMessage(HS_SET_MAGNIFYING_SCALE));
	slider->ReturnSlider()->SetValue(value);
	slider->MoveTo(screen_point.x - slider->ReturnSlider()->Position()*slider->Bounds().Width(),slider->Frame().top);
	slider->Go();

	return B_SKIP_MESSAGE;
}
