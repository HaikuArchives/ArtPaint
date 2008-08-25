/* 

	Filename:	Brightness.h
	Contents:	Declarations for brightness add-on.	
	Author:		Heikki Suhonen
	
*/


#ifndef TEXTURIZER_H
#define TEXTURIZER_H

#include "WindowGUIManipulator.h"
#include "Controls.h"

class	TexturizerManipulatorSettings : public ManipulatorSettings {
public:
		TexturizerManipulatorSettings()
			: ManipulatorSettings() {
		}

		TexturizerManipulatorSettings(const TexturizerManipulatorSettings& s)
			: ManipulatorSettings() {
		}


		TexturizerManipulatorSettings& operator=(const TexturizerManipulatorSettings& s) {
			return *this;
		}


		bool operator==(TexturizerManipulatorSettings s) {
			return TRUE;
		}
		
		bool operator!=(TexturizerManipulatorSettings s) {
			return !(*this==s);
		}

};


class TexturizerManipulatorView;

class TexturizerManipulator : public WindowGUIManipulator {
			BBitmap	*preview_bitmap;
			BBitmap	*copy_of_the_preview_bitmap;
						
			TexturizerManipulatorSettings	settings;
			TexturizerManipulatorSettings	previous_settings;
			
			TexturizerManipulatorView		*config_view;
			

			BBitmap		*noise_map;
				
			BBitmap*	MakeNoiseMap(BRect);
			
			
			float		LinearInterpolation(float a,float b,float x) { return a*(1-x) + b*x; }
			float		CosineInterpolation(float a,float b,float x);	

			
			// Here x's and y's are between 0 and 1
			float		Noise(int32 x, int32 y);
			float		SmoothNoise(float x, float y);
			float		InterpolatedNoise(float x, float y);
			float		PerlinNoise_2D(float x, float y);
			
			
			

		
			// The next attributes will be used by the thread_function.
			int32	number_of_threads;
			int32	current_resolution;

			TexturizerManipulatorSettings	current_settings;
			
			Selection	*current_selection;				
			
			BBitmap		*source_bitmap;
			BBitmap		*target_bitmap;
			BStatusBar	*progress_bar;
			
			void	start_threads();	

	static	int32	thread_entry(void*);
			int32	thread_function(int32);


public:
			TexturizerManipulator(BBitmap*);
			~TexturizerManipulator();
			
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
};

class TexturizerManipulatorView : public WindowGUIManipulatorView {
		BMessenger						target;
		TexturizerManipulator			*manipulator;
		TexturizerManipulatorSettings	settings;	
		
//		ControlSlider					*brightness_slider;


		bool							started_adjusting;
public:
		TexturizerManipulatorView(TexturizerManipulator*,BMessenger*);

void	AllAttached();
void	AttachedToWindow();
void	MessageReceived(BMessage*);
void	ChangeSettings(ManipulatorSettings*);
};

#endif



