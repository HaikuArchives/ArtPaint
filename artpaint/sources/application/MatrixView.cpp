/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <InterfaceDefs.h>
#include <stdio.h>

#include "MatrixView.h"


MatrixView::MatrixView(int32 cell_w,int32 cell_h,int32 spacing)
	: 	BView(BRect(0,0,0,0),"matrix_view",B_FOLLOW_ALL_SIDES,B_FRAME_EVENTS)
		,cell_height(cell_h), cell_width(cell_w), cell_spacing(spacing)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void MatrixView::AttachedToWindow()
{
	FrameResized(Frame().Width(),Frame().Height());
}


void MatrixView::FrameResized(float width,float)
{
	width -= cell_spacing;
	int32 row_length = (int32)width / (cell_width + cell_spacing);
	row_length = max_c(row_length,1);

	for (int32 i=0;i<CountChildren();i++) {
		int32 y = (i / row_length)*(cell_height+cell_spacing) + cell_spacing;
		int32 x = (i % row_length)*(cell_width+cell_spacing) + cell_spacing;
		ChildAt(i)->MoveTo(x,y);
	}
}

void MatrixView::GetPreferredSize(float *width,float *height)
{
//	// The preferred size depends on the current size
//	if (Bounds().IntegerWidth() >= Bounds().IntegerHeight()) {
		// If the bounds are wider than high, use as many rows as possible
		int32 i_height = Bounds().IntegerHeight();
		int32 rows = min_c(CountChildren(),max_c(1,(i_height-cell_spacing)/(cell_height+cell_spacing)));

		int32 columns = (int32)max_c(1,ceil((float)CountChildren()/(float)rows));
		rows = (int32)max_c(1,ceil((float)CountChildren()/(float)columns));

		*height = rows*(cell_height+cell_spacing)+cell_spacing;
		*width = columns*(cell_width+cell_spacing)+cell_spacing;

		*width = max_c(*width,2*cell_spacing+cell_width);
//		printf("Did a vertical\n");
//	}
//	else {
//		// if the bounds is higher than wide, use as many columns as possible
//		int32 i_width = Bounds().IntegerWidth();
//		int32 columns = min_c(CountChildren(),max_c(1,(i_width-cell_spacing)/(cell_width+cell_spacing)));
//
//		int32 rows = max_c(1,ceil((float)CountChildren()/(float)columns));
//		columns = max_c(1,ceil((float)CountChildren()/(float)rows));
//
//		*height = rows*(cell_height+cell_spacing)+cell_spacing;
//		*width = columns*(cell_width+cell_spacing)+cell_spacing;
//
//		*height = max_c(*height,2*cell_spacing+cell_height);
//		printf("Did a horizontal\n");
//	}
}

status_t MatrixView::AddSubView(BView *view)
{
	if (view == NULL)
		return B_ERROR;

	if ((view->Bounds().IntegerWidth() != (cell_width-1)) ||
		(view->Bounds().IntegerHeight() != (cell_height-1))) {
		return B_ERROR;
	}

	// position the view correctly
	if (LockLooper() == true) {
		AddChild(view);
		FrameResized(Frame().Width(),Frame().Height());
		UnlockLooper();
	}
	else {
		AddChild(view);
	}
	return B_OK;
}
