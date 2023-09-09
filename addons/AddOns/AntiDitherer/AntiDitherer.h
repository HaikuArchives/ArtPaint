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


#include <Bitmap.h>


namespace ArtPaint {
	namespace Interface {
		class NumberControl;
	}
}
using ArtPaint::Interface::NumberControl;


class	AntiDithererManipulatorSettings : public ManipulatorSettings {
public:
	AntiDithererManipulatorSettings()
		: ManipulatorSettings() {
		block_size = 4;
		reduce_resolution = false;
	}

	AntiDithererManipulatorSettings(const AntiDithererManipulatorSettings& s)
		: ManipulatorSettings() {
		block_size = s.block_size;
		reduce_resolution = s.reduce_resolution;
	}

	AntiDithererManipulatorSettings& operator=(const AntiDithererManipulatorSettings& s) {
		block_size = s.block_size;
		reduce_resolution = s.reduce_resolution;
		return *this;
	}

	bool operator==(AntiDithererManipulatorSettings s) {
		return ((block_size == s.block_size) && (reduce_resolution == s.reduce_resolution));
	}

	bool operator!=(AntiDithererManipulatorSettings s) {
		return !(*this == s);
	}

	int32	block_size;
	bool	reduce_resolution;
};


class AntiDithererManipulatorView;

class AntiDithererManipulator : public WindowGUIManipulator {
	BBitmap*	preview_bitmap;
	BBitmap*	copy_of_the_preview_bitmap;

	AntiDithererManipulatorSettings	settings;
	AntiDithererManipulatorSettings	previous_settings;

	AntiDithererManipulatorView*	config_view;

	BBitmap*	source_bitmap;
	BBitmap*	target_bitmap;

	void		anti_dither();
	Selection*	selection;

public:
				AntiDithererManipulator(BBitmap*);
				~AntiDithererManipulator();

	int32		PreviewBitmap(bool full_quality = FALSE, BRegion* = NULL);
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


#define	REDUCE_RESOLUTION_ADJUSTED		'Rrea'
#define	BLOCK_SIZE_ADJUSTED				'Blsa'

class AntiDithererManipulatorView : public WindowGUIManipulatorView {
	BMessenger						target;
	AntiDithererManipulator*		manipulator;
	AntiDithererManipulatorSettings	settings;

	BSpinner*						block_size_control;
	BCheckBox*						reduce_resolution_box;

	bool							started_adjusting;
public:
		AntiDithererManipulatorView(AntiDithererManipulator*, const BMessenger& target);

	void	AllAttached();
	void	AttachedToWindow();
	void	MessageReceived(BMessage*);
	void	ChangeSettings(ManipulatorSettings*);
};

#endif
