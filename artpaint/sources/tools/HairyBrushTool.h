/*

	Filename:	HairyBrushTool.h
	Contents:	HairyBrushTool-class declaration
	Author:		Heikki Suhonen

*/



#ifndef HAIRY_BRUSH_TOOL_H
#define HAIRY_BRUSH_TOOL_H

#include "DrawingTool.h"
#include "CoordinateQueue.h"
#include "BitmapDrawer.h"


#define COLOR_VARIANCE_CHANGED	'Cvar'
#define	COLOR_AMOUNT_CHANGED	'Camt'

// This class is defines the first actual tool.
class HairyBrushTool : public DrawingTool {
//	CoordinateQueue	*coordinate_queue;
//
//	bool		reading_coordinates;

	ImageView	*image_view;

//static	int32	CoordinateReader(void*);
//		int32	read_coordinates();


public:
				HairyBrushTool();
virtual			~HairyBrushTool();

ToolScript*		UseTool(ImageView*,uint32,BPoint,BPoint);
		int32	UseToolWithScript(ToolScript*,BBitmap*);

virtual	BView*	makeConfigView();
const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();
};



class HairyBrushToolConfigView : public DrawingToolConfigView {
		ControlSliderBox	*hair_amount_slider;
		ControlSliderBox	*width_slider;
		ControlSlider		*color_variance_slider;
		ControlSlider		*color_amount_slider;

public:
		HairyBrushToolConfigView(BRect rect,DrawingTool *t);

void	AttachedToWindow();
void	MessageReceived(BMessage*);
};



inline float random_round(float number,float r)
{
	float dec = number - floor(number);
	if (dec < r)
		return floor(number);
	else
		return ceil(number);
}


#endif
