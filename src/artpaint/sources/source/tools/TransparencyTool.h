/* 

	Filename:	TransparencyTool.h
	Contents:	TransparencyTool-class declaration.	
	Author:		Heikki Suhonen
	
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