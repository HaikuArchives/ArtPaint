/*

	Filename:	ScaleManipulator.h
	Contents:	ScaleManipulator-class declaration
	Author:		Heikki Suhonen

*/


#ifndef SCALE_MANIPULATOR_H
#define	SCALE_MANIPULATOR_H

#include "WindowGUIManipulator.h"
#include "Controls.h"

#define	WIDTH_CHANGED		'Wich'
#define	HEIGHT_CHANGED		'Hech'

#define MULTIPLY_WIDTH		'mlWi'
#define MULTIPLY_HEIGHT		'mlHe'

#define RESTORE_WIDTH		'Rswd'
#define RESTORE_HEIGHT		'Rshg'

#define	PROPORTION_CHANGED	'Prpc'

class ScaleManipulatorView;

class ScaleManipulatorSettings : public ManipulatorSettings {
public:
		ScaleManipulatorSettings()
			: ManipulatorSettings() {
				width_coefficient = 1.0;
				height_coefficient = 1.0;
			 }

		ScaleManipulatorSettings(ScaleManipulatorSettings *s)
			: ManipulatorSettings() {
				width_coefficient = s->width_coefficient;
				height_coefficient = s->height_coefficient;
			}


float	height_coefficient;
float	width_coefficient;
};



class ScaleManipulator : public WindowGUIManipulator {
	BBitmap						*preview_bitmap;
	BBitmap						*copy_of_the_preview_bitmap;

	ScaleManipulatorView		*configuration_view;
	ScaleManipulatorSettings	*settings;



	float						original_width;
	float						original_height;

public:
			ScaleManipulator(BBitmap*);
			~ScaleManipulator();

BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap *original,Selection*,BStatusBar*);
int32		PreviewBitmap(Selection*,bool,BRegion* =NULL);

void		MouseDown(BPoint,uint32,BView*,bool);
void		SetValues(float,float);

BView*		MakeConfigurationView(BMessenger*);
void		Reset(Selection*);
void		SetPreviewBitmap(BBitmap*);

const	char*	ReturnHelpString();
const	char*	ReturnName();


ManipulatorSettings*	ReturnSettings();
};



class ScaleManipulatorView : public WindowGUIManipulatorView {
		ScaleManipulator	*manipulator;
		float				original_width;
		float				original_height;
		float				current_width;
		float				current_height;
		bool				maintain_proportions;

		NumberControl		*width_control;
		NumberControl		*height_control;

		BMessenger *target;

public:
		ScaleManipulatorView(BRect,ScaleManipulator*,BMessenger*);
		~ScaleManipulatorView();

void	AttachedToWindow();

void	MessageReceived(BMessage*);
void	SetValues(float,float);
bool	MaintainProportions() { return maintain_proportions; }
};
#endif
