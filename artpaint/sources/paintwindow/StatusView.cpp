/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "StatusView.h"

#include "ColorPalette.h"
#include "PaintApplication.h"
#include "MessageConstants.h"
#include "HSPictureButton.h"
#include "Patterns.h"
#include "MagnificationView.h"
#include "ResourceServer.h"
#include "StringServer.h"


#include <Region.h>
#include <StatusBar.h>
#include <Window.h>


#include <stdio.h>


StatusView::StatusView(BRect frame)
	: BView(frame,"status view", B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT,
		B_WILL_DRAW)
	, fOk(NULL)
	, fCancel(NULL)
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
	fHelpView = new BStringView(BRect(coordinate_box->Frame().LeftBottom()
		+ BPoint(0,2), coordinate_box->Frame().LeftBottom() + BPoint(0,2)),
		"message view", "");
	fHelpView->ResizeTo(Bounds().Width() - 4.0, height);
	AddChild(fHelpView);
	fHelpView->SetResizingMode(B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);

	// Here we resize and reposition the view
	MoveBy(0,-(fHelpView->Frame().bottom+2));
	ResizeBy(0,fHelpView->Frame().bottom+2);

	// The color-container will be created last, because it needs to have
	// the correct height in constructor.
	rect.left = Bounds().right-52;
	rect.top = 1;
	rect.right = Bounds().right - 2;
	rect.bottom = Bounds().bottom - 1;

	int32 color_count = ColorSet::currentSet()->sizeOfSet();
	color_container = new ColorContainer(rect, color_count, B_FOLLOW_TOP
		| B_FOLLOW_RIGHT, true, true);
	AddChild(color_container);

	rect = BRect(color_container->Frame().LeftTop()
		- BPoint(color_container->Frame().Height() + 2, 0),
		color_container->Frame().LeftBottom() - BPoint(2, 0));
	selected_colors = new SelectedColorsView(rect);
	AddChild(selected_colors);
	selected_colors->SetResizingMode(B_FOLLOW_TOP | B_FOLLOW_RIGHT);
	fHelpView->ResizeTo(selected_colors->Frame().left
		- fHelpView->Frame().left - 2, fHelpView->Bounds().Height());

	// Here create the OK- and Cancel-buttons.
	BMessage *ok_message = new BMessage(HS_MANIPULATOR_FINISHED);
	ok_message->AddBool("status",TRUE);
	BMessage *cancel_message = new BMessage(HS_MANIPULATOR_FINISHED);
	cancel_message->AddBool("status",FALSE);

	ResourceServer* server = ResourceServer::Instance();
	if (server) {
		BPicture on_picture;
		BPicture off_picture;

		server->GetPicture(OK_BUTTON, &on_picture);
		server->GetPicture(OK_BUTTON_PUSHED, &off_picture);

		// Create the OK-button
		fOk = new HSPictureButton(BRect(1, 0, 16, 15), &off_picture,
			&on_picture, ok_message, NULL, "Push here to confirm changes.");
		fOk->ResizeToPreferred();
		fOk->SetResizingMode(B_FOLLOW_TOP|B_FOLLOW_RIGHT);

		// Create the cancel-button.
		server->GetPicture(CANCEL_BUTTON, &on_picture);
		server->GetPicture(CANCEL_BUTTON_PUSHED, &off_picture);

		fCancel = new HSPictureButton(BRect(20, 0, 35, 16), &off_picture,
			&on_picture, cancel_message, NULL, "Push here to cancel changes.");
		fCancel->ResizeToPreferred();
		fCancel->SetResizingMode(B_FOLLOW_TOP|B_FOLLOW_RIGHT);
	}

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


StatusView::~StatusView()
{
	if (coordinate_box->Parent() == NULL)
		delete coordinate_box;

	if (mag_state_view->Parent() == NULL)
		delete mag_state_view;

	if (fHelpView->Parent() == NULL)
		delete fHelpView;

	if (selected_colors->Parent() == NULL)
		delete selected_colors;

	if (color_container->Parent() == NULL)
		delete color_container;

	if (fOk && fOk->Parent() == NULL)
		delete fOk;

	if (fCancel && fCancel->Parent() == NULL)
		delete fCancel;
}


status_t
StatusView::DisplayManipulatorView(BView *manip_view)
{
	// Remove the status-bar if it is still here
	if (status_bar) {
		status_bar->RemoveSelf();
		delete status_bar;
		status_bar = NULL;
	}

	// Remove also the tool-view and colors-view and palette view
	color_container->RemoveSelf();
	selected_colors->RemoveSelf();

	if (manipulator_view)
		manipulator_view->RemoveSelf();

	manipulator_view = manip_view;
	if (manipulator_view)
		manipulator_view->SetResizingMode(B_FOLLOW_TOP | B_FOLLOW_RIGHT);

	fOk->MoveTo(Bounds().right - fOk->Frame().Width() - 2, 1);
	fCancel->MoveTo(fOk->Frame().left
		- fCancel->Frame().Width() - 2, 1);
	if (manipulator_view) {
		manipulator_view->MoveTo(fCancel->Frame().left
			- manipulator_view->Frame().Width() - 5, 1);
	}

	// Center the buttons vertically
	fCancel->MoveTo(fCancel->Frame().left,
		(Bounds().Height() - fCancel->Frame().Height()) / 2);
	fOk->MoveTo(fOk->Frame().left,
		(Bounds().Height() - fOk->Frame().Height()) / 2);

	if (manipulator_view) {
		manipulator_view->MoveTo(manipulator_view->Frame().left,
			(Bounds().Height() - manipulator_view->Frame().Height()) / 2);
	}

	AddChild(fCancel);
	AddChild(fOk);
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
	fOk->SetTarget(Window()->FindView("image_view"),Window());
	fCancel->SetTarget(Window()->FindView("image_view"),Window());

	// Resize the help-view
	BView* view = manipulator_view;
	if (!view)
		view = fCancel;
	fHelpView->ResizeBy((view->Frame().left - 2) - fHelpView->Frame().right,0);

	return B_OK;
}


BStatusBar*
StatusView::DisplayProgressIndicator()
{
	LockLooper();
	color_container->RemoveSelf();
	selected_colors->RemoveSelf();
	mag_state_view->RemoveSelf();
	coordinate_box->RemoveSelf();
	fOk->RemoveSelf();
	fCancel->RemoveSelf();
	fHelpView->RemoveSelf();

	if (manipulator_view != NULL) {
		manipulator_view->RemoveSelf();
		// This will be deleted elsewhere. (in manipulators destructor)
		manipulator_view = NULL;
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


status_t
StatusView::DisplayToolsAndColors()
{
	if (BView* view = Window()->FindView("image_view"))
		mag_state_view->SetTarget(view);

	if (status_bar) {
		status_bar->RemoveSelf();
		delete status_bar;
		status_bar = NULL;
	}

	fOk->RemoveSelf();
	fCancel->RemoveSelf();

	if (manipulator_view) {
		manipulator_view->RemoveSelf();
		// This will be deleted elsewhere. (in manipulators destructor)
		manipulator_view = NULL;
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
	if (fHelpView->Parent() == NULL)
		AddChild(fHelpView);

	if (color_container->Parent() == NULL)
		AddChild(color_container);

	color_container->MoveTo(Bounds().right-color_container->Frame().Width()-2,1);
	selected_colors->MoveTo(color_container->Frame().left
		- selected_colors->Frame().Width() - 2, selected_colors->Frame().top);
	fHelpView->ResizeTo(selected_colors->Frame().left
		- fHelpView->Frame().left - 2, fHelpView->Bounds().Height());

	return B_OK;
}


status_t
StatusView::RemoveToolsAndColors()
{
	if (Window()->Lock()) {
		color_container->RemoveSelf();
		selected_colors->RemoveSelf();
		Window()->Unlock();
	}
	return B_OK;
}


status_t
StatusView::DisplayNothing()
{
	// TODO: Why locked and unlocked?
	if (LockLooper()) {
		while (BView *view = ChildAt(0))
			RemoveChild(view);
		AddChild(fHelpView);
		UnlockLooper();
	} else {
		while (BView *view = ChildAt(0))
			RemoveChild(view);
		AddChild(fHelpView);
	}
	return B_OK;
}


void
StatusView::SetCoordinates(BPoint point, BPoint reference, bool use_reference)
{
	int32 x=(int32)point.x;
	int32 y=(int32)point.y;
	char coords[40];
	if (use_reference) {
		int32 dx = (int32)fabs(point.x - reference.x) + 1;
		int32 dy = (int32)fabs(point.y - reference.y) + 1;
		sprintf(coords,"X: %ld (%ld) Y: %ld (%ld)",x,dx,y,dy);
	} else {
		sprintf(coords,"X: %ld   Y: %ld",x,y);
	}
	coordinate_view->SetText(coords);
}


void
StatusView::SetMagnifyingScale(float mag)
{
	mag_state_view->SetMagnificationLevel(mag);
}


// #pragma mark -- SelectedColorsView


BList SelectedColorsView::list_of_views(10);


SelectedColorsView::SelectedColorsView(BRect frame)
	: BBox(frame, "selected colors view")
{
	SetBorder(B_NO_BORDER);

	list_of_views.AddItem(this);
	ResizeTo(frame.Height(),frame.Height());
	foreground_color_percentage = 0.6;
}


SelectedColorsView::~SelectedColorsView()
{
	// remove ourselves from the list
	list_of_views.RemoveItem(this);
}


void
SelectedColorsView::Draw(BRect area)
{
	BBox::Draw(area);

	BRect foreground_rect = Bounds();
	foreground_rect.right = floor(foreground_rect.right*foreground_color_percentage);
	foreground_rect.bottom = floor(foreground_rect.bottom*foreground_color_percentage);
	foreground_rect.left += 2;
	foreground_rect.top += 2;

	BRegion background_region;
	BRect rect = foreground_rect;
	rect.OffsetBy(Bounds().right - rect.right - 2, Bounds().bottom - rect.bottom -2);
	rect.InsetBy(1, 1);
	background_region.Set(rect);
	foreground_rect.InsetBy(-1,-1);
	background_region.Exclude(foreground_rect);
	foreground_rect.InsetBy(1,1);

	SetHighColor(255, 255, 255, 255);
	StrokeRect(foreground_rect);
	foreground_rect.InsetBy(1, 1);
	foreground_rect = foreground_rect & area;

	SetHighAndLowColors(((PaintApplication*)be_app)->Color(TRUE));
	FillRect(foreground_rect, HS_2X2_BLOCKS);

	SetHighColor(0, 0, 0, 255);
	StrokeLine(BPoint(foreground_rect.right + 2, rect.top - 1),
		BPoint(rect.right + 1, rect.top - 1));
	StrokeLine(BPoint(rect.right + 1, rect.top),
		BPoint(rect.right + 1, rect.bottom));
	StrokeLine(BPoint(rect.right + 1, rect.bottom + 1),
		BPoint(rect.left - 1, rect.bottom + 1));
	StrokeLine(BPoint(rect.left - 1, rect.bottom),
		BPoint(rect.left - 1, foreground_rect.bottom + 2));
	StrokeLine(BPoint(rect.left, foreground_rect.bottom + 2),
		BPoint(foreground_rect.right + 2, foreground_rect.bottom + 2));
	StrokeLine(BPoint(foreground_rect.right + 2, foreground_rect.bottom + 1),
		BPoint(foreground_rect.right + 2, rect.top));

	SetHighAndLowColors(((PaintApplication*)be_app)->Color(FALSE));
	FillRegion(&background_region, HS_2X2_BLOCKS);
}


void
SelectedColorsView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		// In this case the color for one of the mousebuttons has changed, draw
		// again completely. This might come from ColorContainer::MouseDown or
		// ColorWindow::MessageReceived where it is sent by using the
		// SendMessageToAll member-function, it informs us that the color for
		// oneof the mousebuttons has changed
		case HS_COLOR_CHANGED: {
			Invalidate();
		}	break;

		case B_PASTE: {
			if (message->WasDropped()) {
				// Here we see on to which button it was dropped and then
				// try to extract a color from the message
				rgb_color *color;
				int32 color_size;
				if (message->FindData("RGBColor",B_RGB_COLOR_TYPE,
						(const void**)&color, &color_size) == B_OK) {
					BPoint drop_point = message->DropPoint();
					drop_point = ConvertFromScreen(drop_point);
					((PaintApplication*)be_app)->SetColor(*color,
						IsPointOverForegroundColor(drop_point));
					// also inform the selected colors' views
					SelectedColorsView::SendMessageToAll(HS_COLOR_CHANGED);
				}
			}
		}	break;

		default: {
			BBox::MessageReceived(message);
		}	break;
	}
}


void
SelectedColorsView::MouseDown(BPoint point)
{
	// this function should do something appropriate with the color
	// that is selected
	uint32 buttons;
	GetMouse(&point, &buttons);

	// here check that the point is inside the view
	if ((buttons & B_PRIMARY_MOUSE_BUTTON) && Bounds().Contains(point)) {
		bool fore = IsPointOverForegroundColor(point);

		rgb_color c = ((PaintApplication*)be_app)->Color(fore);

		ColorPaletteWindow::showPaletteWindow();
		ColorPaletteWindow::ChangePaletteColor(c);
	} else {
		rgb_color foreground = ((PaintApplication*)be_app)->Color(true);
		rgb_color background = ((PaintApplication*)be_app)->Color(false);

		((PaintApplication*)be_app)->SetColor(background,true);
		((PaintApplication*)be_app)->SetColor(foreground,false);

		SelectedColorsView::SendMessageToAll(HS_COLOR_CHANGED);
	}
}


void
SelectedColorsView::MouseMoved(BPoint, uint32 transit, const BMessage*)
{
	if (transit == B_ENTERED_VIEW && Window()->IsActive()) {
		BMessage message(HS_TEMPORARY_HELP_MESSAGE);
		message.AddString("message",
			StringServer::ReturnString(SELECTED_COLORS_VIEW_MESSAGE1_STRING));
		Window()->PostMessage(&message, Window());
	}

	if (transit == B_EXITED_VIEW)
		Window()->PostMessage(HS_TOOL_HELP_MESSAGE, Window());
}


void
SelectedColorsView::SendMessageToAll(uint32 what)
{
	for (int32 i = 0; i < list_of_views.CountItems(); ++i) {
		if (SelectedColorsView* view =
			static_cast<SelectedColorsView*> (list_of_views.ItemAt(i))) {
			view->Window()->PostMessage(what, view);
		}
	}
}


void
SelectedColorsView::sendMessageToAll(BMessage *message)
{
	SelectedColorsView *help;

	for (int32 i=0;i<list_of_views.CountItems();i++) {
		help = (SelectedColorsView*)list_of_views.ItemAt(i);
		help->Window()->PostMessage(message,help);
	}
}


void
SelectedColorsView::SetHighAndLowColors(const rgb_color &c)
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


bool
SelectedColorsView::IsPointOverForegroundColor(BPoint point)
{
	BRect rect = Bounds();
	rect.right *= foreground_color_percentage;
	rect.bottom *= foreground_color_percentage;
	rect.top += 2;
	rect.left += 2;

	if (rect.Contains(point))
		return true;
	return false;
}
