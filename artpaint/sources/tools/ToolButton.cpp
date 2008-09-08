/*

	Filename:	ToolButton.cpp
	Contents:	ToolButton-class definitions.
	Author:		Heikki Suhonen

*/


#include <stdio.h>

#include "ToolButton.h"
#include "ToolImages.h"
#include "MessageConstants.h"
#include "ToolSetupWindow.h"
#include "UtilityClasses.h"

BList* ToolButton::tool_button_list = new BList();
ToolButton* ToolButton::active_button = NULL;
int32 ToolButton::active_tool = -1;

ToolButton::ToolButton(BRect frame,int32 tool,const char *name)
	: BPictureButton(frame,"tool_button",ToolImages::getPicture(tool,BIG_TOOL_PICTURE_SIZE,0),ToolImages::getPicture(tool,BIG_TOOL_PICTURE_SIZE,1),NULL)
	, tool_name(name)

{
	BMessage *model_message = new BMessage(HS_TOOL_CHANGED);
	model_message->AddInt32("tool",tool);
	model_message->AddInt32("buttons",0);
	SetMessage(model_message);

	tool_type = tool;
//	tool_name = name;

	SetBehavior(B_TWO_STATE_BUTTON);

	tool_button_list->AddItem(this);

	if (active_tool == tool)
		SetValue(B_CONTROL_ON);
	else
		SetValue(B_CONTROL_OFF);

	help_window = NULL;
}


ToolButton::~ToolButton()
{
	tool_button_list->RemoveItem(this);
	if (active_button == this) {
		active_button = NULL;
		active_tool = -1;
	}
}


void ToolButton::MouseDown(BPoint point)
{
	uint32 buttons;
	GetMouse(&point,&buttons);

	if (Value() == B_CONTROL_OFF)
		BPictureButton::MouseDown(point);

	if (buttons & B_SECONDARY_MOUSE_BUTTON)
		ToolSetupWindow::showWindow(tool_type);

	/* The third button might be used for example
	to load the mouse with single shot operations
	if (buttons & B_TERTIARY_MOUSE_BUTTON) {

	}*/

}

void ToolButton::MouseMoved(BPoint point, uint32 transit,const BMessage*)
{
	if (transit == B_ENTERED_VIEW) {
		SetFlags(Flags() | B_PULSE_NEEDED);
	}
	else if (transit == B_EXITED_VIEW) {
		SetFlags(Flags() & ~B_PULSE_NEEDED);
		if (help_window != NULL) {
			help_window->PostMessage(B_QUIT_REQUESTED,help_window);
			help_window = NULL;
		}
	}
	else if (transit == B_INSIDE_VIEW) {
		uint32 buttons;
		GetMouse(&point,&buttons);
		if ((help_window != NULL) && (opening_point != point)) {
			help_window->PostMessage(B_QUIT_REQUESTED,help_window);
			help_window = NULL;
		}
	}

}


void ToolButton::Pulse()
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
		help_window = new HelpWindow(ConvertToScreen(location),tool_name);
		help_window->Show();
		opening_point = location;
	}
}

void ToolButton::ChangeActiveButton(int32 new_tool)
{
	if (active_tool != new_tool) {
		active_tool = new_tool;
		if (active_button != NULL) {
			active_button->SetValue(B_CONTROL_OFF);
			active_button = NULL;
		}

		bool continue_search = TRUE;
		int32 i = 0;

		while (continue_search) {
			ToolButton *button = (ToolButton*)tool_button_list->ItemAt(i);
			if (button->tool_type == new_tool) {
				button->SetValue(B_CONTROL_ON);
				continue_search = FALSE;
				active_button = button;
			}
			i++;
			if (i>=tool_button_list->CountItems())
				continue_search = FALSE;
		}
	}
}


