/* 

	Filename:	ColorSelectorTool.h
	Contents:	ColorSelectorTool-class declaration.	
	Author:		Heikki Suhonen
	
*/


#ifndef COLOR_SELECTOR_TOOL_H
#define COLOR_SELECTOR_TOOL_H

#include <StringView.h>

#include "DrawingTool.h"

class ColorSelectorTool : public DrawingTool {

public:
		ColorSelectorTool();
virtual	~ColorSelectorTool();
		
ToolScript*	UseTool(ImageView*,uint32,BPoint,BPoint);

BView*	makeConfigView();
const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();
};

class ColorSelectorView : public BView {
		uint32		selected_color;
		BStringView	*red_view;
		BStringView *green_view;
		BStringView	*blue_view;	
		BStringView	*alpha_view;
		
public:
		ColorSelectorView(BRect);

void	Draw(BRect);
		
void	ChangeValue(uint32);		
};

class ColorSelectorWindow : public BWindow {
		BRect 				screen_bounds;
		ColorSelectorView	*cs_view;		
public:
		ColorSelectorWindow(BPoint);
		
void	ChangeValue(uint32);		
void	Move(BPoint);
};


class ColorSelectorToolConfigView : public DrawingToolConfigView {
		ControlSliderBox	*size_slider;
public:
		ColorSelectorToolConfigView(BRect rect,DrawingTool *t);
		
void	AttachedToWindow();		
};
#endif