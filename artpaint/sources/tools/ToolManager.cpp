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

#include "ToolManager.h"

#include "BrushEditor.h"
#include "Cursors.h"
#include "DrawingTools.h"
#include "ImageView.h"
#include "SettingsServer.h"
#include "ToolEventAdapter.h"
#include "ToolSelectionWindow.h"
#include "ToolSetupWindow.h"


#include <MenuItem.h>
#include <File.h>
#include <PopUpMenu.h>


#include <map>


struct ToolManagerClient {
						ToolManagerClient(ImageView *view)
							: fLastUpdatedRect(BRect())
							, fClient(view)
							, fActiveTool(NULL)
							, fNextClient(NULL) {}

	BRect				fLastUpdatedRect;

	ImageView*			fClient;
	DrawingTool*		fActiveTool;
	ToolManagerClient*	fNextClient;
};


// #pragma mark -- ToolManager


ToolManager* tool_manager;

typedef std::map<int32, DrawingTool*> ToolMap;
ToolMap gManagerToolMap;

#define	TOOL_SETTINGS_FILE_ID		0x12345678
#define	TOOL_SETTINGS_FILE_VERSION	0x00000001


status_t
ToolManager::CreateToolManager()
{
	if (tool_manager == NULL) {
		tool_manager = new ToolManager();
		return B_OK;
	}
	return B_ERROR;
}


status_t
ToolManager::DestroyToolManager()
{
	if (tool_manager) {
		delete tool_manager;
		tool_manager = NULL;
		return B_OK;
	}
	return B_ERROR;
}


// These are the ImageView-interface functions.
ToolScript*
ToolManager::StartTool(ImageView *view, uint32 buttons, BPoint bitmap_point,
	BPoint view_point, int32 toolType)
{
	DrawingTool* activeTool = NULL;

	if (toolType != 0) {
		ToolMap::const_iterator it = gManagerToolMap.find(toolType);
		if (it != gManagerToolMap.end())
			activeTool = it->second;
	}

	if (activeTool == NULL)
		activeTool = fActiveTool;

	ToolManagerClient* clientData = _ReturnClientData(view);

	// If the client has not yet consumed the updated rect,
	if (clientData->fLastUpdatedRect.IsValid()) {
		// we return NULL.
		return NULL;
	}

	// Here start the correct tool.
	clientData->fActiveTool = activeTool;
	if (clientData->fActiveTool != NULL) {
		view->SetToolHelpString(activeTool->HelpString(true));
		ToolScript *the_script = clientData->fActiveTool->UseTool(view,
			buttons, bitmap_point, view_point);
		view->SetToolHelpString(activeTool->HelpString(false));

		// When the tool has finished we should record the updated_rect into
		// clients data.
		clientData->fLastUpdatedRect = clientData->fActiveTool->LastUpdatedRect();
		clientData->fActiveTool->SetLastUpdatedRect(BRect());
		clientData->fActiveTool = NULL;

		return the_script;
	}
	return NULL;
}


status_t
ToolManager::MouseDown(ImageView *view, BPoint view_point, BPoint bitmap_point,
	uint32 buttons, int32 clicks)
{
	// Here inform the client_data->fActiveTool about the mouse-event if it
	// accepts that information.
	ToolManagerClient *client_data = _ReturnClientData(view);

	if (client_data->fActiveTool != NULL) {
		ToolEventAdapter *adapter =
			dynamic_cast<ToolEventAdapter*>(client_data->fActiveTool);
		if (adapter != NULL) {
			adapter->SetClickEvent(view_point,bitmap_point,buttons,clicks);
		}
	}
	return B_OK;
}


status_t
ToolManager::KeyDown(ImageView* view, const char* bytes, int32 numBytes)
{
	// Here inform the client_data->fActiveTool about the key-event if it
	// accepts that information.
	ToolManagerClient *client_data = _ReturnClientData(view);

	if (client_data != NULL) {
		if (client_data->fActiveTool != NULL) {
			ToolEventAdapter *adapter =
				dynamic_cast<ToolEventAdapter*>(client_data->fActiveTool);
			if (adapter != NULL) {
				adapter->SetKeyEvent(bytes,numBytes);
			}
		}
	}
	return B_OK;
}


status_t
ToolManager::KeyUp(ImageView *view,const char *bytes,int32 numBytes)
{
	// Here inform the client_data->fActiveTool about the key-event if it
	// accepts that information.
	return B_OK;
}


BRect
ToolManager::LastUpdatedRect(ImageView *view)
{
	ToolManagerClient *client_data = _ReturnClientData(view);

	BRect temp_rect = client_data->fLastUpdatedRect;
	client_data->fLastUpdatedRect = BRect(0, 0, -1, -1);

	return temp_rect;
}


status_t
ToolManager::ChangeTool(int32 toolType)
{
	ToolMap::const_iterator it = gManagerToolMap.find(toolType);
	if (it != gManagerToolMap.end()) {
		fActiveTool = it->second;

		ToolSelectionWindow::ChangeTool(toolType);
		ToolSetupWindow::CurrentToolChanged(toolType);

		if (SettingsServer* server = SettingsServer::Instance())
			server->SetValue(SettingsServer::Application, skTool, toolType);
		return B_OK;
	}
	return B_ERROR;
}


DrawingTool*
ToolManager::ReturnTool(int32 toolType) const
{
	ToolMap::const_iterator it = gManagerToolMap.find(toolType);
	if (it != gManagerToolMap.end())
		return it->second;
	return NULL;
}


const void*
ToolManager::ReturnCursor() const
{
	if (fActiveTool == NULL)
		return B_HAND_CURSOR;

	if (SettingsServer* server = SettingsServer::Instance()) {
		BMessage settings;
		server->GetApplicationSettings(&settings);

		int32 cursorMode = TOOL_CURSOR_MODE;
		settings.FindInt32(skCursorMode, &cursorMode);

		if (cursorMode == TOOL_CURSOR_MODE)
			return fActiveTool->ToolCursor();
	}

	return HS_CROSS_CURSOR;
}


int32
ToolManager::ReturnActiveToolType() const
{
	if (fActiveTool != NULL)
		return fActiveTool->Type();
	return NO_TOOL;
}


status_t
ToolManager::SetCurrentBrush(brush_info *binfo)
{
	// Set the new brush for all tools that use brushes.
	BrushTool *brush_tool = dynamic_cast<BrushTool*>(ReturnTool(BRUSH_TOOL));

	if (brush_tool != NULL) {
		Brush *a_brush = brush_tool->GetBrush();
		a_brush->ModifyBrush(*binfo);
		BrushEditor::BrushModified();
		a_brush->CreateDiffBrushes();
	}
	return B_OK;
}


status_t
ToolManager::NotifyViewEvent(ImageView *view, image_view_event_type type)
{
	if (fActiveTool) {
		ToolManagerClient *client_data = _ReturnClientData(view);
		bool show = (client_data->fActiveTool == fActiveTool);

		if (type == CURSOR_ENTERED_VIEW) {
				view->SetToolHelpString(fActiveTool->HelpString(show));
		} else if (type == TOOL_ACTIVATED) {
			view->SetToolHelpString(fActiveTool->HelpString(show));
		} else if (type == CURSOR_EXITED_VIEW) {
			view->SetToolHelpString("");
		}
		return B_OK;
	}
	return B_ERROR;
}


BView*
ToolManager::ReturnConfigurationView(int32 tool_type)
{
	if (DrawingTool* tool = ReturnTool(tool_type))
		return tool->ConfigView();
	return NULL;
}


BPopUpMenu*
ToolManager::ToolPopUpMenu()
{
	if (!fToolPopUpMenu) {
		fToolPopUpMenu = new BPopUpMenu("");

		ToolMap::const_iterator it = gManagerToolMap.begin();
		for (it = it; it != gManagerToolMap.end(); ++it) {
			fToolPopUpMenu->AddItem(new BMenuItem(it->second->Name(),
				new BMessage(it->second->Type())));
		}
	}
	return fToolPopUpMenu;
}


status_t
ToolManager::ReadToolSettings(BFile &file)
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
	while (file.Read(&marker, sizeof(int32)) == sizeof(int32)) {
		if (is_little_endian)
			marker = B_LENDIAN_TO_HOST_INT32(marker);
		else
			marker = B_BENDIAN_TO_HOST_INT32(marker);

		ToolMap::const_iterator it = gManagerToolMap.find(marker);
		if (it != gManagerToolMap.end())
			it->second->readSettings(file, is_little_endian);
	}
	return B_OK;
}


status_t
ToolManager::WriteToolSettings(BFile &file)
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

	ToolMap::const_iterator it = gManagerToolMap.begin();
	for (it = it; it != gManagerToolMap.end(); ++it)
		it->second->writeSettings(file);

	return B_OK;
}


// Here begin the definitions of private ToolManager-class functions.
ToolManager::ToolManager()
	: fToolPopUpMenu(NULL)
	, fActiveTool(NULL)
	, fClientListHead(NULL)
{
	_AddTool(new FreeLineTool());
	_AddTool(new StraightLineTool());
	_AddTool(new RectangleTool());
	_AddTool(new EllipseTool());
	_AddTool(new BrushTool());
	_AddTool(new HairyBrushTool());
	_AddTool(new AirBrushTool());
	_AddTool(new BlurTool());
	_AddTool(new FillTool());
	_AddTool(new TextTool());
	_AddTool(new TransparencyTool());
	_AddTool(new EraserTool());
	_AddTool(new SelectorTool());
	_AddTool(new ColorSelectorTool());
}


ToolManager::~ToolManager()
{
	// Delete the client structs
	ToolManagerClient *client = fClientListHead;
	while (client != NULL) {
		ToolManagerClient *helper = client->fNextClient;
		delete client;
		client = helper;
	}

	// Delete the tools
	ToolMap::const_iterator it = gManagerToolMap.begin();
	for (it = it; it != gManagerToolMap.end(); ++it)
		delete it->second;

	delete fToolPopUpMenu;
}


void
ToolManager::_AddTool(DrawingTool* tool)
{
	gManagerToolMap.insert(std::make_pair(tool->Type(), tool));
}


ToolManagerClient*
ToolManager::_ReturnClientData(ImageView *view)
{
	// What is the purpose of this function. What are the clients. Some
	// commentation about them would not be a bad idea.

	// First search from the client-list for an entry with view.
	ToolManagerClient *current_entry = fClientListHead;
	ToolManagerClient *previous_entry = NULL;
	while ((current_entry != NULL) && (current_entry->fClient != view)) {
		previous_entry = current_entry;
		current_entry = current_entry->fNextClient;
	}

	if (current_entry != NULL) {
		if (previous_entry != NULL) {
			previous_entry->fNextClient = current_entry->fNextClient;
		}

		// This if is very important.
		if (current_entry != fClientListHead) {
			current_entry->fNextClient = fClientListHead;
			fClientListHead = current_entry;
		}
	} else if (current_entry == NULL) {
		current_entry = new ToolManagerClient(view);
		current_entry->fNextClient = fClientListHead;
		fClientListHead = current_entry;
	}

	return current_entry;
}
