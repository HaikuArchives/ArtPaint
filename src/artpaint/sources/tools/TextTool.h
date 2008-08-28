/* 

	Filename:	TextTool.h
	Contents:	TextTool-class declaration.	
	Author:		Heikki Suhonen
	
*/


#ifndef TEXT_TOOL_H
#define TEXT_TOOL_H

// Text tool is a fake tool that is just used to start the text manipulator


#include "DrawingTool.h"

class Selection;

class TextTool : public DrawingTool {
public:
		TextTool();
virtual	~TextTool();
		
ToolScript*	UseTool(ImageView*,uint32,BPoint,BPoint);
int32		UseToolWithScript(ToolScript*,BBitmap*);
const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();
};
#endif