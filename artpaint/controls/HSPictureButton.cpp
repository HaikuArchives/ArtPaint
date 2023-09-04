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

#include "HSPictureButton.h"

#include "MessageConstants.h"
#include "ToolSetupWindow.h"


#include <Bitmap.h>
#include <View.h>


HSPictureButton::HSPictureButton(BRect frame, BPicture* off, BPicture* on, BMessage* message,
	const char* helpMessage, const char* longHelpMessage, uint32 behavior, uint32 resizingMode,
	uint32 flags)
	:
	BPictureButton(frame, "?", off, on, message, behavior, resizingMode, flags),
	fLongHelpMessage(longHelpMessage)
{
	if (helpMessage)
		SetToolTip(helpMessage);
}


HSPictureButton::HSPictureButton(BRect frame, BBitmap* off, BBitmap* on, BMessage* message,
	const char* helpMessage, const char* longHelpMessage, uint32 behavior, uint32 mode,
	uint32 flags)
	:
	BPictureButton(frame, "?", NULL, NULL, message, behavior, mode, flags),
	fLongHelpMessage(longHelpMessage)
{
	if (helpMessage)
		SetToolTip(helpMessage);

	BRect rect(0, 0, 0, 0);
	BBitmap* offScreen = new BBitmap(rect, B_RGB_32_BIT, true);
	BView* offView = new BView(rect, "", B_FOLLOW_ALL, 0);

	offScreen->AddChild(offView);
	offScreen->Lock();

	offView->SetHighColor(255, 0, 0);
	offView->SetLowColor(0, 0, 120);
	offView->SetDrawingMode(B_OP_COPY);

	offView->BeginPicture(new BPicture());
	offView->DrawBitmap(on, BPoint(0, 0));
	SetEnabledOn(offView->EndPicture());

	offView->BeginPicture(new BPicture());
	offView->DrawBitmap(off, BPoint(0, 0));
	SetEnabledOff(offView->EndPicture());

	offScreen->Unlock();

	delete offScreen;
}


HSPictureButton::~HSPictureButton()
{
}


void
HSPictureButton::MouseDown(BPoint location)
{
	// here we take the button that was pressed and the click number
	int32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	if (BMessage* message = Message()) {
		// Set the invokers message to contain the mouse-button that was last
		// pressed.
		if (message->HasUInt32("buttons"))
			message->ReplaceUInt32("buttons", buttons);
	}

	BPictureButton::MouseDown(location);
}


void
HSPictureButton::MouseMoved(BPoint point, uint32 transit, const BMessage* msg)
{
	if (transit == B_ENTERED_VIEW) {
		if (fLongHelpMessage.Length() > 0 && Window()->IsActive()) {
			BMessage message(HS_TEMPORARY_HELP_MESSAGE);
			message.AddString("message", fLongHelpMessage);
			Window()->PostMessage(&message, Window());
		}
	} else if (transit == B_EXITED_VIEW) {
		if (fLongHelpMessage.Length() > 0 && Window()->IsActive())
			Window()->PostMessage(HS_TOOL_HELP_MESSAGE, Window());
	}
}
