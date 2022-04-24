/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef _INTERFERENCE_H
#define _INTERFERENCE_H

#include <stdio.h>
#include <CheckBox.h>
#include <Messenger.h>
#include <Slider.h>
#include <Spinner.h>
#include <TextControl.h>

#include "ManipulatorSettings.h"
#include "WindowGUIManipulator.h"


#define MIN_WAVE_LENGTH   	2
#define MAX_WAVE_LENGTH		100


class InterferenceManipulatorSettings : public ManipulatorSettings {
public:
		InterferenceManipulatorSettings()
			: ManipulatorSettings() {
			centerA = BPoint(0,0);
			waveLengthA = 30;
			centerB = BPoint(0,0);
			waveLengthB = 30;
			grayscale = B_CONTROL_OFF;
		}

		InterferenceManipulatorSettings(const InterferenceManipulatorSettings& s)
			: ManipulatorSettings() {
			centerA = s.centerA;
			waveLengthA = s.waveLengthA;
			centerB = s.centerB;
			waveLengthB = s.waveLengthB;
			grayscale = s.grayscale;
		}

		InterferenceManipulatorSettings& operator=(const InterferenceManipulatorSettings& s) {
			centerA = s.centerA;
			waveLengthA = s.waveLengthA;
			centerB = s.centerB;
			waveLengthB = s.waveLengthB;
			grayscale = s.grayscale;
			return *this;
		}

		bool operator==(InterferenceManipulatorSettings s) {
			return ((centerA == s.centerA) && (waveLengthA == s.waveLengthA) &&
					(centerB == s.centerB) && (waveLengthB == s.waveLengthB) &&
					(grayscale == s.grayscale));
		}

		BPoint	centerA;
		BPoint	centerB;
		float	waveLengthA;
		float	waveLengthB;
		int32	grayscale;
};


class ManipulatorInformer;


class InterferenceManipulatorView;


class InterferenceManipulator : public WindowGUIManipulator {
	BBitmap*						preview_bitmap;
	BBitmap*						copy_of_the_preview_bitmap;

	InterferenceManipulatorSettings	settings;
	InterferenceManipulatorSettings	previous_settings;

	InterferenceManipulatorView*	config_view;
	float*							sin_table;

	ManipulatorInformer				*informer;

	void		MakeInterference(BBitmap*, InterferenceManipulatorSettings*,
					Selection*);

public:
				InterferenceManipulator(BBitmap*, ManipulatorInformer*);
				~InterferenceManipulator();

	void		MouseDown(BPoint,uint32 buttons, BView*, bool);
	int32		PreviewBitmap(Selection*, bool full_quality = FALSE,
					BRegion* = NULL);
	BBitmap*	ManipulateBitmap(ManipulatorSettings*, BBitmap*,
					Selection*, BStatusBar*);
	void		Reset(Selection*);
	void		SetPreviewBitmap(BBitmap*);
	const char*	ReturnHelpString();
	const char*	ReturnName();

	ManipulatorSettings*	ReturnSettings();

	BView*		MakeConfigurationView(const BMessenger& target);

	void		ChangeSettings(ManipulatorSettings*);
};



class InterferenceManipulatorView : public WindowGUIManipulatorView {
	BMessenger*					target;
	InterferenceManipulator*	manipulator;
	BSlider*					waveLengthSliderA;
	BSlider*					waveLengthSliderB;
	BSpinner*					centerAX;
	BSpinner*					centerAY;
	BSpinner*					centerBX;
	BSpinner*					centerBY;
	BCheckBox*					grayScale;

	InterferenceManipulatorSettings	settings;

	bool			preview_started;

public:
				InterferenceManipulatorView(BRect,
					InterferenceManipulator*, const BMessenger&);
				~InterferenceManipulatorView() { delete target; }

	void		AttachedToWindow();
	void		AllAttached();
	void		MessageReceived(BMessage*);
	void		ChangeSettings(InterferenceManipulatorSettings* s);
};


#ifdef __POWERPC__
inline 	asm	float reciprocal_of_square_root(float number)
{
	machine		604
	frsqrte		fp1,number;	// Estimates reciprocal of square-root
	blr
}
#else
float reciprocal_of_square_root(float number)
{
	return 1.0 / sqrt(number);
}
#endif

#endif
