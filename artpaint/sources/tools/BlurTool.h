/* 

	Filename:	BlurTool.h
	Contents:	BlurTool-class declaration.	
	Author:		Heikki Suhonen
	
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