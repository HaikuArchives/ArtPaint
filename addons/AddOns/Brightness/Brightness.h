/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef THRESHOLD_H
#define THRESHOLD_H

#include "WindowGUIManipulator.h"
#include "Controls.h"

class	BrightnessManipulatorSettings : public ManipulatorSettings {
public:
		BrightnessManipulatorSettings()
			: ManipulatorSettings() {
			brightness = 100;
		}

		BrightnessManipulatorSettings(const BrightnessManipulatorSettings& s)
			: ManipulatorSettings() {
			brightness = s.brightness;
		}


		BrightnessManipulatorSettings& operator=(const BrightnessManipulatorSettings& s) {
			brightness = s.brightness;
			return *this;
		}


		bool operator==(BrightnessManipulatorSettings s) {
			return (brightness == s.brightness);
		}

		bool operator!=(BrightnessManipulatorSettings s) {
			return !(*this==s);
		}

int32	brightness;
};


class BrightnessManipulatorView;

class BrightnessManipulator : public WindowGUIManipulator {
			BBitmap	*preview_bitmap;
			BBitmap	*copy_of_the_preview_bitmap;

			int32	lowest_available_quality;
			int32	highest_available_quality;
			int32	last_calculated_resolution;

			BrightnessManipulatorSettings	settings;
			BrightnessManipulatorSettings	previous_settings;

			BrightnessManipulatorView		*config_view;


			// The next attributes will be used by the thread_function.
			int32	number_of_threads;
			int32	current_resolution;

			BrightnessManipulatorSettings	current_settings;

			Selection	*current_selection;

			BBitmap		*source_bitmap;
			BBitmap		*target_bitmap;
			BStatusBar	*progress_bar;

			void	start_threads();

	static	int32	thread_entry(void*);
			int32	thread_function(int32);


public:
			BrightnessManipulator(BBitmap*);
			~BrightnessManipulator();

void		MouseDown(BPoint,uint32 buttons,BView*,bool);
int32		PreviewBitmap(Selection*,bool full_quality=FALSE,BRegion* =NULL);
BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap*,Selection*,BStatusBar*);
void		Reset(Selection*);
void		SetPreviewBitmap(BBitmap*);
char*		ReturnHelpString();
char*		ReturnName();

ManipulatorSettings*	ReturnSettings();

BView*		MakeConfigurationView(const BMessenger& target);

void		ChangeSettings(ManipulatorSettings*);
};



#define	BRIGHTNESS_ADJUSTED				'Thad'
#define	BRIGHTNESS_ADJUSTING_FINISHED	'Thaf'

class BrightnessManipulatorView : public WindowGUIManipulatorView {
		BMessenger						target;
		BrightnessManipulator			*manipulator;
		BrightnessManipulatorSettings	settings;

		ControlSlider					*brightness_slider;


		bool							started_adjusting;
public:
		BrightnessManipulatorView(BrightnessManipulator*,const BMessenger& target);
		~BrightnessManipulatorView();

void	AllAttached();
void	AttachedToWindow();
void	MessageReceived(BMessage*);
void	ChangeSettings(ManipulatorSettings*);
};

#endif



