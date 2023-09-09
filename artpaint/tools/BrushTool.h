/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef BRUSH_TOOL_H
#define BRUSH_TOOL_H

#include "DrawingTool.h"


class Brush;
class CoordinateQueue;
class ImageView;
class Selection;
class ToolScript;


class BrushTool : public DrawingTool {
public:
								BrushTool();
	virtual						~BrushTool();

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*				ConfigView();
			const void*			ToolCursor() const;
			const char*			HelpString(bool isInUse) const;

			status_t			readSettings(BFile& file, bool isLittleEndian);
			status_t			writeSettings(BFile& file);

private:
			BPoint				last_point;
			bool				reading_coordinates;

			Selection*			selection;
			ImageView*			image_view;
			CoordinateQueue*	coordinate_queue;
};


class BrushToolConfigView : public DrawingToolConfigView {
public:
								BrushToolConfigView(DrawingTool* tool);
	virtual						~BrushToolConfigView() {}
};

#endif	// BRUSH_TOOL_H
