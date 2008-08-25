/* 

	Filename:	AirBrushTool.h
	Contents:	AirBrushTool-class declaration.	
	Author:		Heikki Suhonen
	
*/


#ifndef AIR_BRUSH_TOOL_H
#define AIR_BRUSH_TOOL_H

#include "DrawingTool.h"


class AirBrushTool : public DrawingTool {
		int32	*sqrt_table;
		
public:
		AirBrushTool();
virtual	~AirBrushTool();
		
ToolScript*	UseTool(ImageView*,uint32,BPoint,BPoint);
int32		UseToolWithScript(ToolScript*,BBitmap*);

BView*	makeConfigView();
const	void*	ReturnToolCursor();
const	char*	ReturnHelpString(bool);
};




class AirBrushToolConfigView : public DrawingToolConfigView {
		ControlSliderBox	*size_slider;
		ControlSliderBox	*flow_slider;		
		BRadioButton		*mode_button_1;
		BRadioButton		*mode_button_2;
public:
		AirBrushToolConfigView(BRect rect, DrawingTool *t);

void	AttachedToWindow();
};

#endif