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
			return !(*this==s);
		}

int32	mode;
};



class ColorSeparatorManipulatorView;

class ColorSeparatorManipulator : public WindowGUIManipulator {
			BBitmap	*preview_bitmap;
			BBitmap	*copy_of_the_preview_bitmap;

			ColorSeparatorManipulatorSettings	settings;
			ColorSeparatorManipulatorSettings	previous_settings;

			ColorSeparatorManipulatorView		*config_view;

			BBitmap	*source_bitmap;
			BBitmap	*target_bitmap;

void		separate_colors();

public:
			ColorSeparatorManipulator(BBitmap*);
			~ColorSeparatorManipulator();

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



#define	MENU_ENTRY_CHANGED		'Menc'

class ColorSeparatorManipulatorView : public WindowGUIManipulatorView {
		BMessenger						target;
		ColorSeparatorManipulator			*manipulator;
		ColorSeparatorManipulatorSettings	settings;

		BMenuField						*cmyk_menu_field;

		bool							started_adjusting;
public:
		ColorSeparatorManipulatorView(ColorSeparatorManipulator*,const BMessenger&);
		~ColorSeparatorManipulatorView();

void	AllAttached();
void	AttachedToWindow();
void	MessageReceived(BMessage*);
void	ChangeSettings(ManipulatorSettings*);
};

#endif



