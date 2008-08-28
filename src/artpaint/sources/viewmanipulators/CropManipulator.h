/* 

	Filename:	CropManipulator.h
	Contents:	CropManipulator-class declaration	
	Author:		Heikki Suhonen
	
*/


#ifndef CROP_MANIPULATOR_H
#define	CROP_MANIPULATOR_H

#include "WindowGUIManipulator.h"
#include "Controls.h"

#define	TOP_CHANGED		'Toch'
#define	BOTTOM_CHANGED	'Boch'
#define LEFT_CHANGED	'Lech'
#define RIGHT_CHANGED	'Rich'

class CropManipulatorView;
class CropManipulatorSettings;

class CropManipulator : public WindowGUIManipulator {
	BBitmap		*target_bitmap;
	float 		min_x,max_x;
	float		min_y,max_y;
	
	float		previous_left;
	float		previous_right;
	float		previous_top;
	float		previous_bottom;

	
	CropManipulatorSettings	*settings;
	CropManipulatorView		*config_view;

	bool		move_left;
	bool		move_right;
	bool		move_top;
	bool		move_bottom;
	bool 		move_all;
		
public:
			CropManipulator(BBitmap*);
			~CropManipulator();

void		MouseDown(BPoint,uint32,BView*,bool);

BRegion		Draw(BView*,float);

BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap *original,Selection*,BStatusBar*);	
void		SetValues(float,float,float,float);

int32		PreviewBitmap(Selection*,bool full_quality=FALSE,BRegion *updated_region=NULL);


BView*		MakeConfigurationView(BMessenger*);
void		Reset(Selection*) {};
void		SetPreviewBitmap(BBitmap*);

const	char*	ReturnHelpString();
const	char*	ReturnName();


ManipulatorSettings*	ReturnSettings();
};



class CropManipulatorSettings : public ManipulatorSettings {
public:
	CropManipulatorSettings()
		: ManipulatorSettings() {
		left = right = 0;
		top = bottom = 0;
	}

	CropManipulatorSettings(CropManipulatorSettings *s)
		: ManipulatorSettings() {
		left = s->left;
		right = s->right;
		top = s->top;
		bottom = s->bottom;
	}
		
	float	left;
	float	right;
	float	top;
	float	bottom;
};



class CropManipulatorView : public WindowGUIManipulatorView {
		float 		left;
		float 		right;
		float 		top;
		float 		bottom;


		NumberControl	*left_control;
		NumberControl	*right_control;
		NumberControl	*top_control;
		NumberControl	*bottom_control;
		
		BMessenger		*target;
		CropManipulator	*manipulator;
public:
		CropManipulatorView(BRect,CropManipulator*,BMessenger*);

void	AttachedToWindow();
void	MessageReceived(BMessage*);

void	SetValues(float,float,float,float);
};

#endif