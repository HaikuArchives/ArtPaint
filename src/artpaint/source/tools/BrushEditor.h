/* 

	Filename:	BrushEditor.h
	Contents:	BrushEditor-class declarations	
	Author:		Heikki Suhonen
	
*/



/*
	The class BrushEditor allows user to edit brushes. It will
	display a preview of the brush and necessary controls for modifying it.
	A brush-editor has one brush being edited at a time. It accepts and 
	generates brush drags and drops. The brush can be queried from the editor
	and it can be copied to another object. BrushEditor tries to fit itself
	inside the parameter rectangle, but if it does not fit it will be as small
	as possible.
	
*/

#ifndef BRUSH_EDITOR_H
#define BRUSH_EDITOR_H

#include <Box.h>

#define	BRUSH_WIDTH_CHANGED		'BWCh'
#define BRUSH_HEIGHT_CHANGED	'BHCh'	
#define BRUSH_EDGE_CHANGED		'BeCh'
#define BRUSH_SHAPE_CHANGED		'Bshc'

#define	BRUSH_ALTERED			'Bral'
#define	BRUSH_STORING_REQUESTED	'Bsrq'

#include <RadioButton.h>
#include <Button.h>

#include "Brush.h"

class BrushView;
class ControlSliderBox;

class BrushEditor : public BBox {

Brush				*the_brush;
BrushView			*brush_view;
brush_info			b_info;

ControlSliderBox	*width_slider;
ControlSliderBox	*height_slider;
ControlSliderBox	*fade_slider;
BRadioButton		*rectangle_button;
BRadioButton		*ellipse_button;
BButton				*store_button;

static	BrushEditor	*the_editor;

		BrushEditor(BRect,Brush*);
		~BrushEditor();


public:
void	AttachedToWindow();		
void	MessageReceived(BMessage *message);


static	BView*	CreateBrushEditor(BRect,Brush*);
static	void	BrushModified();
};



class BrushView : public BView {

Brush	*the_brush;
BBitmap	*brush_preview;
bool		draw_controls;

public:
		BrushView(BRect,Brush*);
		~BrushView();

void	Draw(BRect);
void	MessageReceived(BMessage*);
void	MouseDown(BPoint);
void	MouseMoved(BPoint,uint32,const BMessage*);

void	BrushModified();		
void	ChangeBrush(Brush*);
};
#endif