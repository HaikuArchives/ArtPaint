/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009 - 2010, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef TOOL_MANAGER_H
#define	TOOL_MANAGER_H

#include <SupportDefs.h>


#include "Brush.h"


class BFile;
class BPoint;
class BPopUpMenu;
class BRect;
class BView;
class DrawingTool;
class ImageView;
class ToolScript;


struct brush_info;
struct ToolManagerClient;


enum image_view_event_type {
	CURSOR_ENTERED_VIEW,
	CURSOR_EXITED_VIEW,
	TOOL_ACTIVATED,
	VIEW_DESTROYED
};


/* !
	ImageViews are the main clients of ToolManager-class. A client will be
	added when it first calls some function of ToolManager. A client is
	removed when it explicitily informs us that it is going away through the
	NotifyViewEvent function. The clients will be kept in a straight list in
	which a client is moved to the head of the list whenever it calls one of the
	ToolManager-functions. The function ReturnClientData returns a pointer to
	ToolManagerClient-object and moves the object to correct position in the
	list. If the object does not exist yet, it will be created.
*/

class ToolManager {
public:
	static	ToolManager&			Instance();

	static	status_t				CreateToolManager();
	static	status_t				DestroyToolManager();

			ToolScript*				StartTool(ImageView*, uint32, BPoint, BPoint,
										int32 toolType = 0);

			status_t				MouseDown(ImageView*, BPoint, BPoint, uint32,
										int32);

			status_t				KeyUp(ImageView*, const char*, int32);
			status_t				KeyDown(ImageView*, const char*, int32);

			BRect					LastUpdatedRect(ImageView*);

			status_t				ChangeTool(int32);
			const void*				ReturnCursor() const;
			DrawingTool*			ReturnTool(int32) const;
			DrawingTool*			ReturnActiveTool() const { return fActiveTool; }
			int32					ReturnActiveToolType() const;
			BView*					ConfigView(int32);
			status_t				SetCurrentBrush(brush_info*);
			Brush*					GetCurrentBrush();
			BPopUpMenu*				ToolPopUpMenu();

			// The next function can be used to notify the ToolManager about
			// other view-events than what have to do with drawing.
			status_t				NotifyViewEvent(ImageView*,
										image_view_event_type eventType);

			status_t				ReadToolSettings(BFile& file);
			status_t				WriteToolSettings(BFile& file);

private:
									ToolManager();
									~ToolManager();

			void					_AddTool(DrawingTool* tool);
			ToolManagerClient*		_ReturnClientData(ImageView* imageView);

private:
			BPopUpMenu*				fToolPopUpMenu;
			DrawingTool*			fActiveTool;
			Brush*					fActiveBrush;
			ToolManagerClient*		fClientListHead;

	static	ToolManager*			fToolManager;
};

#endif	// TOOL_MANAGER_H
