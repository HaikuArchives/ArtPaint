/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>

#include "PopUpColorWindow.h"
#include "Controls.h"
#include "UtilityClasses.h"

PopUpColorWindow::PopUpColorWindow(BRect frame,BMessenger *t,BMessage *default_out_message)
	: BWindow(frame,"pop up color window",B_BORDERED_WINDOW,0)
{
	// We could also reposition ourselves so that we are
	// fully visible on the screen.
	target = t;
	default_message = default_out_message;

	frame.OffsetTo(BPoint(0,0));
	frame.InsetBy(1,1);
	ColorWell *color_well = new ColorWell(frame,new BMessenger(this));
	AddChild(color_well);
	// Finally show ourselves
	Show();
}


PopUpColorWindow::~PopUpColorWindow()
{
	delete target;
}

void PopUpColorWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case COLOR_SELECTED:
			// Here we generate a röColor -like message and post
			// it to our target. We also close ourselves.
			if (default_message == NULL) {
				default_message = new BMessage(B_PASTE);
			}
			rgb_color c = BGRAColorToRGB(message->FindInt32("color"));
			default_message->AddData("RGBColor",B_RGB_COLOR_TYPE,&c,sizeof(struct rgb_color));
			target->SendMessage(default_message);
			delete default_message;
			Close();
			break;
		BWindow::MessageReceived(message);
			break;
	}
}

void PopUpColorWindow::WindowActivated(bool active)
{
	// This function is implemented to close this window
	// when it is deactivated.
	if (active == FALSE) {
		Lock();
		Quit();
		Unlock();
	}
}





ColorWell::ColorWell(BRect frame,BMessenger *t)
	: BView(frame,"a color well",B_FOLLOW_ALL,B_WILL_DRAW)
{
	// Here we create a colormap. The colormap will be almost as big as the frame.
	// A frame other than one starting from (0,0) causes some very strange
	// effects in the bitmap.
	frame.InsetBy(1,1);
	frame.OffsetTo(BPoint(0,0));
	BRect slider_frame = BRect(1,frame.bottom-20,frame.right,frame.bottom-1);
	BMessage *message = new BMessage(BLUE_VALUE_CHANGED);
	message->AddInt32("value",0);
	slider = new ControlSliderBox(slider_frame,"blue slider","Blue","0",message,0,255);
	AddChild(slider);
	slider->MoveTo(slider->Frame().left,slider_frame.bottom-(slider->Frame().Height()));
	frame.bottom = slider->Frame().top-1;
	color_map = new BBitmap(frame,B_RGB_32_BIT);
	blue_value = 0;
	create_color_map();
	target = t;
}

ColorWell::~ColorWell()
{
	delete color_map;
	delete target;
}

void ColorWell::AttachedToWindow()
{
	slider->SetTarget(new BMessenger(this));
}

void ColorWell::Draw(BRect)
{
	DrawBitmap(color_map,BPoint(0,0));
	SetHighColor(0,0,255,255);
	StrokeRect(Bounds());
}

void ColorWell::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case BLUE_VALUE_CHANGED:
			blue_value = message->FindInt32("value");
			create_color_map();
			Draw(Bounds());
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}

void ColorWell::MouseDown(BPoint location)
{
	// Here we post a selection message to the window.
	uint32 buttons;
	GetMouse(&location,&buttons);
	while (buttons) {

		GetMouse(&location,&buttons);
		snooze(20 * 1000);
	}

	// We send a message to our target containing the selected color.
	uint32 rows = color_map->Bounds().IntegerHeight()+1;
	uint32 columns = color_map->BytesPerRow()/4;
	uint32 red_val = (uint32)min_c(255,max_c(0,(location.x-1)/columns*255.0));
	uint32 green_val = (uint32)min_c(255,max_c(0,(location.y-1)/rows*255.0));
	uint32 color_value = (blue_value<<24)|(green_val<<16)|(red_val<<8)|0xFF;
	BMessage *message = new BMessage(COLOR_SELECTED);
	message->AddInt32("color",color_value);
	target->SendMessage(message);
	delete message;
}


void ColorWell::create_color_map()
{
	uint32 *bits = (uint32*)color_map->Bits();
	int32 bpr = color_map->BytesPerRow()/4;
	int32 rows = color_map->Bounds().IntegerHeight()+1;

	uint32 red_val;
	uint32 green_val;

	for (int32 y=0;y<rows;y++) {
		for (int32 x=0;x<bpr;x++) {
			red_val = (uint32)((float)x/(float)bpr * 255);
			green_val = (uint32)((float)y/(float)rows * 255);
			*bits++ =  (red_val<< 8) & 0xFF00 |  (green_val << 16) & 0xFF0000 | blue_value<<24;
		}
	}
}
