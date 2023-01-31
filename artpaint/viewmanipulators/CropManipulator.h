/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef CROP_MANIPULATOR_H
#define	CROP_MANIPULATOR_H

#include "WindowGUIManipulator.h"
#include "ManipulatorSettings.h"


#include <Button.h>
#include <CheckBox.h>
#include <Messenger.h>


#define	CROP_TO_SELECTION		'cr2S'
#define RESET_CROP				'rsCr'
#define	TOGGLE_LOCK_ASPECT		'tgLa'


class CropManipulatorView;
class CropManipulatorSettings;


namespace ArtPaint {
	namespace Interface {
		class NumberControl;
	}
}

using ArtPaint::Interface::NumberControl;


class CropManipulator : public WindowGUIManipulator {
		BBitmap*	ManipulateBitmap(BBitmap* b, BStatusBar* stb)
						{ return WindowGUIManipulator::ManipulateBitmap(b, stb); }

		BBitmap*	target_bitmap;
		BBitmap*	preview_bitmap;
		BBitmap*	copy_of_the_preview_bitmap;

		float 		min_x, max_x;
		float		min_y, max_y;

		float		previous_left;
		float		previous_right;
		float		previous_top;
		float		previous_bottom;

		BPoint		previous_point;

		CropManipulatorSettings* 	settings;
		CropManipulatorView*		config_view;

		bool		move_left;
		bool		move_right;
		bool		move_top;
		bool		move_bottom;
		bool 		move_all;

		bool		use_selected;
		bool		lock_aspect;

		Selection*	selection;
public:
					CropManipulator(BBitmap*);
					~CropManipulator();

		void		MouseDown(BPoint, uint32, BView*, bool);

		BRegion		Draw(BView*, float);

		BBitmap*	ManipulateBitmap(ManipulatorSettings*, BBitmap* original,
						BStatusBar*);
		void		SetValues(float, float, float, float);

		int32		PreviewBitmap(bool full_quality = false,
						BRegion* updated_region = NULL);

		BView*		MakeConfigurationView(const BMessenger& target);
		void		Reset();
		void		SetPreviewBitmap(BBitmap*);

const	char*		ReturnHelpString();
const	char*		ReturnName();

		ManipulatorSettings*	ReturnSettings();

		void		UseSelected() { use_selected = TRUE; }
		void		LockAspect(bool lock) { lock_aspect = lock; }

		void		SetSelection(Selection* new_selection)
						{ selection = new_selection; };

		void		UpdateSettings();
};


class CropManipulatorSettings : public ManipulatorSettings {
public:
	CropManipulatorSettings()
		: ManipulatorSettings() {
		left = right = 0;
		top = bottom = 0;
	}

	CropManipulatorSettings(CropManipulatorSettings* s)
		: ManipulatorSettings() {
		left = s->left;
		right = s->right;
		top = s->top;
		bottom = s->bottom;
	}

	float	left;
	float	right;
	float	top;
	float	bottom;
};


class CropManipulatorView : public WindowGUIManipulatorView {
public:
							CropManipulatorView(CropManipulator* manipulator,
								const BMessenger& target);
virtual						~CropManipulatorView() {}

virtual	void				AttachedToWindow();
virtual	void				MessageReceived(BMessage* message);

		void				SetValues(float left, float right, float top,
								float bottom);
		void				GetControlValues(float& left, float& right,
								float& top, float& bottom);

private:
		float				left;
		float				right;
		float				top;
		float				bottom;

		NumberControl*		fTopCrop;
		NumberControl*		fLeftCrop;
		NumberControl*		fRightCrop;
		NumberControl*		fBottomCrop;

		BButton*			fSelected;
		BButton*			fReset;
		BCheckBox*			fLockAspect;

		BMessenger			fTarget;
		CropManipulator*	fManipulator;
};


#endif	// CROP_MANIPULATOR_H
