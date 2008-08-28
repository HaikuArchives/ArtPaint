/* 

	Filename:	EraserTool.h
	Contents:	EraserTool-class declaration	
	Author:		Heikki Suhonen
	
*/



#ifndef ERASER_TOOL_H
#define ERASER_TOOL_H

#include "LineTool.h"
#include "CoordinateQueue.h"
#include "BitmapDrawer.h"



// This class is defines the first actual tool.
class EraserTool : public LineTool {
	CoordinateQueue	*coordinate_queue;

	bool		reading_coordinates;

	ImageView	*image_view;	
		
static	int32	CoordinateReader(void*);
		int32	read_coordinates();

						
public:
				EraserTool();
virtual			~EraserTool();
	
ToolScript*		UseTool(ImageView*,uint32,BPoint,BPoint);	
		int32	UseToolWithScript(ToolScript*,BBitmap*);

		BView*	makeConfigView();
const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();
};



class EraserToolConfigView : public DrawingToolConfigView {
		ControlSliderBox	*size_slider;
		BRadioButton		*mode_button_1;
		BRadioButton		*mode_button_2;

		
		
public:
		EraserToolConfigView(BRect rect,DrawingTool *t);
	
void	AttachedToWindow();	
};


#endif