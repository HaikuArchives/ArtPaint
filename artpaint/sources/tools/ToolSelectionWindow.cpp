/*

	Filename:	ToolSelectionWindow.cpp
	Contents:	Definitions for window that is used to select the tools
	Author:		Heikki Suhonen

*/

#include "ToolSelectionWindow.h"

#include "DrawingTools.h"
#include "FloaterManager.h"
#include "MatrixView.h"
#include "MessageFilters.h"
#include "PaintApplication.h"
#include "Settings.h"
#include "StringServer.h"
#include "Tools.h"
#include "ToolButton.h"
#include "ToolImages.h"
#include "ToolManager.h"
#include "ToolSetupWindow.h"


ToolSelectionWindow* ToolSelectionWindow::fSelectionWindow = NULL;


ToolSelectionWindow::ToolSelectionWindow(BRect frame)
	: BWindow(frame, StringServer::ReturnString(TOOLS_STRING),
		B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_H_RESIZABLE |
		B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK | B_AVOID_FRONT)
{
	int32 pictureSize = BIG_TOOL_PICTURE_SIZE;
	((PaintApplication*)be_app)->Settings()->tool_select_window_visible = true;

	MatrixView* buttonMatrix = new MatrixView(pictureSize, pictureSize, EXTRA_EDGE);

	BRect buttonRect(0, 0, pictureSize - 1, pictureSize - 1);
	const DrawingTool* addedTool = tool_manager->ReturnTool(FREE_LINE_TOOL);
	buttonMatrix->AddSubView(
		new ToolButton(buttonRect, addedTool->GetType(), addedTool->GetName()));

	addedTool = tool_manager->ReturnTool(STRAIGHT_LINE_TOOL);
	buttonMatrix->AddSubView(
		new ToolButton(buttonRect, addedTool->GetType(), addedTool->GetName()));

	addedTool = tool_manager->ReturnTool(RECTANGLE_TOOL);
	buttonMatrix->AddSubView(
		new ToolButton(buttonRect, addedTool->GetType(), addedTool->GetName()));

	addedTool = tool_manager->ReturnTool(ELLIPSE_TOOL);
	buttonMatrix->AddSubView(
		new ToolButton(buttonRect, addedTool->GetType(), addedTool->GetName()));

	// Here we could add a separator to the tool window.

	addedTool = tool_manager->ReturnTool(BRUSH_TOOL);
	buttonMatrix->AddSubView(
		new ToolButton(buttonRect, addedTool->GetType(), addedTool->GetName()));

	addedTool = tool_manager->ReturnTool(HAIRY_BRUSH_TOOL);
	buttonMatrix->AddSubView(
		new ToolButton(buttonRect, addedTool->GetType(), addedTool->GetName()));

	addedTool = tool_manager->ReturnTool(AIR_BRUSH_TOOL);
	buttonMatrix->AddSubView(
		new ToolButton(buttonRect, addedTool->GetType(), addedTool->GetName()));

	addedTool = tool_manager->ReturnTool(BLUR_TOOL);
	buttonMatrix->AddSubView(
		new ToolButton(buttonRect, addedTool->GetType(), addedTool->GetName()));

	addedTool = tool_manager->ReturnTool(FILL_TOOL);
	buttonMatrix->AddSubView(
		new ToolButton(buttonRect, addedTool->GetType(), addedTool->GetName()));

	addedTool = tool_manager->ReturnTool(TEXT_TOOL);
	buttonMatrix->AddSubView(
		new ToolButton(buttonRect, addedTool->GetType(), addedTool->GetName()));

	addedTool = tool_manager->ReturnTool(TRANSPARENCY_TOOL);
	buttonMatrix->AddSubView(
		new ToolButton(buttonRect, addedTool->GetType(), addedTool->GetName()));

	addedTool = tool_manager->ReturnTool(ERASER_TOOL);
	buttonMatrix->AddSubView(
		new ToolButton(buttonRect, addedTool->GetType(), addedTool->GetName()));

	addedTool = tool_manager->ReturnTool(SELECTOR_TOOL);
	buttonMatrix->AddSubView(
		new ToolButton(buttonRect, addedTool->GetType(), addedTool->GetName()));

	addedTool = tool_manager->ReturnTool(COLOR_SELECTOR_TOOL);
	buttonMatrix->AddSubView(
		new ToolButton(buttonRect, addedTool->GetType(), addedTool->GetName()));

	AddChild(buttonMatrix);
	buttonMatrix->ResizeTo(Bounds().Width(), Bounds().Height());

	window_feel feel =
		((PaintApplication*)be_app)->Settings()->tool_select_window_feel;
	SetFeel(feel);
	if (feel == B_NORMAL_WINDOW_FEEL)
		SetLook(B_TITLED_WINDOW_LOOK);
	else
		SetLook(B_FLOATING_WINDOW_LOOK);

	float minDimension = 2 * EXTRA_EDGE + pictureSize;
	float maxDimension = 1 + EXTRA_EDGE + buttonMatrix->CountChildren() *
		(pictureSize + EXTRA_EDGE);
	SetSizeLimits(minDimension, maxDimension, minDimension, maxDimension);

	// Add a filter that will be used to catch mouse-down-messages in order
	// to activate this window when required
	if (Lock()) {
		BMessageFilter* activation_filter = new BMessageFilter(B_ANY_DELIVERY,
			B_ANY_SOURCE, B_MOUSE_DOWN, window_activation_filter);
		AddCommonFilter(activation_filter);
		AddCommonFilter(new BMessageFilter(B_KEY_DOWN,AppKeyFilterFunction));
		Unlock();
	}

	ToolButton::ChangeActiveButton(tool_manager->ReturnActiveToolType());

	Show();
	fSelectionWindow = this;

	// NOTE: this is broken/ not implemented in Haiku, so the tools window
	//		 will not show up horizontal as it should be, enable if implemented
	//SetWindowAlignment(B_PIXEL_ALIGNMENT, 0, 0, picture_size + EXTRA_EDGE,
	//	EXTRA_EDGE + 1, 0, 0, picture_size + EXTRA_EDGE, EXTRA_EDGE + 1);

	// remove this if SetWindowAlignment is implemented
	ResizeBy(0.0, maxDimension - pictureSize);

	FloaterManager::AddFloater(this);
}


ToolSelectionWindow::~ToolSelectionWindow()
{
	// Record our frame to the settings.
	((PaintApplication*)be_app)->Settings()->tool_select_window_frame = Frame();
	((PaintApplication*)be_app)->Settings()->tool_select_window_visible = false;

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
			if (message->FindInt32("tool",&tool) == B_OK) {
				// this comes from a tool's picture-button and
				// informs that a tool for a mouse-button has changed
				tool_manager->ChangeTool(tool);
				ToolButton::ChangeActiveButton(tool);
				ToolSetupWindow::changeToolForTheSetupWindow(tool);
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
		new ToolSelectionWindow(
			((PaintApplication*)be_app)->Settings()->tool_select_window_frame);
	}

	fSelectionWindow->MoveTo(FitRectToScreen(fSelectionWindow->Frame()).LeftTop());
}


void
ToolSelectionWindow::setFeel(window_feel feel)
{
	((PaintApplication*)be_app)->Settings()->tool_select_window_feel = feel;

	if (fSelectionWindow) {
		fSelectionWindow->SetFeel(feel);

		window_look look = B_TITLED_WINDOW_LOOK;
		if (feel != B_NORMAL_WINDOW_FEEL)
			look = B_FLOATING_WINDOW_LOOK;
		fSelectionWindow->SetLook(look);
	}
}


void
ToolSelectionWindow::ChangeTool(int32 tool)
{
	if (fSelectionWindow && fSelectionWindow->Lock()) {
		ToolButton::ChangeActiveButton(tool);
		fSelectionWindow->Unlock();
	}
}
