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
#include "PaintApplication.h"
#include "Settings.h"
#include "ToolEventAdapter.h"
#include "ToolSelectionWindow.h"
#include "ToolSetupWindow.h"


#include <MenuItem.h>
#include <PopUpMenu.h>


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


// #pragma mark -- ToolListEntry


class ToolListEntry {
public:
	static	void			AddTool(DrawingTool*);
	static	DrawingTool*	ReturnTool(int32);
	static	DrawingTool*	ReturnToolAt(int32);
	static	void			DeleteToolEntries();

private:
	DrawingTool		*the_tool;
	int32			tool_type;
	ToolListEntry	*next_tool;

	static	ToolListEntry	*list_head;
	ToolListEntry(DrawingTool*);
	~ToolListEntry();
};


ToolListEntry* ToolListEntry::list_head = NULL;


ToolListEntry::ToolListEntry(DrawingTool *tool)
{
	the_tool = tool;
	tool_type = tool->Type();
	next_tool = NULL;
}


ToolListEntry::~ToolListEntry()
{
	delete the_tool;
}


void
ToolListEntry::AddTool(DrawingTool *tool)
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


DrawingTool*
ToolListEntry::ReturnTool(int32 tool_type)
{
	ToolListEntry *tool = list_head;
	while ((tool != NULL) && (tool->tool_type != tool_type))
		tool = tool->next_tool;

	if (tool)
		return tool->the_tool;
	return NULL;
}


DrawingTool*
ToolListEntry::ReturnToolAt(int32 index)
{
	ToolListEntry *tool = list_head;
	while ((tool != NULL) && (index > 0)) {
		tool = tool->next_tool;
		index--;
	}

	if (tool)
		return tool->the_tool;
	return NULL;
}


void
ToolListEntry::DeleteToolEntries()
{
	ToolListEntry *entry = list_head;
	while (entry != NULL) {
		ToolListEntry *helper = entry->next_tool;
		delete entry;
		entry = helper;
	}
}


// #pragma mark -- ToolManager


ToolManager* tool_manager;
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

	if (toolType != 0)
		activeTool = ToolListEntry::ReturnTool(toolType);

	if (activeTool == NULL)
		activeTool = fActiveTool;

	ToolManagerClient* clientData = ReturnClientData(view);

	// If the client has not yet consumed the updated rect,
	if (clientData->fLastUpdatedRect.IsValid()) {
		// we return NULL.
		return NULL;
	}

	// Here start the correct tool.
	clientData->fActiveTool = activeTool;
	if (clientData->fActiveTool != NULL) {
		view->SetToolHelpString(activeTool->HelpString(true));
		ToolScript *the_script = clientData->fActiveTool->UseTool(view,buttons,bitmap_point,view_point);
		view->SetToolHelpString(activeTool->HelpString(false));

		// When the tool has finished we should record the updated_rect into clients data.
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
	ToolManagerClient *client_data = ReturnClientData(view);

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
	ToolManagerClient *client_data = ReturnClientData(view);

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
	ToolManagerClient *client_data = ReturnClientData(view);

	BRect temp_rect = client_data->fLastUpdatedRect;
	client_data->fLastUpdatedRect = BRect(0, 0, -1, -1);

	return temp_rect;
}


status_t
ToolManager::ChangeTool(int32 tool_type)
{
	fActiveTool = ToolListEntry::ReturnTool(tool_type);
	ToolSetupWindow::CurrentToolChanged(tool_type);
	ToolSelectionWindow::ChangeTool(tool_type);

	((PaintApplication*)be_app)->GlobalSettings()->primary_tool = tool_type;

	return B_OK;
}


const DrawingTool*
ToolManager::ReturnTool(int32 tool_type) const
{
	return ToolListEntry::ReturnTool(tool_type);
}


const void*
ToolManager::ReturnCursor() const
{
	if (fActiveTool == NULL)
		return B_HAND_CURSOR;

	int32 cursor_mode = ((PaintApplication*)be_app)->GlobalSettings()->cursor_mode;
	if (cursor_mode == TOOL_CURSOR_MODE)
		return fActiveTool->ToolCursor();

	return HS_CROSS_CURSOR;
}


int32
ToolManager::ReturnActiveToolType()
{
	if (fActiveTool != NULL)
		return fActiveTool->Type();

	return NO_TOOL;
}


status_t
ToolManager::SetCurrentBrush(brush_info *binfo)
{
	// Set the new brush for all tools that use brushes.
	BrushTool *brush_tool =
		dynamic_cast<BrushTool*>(ToolListEntry::ReturnTool(BRUSH_TOOL));

	if (brush_tool != NULL) {
		Brush *a_brush = brush_tool->GetBrush();
		a_brush->ModifyBrush(*binfo);
		BrushEditor::BrushModified();
		a_brush->CreateDiffBrushes();
//		ToolSetupWindow::changeToolForTheSetupWindow(BRUSH_TOOL);
//		ToolSetupWindow::updateTool(BRUSH_TOOL);
	}

	return B_OK;
}


status_t
ToolManager::NotifyViewEvent(ImageView *view, image_view_event_type type)
{
	if (fActiveTool) {
		ToolManagerClient *client_data = ReturnClientData(view);
		bool show = (client_data->fActiveTool == fActiveTool);

		if (type == CURSOR_ENTERED_VIEW) {
				view->SetToolHelpString(fActiveTool->HelpString(show));
		} else if (type == TOOL_ACTIVATED) {
			view->SetToolHelpString(fActiveTool->HelpString(show));
		} else if (type == CURSOR_EXITED_VIEW) {
			view->SetToolHelpString("");
		}
	}
	return B_OK;
}


BView*
ToolManager::ReturnConfigurationView(int32 tool_type)
{
	if (DrawingTool* tool = ToolListEntry::ReturnTool(tool_type))
		return tool->makeConfigView();
	return NULL;
}


BPopUpMenu*
ToolManager::ToolPopUpMenu()
{
	if (!fToolPopUpMenu) {
		fToolPopUpMenu = new BPopUpMenu("");

		int32 i = 0;
		while (DrawingTool* tool = ToolListEntry::ReturnToolAt(i++)) {
			fToolPopUpMenu->AddItem(new BMenuItem(tool->Name(),
				new BMessage(tool->Type())));
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
	: fToolPopUpMenu(NULL)
	, fActiveTool(NULL)
	, fClientListHead(NULL)
{
	ToolListEntry::AddTool(new FreeLineTool());
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
}


ToolManager::~ToolManager()
{
	// Here we must make sure that the tools haver finished their jobs
	// and that each client knows that the tools are not available anymore.


	// Delete the client-structs
	ToolManagerClient *client = fClientListHead;
	while (client != NULL) {
		ToolManagerClient *helper = client->fNextClient;
		delete client;
		client = helper;
	}
	// Delete the tool-list-entries
	ToolListEntry::DeleteToolEntries();

	delete fToolPopUpMenu;
}


ToolManagerClient*
ToolManager::ReturnClientData(ImageView *view)
{
	// What is the purpose of this function. What are the
	// clients. Some commentation about them would not
	// be a bad idea.
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
		if (current_entry != fClientListHead) {	// This if is very important.
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
