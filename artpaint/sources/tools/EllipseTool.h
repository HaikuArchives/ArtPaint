/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef ELLIPSE_TOOL_H
#define ELLIPSE_TOOL_H

#include "DrawingTool.h"

class EllipseTool : public DrawingTool {

public:
		EllipseTool();
virtual	~EllipseTool();

ToolScript*	UseTool(ImageView*,uint32,BPoint,BPoint);
int32		UseToolWithScript(ToolScript*,BBitmap*);

BView*	makeConfigView();
const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();
};



class EllipseToolConfigView : public DrawingToolConfigView {
		BCheckBox			*fill_checkbox;
		BRadioButton		*radio_button_1;
		BRadioButton		*radio_button_2;


public:
		EllipseToolConfigView(BRect rect,DrawingTool *t);

void	AttachedToWindow();
};
#endif
