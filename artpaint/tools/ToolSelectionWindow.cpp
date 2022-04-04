/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "ToolSelectionWindow.h"

#include "DrawingTools.h"
#include "FloaterManager.h"
#include "MatrixView.h"
#include "MessageConstants.h"
#include "MessageFilters.h"
#include "SettingsServer.h"
#include "Tools.h"
#include "ToolButton.h"
#include "ToolManager.h"
#include "ToolSetupWindow.h"
#include "UtilityClasses.h"


#include <Catalog.h>


#include <map>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


const int32 kExtraEdge	= 4;
typedef std::map<int32, ToolButton* > ToolMap;


ToolMap gToolMap;
ToolSelectionWindow* ToolSelectionWindow::fSelectionWindow = NULL;


ToolSelectionWindow::ToolSelectionWindow(BRect frame)
	: BWindow(frame, B_TRANSLATE("Tools"),
		B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_H_RESIZABLE |
		B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK | B_AVOID_FRONT)
{
	int32 pictureSize = LARGE_TOOL_ICON_SIZE + 4.0;

	window_feel feel = B_NORMAL_WINDOW_FEEL;
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skSelectToolWindowVisible,
			true);

		BMessage settings;
		server->GetApplicationSettings(&settings);
		settings.FindInt32(skSelectToolWindowFeel, (int32*)&feel);
	}

	fMatrixView = new MatrixView(pictureSize, pictureSize, kExtraEdge);

	_AddTool(ToolManager::Instance().ReturnTool(FREE_LINE_TOOL));
	_AddTool(ToolManager::Instance().ReturnTool(STRAIGHT_LINE_TOOL));
	_AddTool(ToolManager::Instance().ReturnTool(RECTANGLE_TOOL));
	_AddTool(ToolManager::Instance().ReturnTool(ELLIPSE_TOOL));

	// Here we could add a separator to the tool window.

	_AddTool(ToolManager::Instance().ReturnTool(BRUSH_TOOL));
	_AddTool(ToolManager::Instance().ReturnTool(HAIRY_BRUSH_TOOL));
	_AddTool(ToolManager::Instance().ReturnTool(AIR_BRUSH_TOOL));
	_AddTool(ToolManager::Instance().ReturnTool(BLUR_TOOL));
	_AddTool(ToolManager::Instance().ReturnTool(FILL_TOOL));
	_AddTool(ToolManager::Instance().ReturnTool(TEXT_TOOL));
	_AddTool(ToolManager::Instance().ReturnTool(TRANSPARENCY_TOOL));
	_AddTool(ToolManager::Instance().ReturnTool(ERASER_TOOL));
	_AddTool(ToolManager::Instance().ReturnTool(SELECTOR_TOOL));
	_AddTool(ToolManager::Instance().ReturnTool(COLOR_SELECTOR_TOOL));

	ToolMap::iterator it = gToolMap.find(ToolManager::Instance().ReturnActiveToolType());
	if (it != gToolMap.end())
		it->second->SetValue(B_CONTROL_ON);

	AddChild(fMatrixView);
	fMatrixView->ResizeTo(Bounds().Width(), Bounds().Height());

	SetFeel(feel);
	if (feel == B_NORMAL_WINDOW_FEEL)
		SetLook(B_TITLED_WINDOW_LOOK);
	else
		SetLook(B_FLOATING_WINDOW_LOOK);

	float minDimension = 2 * kExtraEdge + pictureSize;
	float maxDimension = 1 + kExtraEdge + fMatrixView->CountChildren() *
		(pictureSize + kExtraEdge);
	SetSizeLimits(minDimension, maxDimension, minDimension, maxDimension);

	if (Lock()) {
		AddCommonFilter(new BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE,
			B_MOUSE_DOWN, window_activation_filter));
		AddCommonFilter(new BMessageFilter(B_KEY_DOWN, AppKeyFilterFunction));
		Unlock();
	}

	Show();

	// NOTE: this is broken/ not implemented in Haiku, so the tools window
	//		 will not show up horizontal as it should be, enable if implemented
	//SetWindowAlignment(B_PIXEL_ALIGNMENT, 0, 0, picture_size + kExtraEdge,
	//	kExtraEdge + 1, 0, 0, picture_size + kExtraEdge, kExtraEdge + 1);

	// remove this if SetWindowAlignment is implemented
	ResizeBy(0.0, maxDimension - pictureSize);

	fSelectionWindow = this;
	FloaterManager::AddFloater(this);
}


ToolSelectionWindow::~ToolSelectionWindow()
{
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skSelectToolWindowVisible,
			false);
		server->SetValue(SettingsServer::Application, skSelectToolWindowFrame,
			Frame());
	}

	fSelectionWindow = NULL;
	FloaterManager::RemoveFloater(this);
}


void
ToolSelectionWindow::FrameResized(float width, float height)
{
	BView* matrix = ChildAt(0);
	if (matrix != NULL) {
		float tmpHeight;
		matrix->GetPreferredSize(&width, &tmpHeight);
		if ((width != Bounds().Width()) || (tmpHeight != Bounds().Height()))
			ResizeTo(width, height);
	}
}


void
ToolSelectionWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case HS_TOOL_CHANGED: {
			int32 tool;
			if (message->FindInt32(skTool, &tool) == B_OK) {
				ToolManager::Instance().ChangeTool(tool);

				uint32 button;
				if (message->FindUInt32("buttons", &button) == B_OK
					&& (button & B_SECONDARY_MOUSE_BUTTON)) {
					ToolSetupWindow::ShowToolSetupWindow(tool);
				}
			}
		} break;

		default: {
			BWindow::MessageReceived(message);
		}	break;
	}
}


void
ToolSelectionWindow::showWindow()
{
	if (fSelectionWindow) {
		if (fSelectionWindow->Lock()) {
			fSelectionWindow->SetWorkspaces(B_CURRENT_WORKSPACE);

			if (fSelectionWindow->IsHidden())
				fSelectionWindow->Show();

			if (!fSelectionWindow->IsActive())
				fSelectionWindow->Activate(true);

			fSelectionWindow->Unlock();
		}
	} else {
		BRect frame(10, 31, 54, 596);
		if (SettingsServer* server = SettingsServer::Instance()) {
			BMessage settings;
			server->GetApplicationSettings(&settings);
			settings.FindRect(skSelectToolWindowFrame, &frame);
		}
		new ToolSelectionWindow(frame);
	}

	fSelectionWindow->MoveTo(FitRectToScreen(fSelectionWindow->Frame()).LeftTop());
}


void
ToolSelectionWindow::setFeel(window_feel feel)
{
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skSelectToolWindowFeel,
			int32(feel));
	}

	if (fSelectionWindow) {
		window_look look = B_TITLED_WINDOW_LOOK;
		if (feel != B_NORMAL_WINDOW_FEEL)
			look = B_FLOATING_WINDOW_LOOK;

		fSelectionWindow->SetFeel(feel);
		fSelectionWindow->SetLook(look);
	}
}


void
ToolSelectionWindow::ChangeTool(int32 tool)
{
	if (fSelectionWindow && fSelectionWindow->Lock()) {
		ToolMap::const_iterator it = gToolMap.find(tool);
		if (it != gToolMap.end())
			it->second->SetValue(B_CONTROL_ON);
		fSelectionWindow->Unlock();
	}
}


void
ToolSelectionWindow::_AddTool(const DrawingTool* tool)
{
	BMessage* message = new BMessage(HS_TOOL_CHANGED);
	message->AddUInt32("buttons", 0);
	message->AddInt32(skTool, tool->Type());

	ToolButton* button = new ToolButton(tool->Name(), message, tool->Icon());
	button->ResizeToPreferred();
	fMatrixView->AddSubView(button);

	gToolMap.insert(std::make_pair(tool->Type(), button));
}
