/* 

	Filename:	Sharpness.h
	Contents:	Declarations for sharpness add-on.	
	Author:		Heikki Suhonen
	
*/


#ifndef SHARPNESS_H
#define SHARPNESS_H

#include "WindowGUIManipulator.h"
#include "Controls.h"


#define	BLUR_AMOUNT	3.0

class ImageProcessingLibrary;

class	SharpnessManipulatorSettings : public ManipulatorSettings {
public:
		SharpnessManipulatorSettings()
			: ManipulatorSettings() {
			sharpness = 100;
			blur_size = 10.0;
		}

		SharpnessManipulatorSettings(const SharpnessManipulatorSettings& s)
			: ManipulatorSettings() {
			sharpness = s.sharpness;
			blur_size = s.blur_size;
		}


		SharpnessManipulatorSettings& operator=(const SharpnessManipulatorSettings& s) {
			sharpness = s.sharpness;
			blur_size = s.blur_size;
			return *this;
		}


		bool operator==(SharpnessManipulatorSettings s) {
			return (sharpness == s.sharpness) && (blur_size == s.blur_size);
		}
		
		bool operator!=(SharpnessManipulatorSettings s) {
			return !(*this==s);
		}

int32	sharpness;
float	blur_size;
};


class SharpnessManipulatorView;

class SharpnessManipulator : public WindowGUIManipulator {
			BBitmap	*preview_bitmap;
			BBitmap	*copy_of_the_preview_bitmap;

			BBitmap	*blurred_image;
						
			int32	lowest_available_quality;
			int32	highest_available_quality;
			int32	last_calculated_resolution;

			SharpnessManipulatorSettings	settings;
			SharpnessManipulatorSettings	previous_settings;
			
			SharpnessManipulatorView		*config_view;
			
		
			// The next attributes will be used by the thread_function.
			int32	current_resolution;

			SharpnessManipulatorSettings	current_settings;
			
			Selection	*current_selection;				
			
			BBitmap		*source_bitmap;
			BBitmap		*target_bitmap;
			BStatusBar	*progress_bar;
			
			void	start_threads();	

	static	int32	thread_entry(void*);
			int32	thread_function(int32);

			int32	processor_count;

			void	CalculateLuminanceImage(BBitmap*);

public:
			SharpnessManipulator(BBitmap*);
			~SharpnessManipulator();
			
void		MouseDown(BPoint,uint32 buttons,BView*,bool);
int32		PreviewBitmap(Selection*,bool full_quality=FALSE,BRegion* =NULL);
BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap*,Selection*,BStatusBar*);	
void		Reset(Selection*);
void		SetPreviewBitmap(BBitmap*);
char*		ReturnHelpString();
char*		ReturnName();

ManipulatorSettings*	ReturnSettings();

BView*		MakeConfigurationView(BMessenger*);

void		ChangeSettings(ManipulatorSettings*);		

ImageProcessingLibrary	*ipLibrary;
};



#define	SHARPNESS_ADJUSTED				'Shad'
#define	SHARPNESS_ADJUSTING_FINISHED	'Shaf'

#define	BLUR_ADJUSTING_FINISHED			'Blaf'


class SharpnessManipulatorView : public WindowGUIManipulatorView {
		BMessenger						target;
		SharpnessManipulator			*manipulator;
		SharpnessManipulatorSettings	settings;	
		
		ControlSlider					*sharpness_slider;
		ControlSlider					*blur_size_slider;

		bool							started_adjusting;
public:
		SharpnessManipulatorView(SharpnessManipulator*,BMessenger*);

void	AllAttached();
void	AttachedToWindow();
void	MessageReceived(BMessage*);
void	ChangeSettings(ManipulatorSettings*);
};

#endif



