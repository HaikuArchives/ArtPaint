/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef BRIGHTNESS_H
#define BRIGHTNESS_H

#include "WindowGUIManipulator.h"
#include "Controls.h"

class	SaturationManipulatorSettings : public ManipulatorSettings {
public:
		SaturationManipulatorSettings()
			: ManipulatorSettings() {
			saturation = 100;
		}

		SaturationManipulatorSettings(const SaturationManipulatorSettings& s)
			: ManipulatorSettings() {
			saturation = s.saturation;
		}


		SaturationManipulatorSettings& operator=(const SaturationManipulatorSettings& s) {
			saturation = s.saturation;
			return *this;
		}


		bool operator==(SaturationManipulatorSettings s) {
			return (saturation == s.saturation);
		}

		bool operator!=(SaturationManipulatorSettings s) {
			return !(*this==s);
		}

int32	saturation;
};


class SaturationManipulatorView;

class SaturationManipulator : public WindowGUIManipulator {
			BBitmap	*preview_bitmap;
			BBitmap	*copy_of_the_preview_bitmap;

			BBitmap	*luminance_image;

			int32	lowest_available_quality;
			int32	highest_available_quality;
			int32	last_calculated_resolution;

			SaturationManipulatorSettings	settings;
			SaturationManipulatorSettings	previous_settings;

			SaturationManipulatorView		*config_view;


			// The next attributes will be used by the thread_function.
			int32	number_of_threads;
			int32	current_resolution;

			SaturationManipulatorSettings	current_settings;

			Selection	*current_selection;

			BBitmap		*source_bitmap;
			BBitmap		*target_bitmap;
			BStatusBar	*progress_bar;

			void	start_threads();

	static	int32	thread_entry(void*);
			int32	thread_function(int32);


			void	CalculateLuminanceImage(BBitmap*);

public:
			SaturationManipulator(BBitmap*);
			~SaturationManipulator();

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



#define	SATURATION_ADJUSTED				'Thad'
#define	SATURATION_ADJUSTING_FINISHED	'Thaf'

class SaturationManipulatorView : public WindowGUIManipulatorView {
		BMessenger						target;
		SaturationManipulator			*manipulator;
		SaturationManipulatorSettings	settings;

		ControlSlider					*saturation_slider;


		bool							started_adjusting;
public:
		SaturationManipulatorView(SaturationManipulator*,BMessenger*);

void	AllAttached();
void	AttachedToWindow();
void	MessageReceived(BMessage*);
void	ChangeSettings(ManipulatorSettings*);
};

#endif



