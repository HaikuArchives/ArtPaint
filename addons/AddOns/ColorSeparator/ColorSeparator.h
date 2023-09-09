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


enum {
	SHOW_CYAN,
	SHOW_MAGENTA,
	SHOW_YELLOW,
	SHOW_BLACK
};

class	ColorSeparatorManipulatorSettings : public ManipulatorSettings {
public:
	ColorSeparatorManipulatorSettings()
		: ManipulatorSettings() {
		mode = SHOW_BLACK;
	}

	ColorSeparatorManipulatorSettings(const ColorSeparatorManipulatorSettings& s)
		: ManipulatorSettings() {
		mode = s.mode;
	}

	ColorSeparatorManipulatorSettings& operator=(const ColorSeparatorManipulatorSettings& s) {
		mode = s.mode;
		return *this;
	}

	bool operator==(ColorSeparatorManipulatorSettings s) {
		return (mode == s.mode);
	}

	bool operator!=(ColorSeparatorManipulatorSettings s) {
		return !(*this == s);
	}

	int32	mode;
};



class ColorSeparatorManipulatorView;

class ColorSeparatorManipulator : public WindowGUIManipulator {
	BBitmap* 	preview_bitmap;
	BBitmap* 	copy_of_the_preview_bitmap;

	ColorSeparatorManipulatorSettings	settings;
	ColorSeparatorManipulatorSettings	previous_settings;

	ColorSeparatorManipulatorView*		config_view;

	BBitmap* 	source_bitmap;
	BBitmap* 	target_bitmap;

	void		separate_colors();

	Selection*	selection;

public:
				ColorSeparatorManipulator(BBitmap*);
				~ColorSeparatorManipulator();

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


#define	MENU_ENTRY_CHANGED		'Menc'

class ColorSeparatorManipulatorView : public WindowGUIManipulatorView {
	BMessenger	target;
	ColorSeparatorManipulator*			manipulator;
	ColorSeparatorManipulatorSettings	settings;

	BMenuField*	cmyk_menu_field;

	bool		started_adjusting;
public:
			ColorSeparatorManipulatorView(ColorSeparatorManipulator*, const BMessenger&);

	void	AttachedToWindow();
	void	MessageReceived(BMessage*);
	void	ChangeSettings(ManipulatorSettings*);
};

#endif
