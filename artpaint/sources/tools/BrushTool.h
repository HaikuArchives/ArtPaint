/*

	Filename:	BrushTool.h
	Contents:	Declarations for a basic brush tool class.
	Author:		Heikki Suhonen

*/



#ifndef BRUSH_TOOL_H
#define BRUSH_TOOL_H

#include "Brush.h"
#include "DrawingTool.h"
#include "PixelOperations.h"
#include "CoordinateQueue.h"
#include "Selection.h"

class BrushTool : public DrawingTool {
		CoordinateQueue	*coordinate_queue;
		bool			reading_coordinates;
		ImageView		*image_view;

		Brush			*brush;
		uint32			*bits;
		int32			bpr;
		BPoint			last_point;
		int32 			left_bound;
		int32			right_bound;
		int32			top_bound;
		int32			bottom_bound;

		Selection		*selection;


		BRect	draw_line(BPoint,BPoint,uint32);
inline	void	draw_brush_handle_selection(BPoint,int32,int32,uint32);
inline	void	draw_brush(BPoint,int32,int32,uint32);
		void	test_brush(BPoint,uint32);
		void	test_brush2(BPoint,uint32);

//static	int32	CoordinateReader(void*);
//		int32	read_coordinates();
//

public:
		BrushTool();
virtual	~BrushTool();

ToolScript*	UseTool(ImageView*,uint32,BPoint,BPoint);
int32		UseToolWithScript(ToolScript*,BBitmap*);

BView* 	makeConfigView();
void	UpdateConfigView(BView*);

const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();

status_t	readSettings(BFile&,bool);
status_t	writeSettings(BFile&);

Brush*	GetBrush() { return brush; }
};

inline void BrushTool::draw_brush_handle_selection(BPoint point,int32 dx,int32 dy,uint32 c)
{
	span 	*spans;
	uint32	**brush_matrix;
	uint32 	*target_bits = bits;
	int32  	px = (int32)point.x;
	int32 	py = (int32)point.y;
	int32 	y;
	brush_matrix = brush->GetData(&spans,dx,dy);

	while ( (spans != NULL) && (spans->row+py <= bottom_bound) ) {
		int32 left = max_c(px+spans->span_start,left_bound) ;
		int32 right = min_c(px+spans->span_end,right_bound);
		y = spans->row;
		if (y+py >= top_bound) {
			target_bits = bits + (y+py)*bpr + left;		// This works even if there are many spans in one row.
			for (int32 x=left;x<=right;x++) {
				if (selection->ContainsPoint(x,y+py))
					*target_bits = mix_2_pixels_fixed(c,*target_bits,brush_matrix[y][x-px]);

				target_bits++;
			}
		}
		spans = spans->next;
	}
}

inline void BrushTool::draw_brush(BPoint point,int32 dx,int32 dy,uint32 c)
{
	span 	*spans;
	uint32	**brush_matrix;
	uint32 	*target_bits = bits;
	int32  	px = (int32)point.x;
	int32 	py = (int32)point.y;
	int32 	y;
	brush_matrix = brush->GetData(&spans,dx,dy);

	while ( (spans != NULL) && (spans->row+py <= bottom_bound) ) {
		int32 left = max_c(px+spans->span_start,left_bound) ;
		int32 right = min_c(px+spans->span_end,right_bound);
		y = spans->row;
		if (y+py >= top_bound) {
			target_bits = bits + (y+py)*bpr + left;		// This works even if there are many spans in one row.
			for (int32 x=left;x<=right;x++) {
				*target_bits = mix_2_pixels_fixed(c,*target_bits,brush_matrix[y][x-px]);
				target_bits++;
			}
		}
		spans = spans->next;
	}
}




class BrushToolConfigView : public DrawingToolConfigView {
public:
		BrushToolConfigView(BRect rect,DrawingTool *t);

void	AttachedToWindow();
};

#endif
