/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <stdio.h>

#include "MessageConstants.h"
#include "RGBControl.h"
#include "ViewSetupWindow.h"

// These pointers are NULL in the beginning. This window has
// not been created yet.
BWindow* ViewSetupWindow::target_window = NULL;
BView* ViewSetupWindow::target_view = NULL;
ViewSetupWindow* ViewSetupWindow::setup_window = NULL;


ViewSetupWindow::ViewSetupWindow(BRect frame)
	:
	BWindow(frame, "", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	if (target_view != NULL) {
		// here open the controls that affect the view
		rgb_color c = {0, 0, 0, 0};
		rgb_control = new RGBControl(BPoint(0, 0), c);
		AddChild(rgb_control);
		ResizeTo(rgb_control->Frame().Width(), rgb_control->Frame().Height());
		rgb_control->SetTarget(target_view);
//		invoker_message = new BMessage(HS_AREA_COLOR_CHANGED);
//		invoker_message->AddInt32("color",0);
//		rgb_control->SetMessage(invoker_message);
	}
}


ViewSetupWindow::~ViewSetupWindow()
{
	// NULLify all static pointers
	target_window = NULL;
	target_view = NULL;
	setup_window = NULL;
}


void
ViewSetupWindow::showViewSetupWindow(BWindow* target_w, BView* target_v)
{
	BWindow* old_target = target_window;

	target_window = target_w;
	target_view = target_v;

	// If there is no setup-window we should open one.
	if (setup_window == NULL)
		setup_window = new ViewSetupWindow(BRect(10, 10, 100, 100));

	// If the window is different than previous, we should
	// change this window to reflect the new window's settings.
	if (target_window != old_target) {
		char title[1000];
		sprintf(title, "Settings: %s", target_window->Title());
		setup_window->SetTitle(title);
	}

	if (setup_window->IsHidden())
		setup_window->Show();
	if (!setup_window->IsActive())
		setup_window->Activate(TRUE);
}


void
ViewSetupWindow::closeViewSetupWindow(BWindow* target_w)
{
	// We will only close if we were called by the target window.
	if (target_w == target_window) {
		setup_window->Close();
		delete setup_window;
		setup_window = NULL;
		target_window = NULL;
		target_view = NULL;
	}
}
