/* 

	Filename:	ColorBalance.h
	Contents:	ColorBalance-manipulator declaration.	
	Author:		Heikki Suhonen
	
*/


#ifndef COLOR_BALANCE_H
#define COLOR_BALANCE_H

#include "WindowGUIManipulator.h"
#include "Controls.h"



class ColorBalanceManipulatorSettings : public ManipulatorSettings {
public:
		ColorBalanceManipulatorSettings()
			: ManipulatorSettings() {
			red_difference = 0;
			blue_difference = 0;
			green_difference = 0;	
		}

		ColorBalanceManipulatorSettings(ColorBalanceManipulatorSettings *s)
			: ManipulatorSettings() {
			red_difference = s->red_difference;
			blue_difference = s->blue_difference;
			green_difference = s->green_difference;
		}

		ColorBalanceManipulatorSettings& operator=(const ColorBalanceManipulatorSettings& s) {
			red_difference = s.red_difference;
			blue_difference = s.blue_difference;
			green_difference = s.green_difference;			
			return *this;
		}	

		bool operator==(ColorBalanceManipulatorSettings s) {
			return ((red_difference == s.red_difference) &&
					(blue_difference == s.blue_difference) &&
					(green_difference == s.green_difference));
		}


		int32	red_difference;
		int32	blue_difference;
		int32	green_difference;		
};


class ColorBalanceManipulatorView;


class ColorBalanceManipulator : public WindowGUIManipulator {
		BBitmap	*preview_bitmap;
		BBitmap	*copy_of_the_preview_bitmap;
		

		ColorBalanceManipulatorSettings	settings;
		ColorBalanceManipulatorSettings	previous_settings;			

		ColorBalanceManipulatorView		*config_view;
		
		int32	lowest_allowed_quality;
		int32	last_used_quality;
		
		

public:
			ColorBalanceManipulator(BBitmap*);
			~ColorBalanceManipulator();
			
int32		PreviewBitmap(Selection*,bool full_quality=FALSE,BRegion* =NULL);
BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap*,Selection*,BStatusBar*);	
void		Reset(Selection*);
void		SetPreviewBitmap(BBitmap*);
char*		ReturnHelpString() { return "Use the sliders to adjust the color balance."; }
char*		ReturnName() { return "Color Balance"; }

ManipulatorSettings*	ReturnSettings();
BView*	MakeConfigurationView(BMessenger*);

void	ChangeSettings(ManipulatorSettings*);
};


class ColorBalanceManipulatorView : public WindowGUIManipulatorView {
		ColorBalanceManipulator	*manipulator;
		BMessenger				*target;
		
		ControlSlider			*red_slider;
		ControlSlider			*blue_slider;
		ControlSlider			*green_slider;

		ColorBalanceManipulatorSettings	settings;


		bool					preview_started;
public:
		ColorBalanceManipulatorView(BRect,ColorBalanceManipulator*,BMessenger*);


void	AttachedToWindow();
void	MessageReceived(BMessage*);

void	ChangeSettings(ColorBalanceManipulatorSettings);
};
#endif



