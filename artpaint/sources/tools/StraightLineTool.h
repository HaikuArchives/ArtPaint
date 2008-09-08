/*

	Filename:	StraightLineTool.h
	Contents:	StraightLineTool-class declaration.
	Author:		Heikki Suhonen

*/


#ifndef STRAIGHT_LINE_TOOL_H
#define STRAIGHT_LINE_TOOL_H

#include "DrawingTool.h"
#include "ToolEventAdapter.h"

class StraightLineTool : public DrawingTool, public ToolEventAdapter {

public:
		StraightLineTool();
virtual	~StraightLineTool();

ToolScript*	UseTool(ImageView*,uint32,BPoint,BPoint);
int32		UseToolWithScript(ToolScript*,BBitmap*);

BView*	makeConfigView();
const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();
};


class StraightLineToolConfigView : public DrawingToolConfigView {
		ControlSliderBox	*size_slider;
		BCheckBox			*anti_aliasing_checkbox;
		BCheckBox			*width_adjusting_checkbox;

public:
		StraightLineToolConfigView(BRect rect,DrawingTool *t);

void	AttachedToWindow();
};

#endif
