/*

	Filename:	HSPictureButton.cpp
	Contents:	HSPictureButton-class definition
	Author:		Heikki Suhonen

*/

#include <Bitmap.h>
#include <stdlib.h>
#include <string.h>
#include <View.h>

#include "HSPictureButton.h"
#include "UtilityClasses.h"
#include "ToolSetupWindow.h"
#include "MessageConstants.h"

HSPictureButton::HSPictureButton(BRect frame,BPicture *off,BPicture *on,BMessage *message,const char *help_msg,const char *longer_help,uint32 behavior,uint32 resizingMode ,uint32 flags)
			:	BPictureButton(frame,"a HSPictureButton",off,on,message,behavior,resizingMode,flags)
{
	// Store the help messages.
	if (help_msg != NULL) {
		help_message = (char*)malloc(strlen(help_msg)+1);
		strcpy(help_message,help_msg);
	}
	else
		help_message = NULL;

	if (longer_help != NULL) {
		long_help_message = (char*)malloc(strlen(longer_help)+1);
		strcpy(long_help_message,longer_help);
	}
	else
		long_help_message = NULL;

	help_window = NULL;
}


HSPictureButton::HSPictureButton(BRect frame,BBitmap *off,BBitmap *on,BMessage *message,const char *help_msg,const char *longer_help,uint32 behavior,uint32 resizingMode ,uint32 flags)
			:	BPictureButton(frame,"a HSPictureButton",new BPicture(),new BPicture(),message,behavior,resizingMode,flags)
{
	// Store the help messages.
	if (help_msg != NULL) {
		help_message = (char*)malloc(strlen(help_msg)+1);
		strcpy(help_message,help_msg);
	}
	else
		help_message = NULL;

	if (longer_help != NULL) {
		long_help_message = (char*)malloc(strlen(longer_help)+1);
		strcpy(long_help_message,longer_help);
	}
	else
		long_help_message = NULL;

	// Construct a bitmap for the purpose of creating BPicture-objects
	// from the bitmaps.

	// The parameter bitmaps should NOT be B_MONOCHROME_1_BIT, because
	// they do not seem to work well with these buttons (or with BPictures).
	BBitmap *off_screen = new BBitmap(BRect(0,0,0,0),B_RGB_32_BIT,TRUE);
	BView *off_view = new BView(BRect(0,0,0,0),"",B_FOLLOW_ALL,0);

	off_screen->AddChild(off_view);

	off_screen->Lock();

	BPicture *off_picture,*on_picture;
	off_view->SetHighColor(255,0,0);
	off_view->SetLowColor(0,0,120);
	off_view->SetDrawingMode(B_OP_COPY);

	off_view->BeginPicture(new BPicture());
	off_view->DrawBitmap(on,BPoint(0,0));
	on_picture = off_view->EndPicture();

	off_view->BeginPicture(new BPicture());
	off_view->DrawBitmap(off,BPoint(0,0));
	off_picture = off_view->EndPicture();

	off_screen->Unlock();

	SetEnabledOn(on_picture);
	SetEnabledOff(off_picture);

	delete off_screen;

	help_window = NULL;
}

HSPictureButton::~HSPictureButton()
{
	delete help_message;
}

void HSPictureButton::MouseDown(BPoint location)
{
	// here we take the button that was pressed and the click number
	int32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);
	int32 num;
	Window()->CurrentMessage()->FindInt32("clicks", &num);
	// call the inherited MouseDown-function

	if (Message() != NULL) {
		// Set the invokers message to contain the mouse-button that was last pressed.
		BMessage *message = Message();
		if (message->HasInt32("buttons")) {
			message->ReplaceInt32("buttons",buttons);
			SetMessage(message);
		}
	}

	BPictureButton::MouseDown(location);
}



void HSPictureButton::MouseMoved(BPoint point, uint32 transit,const BMessage*)
{
	if (transit == B_ENTERED_VIEW) {
		if (help_message != NULL) {
			SetFlags(Flags() | B_PULSE_NEEDED);
		}
		if ((Window()->IsActive() == TRUE) && (long_help_message != NULL)) {
			BMessage *a_message = new BMessage(HS_TEMPORARY_HELP_MESSAGE);
			a_message->AddString("message",long_help_message);
			Window()->PostMessage(a_message,Window());
		}
	}
	else if (transit == B_EXITED_VIEW) {
		if (help_message != NULL) {
			SetFlags(Flags() & ~B_PULSE_NEEDED);
			if (help_window != NULL) {
				help_window->PostMessage(B_QUIT_REQUESTED,help_window);
				help_window = NULL;
			}
		}
		if ((Window()->IsActive() == TRUE) && (long_help_message != NULL)) {
			BMessage *a_message = new BMessage(HS_TOOL_HELP_MESSAGE);
			Window()->PostMessage(a_message,Window());
		}
	}
	else if (transit == B_INSIDE_VIEW) {
		if (help_message != NULL) {
			uint32 buttons;
			GetMouse(&point,&buttons);
			if ((help_window != NULL) && (opening_point != point)) {
				help_window->PostMessage(B_QUIT_REQUESTED,help_window);
				help_window = NULL;
			}
		}
	}
}


void HSPictureButton::Pulse()
{
	// here we open a help window if user has been idle for long enough
	// and help window is not open yet
	BPoint location;
	uint32 buttons;
	GetMouse(&location,&buttons);

	// if the location is not inside the view we should not open a help-window
	if (Bounds().Contains(location) != TRUE) {
		// set the pulse off as the mouse is not anymore inside the view
		SetFlags(Flags() & ~B_PULSE_NEEDED);
	}
	else if ((idle_time() > (600 * 1000)) && help_window == NULL) {
		help_window = new HelpWindow(ConvertToScreen(location),help_message);
		help_window->Show();
		opening_point = location;
	}
}
