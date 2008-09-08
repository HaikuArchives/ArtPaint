/*

	Filename:	FillTool.h
	Contents:	FillTool-class declaration
	Author:		Heikki Suhonen

*/



#ifndef FILL_TOOL_H
#define FILL_TOOL_H

#include <Control.h>
#include <CheckBox.h>

#include "DrawingTool.h"

class Selection;

class FillTool : public DrawingTool {
BBitmap		*filled_bitmap;
BBitmap		*binary_fill_map;

status_t	NormalFill(ImageView*,uint32,BPoint,Selection* =NULL);

void		CheckLowerSpans(BPoint,BitmapDrawer*,PointStack&,int32,int32,uint32,uint32,int32,Selection* =NULL);
void		CheckUpperSpans(BPoint,BitmapDrawer*,PointStack&,int32,int32,uint32,uint32,int32,Selection* =NULL);
void		CheckBothSpans(BPoint,BitmapDrawer*,PointStack&,int32,int32,uint32,uint32,int32,Selection* =NULL);
void		FillSpan(BPoint,BitmapDrawer*,int32,int32,uint32,uint32,int32,Selection* =NULL);


BPoint		GradientFill(ImageView*,uint32,BPoint,BPoint,Selection* =NULL);

BBitmap*	MakeBinaryMap(BitmapDrawer*,int32,int32,int32,int32,uint32,Selection* =NULL);
BBitmap*	MakeFloodBinaryMap(BitmapDrawer*,int32,int32,int32,int32,uint32,BPoint,Selection* =NULL);
void		FillGradient(BitmapDrawer*,BBitmap*,int32,int32,int32,int32,int32,int32,uint32,uint32);
void		FillGradientPreview(BitmapDrawer*,BBitmap*,int32,int32,int32,int32,int32,int32,uint32,uint32);
BRect		calcBinaryMapBounds(BBitmap *boolean_map);


uint32		gradient_color1;
uint32		gradient_color2;
public:
		FillTool();
virtual	~FillTool();

ToolScript*	UseTool(ImageView*,uint32,BPoint,BPoint);
int32		UseToolWithScript(ToolScript*,BBitmap*);

BView*	makeConfigView();


status_t	readSettings(BFile&,bool);
status_t	writeSettings(BFile&);

void			SetGradient(uint32 c1,uint32 c2) { gradient_color1 = c1; gradient_color2 = c2; }
const	void*	ReturnToolCursor();
const	char*	ReturnHelpString(bool);
};



class ColorView;
class GradientView;

#define	GRADIENT_ADJUSTED	'gRad'

class FillToolConfigView : public DrawingToolConfigView {
		BCheckBox			*flood_checkbox;
		BCheckBox			*gradient_checkbox;
		BCheckBox			*preview_checkbox;
//		ColorView			*color_view;
		GradientView		*gradient_view;
		ControlSliderBox	*tolerance_slider;

public:
		FillToolConfigView(BRect rect,DrawingTool *t,uint32 c1,uint32 c2);

void		AttachedToWindow();
void		MessageReceived(BMessage*);
};


class GradientView : public BControl {
		rgb_color	color1;
		rgb_color	color2;

		BBitmap		*gradient_map;

void		CalculateGradient();
public:
		GradientView(BRect,uint32,uint32);
		~GradientView();

void		AttachedToWindow();
void		Draw(BRect);
void		MessageReceived(BMessage*);
void		MouseDown(BPoint);
};

#endif
