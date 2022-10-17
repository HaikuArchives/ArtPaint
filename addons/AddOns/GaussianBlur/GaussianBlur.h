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

#include "ManipulatorSettings.h"
#include "WindowGUIManipulator.h"


class BSlider;
class ImageProcessingLibrary;


class	GaussianBlurManipulatorSettings : public ManipulatorSettings {
public:
		GaussianBlurManipulatorSettings()
			: ManipulatorSettings() {
			blur = 1.0;
		}

		GaussianBlurManipulatorSettings(const GaussianBlurManipulatorSettings& s)
			: ManipulatorSettings() {
			blur = s.blur;
		}


		GaussianBlurManipulatorSettings& operator=(const GaussianBlurManipulatorSettings& s) {
			blur = s.blur;
			return *this;
		}


		bool operator==(GaussianBlurManipulatorSettings s) {
			return (blur == s.blur);
		}

		bool operator!=(GaussianBlurManipulatorSettings s) {
			return !(*this==s);
		}

float	blur;
};


class GaussianBlurManipulatorView;

class GaussianBlurManipulator : public WindowGUIManipulator {
			BBitmap	*preview_bitmap;
			BBitmap	*copy_of_the_preview_bitmap;
			BBitmap *selection_bitmap;

			int32	lowest_available_quality;
			int32	highest_available_quality;
			int32	last_calculated_resolution;

			GaussianBlurManipulatorSettings	settings;
			GaussianBlurManipulatorSettings	previous_settings;

			GaussianBlurManipulatorView		*config_view;


			// The next attributes will be used by the thread_function.
			int32	number_of_threads;
			int32	current_resolution;

			GaussianBlurManipulatorSettings	current_settings;

			Selection	*selection;

			BBitmap		*source_bitmap;
			BBitmap		*target_bitmap;
			BStatusBar	*progress_bar;

			int32		processor_count;

public:
			GaussianBlurManipulator(BBitmap*);
			~GaussianBlurManipulator();

void		MouseDown(BPoint,uint32 buttons,BView*,bool);
int32		PreviewBitmap(bool full_quality = FALSE, BRegion* =NULL);
BBitmap*	ManipulateBitmap(ManipulatorSettings*, BBitmap*, BStatusBar*);
void		Reset();
void		SetPreviewBitmap(BBitmap*);
const char*	ReturnHelpString();
const char*	ReturnName();

ManipulatorSettings*	ReturnSettings();

BView*		MakeConfigurationView(const BMessenger& target);

void		ChangeSettings(ManipulatorSettings*);
ImageProcessingLibrary	*ipLibrary;
void		SetSelection(Selection* new_selection)
				{ selection = new_selection; };
};


#define	BLUR_ADJUSTING_FINISHED		'Thaf'

class GaussianBlurManipulatorView : public WindowGUIManipulatorView {
		BMessenger						target;
		GaussianBlurManipulator			*manipulator;
		GaussianBlurManipulatorSettings	settings;

		BSlider							*blur_slider;


		bool							started_adjusting;
public:
		GaussianBlurManipulatorView(GaussianBlurManipulator*,const BMessenger& target);
		~GaussianBlurManipulatorView();

void	AllAttached();
void	AttachedToWindow();
void	MessageReceived(BMessage*);
void	ChangeSettings(ManipulatorSettings*);


};

#endif



