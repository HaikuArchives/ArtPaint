/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Message.h>

#include "WindowGUIManipulator.h"


WindowGUIManipulatorView::WindowGUIManipulatorView(BRect rect)
	: BView(rect,"window_gui_manipulator_view",B_FOLLOW_ALL_SIDES,B_WILL_DRAW)
{

}


void WindowGUIManipulatorView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
}


void WindowGUIManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		default:
			BView::MessageReceived(message);
			break;
	}
}
