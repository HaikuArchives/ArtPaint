/*

	Filename:	ToolManager.cpp
	Contents:	ToolManager-class definitions.
	Author:	Heikki Suhonen

*/

#include <MenuItem.h>
#include <PopUpMenu.h>
#include <stdio.h>

#include "ImageView.h"
#include "ToolManager.h"
#include "DrawingTools.h"
#include "BrushEditor.h"
#include "Settings.h"
#include "ToolEventAdapter.h"
#include "ToolSelectionWindow.h"
#include "ToolSetupWindow.h"
#include "PaintApplication.h"
#include "Cursors.h"

ToolManager	*tool_manager;

// These functions are the public interface to ToolManager-class
status_t ToolManager::CreateToolManager()
{
	if (tool_manager == NULL) {
		tool_manager = new ToolManager();
		return B_NO_ERROR;
	}
	else {
		return B_ERROR;
	}
}

status_t ToolManager::DestroyToolManager()
{
	if (tool_manager != NULL) {
		delete tool_manager;
		tool_manager = NULL;
		return B_NO_ERROR;
	}
	else {
		return B_ERROR;
	}
}



// These are the ImageView-interface functions.
ToolScript* ToolManager::StartTool(ImageView *view,uint32 buttons,BPoint bitmap_point,BPoint view_point, int32 toolType)
{
	DrawingTool *used_tool = NULL;

	if (toolType != 0) {
		used_tool = ToolListEntry::ReturnTool(toolType);
	}

	if (used_tool == NULL) {
		used_tool = active_drawing_tool;
	}

	ToolManagerClient *client_data = ReturnClientData(view);

	if (client_data->last_updated_rect.IsValid() == TRUE) {
		// The client has not yet consumed the updated rect.
		// We return NULL.
		return NULL;
	}

	// Here start the correct tool.
	client_data->active_tool = used_tool;
	if (client_data->active_tool != NULL) {
		view->SetToolHelpString(used_tool->ReturnHelpString(TRUE));
		ToolScript *the_script = client_data->active_tool->UseTool(view,buttons,bitmap_point,view_point);
		view->SetToolHelpString(used_tool->ReturnHelpString(FALSE));

		// When the tool has finished we should record the updated_rect into clients data.
		client_data->last_updated_rect = client_data->active_tool->LastUpdatedRect();
		client_data->active_tool = NULL;

		return the_script;
	}
	else
		return NULL;
}



status_t ToolManager::MouseDown(ImageView *view,BPoint view_point,BPoint bitmap_point,uint32 buttons,int32 clicks)
{
	// Here inform the client_data->active_tool about the mouse-event if it accepts
	// that information.
	ToolManagerClient *client_data = ReturnClientData(view);

	if (client_data->active_tool != NULL) {
		ToolEventAdapter *adapter = dynamic_cast<ToolEventAdapter*>(client_data->active_tool);
		if (adapter != NULL) {
			adapter->SetClickEvent(view_point,bitmap_point,buttons,clicks);
		}
	}

	return B_NO_ERROR;
}


status_t ToolManager::KeyDown(ImageView *view,const char *bytes,int32 numBytes)
{
	// Here inform the client_data->active_tool about the key-event if it accepts
	// that information.
	ToolManagerClient *client_data = ReturnClientData(view);

	if (client_data != NULL) {
		if (client_data->active_tool != NULL) {
			ToolEventAdapter *adapter = dynamic_cast<ToolEventAdapter*>(client_data->active_tool);
			if (adapter != NULL) {
				adapter->SetKeyEvent(bytes,numBytes);
			}
		}
	}

	return B_NO_ERROR;
}

status_t ToolManager::KeyUp(ImageView *view,const char *bytes,int32 numBytes)
{
	// Here inform the client_data->active_tool about the key-event if it accepts
	// that information.

	return B_NO_ERROR;
}


BRect ToolManager::LastUpdatedRect(ImageView *view)
{
	ToolManagerClient *client_data = ReturnClientData(view);

	BRect temp_rect = client_data->last_updated_rect;
	client_data->last_updated_rect = BRect(0,0,-1,-1);

	return temp_rect;
}



status_t ToolManager::ChangeTool(int32 tool_type)
{
	active_drawing_tool = ToolListEntry::ReturnTool(tool_type);
	ToolSetupWindow::changeToolForTheSetupWindow(tool_type);
	ToolSelectionWindow::ChangeTool(tool_type);

	((PaintApplication*)be_app)->Settings()->primary_tool = tool_type;

	return B_NO_ERROR;
}


const DrawingTool* ToolManager::ReturnTool(int32 tool_type)
{
	return ToolListEntry::ReturnTool(tool_type);
}


const void* ToolManager::ReturnCursor()
{
	int32 cursor_mode = ((PaintApplication*)be_app)->Settings()->cursor_mode;
	if ((active_drawing_tool != NULL) && (cursor_mode == TOOL_CURSOR_MODE)) {
		return active_drawing_tool->ReturnToolCursor();
	}
	else if (active_drawing_tool == NULL) {
		return B_HAND_CURSOR;
	}
	else {
		return HS_CROSS_CURSOR;
	}
}


int32 ToolManager::ReturnActiveToolType()
{
	if (active_drawing_tool != NULL)
		return active_drawing_tool->GetType();
	else
		return NO_TOOL;

}

status_t ToolManager::SetCurrentBrush(brush_info *binfo)
{
	// Set the new brush for all tools that use brushes.
	BrushTool *brush_tool = dynamic_cast<BrushTool*>(ToolListEntry::ReturnTool(BRUSH_TOOL));

	if (brush_tool != NULL) {
		Brush *a_brush = brush_tool->GetBrush();
		a_brush->ModifyBrush(*binfo);
		BrushEditor::BrushModified();
		a_brush->CreateDiffBrushes();
//		ToolSetupWindow::changeToolForTheSetupWindow(BRUSH_TOOL);
//		ToolSetupWindow::updateTool(BRUSH_TOOL);
	}

	return B_NO_ERROR;
}


status_t ToolManager::NotifyViewEvent(ImageView *view,image_view_event_type type)
{
	ToolManagerClient *client_data = ReturnClientData(view);
	if (type == CURSOR_ENTERED_VIEW) {
		if (active_drawing_tool != NULL) {
			view->SetToolHelpString(active_drawing_tool->ReturnHelpString(client_data->active_tool == active_drawing_tool));
		}
	}
	else if (type == TOOL_ACTIVATED) {
		if (active_drawing_tool != NULL) {
			view->SetToolHelpString(active_drawing_tool->ReturnHelpString(client_data->active_tool == active_drawing_tool));
		}
	}
	else if (type == CURSOR_EXITED_VIEW) {
		if (active_drawing_tool != NULL) {
			view->SetToolHelpString("");
		}
	}

	return B_NO_ERROR;
}

BView* ToolManager::ReturnConfigurationView(int32 tool_type)
{
	DrawingTool *tool = ToolListEntry::ReturnTool(tool_type);

	if (tool != NULL) {
		return tool->makeConfigView();
	}
	else
		return NULL;
}


BPopUpMenu* ToolManager::ReturnToolPopUpMenu()
{
	return tool_pop_up_menu;
}



status_t ToolManager::ReadToolSettings(BFile &file)
{
	// Read the endianness of the file.
	int32 endianness;
	bool is_little_endian;
	if (file.Read(&endianness,sizeof(int32)) != sizeof(int32)) {
		return B_ERROR;
	}
	if (endianness == 0x00000000)
		is_little_endian = FALSE;
	else
		is_little_endian = TRUE;

	int32 marker;
	if (file.Read(&marker,sizeof(int32)) != sizeof(int32)) {
		return B_ERROR;
	}
	if (is_little_endian)
		marker = B_LENDIAN_TO_HOST_INT32(marker);
	else
		marker = B_BENDIAN_TO_HOST_INT32(marker);

	if (marker != TOOL_SETTINGS_FILE_ID)
		return B_ERROR;

	if (file.Read(&marker,sizeof(int32)) != sizeof(int32)) {
		return B_ERROR;
	}
	if (is_little_endian)
		marker = B_LENDIAN_TO_HOST_INT32(marker);
	else
		marker = B_BENDIAN_TO_HOST_INT32(marker);

	if (marker != TOOL_SETTINGS_FILE_VERSION)
		return B_ERROR;

	// Here we can start reading the actual tools
	while (file.Read(&marker,sizeof(int32)) == sizeof(int32)) {
		if (is_little_endian)
			marker = B_LENDIAN_TO_HOST_INT32(marker);
		else
			marker = B_BENDIAN_TO_HOST_INT32(marker);

		DrawingTool *tool = ToolListEntry::ReturnTool(marker);
		if (tool != NULL) {
			tool->readSettings(file,is_little_endian);
		}
	}

	return B_NO_ERROR;
}


status_t ToolManager::WriteToolSettings(BFile &file)
{
	int32 endianness;
	if (B_HOST_IS_LENDIAN == 1)
		endianness = 0xFFFFFFFF;
	else
		endianness = 0x00000000;

	if (file.Write(&endianness,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	int32 id = TOOL_SETTINGS_FILE_ID;
	if (file.Write(&id,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	int32 version = TOOL_SETTINGS_FILE_VERSION;
	if (file.Write(&version,sizeof(int32)) != sizeof(int32))
		return B_ERROR;


	int32 index = 0;
	DrawingTool *tool;
	while ((tool = ToolListEntry::ReturnToolAt(index)) != NULL) {
		tool->writeSettings(file);
		index++;
	}

	return B_OK;
}


// Here begin the definitions of private ToolManager-class functions.
ToolManager::ToolManager()
{
	client_list_head = NULL;
	active_drawing_tool = NULL;

	// Add the tools to the tool-list
	ToolListEntry::AddTool(new SimpleTool());
	ToolListEntry::AddTool(new StraightLineTool());
	ToolListEntry::AddTool(new RectangleTool());
	ToolListEntry::AddTool(new EllipseTool());
	ToolListEntry::AddTool(new BrushTool());
	ToolListEntry::AddTool(new HairyBrushTool());
	ToolListEntry::AddTool(new AirBrushTool());
	ToolListEntry::AddTool(new BlurTool());
	ToolListEntry::AddTool(new FillTool());
	ToolListEntry::AddTool(new TextTool());
	ToolListEntry::AddTool(new TransparencyTool());
	ToolListEntry::AddTool(new EraserTool());
	ToolListEntry::AddTool(new SelectorTool());
	ToolListEntry::AddTool(new ColorSelectorTool());


	tool_pop_up_menu = new BPopUpMenu("tool_pop_up_menu");

	int32 i=0;
	DrawingTool *tool = ToolListEntry::ReturnToolAt(i++);

	while (tool != NULL) {
		BMessage *message = new BMessage(tool->GetType());
		tool_pop_up_menu->AddItem(new BMenuItem(tool->GetName(),message));
		tool = ToolListEntry::ReturnToolAt(i++);
	}
}


ToolManager::~ToolManager()
{
	// Here we must make sure that the tools haver finished their jobs
	// and that each client knows that the tools are not available anymore.




	// Delete the client-structs
	ToolManagerClient *client = client_list_head;
	while (client != NULL) {
		ToolManagerClient *helper = client->next_client;
		delete client;
		client = helper;
	}
	// Delete the tool-list-entries
	ToolListEntry::DeleteToolEntries();

	delete tool_pop_up_menu;
}



ToolManagerClient* ToolManager::ReturnClientData(ImageView *view)
{
	// What is the purpose of this function. What are the
	// clients. Some commentation about them would not
	// be a bad idea.
	// First search from the client-list for an entry with view.
	ToolManagerClient *current_entry = client_list_head;
	ToolManagerClient *previous_entry = NULL;
	while ((current_entry != NULL) && (current_entry->the_client != view)) {
		previous_entry = current_entry;
		current_entry = current_entry->next_client;
	}

	if (current_entry != NULL) {
		if (previous_entry != NULL) {
			previous_entry->next_client = current_entry->next_client;
		}
		if (current_entry != client_list_head) {	// This if is very important.
			current_entry->next_client = client_list_head;
			client_list_head = current_entry;
		}
	}
	else if (current_entry == NULL) {
		current_entry = new ToolManagerClient(view);
		current_entry->next_client = client_list_head;
		client_list_head = current_entry;
	}

	return current_entry;
}




// Here begin the definitions for ToolListEntry-class.
ToolListEntry* ToolListEntry::list_head = NULL;

ToolListEntry::ToolListEntry(DrawingTool *tool)
{
	the_tool = tool;
	tool_type = tool->GetType();
	next_tool = NULL;
}


ToolListEntry::~ToolListEntry()
{
	delete the_tool;
}


void ToolListEntry::AddTool(DrawingTool *tool)
{
	ToolListEntry *new_entry = new ToolListEntry(tool);

	ToolListEntry *previous_entry = list_head;
	while ((previous_entry != NULL) && (previous_entry->next_tool != NULL)) {
		previous_entry = previous_entry->next_tool;
	}

	if (previous_entry == NULL) {
		list_head = new_entry;
	}
	else {
		previous_entry->next_tool = new_entry;
	}
}

DrawingTool* ToolListEntry::ReturnTool(int32 tool_type)
{
	ToolListEntry *tool = list_head;
	while ((tool != NULL) && (tool->tool_type != tool_type))
		tool = tool->next_tool;

	if (tool != NULL)
		return tool->the_tool;
	else
		return NULL;
}

DrawingTool* ToolListEntry::ReturnToolAt(int32 index)
{
	ToolListEntry *tool = list_head;
	while ((tool != NULL) && (index > 0)) {
		tool = tool->next_tool;
		index--;
	}

	if (tool != NULL)
		return tool->the_tool;
	else
		return NULL;
}

void ToolListEntry::DeleteToolEntries()
{
	ToolListEntry *entry = list_head;
	while (entry != NULL) {
		ToolListEntry *helper = entry->next_tool;
		delete entry;
		entry = helper;
	}
}
