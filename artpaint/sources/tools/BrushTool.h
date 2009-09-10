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

			BView*				makeConfigView();
			void				UpdateConfigView(BView*);

			const void*			ToolCursor() const;
			const char*			HelpString(bool isInUse) const;


			status_t			readSettings(BFile& file, bool isLittleEndian);
			status_t			writeSettings(BFile& file);

			Brush*				GetBrush() { return brush; }

private:
			BRect				draw_line(BPoint,BPoint,uint32);
	inline	void				draw_brush_handle_selection(BPoint, int32,
									int32,uint32);
	inline	void				draw_brush(BPoint,int32,int32,uint32);
			void				test_brush(BPoint,uint32);
			void				test_brush2(BPoint,uint32);

//			int32				read_coordinates();
//	static	int32				CoordinateReader(void*);


private:
			BPoint				last_point;
			bool				reading_coordinates;

			int32				bpr;
			int32				left_bound;
			int32				right_bound;
			int32				top_bound;
			int32				bottom_bound;

			uint32*				bits;
			Brush*				brush;
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
