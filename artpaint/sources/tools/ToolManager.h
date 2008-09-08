/*

	Filename:	ToolManager.h
	Contents:	ToolManager-class declarations.
	Author:		Heikki Suhonen

*/


#ifndef TOOL_MANAGER_H
#define	TOOL_MANAGER_H


#define	TOOL_SETTINGS_FILE_ID		0x12345678
#define	TOOL_SETTINGS_FILE_VERSION	0x00000001

enum image_view_event_type {
	CURSOR_ENTERED_VIEW,
	CURSOR_EXITED_VIEW,
	TOOL_ACTIVATED,
	VIEW_DESTROYED
};

#include <PopUpMenu.h>
#include <File.h>

class ImageView;
class DrawingTool;
class ToolScript;
struct brush_info;


struct ToolManagerClient {
		ToolManagerClient(ImageView *view) {
			the_client = view;
			next_client = NULL;
			last_updated_rect = BRect(0,0,-1,-1);
			active_tool = NULL;
		}

ImageView			*the_client;
// Here we can store some status-information about the client such as the last updated rectangle.
BRect				last_updated_rect;
DrawingTool			*active_tool;
ToolManagerClient	*next_client;
};



class ToolListEntry {
		DrawingTool		*the_tool;
		int32			tool_type;
		ToolListEntry	*next_tool;

static	ToolListEntry	*list_head;
		ToolListEntry(DrawingTool*);
		~ToolListEntry();
public:
static	void			AddTool(DrawingTool*);
static	DrawingTool*	ReturnTool(int32);
static	DrawingTool*	ReturnToolAt(int32);
static	void			DeleteToolEntries();
};


class ToolManager {
		// ImageViews are the main clients of ToolManager-class. A client will be
		// added when it first calls some function of ToolManager. A client is removed
		// when it explicitily informs us that it is going away through the NotifyViewEvent-
		// function. The clients will be kept in a straight list in which a client is moved to
		// the head of the list whenever it calls one of the ToolManager-functions. The function
		// ReturnClientData returns a pointer to ToolManagerClient-object and moves the object
		// to correct position in the list. If the object does not exist yet, it will be created.

ToolManagerClient	*client_list_head;

DrawingTool			*active_drawing_tool;

BPopUpMenu			*tool_pop_up_menu;



ToolManagerClient*	ReturnClientData(ImageView*);



					ToolManager();
					~ToolManager();
public:
static	status_t	CreateToolManager();
static	status_t	DestroyToolManager();

		ToolScript*	StartTool(ImageView*,uint32,BPoint,BPoint,int32 toolType = 0);
		status_t	MouseDown(ImageView*,BPoint,BPoint,uint32,int32);
		status_t	KeyDown(ImageView*,const char*,int32);
		status_t	KeyUp(ImageView*,const char*,int32);
		BRect		LastUpdatedRect(ImageView*);

		status_t	ChangeTool(int32);
const	void*		ReturnCursor();
const	DrawingTool*	ReturnTool(int32);
		int32		ReturnActiveToolType();
		BView*		ReturnConfigurationView(int32);
		status_t	SetCurrentBrush(brush_info*);

		BPopUpMenu*	ReturnToolPopUpMenu();

// The next function can be used to notify the ToolManager about
// other view-events than what have to do with drawing.
		status_t	NotifyViewEvent(ImageView*,image_view_event_type);

		status_t	ReadToolSettings(BFile&);
		status_t	WriteToolSettings(BFile&);
};


extern ToolManager	*tool_manager;

#endif
