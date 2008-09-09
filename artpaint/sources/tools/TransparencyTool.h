/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef TRANSPARENCY_TOOL_H
#define TRANSPARENCY_TOOL_H

#include "DrawingTool.h"


class TransparencyTool : public DrawingTool {

public:
		TransparencyTool();
virtual	~TransparencyTool();

ToolScript*	UseTool(ImageView*,uint32,BPoint,BPoint);
int32		UseToolWithScript(ToolScript*,BBitmap*);

BView*	makeConfigView();
const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();
};



class TransparencyToolConfigView : public DrawingToolConfigView {
		ControlSliderBox	*size_slider;
		ControlSliderBox	*speed_slider;
		ControlSliderBox	*transparency_slider;

public:
		TransparencyToolConfigView(BRect rect,DrawingTool *t);

void	AttachedToWindow();
};

#endif
