/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <Message.h>
#include <Messenger.h>
#include <Region.h>
#include <Resources.h>
#include <StatusBar.h>
#include <stdio.h>

#include "StatusView.h"
#include "ColorPalette.h"
#include "PaintApplication.h"
#include "MessageConstants.h"
#include "ToolImages.h"
#include "ToolSetupWindow.h"
#include "Controls.h"
#include "UtilityClasses.h"
#include "HSPictureButton.h"
#include "Patterns.h"
#include "MagnificationView.h"
#include "SymbolImageServer.h"
#include "StringServer.h"

StatusView::StatusView(BRect frame)
				: BView(frame,"status view",B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT,B_WILL_DRAW)
{
	status_bar = NULL;
	manipulator_view = NULL;

	// First add the coordinate-view.
	float width, height;
	coordinate_view = new BStringView(frame,"coordinate_view","X: , Y:");
	coordinate_view->GetPreferredSize(&width, &height);
	width = coordinate_view->StringWidth("X: 9999 (-9999) , Y: 9999 (-9999)");

	coordinate_view->ResizeTo(width, height + 4.0);
	coordinate_view->MoveTo(4.0, 2.0);
	BRect rect(coordinate_view->Bounds());
	rect.InsetBy(-4,-2);
	rect.OffsetTo(1,1);
	coordinate_box = new BBox(rect);
	AddChild(coordinate_box);
	coordinate_box->AddChild(coordinate_view);

	rect = coordinate_box->Frame();
	rect.OffsetBy(rect.Width() + 5.0, 0.0);
	mag_state_view = new MagnificationView(rect);
	AddChild(mag_state_view);

	// Then we create the message view for displaying help messages.
	// It will be under the other views and left from the color container.
	message_view = new BStringView(BRect(coordinate_box->Frame().LeftBottom()
		+ BPoint(0,2), coordinate_box->Frame().LeftBottom() + BPoint(0,2)),
		"message view", "");
	message_view->ResizeTo(Bounds().Width() - 4.0, height);
	AddChild(message_view);
	message_view->SetResizingMode(B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);

	// Here we resize and reposition the view
	MoveBy(0,-(message_view->Frame().bottom+2));
	ResizeBy(0,message_view->Frame().bottom+2);

	// The color-container will be created last, because it needs to have
	// the correct height in constructor.
	rect.left = Bounds().right-52;
	rect.top = 1;
	rect.right = Bounds().right - 2;
	rect.bottom = Bounds().bottom - 1;

	int32 color_count = ColorSet::currentSet()->sizeOfSet();
	color_container = new ColorContainer(rect,color_count,B_FOLLOW_TOP|B_FOLLOW_RIGHT,TRUE,TRUE);
	AddChild(color_container);

	rect = BRect(color_container->Frame().LeftTop()-BPoint(color_container->Frame().Height()+2,0),color_container->Frame().LeftBottom()-BPoint(2,0));
	selected_colors = new SelectedColorsView(rect);
	AddChild(selected_colors);
	selected_colors->SetResizingMode(B_FOLLOW_TOP|B_FOLLOW_RIGHT);
	message_view->ResizeTo(selected_colors->Frame().left - message_view->Frame().left-2,message_view->Bounds().Height());

	// Here create the OK- and Cancel-buttons.
	BMessage *ok_message = new BMessage(HS_MANIPULATOR_FINISHED);
	ok_message->AddBool("status",TRUE);
	BMessage *cancel_message = new BMessage(HS_MANIPULATOR_FINISHED);
	cancel_message->AddBool("status",FALSE);

	// Create the OK-button
	int32 button_w;
	int32 button_h;
	BPicture *on_picture = SymbolImageServer::ReturnSymbolAsPicture(OK_BUTTON,button_w,button_h);
	BPicture *off_picture = SymbolImageServer::ReturnSymbolAsPicture(OK_BUTTON_PUSHED,button_w,button_h);
	ok_button = new HSPictureButton(BRect(1,0,button_w-1+1,button_h-1),off_picture,on_picture,ok_message,NULL,"Push here to confirm changes.");
	ok_button->SetResizingMode(B_FOLLOW_TOP|B_FOLLOW_RIGHT);

	// Create the cancel-button.
	on_picture = SymbolImageServer::ReturnSymbolAsPicture(CANCEL_BUTTON,button_w,button_h);
	off_picture = SymbolImageServer::ReturnSymbolAsPicture(CANCEL_BUTTON_PUSHED,button_w,button_h);
	cancel_button = new HSPictureButton(BRect(20,0,button_w+20-1,button_h-1),off_picture,on_picture,cancel_message,NULL,"Push here to cancel changes.");
	cancel_button->SetResizingMode(B_FOLLOW_TOP|B_FOLLOW_RIGHT);

	// Here we set the background-color for status_view and all it's
	// child views.
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


StatusView::~StatusView()
{
	if (coordinate_box->Parent() == NULL)
		delete coordinate_box;

	if (mag_state_view->Parent() == NULL)
		delete mag_state_view;

	if (message_view->Parent() == NULL)
		delete message_view;

	if (selected_colors->Parent() == NULL)
		delete selected_colors;

	if (color_container->Parent() == NULL)
		delete color_container;

	if (ok_button->Parent() == NULL)
		delete ok_button;

	if (cancel_button->Parent() == NULL)
		delete cancel_button;
}

status_t StatusView::DisplayManipulatorView(BView *manip_view)
{
	// Remove the status-bar if it is still here
	if (status_bar != NULL) {
		status_bar->RemoveSelf();
		delete status_bar;
		status_bar = NULL;
	}

	// Remove also the tool-view and colors-view and palette view
	color_container->RemoveSelf();
	selected_colors->RemoveSelf();

	if (manipulator_view != NULL) {
		manipulator_view->RemoveSelf();
	}

	manipulator_view = manip_view;
	if (manipulator_view != NULL)
		manipulator_view->SetResizingMode(B_FOLLOW_TOP|B_FOLLOW_RIGHT);

	ok_button->MoveTo(Bounds().right - ok_button->Frame().Width()-2,1);
	cancel_button->MoveTo(ok_button->Frame().left - cancel_button->Frame().Width() - 2,1);
	if (manipulator_view != NULL)
		manipulator_view->MoveTo(cancel_button->Frame().left-manipulator_view->Frame().Width()-5,1);

	// Center the buttons vertically
	cancel_button->MoveTo(cancel_button->Frame().left,(Bounds().Height()-cancel_button->Frame().Height())/2);
	ok_button->MoveTo(ok_button->Frame().left,(Bounds().Height()-ok_button->Frame().Height())/2);
	if (manipulator_view != NULL)
		manipulator_view->MoveTo(manipulator_view->Frame().left,(Bounds().Height() - manipulator_view->Frame().Height())/2);

	AddChild(cancel_button);
	AddChild(ok_button);
	if (manipulator_view != NULL) {
		// Make a nice box for the manipulator's view
		BRect frame = manipulator_view->Frame();
		frame.InsetBy(-3,-3);
		manipulator_box = new BBox(frame);
		manipulator_view->MoveTo(3,3);
		AddChild(manipulator_box);
		manipulator_box->AddChild(manipulator_view);
		manipulator_box->SetResizingMode(B_FOLLOW_TOP|B_FOLLOW_RIGHT);
	}
	// Set the proper target for the button.
	ok_button->SetTarget(Window()->FindView("image_view"),Window());
	cancel_button->SetTarget(Window()->FindView("image_view"),Window());


	// Resize the help-view
	if (manipulator_view != NULL)
		message_view->ResizeBy((manipulator_box->Frame().left-2) - message_view->Frame().right,0);
	else
		message_view->ResizeBy((cancel_button->Frame().left-2) - message_view->Frame().right,0);
	return B_NO_ERROR;
}

BStatusBar* StatusView::DisplayProgressIndicator()
{
	LockLooper();
	color_container->RemoveSelf();
	selected_colors->RemoveSelf();
	mag_state_view->RemoveSelf();
	coordinate_box->RemoveSelf();
	ok_button->RemoveSelf();
	cancel_button->RemoveSelf();
	message_view->RemoveSelf();

	if (manipulator_view != NULL) {
		manipulator_view->RemoveSelf();
		manipulator_view = NULL;	// This will be deleted elsewhere. (in manipulators destructor)
		if (manipulator_box != NULL) {
			manipulator_box->RemoveSelf();
			delete manipulator_box;
			manipulator_box = NULL;
		}
	}

	if (status_bar == NULL) {
		BRect frame;
		frame.left = 1;
		frame.top = 1;
		frame.right = frame.left + 150;
		frame.bottom = Bounds().bottom-1;
		status_bar = new BStatusBar(frame,"progress indicator");
	}
	if (status_bar->Parent() == NULL)
		AddChild(status_bar);

	Window()->Unlock();

	return status_bar;
}


status_t StatusView::DisplayToolsAndColors()
{
	if (Window()->FindView("image_view") != NULL) {
		BMessenger messenger(Window()->FindView("image_view"));
		mag_state_view->SetTarget(messenger);
	}
	if (status_bar != NULL) {
		status_bar->RemoveSelf();
		delete status_bar;
		status_bar = NULL;
	}
	ok_button->RemoveSelf();
	cancel_button->RemoveSelf();

	if (manipulator_view != NULL) {
		manipulator_view->RemoveSelf();
		manipulator_view = NULL;	// This will be deleted elsewhere. (in manipulators destructor)
		if (manipulator_box != NULL) {
			manipulator_box->RemoveSelf();
			delete manipulator_box;
			manipulator_box = NULL;
		}
	}

	if (mag_state_view->Parent() == NULL)
		AddChild(mag_state_view);

	if (coordinate_box->Parent() == NULL)
		AddChild(coordinate_box);

	if (selected_colors->Parent() == NULL)
		AddChild(selected_colors);

	// The message-view should be resized and color-container repositioned.
	if (message_view->Parent() == NULL)
		AddChild(message_view);
	if (color_container->Parent() == NULL)
		AddChild(color_container);

	color_container->MoveTo(Bounds().right-color_container->Frame().Width()-2,1);
	selected_colors->MoveTo(color_container->Frame().left-selected_colors->Frame().Width()-2,selected_colors->Frame().top);
	message_view->ResizeTo(selected_colors->Frame().left - message_view->Frame().left-2,message_view->Bounds().Height());

	return B_NO_ERROR;
}


status_t StatusView::RemoveToolsAndColors()
{
	Window()->Lock();
	color_container->RemoveSelf();
	selected_colors->RemoveSelf();
	Window()->Unlock();

	return B_OK;
}


status_t StatusView::DisplayNothing()
{
	if (LockLooper() == true) {
		while (CountChildren() > 0) {
			BView *view = ChildAt(0);
			RemoveChild(view);
		}
		AddChild(message_view);

		UnlockLooper();
		return B_NO_ERROR;
	}
	else {
		while (CountChildren() > 0) {
			BView *view = ChildAt(0);
			RemoveChild(view);
		}
		AddChild(message_view);

		return B_NO_ERROR;
	}
}

void StatusView::SetCoordinates(BPoint point,BPoint reference,bool use_reference)
{
	int32 x=(int32)point.x;
	int32 y=(int32)point.y;
	char coords[40];
	if (use_reference == TRUE) {
		int32 dx = (int32)fabs(point.x - reference.x) + 1;
		int32 dy = (int32)fabs(point.y - reference.y) + 1;
		sprintf(coords,"X: %ld (%ld) Y: %ld (%ld)",x,dx,y,dy);
	}
	else {
		sprintf(coords,"X: %ld   Y: %ld",x,y);
	}
	coordinate_view->SetText(coords);
}

void StatusView::SetMagnifyingScale(float mag)
{
	mag_state_view->SetMagnificationLevel(mag);
}


// here begins the definitions for selected-colors-view-class
BList* SelectedColorsView::list_of_views = new BList(10);

SelectedColorsView::SelectedColorsView(BRect frame)
				:	BBox(frame,"selected colors view")
{
	list_of_views->AddItem(this);
	ResizeTo(frame.Height(),frame.Height());

	SetBorder(B_NO_BORDER);
	foreground_color_percentage = 0.6;
}

SelectedColorsView::~SelectedColorsView()
{
	// remove ourselves from the list
	list_of_views->RemoveItem(this);
}


void SelectedColorsView::Draw(BRect area)
{
	BBox::Draw(area);
	BRect foreground_rect = Bounds();
	foreground_rect.right = floor(foreground_rect.right*foreground_color_percentage);
	foreground_rect.bottom = floor(foreground_rect.bottom*foreground_color_percentage);
	foreground_rect.left += 2;
	foreground_rect.top += 2;

	BRegion background_region;
	BRect rect = foreground_rect;
	rect.OffsetBy(Bounds().right - rect.right-2,Bounds().bottom - rect.bottom -2);
	rect.InsetBy(1,1);
	background_region.Set(rect);
	foreground_rect.InsetBy(-1,-1);
	background_region.Exclude(foreground_rect);
	foreground_rect.InsetBy(1,1);

	SetHighColor(255,255,255,255);
	StrokeRect(foreground_rect);
	foreground_rect.InsetBy(1,1);
	foreground_rect = foreground_rect & area;

	SetHighAndLowColors(((PaintApplication*)be_app)->GetColor(TRUE));
	FillRect(foreground_rect,HS_2X2_BLOCKS);

	SetHighColor(0,0,0,255);
	StrokeLine(BPoint(foreground_rect.right+2,rect.top-1),BPoint(rect.right+1,rect.top-1));
	StrokeLine(BPoint(rect.right+1,rect.top),BPoint(rect.right+1,rect.bottom));
	StrokeLine(BPoint(rect.right+1,rect.bottom+1),BPoint(rect.left-1,rect.bottom+1));
	StrokeLine(BPoint(rect.left-1,rect.bottom),BPoint(rect.left-1,foreground_rect.bottom+2));
	StrokeLine(BPoint(rect.left,foreground_rect.bottom+2),BPoint(foreground_rect.right+2,foreground_rect.bottom+2));
	StrokeLine(BPoint(foreground_rect.right+2,foreground_rect.bottom+1),BPoint(foreground_rect.right+2,rect.top));


	SetHighAndLowColors(((PaintApplication*)be_app)->GetColor(FALSE));
	FillRegion(&background_region,HS_2X2_BLOCKS);
}


void SelectedColorsView::MessageReceived(BMessage *message)
{
	switch (message->what) {
// in this case the color for one of the mousebuttons
// has changed, draw again completely

	// this might come from ColorContainer::MouseDown or ColorWindow::MessageReceived
	// where it is sent by using the sendMessageToAll member-function, it informs us
	// that the color for oneof the mousebuttons has changed
	case HS_COLOR_CHANGED:
		Invalidate();
		break;
	case B_PASTE:
		if (message->WasDropped()) {
			// Here we see on to which button it was dropped and then
			// try to extract a color from the message
			rgb_color *color;
			int32 color_size;
			if (message->FindData("RGBColor",B_RGB_COLOR_TYPE,(const void**)&color,&color_size) == B_OK) {
				BPoint drop_point = message->DropPoint();
				drop_point = ConvertFromScreen(drop_point);
				bool fore = IsPointOverForegroundColor(drop_point);
				((PaintApplication*)be_app)->SetColor(*color,fore);
				// also inform the selected colors' views
				SelectedColorsView::sendMessageToAll(new BMessage(HS_COLOR_CHANGED));
			}
		}
		break;
	default:
		BBox::MessageReceived(message);
		break;
	}
}

void SelectedColorsView::MouseDown(BPoint point)
{
	// this function should do something appropriate with the color
	// that is selected
	uint32 buttons;
	GetMouse(&point,&buttons);
//	while (buttons) {
//		GetMouse(&point,&buttons);
//		snooze(20.0 * 1000.0);
//	}

	// here check that the point is inside the view
	if ((buttons & B_PRIMARY_MOUSE_BUTTON) && Bounds().Contains(point)) {
		bool fore = IsPointOverForegroundColor(point);

		rgb_color c = ((PaintApplication*)be_app)->GetColor(fore);

		ColorPaletteWindow::showPaletteWindow();
		ColorPaletteWindow::ChangePaletteColor(c);
	}
	else {
		rgb_color foreground = ((PaintApplication*)be_app)->GetColor(true);
		rgb_color background = ((PaintApplication*)be_app)->GetColor(false);

		((PaintApplication*)be_app)->SetColor(background,true);
		((PaintApplication*)be_app)->SetColor(foreground,false);

		BMessage *message = new BMessage(HS_COLOR_CHANGED);
		sendMessageToAll(message);
		delete message;
	}
}


void SelectedColorsView::MouseMoved(BPoint, uint32 transit, const BMessage*)
{
	if ((transit == B_ENTERED_VIEW) && (Window()->IsActive())) {
		BMessage *help_message = new BMessage(HS_TEMPORARY_HELP_MESSAGE);
		help_message->AddString("message",StringServer::ReturnString(SELECTED_COLORS_VIEW_MESSAGE1_STRING));
		Window()->PostMessage(help_message,Window());
	}
	if (transit == B_EXITED_VIEW)
		Window()->PostMessage(HS_TOOL_HELP_MESSAGE,Window());
}

void SelectedColorsView::sendMessageToAll(BMessage *message)
{
	SelectedColorsView *help;

	for (int32 i=0;i<list_of_views->CountItems();i++) {
		help = (SelectedColorsView*)list_of_views->ItemAt(i);
		help->Window()->PostMessage(message,help);
	}
}

void SelectedColorsView::SetHighAndLowColors(const rgb_color &c)
{
	rgb_color low = c;
	rgb_color high = c;

	float coeff = c.alpha / 255.0;
	low.red = (uint8)(coeff*c.red);
	low.green = (uint8)(coeff*c.green);
	low.blue = (uint8)(coeff*c.blue);
	low.alpha = 255;

	high.red = (uint8)(coeff*c.red + (1-coeff)*255);
	high.green = (uint8)(coeff*c.green + (1-coeff)*255);
	high.blue = (uint8)(coeff*c.blue + (1-coeff)*255);
	high.alpha = 255;

	SetHighColor(high);
	SetLowColor(low);
}


bool SelectedColorsView::IsPointOverForegroundColor(BPoint point)
{
	BRect rect = Bounds();
	rect.right *= foreground_color_percentage;
	rect.bottom *= foreground_color_percentage;
	rect.top += 2;
	rect.left += 2;

	if (rect.Contains(point))
		return TRUE;
	else
		return FALSE;
}
