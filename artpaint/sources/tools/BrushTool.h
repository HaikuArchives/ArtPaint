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

#include "Brush.h"
#include "CoordinateQueue.h"
#include "DrawingTool.h"
#include "PixelOperations.h"
#include "Selection.h"


class BrushTool : public DrawingTool {
public:
								BrushTool();
	virtual						~BrushTool();

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*				makeConfigView();
			void				UpdateConfigView(BView*);

			const void*			ReturnToolCursor();
			const char*			ReturnHelpString(bool isInUse);


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
								BrushToolConfigView(BRect rect, DrawingTool *t);
};


// #pragma mark -- BrushTool


inline void
BrushTool::draw_brush_handle_selection(BPoint point, int32 dx, int32 dy, uint32 c)
{
	span* spans;
	int32 px = (int32)point.x;
	int32 	py = (int32)point.y;
	uint32** brush_matrix = brush->GetData(&spans, dx, dy);

	uint32* target_bits = bits;
	while ((spans != NULL) && (spans->row + py <= bottom_bound)) {
		int32 left = max_c(px + spans->span_start, left_bound) ;
		int32 right = min_c(px + spans->span_end, right_bound);
		int32 y = spans->row;
		if (y + py >= top_bound) {
			// This works even if there are many spans in one row.
			target_bits = bits + (y + py) * bpr + left;
			for (int32 x = left; x <= right; ++x) {
				if (selection->ContainsPoint(x, y + py)) {
					*target_bits = mix_2_pixels_fixed(c, *target_bits,
						brush_matrix[y][x-px]);
				}
				target_bits++;
			}
		}
		spans = spans->next;
	}
}


inline void
BrushTool::draw_brush(BPoint point, int32 dx, int32 dy, uint32 c)
{
	span* spans;
	int32 px = (int32)point.x;
	int32 py = (int32)point.y;
	uint32** brush_matrix = brush->GetData(&spans, dx, dy);

	uint32* target_bits = bits;
	while ((spans != NULL) && (spans->row + py <= bottom_bound)) {
		int32 left = max_c(px + spans->span_start, left_bound) ;
		int32 right = min_c(px + spans->span_end, right_bound);
		int32 y = spans->row;
		if (y + py >= top_bound) {
			// This works even if there are many spans in one row.
			target_bits = bits + (y + py) * bpr + left;
			for (int32 x = left; x <= right; ++x) {
				*target_bits = mix_2_pixels_fixed(c, *target_bits,
						brush_matrix[y][x-px]);
				target_bits++;
			}
		}
		spans = spans->next;
	}
}

#endif
