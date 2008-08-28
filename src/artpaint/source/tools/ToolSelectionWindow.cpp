/* 

	Filename:	ToolSelectionWindow.cpp
	Contents:	Definitions for window that is used to select the tools	
	Author:		Heikki Suhonen
	
*/

#include <stdio.h>

#include "FloaterManager.h"
#include "MessageConstants.h"
#include "ToolSelectionWindow.h"
#include "ToolImages.h"
#include "Tools.h"
#include "PaintApplication.h"
#include "DrawingTools.h"
#include "UtilityClasses.h"
#include "StatusView.h"
#include "Settings.h"
#include "Controls.h"
#include "MessageFilters.h"
#include "ToolButton.h"
#include "ToolSetupWindow.h"
#include "StringServer.h"
#include "ToolManager.h"
#include "MatrixView.h"

#define	BUTTONS_IN_A_ROW	2
#define	ADDITIONAL_EDGES	4
#define	SPACE_BETWEEN		4


ToolSelectionWindow* ToolSelectionWindow::selection_window = NULL;

ToolSelectionWindow::ToolSelectionWindow(BRect frame)
			:	BWindow(frame,StringServer::ReturnString(TOOLS_STRING),B_FLOATING_WINDOW_LOOK,B_NORMAL_WINDOW_FEEL,B_NOT_H_RESIZABLE|B_NOT_ZOOMABLE|B_WILL_ACCEPT_FIRST_CLICK|B_AVOID_FRONT)
{
	// here we should create buttons for each tool and place
	// them in the window
	// we should then resize the window to fit the buttons
	int32 picture_size = BIG_TOOL_PICTURE_SIZE;
	((PaintApplication*)be_app)->Settings()->tool_select_window_visible = TRUE;
			
	// Here we create a button for each tool.
	MatrixView *button_matrix = new MatrixView(picture_size,picture_size,EXTRA_EDGE);	
	BRect button_rect(0,0,picture_size-1,picture_size-1);
	const DrawingTool *added_tool;
	added_tool = tool_manager->ReturnTool(FREE_LINE_TOOL);
	button_matrix->AddSubView(new ToolButton(button_rect,added_tool->GetType(),added_tool->GetName()));	

	added_tool = tool_manager->ReturnTool(STRAIGHT_LINE_TOOL);
	button_matrix->AddSubView(new ToolButton(button_rect,added_tool->GetType(),added_tool->GetName()));	

	added_tool = tool_manager->ReturnTool(RECTANGLE_TOOL);
	button_matrix->AddSubView(new ToolButton(button_rect,added_tool->GetType(),added_tool->GetName()));	

	added_tool = tool_manager->ReturnTool(ELLIPSE_TOOL);
	button_matrix->AddSubView(new ToolButton(button_rect,added_tool->GetType(),added_tool->GetName()));	

	// Here we could add a separator to the tool window.
	
	added_tool = tool_manager->ReturnTool(BRUSH_TOOL);
	button_matrix->AddSubView(new ToolButton(button_rect,added_tool->GetType(),added_tool->GetName()));	
	
	added_tool = tool_manager->ReturnTool(HAIRY_BRUSH_TOOL);
	button_matrix->AddSubView(new ToolButton(button_rect,added_tool->GetType(),added_tool->GetName()));	

	added_tool = tool_manager->ReturnTool(AIR_BRUSH_TOOL);
	button_matrix->AddSubView(new ToolButton(button_rect,added_tool->GetType(),added_tool->GetName()));	

	added_tool = tool_manager->ReturnTool(BLUR_TOOL);
	button_matrix->AddSubView(new ToolButton(button_rect,added_tool->GetType(),added_tool->GetName()));	


	added_tool = tool_manager->ReturnTool(FILL_TOOL);
	button_matrix->AddSubView(new ToolButton(button_rect,added_tool->GetType(),added_tool->GetName()));	

	added_tool = tool_manager->ReturnTool(TEXT_TOOL);
	button_matrix->AddSubView(new ToolButton(button_rect,added_tool->GetType(),added_tool->GetName()));	

	added_tool = tool_manager->ReturnTool(TRANSPARENCY_TOOL);
	button_matrix->AddSubView(new ToolButton(button_rect,added_tool->GetType(),added_tool->GetName()));	

	added_tool = tool_manager->ReturnTool(ERASER_TOOL);
	button_matrix->AddSubView(new ToolButton(button_rect,added_tool->GetType(),added_tool->GetName()));	


	added_tool = tool_manager->ReturnTool(SELECTOR_TOOL);
	button_matrix->AddSubView(new ToolButton(button_rect,added_tool->GetType(),added_tool->GetName()));	

	added_tool = tool_manager->ReturnTool(COLOR_SELECTOR_TOOL);
	button_matrix->AddSubView(new ToolButton(button_rect,added_tool->GetType(),added_tool->GetName()));	

	AddChild(button_matrix);
	button_matrix->ResizeTo(Bounds().Width(),Bounds().Height());

	window_feel feel = ((PaintApplication*)be_app)->Settings()->tool_select_window_feel;
	SetFeel(feel);
	if (feel == B_NORMAL_WINDOW_FEEL)
		SetLook(B_TITLED_WINDOW_LOOK);
	else
		SetLook(B_FLOATING_WINDOW_LOOK);

	float min_dimension = 2*EXTRA_EDGE+picture_size;
	float max_dimension = 1+EXTRA_EDGE + button_matrix->CountChildren()*(picture_size+EXTRA_EDGE);
	SetSizeLimits(min_dimension,max_dimension,min_dimension,max_dimension);

	// Add a filter that will be used to catch mouse-down-messages in order
	// to activate this window when required
	Lock();
	BMessageFilter *activation_filter = new BMessageFilter(B_ANY_DELIVERY,B_ANY_SOURCE,B_MOUSE_DOWN,window_activation_filter);
	AddCommonFilter(activation_filter);
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN,AppKeyFilterFunction));
	Unlock();

	ToolButton::ChangeActiveButton(tool_manager->ReturnActiveToolType());

	Show();
	selection_window = this;

	SetWindowAlignment(B_PIXEL_ALIGNMENT,0,0,picture_size+EXTRA_EDGE,EXTRA_EDGE+1,0,0,picture_size+EXTRA_EDGE,EXTRA_EDGE+1);

	FloaterManager::AddFloater(this);
}


ToolSelectionWindow::~ToolSelectionWindow()
{
	// Record our frame to the settings.
	((PaintApplication*)be_app)->Settings()->tool_select_window_frame = Frame();
	((PaintApplication*)be_app)->Settings()->tool_select_window_visible = FALSE;

	selection_window = NULL;
	FloaterManager::RemoveFloater(this);
}


void ToolSelectionWindow::FrameResized(float,float h)
{
	BView *matrix = ChildAt(0);
	if (matrix != NULL) {
		float width,height;
		matrix->GetPreferredSize(&width,&height);
		if ((width != Bounds().Width()) || (height != Bounds().Height()))
			ResizeTo(width,h);

////		float cell_size = 32+4;
//		if (Bounds().IntegerWidth() < Bounds().IntegerHeight()) {
//			SetFlags(Flags() & ~B_NOT_H_RESIZABLE | B_NOT_V_RESIZABLE);
//		}
//		else if (Bounds().IntegerWidth() > Bounds().IntegerHeight()) {
//			SetFlags(Flags() & ~B_NOT_V_RESIZABLE | B_NOT_H_RESIZABLE);		
//		}
//		else {
//			SetFlags(Flags() & ~B_NOT_V_RESIZABLE & ~B_NOT_H_RESIZABLE);
//		}
	}	
}

void ToolSelectionWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {

		// this comes from a tool's picture-button and informs that a tool for a mouse-button has
		// changed
		case HS_TOOL_CHANGED:
			int32 tool;
			if (message->FindInt32("tool",&tool) == B_NO_ERROR) {			
				tool_manager->ChangeTool(message->FindInt32("tool"));		
				ToolButton::ChangeActiveButton(tool);
				ToolSetupWindow::changeToolForTheSetupWindow(tool);
			}									
//			SelectedToolsView::sendMessageToAll(message);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void ToolSelectionWindow::showWindow()
{
	if (selection_window == NULL) {
		new ToolSelectionWindow(((PaintApplication*)be_app)->Settings()->tool_select_window_frame);
	}
	else {
		selection_window->Lock();
		selection_window->SetWorkspaces(B_CURRENT_WORKSPACE);
		if (selection_window->IsHidden()) {
			selection_window->Show();
		}
		if (!selection_window->IsActive()) {
			selection_window->Activate(TRUE);
		}
		selection_window->Unlock();
	}
	
	BRect new_rect = FitRectToScreen(selection_window->Frame());
	selection_window->MoveTo(new_rect.LeftTop());	
}


void ToolSelectionWindow::setFeel(window_feel feel)
{
	((PaintApplication*)be_app)->Settings()->tool_select_window_feel = feel;
	
	if (selection_window != NULL) {
		selection_window->SetFeel(feel);

		if (feel == B_NORMAL_WINDOW_FEEL)
			selection_window->SetLook(B_TITLED_WINDOW_LOOK);
		else
			selection_window->SetLook(B_FLOATING_WINDOW_LOOK);
	}
}

void ToolSelectionWindow::ChangeTool(int32 tool)
{
	if (selection_window != NULL) {
		selection_window->Lock();
		ToolButton::ChangeActiveButton(tool);
		selection_window->Unlock();
	}
}