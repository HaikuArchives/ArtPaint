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
#ifndef SCALE_MANIPULATOR_H
#define SCALE_MANIPULATOR_H

#include "ManipulatorSettings.h"
#include "ScaleUtilities.h"
#include "Selection.h"
#include "WindowGUIManipulator.h"


#include <Messenger.h>


#define SCALE_LEFT_CHANGED			'SLch'
#define SCALE_TOP_CHANGED			'STch'
#define	SCALE_WIDTH_CHANGED			'SWch'
#define	SCALE_HEIGHT_CHANGED		'SHch'

#define SCALE_MULTIPLY_WIDTH		'SmWi'
#define SCALE_MULTIPLY_HEIGHT		'SmHe'

#define SCALE_RESTORE_WIDTH			'SRwd'
#define SCALE_RESTORE_HEIGHT		'SRhg'

#define	SCALE_PROPORTION_CHANGED	'SPpc'


class BButton;
class BPopUpMenu;
class ScaleManipulatorView;


namespace ArtPaint {
	namespace Interface {
		class NumberControl;
	}
}
using ArtPaint::Interface::NumberControl;


class ScaleManipulatorSettings : public ManipulatorSettings {
public:
	ScaleManipulatorSettings()
		: ManipulatorSettings() {
		left = right = 0;
		top = bottom = 0;
	}

	ScaleManipulatorSettings(ScaleManipulatorSettings *s)
		: ManipulatorSettings() {
		left = s->left;
		top = s->top;
		right = s->right;
		bottom = s->bottom;
	}

	float 	left;
	float	top;
	float	right;
	float	bottom;
};


class ScaleManipulator : public WindowGUIManipulator {
	BBitmap*	ManipulateBitmap(BBitmap* b, BStatusBar* stb)
	{ return WindowGUIManipulator::ManipulateBitmap(b, stb); }

	BBitmap*	preview_bitmap;
	BBitmap*	copy_of_the_preview_bitmap;

	ScaleManipulatorView		*configuration_view;
	ScaleManipulatorSettings	*settings;

	float		original_left;
	float		original_top;
	float		original_right;
	float		original_bottom;

	float		previous_left;
	float		previous_top;
	float		previous_right;
	float		previous_bottom;

	Selection*	selection;
	BBitmap*	orig_selection_map;
	bool		transform_selection_only;

	BPoint		previous_point;
	bool		reject_mouse_input;

	bool		move_left;
	bool		move_top;
	bool		move_right;
	bool		move_bottom;
	bool		move_all;

	interpolation_type	method;

public:
	ScaleManipulator(BBitmap*);
	~ScaleManipulator();

	BBitmap*	ManipulateBitmap(ManipulatorSettings*, BBitmap *original,
					BStatusBar*);
	int32		PreviewBitmap(bool, BRegion* =NULL);

	void		MouseDown(BPoint, uint32, BView*, bool);

	BRegion		Draw(BView*, float);

	void		SetValues(float, float, float, float);

	BView*		MakeConfigurationView(const BMessenger& target);
	void		Reset();
	void		SetPreviewBitmap(BBitmap*);

	const	char*	ReturnHelpString();
	const	char*	ReturnName();

	ManipulatorSettings*	ReturnSettings();
	void		SetSelection(Selection* new_selection);
	void		SetTransformSelectionOnly(bool select_only)
					{ transform_selection_only = select_only; }
	bool		GetTransformSelectionOnly()
					{ return transform_selection_only; }

	void		SetInterpolationMethod(interpolation_type newMethod)
					{ method = newMethod; }
	void		UpdateSettings();
};


class ScaleManipulatorView : public WindowGUIManipulatorView {
public:
								ScaleManipulatorView(ScaleManipulator*,
									const BMessenger& target);
	virtual						~ScaleManipulatorView() {}

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			bool				MaintainProportions() {
									return maintain_proportions;
								}
			void				SetValues(float left, float top,
									float width, float height);
			void				GetControlValues(float& left, float& top,
									float& width, float& height);
private:
			void				_SetTarget(BView* view);
			void				_SetValues(float left, float top,
									float width, float height);
			BButton*			_MakeButton(const char* label,
									uint32 what, float coefficient);

private:
			BMessenger			fTarget;
			float				original_left;
			float				original_top;
			float				original_width;
			float				original_height;
			float				current_left;
			float				current_top;
			float				current_width;
			float				current_height;
			bool				maintain_proportions;
			interpolation_type	method;

			ScaleManipulator*	fManipulator;
			NumberControl*		left_control;
			NumberControl*		top_control;
			NumberControl*		width_control;
			NumberControl*		height_control;
			BPopUpMenu*			sample_mode_menu;
};

#endif
