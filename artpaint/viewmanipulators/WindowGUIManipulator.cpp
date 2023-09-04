/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "WindowGUIManipulator.h"


WindowGUIManipulatorView::WindowGUIManipulatorView()
	:
	BView("window_gui_manipulator_view", B_WILL_DRAW, NULL)
{
}


WindowGUIManipulatorView::WindowGUIManipulatorView(BRect rect)
	:
	BView(rect, "window_gui_manipulator_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
}


void
WindowGUIManipulatorView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
WindowGUIManipulatorView::MessageReceived(BMessage* message)
{
    BView::MessageReceived(message);
}
