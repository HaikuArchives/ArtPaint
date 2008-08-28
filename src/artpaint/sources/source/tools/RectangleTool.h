/* 

	Filename:	RectangleTool.h
	Contents:	RectangleTool declarations	
	Author:		Heikki Suhonen
	
*/



#ifndef RECTANGLE_TOOL_H
#define RECTANGLE_TOOL_H

#include "DrawingTool.h"
#include "ToolEventAdapter.h"

class RectangleTool : public DrawingTool, public ToolEventAdapter {

public:
		RectangleTool();
virtual	~RectangleTool();

ToolScript*	UseTool(ImageView*,uint32,BPoint,BPoint);
int32		UseToolWithScript(ToolScript*,BBitmap*);

BView*	makeConfigView();
const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();
};


class RectangleToolConfigView : public DrawingToolConfigView {
		BCheckBox 			*fill_checkbox;
		BRadioButton		*radio_button_1;
		BRadioButton		*radio_button_2;
		BCheckBox			*rotation_checkbox;
		BCheckBox			*anti_alias_checkbox;

public:
		RectangleToolConfigView(BRect rect,DrawingTool *t);
		
void	AttachedToWindow();
};

#endif
