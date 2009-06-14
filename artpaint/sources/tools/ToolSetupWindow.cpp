/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <stdio.h>

#include "FloaterManager.h"
#include "ToolSetupWindow.h"
#include "PaintApplication.h"
#include "Tools.h"
#include "UtilityClasses.h"
#include "DrawingTools.h"
#include "Settings.h"
#include "MessageFilters.h"
#include "StringServer.h"
#include "ToolManager.h"


// define the static variable
ToolSetupWindow* ToolSetupWindow::setup_window = NULL;

ToolSetupWindow::ToolSetupWindow(BRect frame,int32 tool_type)
		: BWindow(frame,StringServer::ReturnString(TOOL_SETUP_STRING),B_FLOATING_WINDOW_LOOK,B_NORMAL_WINDOW_FEEL,B_NOT_ZOOMABLE|B_NOT_RESIZABLE|B_WILL_ACCEPT_FIRST_CLICK|B_AVOID_FRONT)
{
	// the frame only indicates position, the window will be
	// resized to fit the views in changeTool-function


//	// window will also be resized to correct width so the menu-container
//	// is fully visible
//	// all other containers must fit into this width
//	ResizeTo(a_menufield->Divider()+a_menufield->Frame().Width()+2*EXTRA_EDGE,Bounds().Height());

	// Here create a container for settings, it will be filled with controls
	// by tool's makeConfigView-function. That function respects the container's
	// width, but will adjust it's height. The function is called from changeTool-function.
	ResizeBy(0,100);
	frame.OffsetTo(0,0);
	settings_container = new BBox(frame,NULL,B_FOLLOW_NONE,B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,B_PLAIN_BORDER);
	AddChild(settings_container);

	current_tool_type = -1;

	// we call the changeTool function that adds the proper views
	// to this window
	changeTool(tool_type);

	// initialize the static variable
	setup_window = this;

	window_feel feel = ((PaintApplication*)be_app)->GlobalSettings()->tool_setup_window_feel;
	SetFeel(feel);
	if (feel == B_NORMAL_WINDOW_FEEL)
		SetLook(B_TITLED_WINDOW_LOOK);
	else
		SetLook(B_FLOATING_WINDOW_LOOK);

	// Add a filter that will be used to catch mouse-down-messages in order
	// to activate this window when required
	Lock();
	BMessageFilter *activation_filter = new BMessageFilter(B_ANY_DELIVERY,B_ANY_SOURCE,B_MOUSE_DOWN,window_activation_filter);
	AddCommonFilter(activation_filter);
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN,AppKeyFilterFunction));
	Unlock();

	// finally show ourselves to the public
	Show();
	((PaintApplication*)be_app)->GlobalSettings()->tool_setup_window_visible = TRUE;
	FloaterManager::AddFloater(this);
}

ToolSetupWindow::~ToolSetupWindow()
{
	// record our frame into apps settings
	((PaintApplication*)be_app)->GlobalSettings()->tool_setup_window_frame = Frame();
	((PaintApplication*)be_app)->GlobalSettings()->tool_setup_window_visible = FALSE;

	// NULLify the static pointer
	setup_window = NULL;
	FloaterManager::RemoveFloater(this);
}


void ToolSetupWindow::MessageReceived(BMessage *message)
{
	// Also report the message-source to SetOption function.
	BHandler *handler;
	message->FindPointer("source",(void**)&handler);
	switch (message->what) {

		// this comes from the tool pop-up-menu that is in the top of the window, the menu is created
		// in PaintApplication::createToolMenu and then copied using instantiation and archiving
		// we should change the window to display this new tool, the data members of message are
		// int32 "tool"
		case TOOL_CHANGED:
			changeTool(message->FindInt32("tool"));
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

void ToolSetupWindow::changeTool(int32 new_tool)
{
	// This function works like this:
	//
	// 1.	Remove any existing controls.
	// 2.	Call tool's makeConfigView-function
	// 3.	Resize the window to right size.
	//


	// Only change the tool if the new tool is different than
	// the previous.
	if (new_tool != current_tool_type) {
		// Record the new tool type in member-variable.
		current_tool_type = new_tool;
		// Also store the new tool into app's settings.
		((PaintApplication*)be_app)->GlobalSettings()->setup_window_tool = new_tool;

		// Remove the controllers of the previous tool.
		int32 children_count = settings_container->CountChildren();
		for (int32 i=0;i<children_count;i++) {
			// Always remove the first child.
			BView *old_controller = settings_container->ChildAt(0);
			old_controller->RemoveSelf();
			delete old_controller;
		}

		// Call tool's makeConfigView-function.
		BView *config_view = tool_manager->ReturnConfigurationView(current_tool_type);
		if (config_view == NULL) {
			config_view = new BBox(BRect(0,0,50,20));
		}
		settings_container->ResizeTo(max_c(config_view->Bounds().Width()+10,100),max_c(config_view->Bounds().Height()+10,40));
		settings_container->AddChild(config_view);
		config_view->MoveTo(5,5);
//		config_view->ResizeTo(settings_container->Frame().Width()-10,config_view->Frame().Height());
		// Resize the window to be just large enough for the new config-view.
		// The settings_container is already in it's right size.
		float w = settings_container->Frame().Width();
		float h = settings_container->Frame().Height();
		ResizeTo(w,h);
	}
}


void ToolSetupWindow::updateTool(int32 updated_tool)
{
	if (setup_window != NULL) {
		setup_window->Lock();
		if (updated_tool == setup_window->current_tool_type) {
			setup_window->target_tool->UpdateConfigView(setup_window->settings_container);
		}
		setup_window->Unlock();
	}
}


void ToolSetupWindow::showWindow(int32 tool_type)
{
	if (setup_window == NULL) {
		if (tool_type == 0)
			tool_type = FREE_LINE_TOOL;
		new ToolSetupWindow(((PaintApplication*)be_app)->GlobalSettings()->tool_setup_window_frame,tool_type);
		tool_type = 0;
	}
	else {
		setup_window->Lock();
		setup_window->SetWorkspaces(B_CURRENT_WORKSPACE);
		if (setup_window->IsHidden()) {
			setup_window->Show();
		}
		if ((!setup_window->IsActive()) && (!setup_window->IsFront())) {
			setup_window->Activate(TRUE);
		}

		setup_window->Unlock();
	}

	BRect new_rect = FitRectToScreen(setup_window->Frame());
	setup_window->MoveTo(new_rect.LeftTop());

	// finally change the tool
	if (tool_type != 0) {
		setup_window->Lock();
		setup_window->changeTool(tool_type);
		setup_window->Unlock();
	}
}

void ToolSetupWindow::setFeel(window_feel feel)
{
	((PaintApplication*)be_app)->GlobalSettings()->tool_setup_window_feel = feel;

	if (setup_window != NULL) {
		setup_window->SetFeel(feel);
		if (feel == B_NORMAL_WINDOW_FEEL)
			setup_window->SetLook(B_TITLED_WINDOW_LOOK);
		else
			setup_window->SetLook(B_FLOATING_WINDOW_LOOK);
	}
}



void ToolSetupWindow::changeToolForTheSetupWindow(int32 new_tool)
{
	if (setup_window != NULL) {
		setup_window->Lock();
		setup_window->changeTool(new_tool);
		setup_window->Unlock();
	}
	else {
		((PaintApplication*)be_app)->GlobalSettings()->setup_window_tool = new_tool;
	}
}

