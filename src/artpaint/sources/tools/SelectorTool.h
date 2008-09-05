/* 

	Filename:	SelectorTool.h
	Contents:	SelectorTool-class declaration.	
	Author:		Heikki Suhonen
	
*/



#ifndef SELECTOR_TOOL_H
#define SELECTOR_TOOL_H

#include <RadioButton.h>

#include "DrawingTool.h"
#include "ToolEventAdapter.h"

class SelectorTool : public DrawingTool, public ToolEventAdapter {
			// These functions handle the magic wand thing.
			// They have been copied from FillTool and altered
			// a little.
void		CheckLowerSpans(BPoint,BitmapDrawer*,PointStack&,int32,int32,uint32,int32,BBitmap*);
void		CheckUpperSpans(BPoint,BitmapDrawer*,PointStack&,int32,int32,uint32,int32,BBitmap*);
void		CheckBothSpans(BPoint,BitmapDrawer*,PointStack&,int32,int32,uint32,int32,BBitmap*);
void		FillSpan(BPoint,BitmapDrawer*,int32, int32, uint32,int32,BBitmap*);
BBitmap*	MakeFloodBinaryMap(BitmapDrawer*,int32,int32,int32,int32,uint32, BPoint);

public: 
		SelectorTool();
virtual	~SelectorTool();
		
ToolScript*		UseTool(ImageView*,uint32,BPoint,BPoint);		
BView*			makeConfigView();
const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();
};



class SelectorToolConfigView : public DrawingToolConfigView {
		BRadioButton		*mode_button_1;
		BRadioButton		*mode_button_2;
		BRadioButton		*shape_button_1;
		BRadioButton		*shape_button_2;
		BRadioButton		*shape_button_3;
		BRadioButton		*shape_button_4;
		ControlSliderBox	*tolerance_slider;
		
public:
		SelectorToolConfigView(BRect rect,DrawingTool *t);
		
void	AttachedToWindow();
};
#endif
