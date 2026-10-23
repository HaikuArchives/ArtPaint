/*
 * Copyright 2026, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#ifndef LEVELS_H
#define LEVELS_H

#include "FloatControl.h"
#include "ManipulatorSettings.h"
#include "NumberControl.h"
#include "PixelOperations.h"
#include "WindowGUIManipulator.h"


class BButton;
class BSlider;
class BMenuField;
class ColorMultiSlider;
class MultiSlider;
class PreviewView;


using ArtPaint::Interface::FloatControl;
using ArtPaint::Interface::NumberControl;


enum levelMode {
	LEVELS_MODE_RGB = 0,
	LEVELS_MODE_RED,
	LEVELS_MODE_GREEN,
	LEVELS_MODE_BLUE
};


class	LevelsManipulatorSettings : public ManipulatorSettings {
public:
	LevelsManipulatorSettings()
		: ManipulatorSettings() {
		for (int i = 0; i < 4; ++i) {
			inputMin[i] = 0;
			inputMax[i] = 255;
			gamma[i] = 1.0;
			outputMin[i] = 0;
			outputMax[i] = 255;
		}
		mode = LEVELS_MODE_RGB;
	}

	LevelsManipulatorSettings(const LevelsManipulatorSettings& s)
		: ManipulatorSettings() {
		for (int i = 0; i < 4; ++i) {
			inputMin[i] = s.inputMin[i];
			inputMax[i] = s.inputMax[i];
			gamma[i] = s.gamma[i];
			outputMin[i] = s.outputMin[i];
			outputMax[i] = s.outputMax[i];
		}
		mode = s.mode;
	}

	LevelsManipulatorSettings& operator=(const LevelsManipulatorSettings& s) {
		for (int i = 0; i < 4; ++i) {
			inputMin[i] = s.inputMin[i];
			inputMax[i] = s.inputMax[i];
			gamma[i] = s.gamma[i];
			outputMin[i] = s.outputMin[i];
			outputMax[i] = s.outputMax[i];
		}

		mode = s.mode;

		return *this;
	}

	bool operator==(LevelsManipulatorSettings s) {
		if (mode != s.mode)
			return false;

		for (int i = 0; i < 4; ++i) {
			if (inputMin[i] != s.inputMin[i])
				return false;
			if (inputMax[i] != s.inputMax[i])
				return false;
			if (outputMin[i] != s.outputMin[i])
				return false;
			if (outputMax[i] != s.outputMax[i])
				return false;
			if (gamma[i] != s.gamma[i])
				return false;
		}

		return true;
	}

	bool operator!=(LevelsManipulatorSettings s) {
		return !(*this == s);
	}

	uint32 	inputMin[4];
	uint32 inputMax[4];
	uint32 outputMin[4];
	uint32 outputMax[4];
	float	gamma[4];
	uint32 mode;
};


class LevelsManipulatorView;

class LevelsManipulator : public WindowGUIManipulator {
			BBitmap* preview_bitmap;
			BBitmap* copy_of_the_preview_bitmap;

			int32	lowest_available_quality;
			int32	highest_available_quality;
			int32	last_calculated_resolution;

			LevelsManipulatorSettings	settings;
			LevelsManipulatorSettings	previous_settings;
			LevelsManipulatorSettings	original_settings;

			LevelsManipulatorView*		config_view;

			// The next attributes will be used by the thread_function.
			int32	number_of_threads;
			int32	current_resolution;

			LevelsManipulatorSettings	current_settings;

			Selection*	selection;

			BBitmap*	source_bitmap;
			BBitmap*	target_bitmap;
			BStatusBar*	progress_bar;

			void	start_threads();

	static	int32	thread_entry(void*);
			int32	thread_function(int32);
	union color_conversion color_map[256];

public:
				LevelsManipulator(BBitmap*);
				~LevelsManipulator();

	int32		PreviewBitmap(bool full_quality = FALSE, BRegion* = NULL);
	BBitmap*	ManipulateBitmap(ManipulatorSettings*, BBitmap*, BStatusBar*);
	void		Reset();
	void		SetPreviewBitmap(BBitmap*);
	const char*	ReturnHelpString();
	const char*	ReturnName();

	ManipulatorSettings*	ReturnSettings();

	BView*		MakeConfigurationView(const BMessenger& target);

	void		ChangeSettings(ManipulatorSettings*);
	void		ResetSettings();
	void		SetSelection(Selection* new_selection)
					{ selection = new_selection; };
	void		MouseDown(BPoint, uint32, BView*, bool);
};



#define INPUT_LEVELS_ADJUSTED				'InAd'
#define OUTPUT_LEVELS_ADJUSTED				'OuAd'
#define INPUT_MIN_ADJUSTED					'ImAd'
#define INPUT_MID_ADJUSTED					'IdAd'
#define INPUT_MAX_ADJUSTED					'IxAd'
#define OUTPUT_MIN_ADJUSTED					'OmAd'
#define OUTPUT_MAX_ADJUSTED					'OxAd'
#define GAMMA_ADJUSTED						'GmAd'
#define INPUT_LEVELS_ADJUSTING				'IAdg'
#define OUTPUT_LEVELS_ADJUSTING				'OAdg'
#define AUTO_LEVELS_ADJUSTED				'AuLv'
#define AUTO_TONES_ADJUSTED					'AuTn'
#define AUTO_COLORS_ADJUSTED				'AuCl'
#define LEVELS_RESET						'lvRs'


class LevelsManipulatorView : public WindowGUIManipulatorView {
public:
	LevelsManipulatorView(LevelsManipulator* ,const BMessenger& target);

	void	AllAttached();
	void	AttachedToWindow();
	void	MessageReceived(BMessage*);
	void	ChangeSettings(ManipulatorSettings*);
	void	SetBitmap(BBitmap*);
	void	GenerateHistograms();
	void	DrawHistogram();
	void	SetBlackPoint(uint32 color);
	void	SetWhitePoint(uint32 color);
	void	SetGrayPoint(uint32 color);

private:
	BMessenger					target;
	LevelsManipulator*			manipulator;
	LevelsManipulatorSettings	settings;

	MultiSlider*				input_values_slider;
	ColorMultiSlider*			output_values_slider;
	NumberControl*				inputMin;
	NumberControl*				inputMid;
	NumberControl* 				inputMax;
	FloatControl*				gamma;
	NumberControl*				outputMin;
	NumberControl* 				outputMax;
	BMenuField*					modeMenu;
	BButton*					autoLevelsButton;
	BButton*					autoTonesButton;
	BButton*					autoColorsButton;
	BButton*					resetButton;

	bool						started_adjusting;
	BList						fColorList;
	BBitmap*					histogramBitmap;
	int32						histogram[4][256];
	float						midRatios[4];
	uint32						min[4];
	uint32						max[4];
	uint32						avg[4];
	float						alpha;
	float						beta;

	BRect						histogramRect;

	BBitmap*					analyzedBitmap;
	uint32						mode;
	PreviewView* 				histogramView;
};

#endif



