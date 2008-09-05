/* 

	Filename:	SimpleTool.h
	Contents:	SimpleTool-class declaration	
	Author:		Heikki Suhonen
	
*/



#ifndef SIMPLE_TOOL_H
#define SIMPLE_TOOL_H

#include "LineTool.h"
#include "CoordinateQueue.h"
#include "BitmapDrawer.h"



// This class is defines the first actual tool.
class SimpleTool : public LineTool {
	CoordinateQueue	*coordinate_queue;

	bool		reading_coordinates;

	ImageView	*image_view;	
		
static	int32	CoordinateReader(void*);
		int32	read_coordinates();

						
public:
			SimpleTool();
virtual		~SimpleTool();
	
ToolScript*	UseTool(ImageView*,uint32,BPoint,BPoint);	
int32		UseToolWithScript(ToolScript*,BBitmap*);

		BView*	makeConfigView();
const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();
};



class SimpleToolConfigView : public DrawingToolConfigView {
		ControlSliderBox	*size_slider;
		
public:
		SimpleToolConfigView(BRect rect,DrawingTool *t);
	
void	AttachedToWindow();	
};


#endif