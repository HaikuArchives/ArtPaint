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

#include "ManipulatorSettings.h"
#include "WindowGUIManipulator.h"


class BSlider;


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
		return !(*this == s);
	}

	int32	saturation;
};


class SaturationManipulatorView;

class SaturationManipulator : public WindowGUIManipulator {
			BBitmap*	preview_bitmap;
			BBitmap*	copy_of_the_preview_bitmap;

			BBitmap*	luminance_image;

			int32		lowest_available_quality;
			int32		highest_available_quality;
			int32		last_calculated_resolution;

			SaturationManipulatorSettings	settings;
			SaturationManipulatorSettings	previous_settings;

			SaturationManipulatorView*		config_view;

			// The next attributes will be used by the thread_function.
			int32		number_of_threads;
			int32		current_resolution;

			SaturationManipulatorSettings	current_settings;

			Selection*	selection;

			BBitmap*	source_bitmap;
			BBitmap*	target_bitmap;
			BStatusBar*	progress_bar;

			void		start_threads();

	static	int32		thread_entry(void*);
			int32		thread_function(int32);


			void		CalculateLuminanceImage(BBitmap*);

public:
						SaturationManipulator(BBitmap*);
						~SaturationManipulator();

			int32		PreviewBitmap(bool full_quality = FALSE, BRegion* =NULL);
			BBitmap*	ManipulateBitmap(ManipulatorSettings*, BBitmap*, BStatusBar*);
			void		Reset();
			void		SetPreviewBitmap(BBitmap*);
			const char*	ReturnHelpString();
			const char*	ReturnName();

			ManipulatorSettings*	ReturnSettings();

			BView*		MakeConfigurationView(const BMessenger& target);

			void		ChangeSettings(ManipulatorSettings*);
			void		SetSelection(Selection* new_selection)
							{ selection = new_selection; };
};



#define	SATURATION_ADJUSTED				'Thad'
#define	SATURATION_ADJUSTING_FINISHED	'Thaf'

class SaturationManipulatorView : public WindowGUIManipulatorView {
		BMessenger		target;
		SaturationManipulator*			manipulator;
		SaturationManipulatorSettings	settings;

		BSlider*		saturation_slider;

		bool			started_adjusting;

public:
						SaturationManipulatorView(SaturationManipulator*,const BMessenger& target);

		void			AllAttached();
		void			AttachedToWindow();
		void			MessageReceived(BMessage*);
		void			ChangeSettings(ManipulatorSettings*);
};

#endif
