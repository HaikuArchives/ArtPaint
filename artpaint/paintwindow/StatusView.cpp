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


#include <Button.h>
#include <LayoutBuilder.h>
#include <Region.h>
#include <StatusBar.h>
#include <Window.h>


#include <stdio.h>


#define TOOLS_VIEW			0
#define PROGRESS_VIEW 		1
#define MANIPULATOR_VIEW	2
#define START_VIEW			3


StatusView::StatusView()
	: BView("status view", B_WILL_DRAW)
	, fOk(NULL)
	, fCancel(NULL)
{
	status_bar = NULL;
	manipulator_view = NULL;

	// First add the coordinate-view.
	coordinate_view = new BStringView("coordinate_view","X: , Y:");

	coordinate_box = new BBox("coordinate box");
	BGroupLayout* boxLayout = BLayoutBuilder::Group<>(coordinate_box, B_HORIZONTAL)
		.Add(coordinate_view)
		.SetInsets(3.0, 5.0, 3.0, 5.0);

	mag_state_view = new MagnificationView();

	// Then we create the message view for displaying help messages.
	// It will be under the other views and left from the color container.
	fHelpView = new BStringView("message view", "");

	BRect rect = BRect(0, 0, 50, 50);
	int32 color_count = ColorSet::currentSet()->sizeOfSet();
	color_container = new ColorContainer(rect, color_count, 0, true, true);

	selected_colors = new SelectedColorsView(rect);
	selected_colors->SetExplicitMinSize(BSize(52, 52));
	selected_colors->SetExplicitMaxSize(BSize(52, 52));

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
		fOk = new HSPictureButton(BRect(0, 0, 15, 15), &off_picture,
			&on_picture, ok_message, NULL, "Push here to confirm changes.");

		// Create the cancel-button.
		server->GetPicture(CANCEL_BUTTON, &on_picture);
		server->GetPicture(CANCEL_BUTTON_PUSHED, &off_picture);

		fCancel = new HSPictureButton(BRect(0, 0, 15, 15), &off_picture,
			&on_picture, cancel_message, NULL, "Push here to cancel changes.");

	}

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	status_bar = new BStatusBar("progress indicator");

	manipulator_view = new BGroupLayout(B_VERTICAL);

	fStartCard = BLayoutBuilder::Group<>(B_VERTICAL, 5.0);

	BGridLayout* toolsAndColorsCard = BLayoutBuilder::Grid<>(5.0, 5.0)
		.AddGlue(0, 0)
		.Add(selected_colors, 1, 0, 1, 2)
		.Add(color_container, 2, 0, 1, 2)
		.SetInsets(0.0, 5.0, 0.0, 0.0);
	toolsAndColorsCard->SetMaxColumnWidth(2, 52);

	BGroupLayout* progressBarCard = BLayoutBuilder::Group<>(B_VERTICAL, 5.0)
		.Add(status_bar)
		.SetInsets(2.0, 0.0, 5.0, 0.0);

	BGroupLayout* manipulatorCard = BLayoutBuilder::Group<>(B_HORIZONTAL, 0.0)
		.AddGlue()
		.AddGroup(B_VERTICAL, 5.0)
			.Add(manipulator_view)
			.AddGroup(B_HORIZONTAL, 5.0)
				.Add(fOk)
				.Add(fCancel)
				.End()
			.End()
		.SetInsets(0.0, 2.0, 0.0, 0.0);

	fCardLayout = BLayoutBuilder::Cards<>()
		.Add(toolsAndColorsCard)
		.Add(progressBarCard)
		.Add(manipulatorCard)
		.Add(fStartCard);

	fStatusView = BLayoutBuilder::Grid<>(this, 5.0, 5.0)
		.Add(coordinate_box, 0, 0)
		.Add(mag_state_view, 0, 1)
		.Add(fHelpView, 0, 2, 3)
		.Add(fCardLayout, 1, 0, 3, 2);
	fStatusView->SetMinColumnWidth(0, StringWidth("X: 9999 (-9999) , Y: 9999 (-9999)"));

	fCardLayout->SetVisibleItem(START_VIEW);
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
StatusView::DisplayStartCard(BBox *card)
{
	if (Window()->Lock()) {
		fStartCard->AddView(card);
		fCardLayout->SetVisibleItem(START_VIEW);
		Window()->Unlock();
	}

	return B_OK;
}


status_t
StatusView::DisplayManipulatorView(BView *manip_view)
{
	if (Window()->Lock()) {
		manipulator_view->AddView(manip_view);

		// Set the proper target for the button.
		fOk->SetTarget(Window()->FindView("image_view"),Window());
		fCancel->SetTarget(Window()->FindView("image_view"),Window());

		fCardLayout->SetVisibleItem(MANIPULATOR_VIEW);
		Window()->Unlock();
	}

	return B_OK;
}


BStatusBar*
StatusView::DisplayProgressIndicator()
{
	if (Window()->Lock()) {
		fCardLayout->SetVisibleItem(PROGRESS_VIEW);
		Window()->Unlock();
	}

	return status_bar;
}


status_t
StatusView::DisplayToolsAndColors()
{
	if (BView* view = Window()->FindView("image_view"))
		mag_state_view->SetTarget(view);

	fCardLayout->SetVisibleItem(TOOLS_VIEW);

	return B_OK;
}


status_t
StatusView::DisplayNothing()
{
	if (Window()->Lock()) {
		fCardLayout->SetVisibleItem(START_VIEW);
		Window()->Unlock();
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


void
StatusView::RemoveManipulator()
{
	if (Window()->Lock()) {
		while(manipulator_view->CountItems()) {
			manipulator_view->RemoveItem(0);
		}
		Window()->Unlock();
	}
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
//	foreground_rect = foreground_rect & area;

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
				ssize_t color_size;
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
