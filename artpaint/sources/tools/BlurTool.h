/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef BLUR_TOOL_H
#define BLUR_TOOL_H

#include "DrawingTool.h"

class Selection;

class BlurTool : public DrawingTool {

Selection	*selection;

public:
		BlurTool();
virtual	~BlurTool();

ToolScript*	UseTool(ImageView*,uint32,BPoint,BPoint);
int32		UseToolWithScript(ToolScript*,BBitmap*);

BView*	makeConfigView();
const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();
};



class BlurToolConfigView : public DrawingToolConfigView {
		ControlSliderBox	*size_slider;
		BCheckBox			*continuity_checkbox;

public:
		BlurToolConfigView(BRect rect, DrawingTool *t);

void	AttachedToWindow();
};

#endif
