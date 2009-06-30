/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef ERASER_TOOL_H
#define ERASER_TOOL_H

#include "LineTool.h"
#include "CoordinateQueue.h"
#include "BitmapDrawer.h"

class ControlSliderBox;

// This class is defines the first actual tool.
class EraserTool : public LineTool {
	CoordinateQueue	*coordinate_queue;

	bool		reading_coordinates;

	ImageView	*image_view;

static	int32	CoordinateReader(void*);
		int32	read_coordinates();


public:
				EraserTool();
virtual			~EraserTool();

ToolScript*		UseTool(ImageView*,uint32,BPoint,BPoint);
		int32	UseToolWithScript(ToolScript*,BBitmap*);

		BView*	makeConfigView();
const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();
};



class EraserToolConfigView : public DrawingToolConfigView {
		ControlSliderBox	*size_slider;
		BRadioButton		*mode_button_1;
		BRadioButton		*mode_button_2;



public:
		EraserToolConfigView(BRect rect,DrawingTool *t);

void	AttachedToWindow();
};


#endif
