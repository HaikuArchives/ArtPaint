/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef CONTRAST_H
#define CONTRAST_H

#include "ManipulatorSettings.h"
#include "WindowGUIManipulator.h"


class BSlider;


class	ContrastManipulatorSettings : public ManipulatorSettings {
public:
		ContrastManipulatorSettings()
			: ManipulatorSettings() {
			contrast = 100;
		}

		ContrastManipulatorSettings(const ContrastManipulatorSettings& s)
			: ManipulatorSettings() {
			contrast = s.contrast;
		}


		ContrastManipulatorSettings& operator=(const ContrastManipulatorSettings& s) {
			contrast = s.contrast;
			return *this;
		}


		bool operator==(ContrastManipulatorSettings s) {
			return (contrast == s.contrast);
		}

		bool operator!=(ContrastManipulatorSettings s) {
			return !(*this==s);
		}

int32	contrast;
};


class ContrastManipulatorView;

class ContrastManipulator : public WindowGUIManipulator {
			BBitmap	*preview_bitmap;
			BBitmap	*copy_of_the_preview_bitmap;

			int32	lowest_available_quality;
			int32	highest_available_quality;
			int32	last_calculated_resolution;

			ContrastManipulatorSettings	settings;
			ContrastManipulatorSettings	previous_settings;

			ContrastManipulatorView		*config_view;


			// The next attributes will be used by the thread_function.
			int32	number_of_threads;
			int32	current_resolution;

			uint8	current_average_luminance;

			ContrastManipulatorSettings	current_settings;

			Selection	*selection;

			BBitmap		*source_bitmap;
			BBitmap		*target_bitmap;
			BStatusBar	*progress_bar;

			void	start_threads();

	static	int32	thread_entry(void*);
			int32	thread_function(int32);


			uint8	CalculateAverageLuminance(BBitmap*);

public:
			ContrastManipulator(BBitmap*);
			~ContrastManipulator();

void		MouseDown(BPoint,uint32 buttons,BView*,bool);
int32		PreviewBitmap(bool full_quality = FALSE, BRegion* =NULL);
BBitmap*	ManipulateBitmap(ManipulatorSettings*, BBitmap*, BStatusBar*);
void		Reset();
void		SetPreviewBitmap(BBitmap*);
const char*	ReturnHelpString();
const char*	ReturnName();

ManipulatorSettings*	ReturnSettings();

BView*		MakeConfigurationView(const BMessenger&);

void		ChangeSettings(ManipulatorSettings*);
void		SetSelection(Selection* new_selection)
				{ selection = new_selection; };
};



#define	CONTRAST_ADJUSTED				'Thad'
#define	CONTRAST_ADJUSTING_FINISHED	'Thaf'

class ContrastManipulatorView : public WindowGUIManipulatorView {
		BMessenger						target;
		ContrastManipulator			*manipulator;
		ContrastManipulatorSettings	settings;

		BSlider						*contrast_slider;


		bool							started_adjusting;
public:
		ContrastManipulatorView(ContrastManipulator*,const BMessenger&);
		~ContrastManipulatorView();

void	AllAttached();
void	AttachedToWindow();
void	MessageReceived(BMessage*);
void	ChangeSettings(ManipulatorSettings*);
};

#endif



