/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef SIMPLE_TOOL_H
#define SIMPLE_TOOL_H

#include "LineTool.h"
#include "CoordinateQueue.h"
#include "BitmapDrawer.h"



// This class is defines the first actual tool.
class SimpleTool : public LineTool {
	CoordinateQueue	*coordinate_queue;

	bool		reading_coordinates;

	ImageView	*image_view;

static	int32	CoordinateReader(void*);
		int32	read_coordinates();


public:
			SimpleTool();
virtual		~SimpleTool();

ToolScript*	UseTool(ImageView*,uint32,BPoint,BPoint);
int32		UseToolWithScript(ToolScript*,BBitmap*);

		BView*	makeConfigView();
const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();
};



class SimpleToolConfigView : public DrawingToolConfigView {
		ControlSliderBox	*size_slider;

public:
		SimpleToolConfigView(BRect rect,DrawingTool *t);

void	AttachedToWindow();
};


#endif
