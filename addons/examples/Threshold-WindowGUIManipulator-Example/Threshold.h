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


class BSlider;


class	ThresholdManipulatorSettings : public ManipulatorSettings {
public:
		ThresholdManipulatorSettings()
			: ManipulatorSettings() {
			threshold = 0;
		}

		ThresholdManipulatorSettings(const ThresholdManipulatorSettings& s)
			: ManipulatorSettings() {
			threshold = s.threshold;
		}


		ThresholdManipulatorSettings& operator=(const ThresholdManipulatorSettings& s) {
			threshold = s.threshold;
			return *this;
		}


		bool operator==(ThresholdManipulatorSettings s) {
			return (threshold == s.threshold);
		}

		bool operator!=(ThresholdManipulatorSettings s) {
			return !(*this==s);
		}

int32	threshold;
};


class ThresholdManipulatorView;

class ThresholdManipulator : public WindowGUIManipulator {
			BBitmap	*preview_bitmap;
			BBitmap	*copy_of_the_preview_bitmap;

			int32	lowest_available_quality;
			int32	highest_available_quality;
			int32	last_calculated_resolution;

			ThresholdManipulatorSettings	settings;
			ThresholdManipulatorSettings	previous_settings;

			ThresholdManipulatorView		*config_view;


			// The next attributes will be used by the thread_function.
			int32	number_of_threads;
			int32	current_resolution;

			ThresholdManipulatorSettings	current_settings;

			Selection	*current_selection;

			BBitmap		*source_bitmap;
			BBitmap		*target_bitmap;
			BStatusBar	*progress_bar;

			void	start_threads();

	static	int32	thread_entry(void*);
			int32	thread_function(int32);


public:
			ThresholdManipulator(BBitmap*);
			~ThresholdManipulator();

void		MouseDown(BPoint,uint32 buttons,BView*,bool);
int32		PreviewBitmap(Selection*,bool full_quality=FALSE,BRegion* =NULL);
BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap*,Selection*,BStatusBar*);
void		Reset(Selection*);
void		SetPreviewBitmap(BBitmap*);
const char*	ReturnHelpString();
const char*	ReturnName();

ManipulatorSettings*	ReturnSettings();

BView*		MakeConfigurationView(const BMessenger& target);

void		ChangeSettings(ManipulatorSettings*);
};



#define	THRESHOLD_ADJUSTED				'Thad'
#define	THRESHOLD_ADJUSTING_FINISHED	'Thaf'

class ThresholdManipulatorView : public WindowGUIManipulatorView {
		BMessenger						target;
		ThresholdManipulator			*manipulator;
		ThresholdManipulatorSettings	settings;

		BSlider							*threshold_slider;


		bool							started_adjusting;
public:
		ThresholdManipulatorView(ThresholdManipulator*, const BMessenger&);

void	AttachedToWindow();
void	MessageReceived(BMessage*);
void	ChangeSettings(ManipulatorSettings*);
};

#endif



