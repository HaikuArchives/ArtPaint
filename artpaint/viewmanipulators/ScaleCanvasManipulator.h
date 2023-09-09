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
#ifndef SCALE_CANVAS_MANIPULATOR_H
#define SCALE_CANVAS_MANIPULATOR_H

#include "ManipulatorSettings.h"
#include "ScaleUtilities.h"
#include "Selection.h"
#include "WindowGUIManipulator.h"


#include <Messenger.h>


#define LEFT_CHANGED		'Ltch'
#define	TOP_CHANGED			'Tpch'
#define WIDTH_CHANGED		'Wdch'
#define HEIGHT_CHANGED		'Hgch'

#define MULTIPLY_WIDTH		'mlWi'
#define MULTIPLY_HEIGHT		'mlHe'

#define RESTORE_WIDTH		'Rswd'
#define RESTORE_HEIGHT		'Rshg'

#define	PROPORTION_CHANGED	'Prpc'

class BButton;
class BPopUpMenu;
class ScaleCanvasManipulatorView;


namespace ArtPaint {
	namespace Interface {
		class NumberControl;
	}
}
using ArtPaint::Interface::NumberControl;


class ScaleCanvasManipulatorSettings : public ManipulatorSettings {
public:
	ScaleCanvasManipulatorSettings()
		: ManipulatorSettings() {
		left = right = 0;
		top = bottom = 0;
	}

	ScaleCanvasManipulatorSettings(ScaleCanvasManipulatorSettings* s)
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



class ScaleCanvasManipulator : public WindowGUIManipulator {
	BBitmap*	ManipulateBitmap(BBitmap* b, BStatusBar* stb)
	{ return WindowGUIManipulator::ManipulateBitmap(b, stb); }

	BBitmap*	preview_bitmap;
	BBitmap*	copy_of_the_preview_bitmap;

	ScaleCanvasManipulatorView		*configuration_view;
	ScaleCanvasManipulatorSettings	*settings;

	float		original_left;
	float		original_top;
	float		original_right;
	float		original_bottom;

	float		previous_left;
	float		previous_right;
	float		previous_top;
	float 		previous_bottom;

	BPoint 		previous_point;

	Selection*	selection;

	bool		move_left;
	bool		move_right;
	bool		move_top;
	bool		move_bottom;
	bool 		move_all;

	interpolation_type	method;

public:
				ScaleCanvasManipulator(BBitmap*);
				~ScaleCanvasManipulator();

	BBitmap*	ManipulateBitmap(ManipulatorSettings*, BBitmap *original, BStatusBar*);
	int32		PreviewBitmap(bool, BRegion* =NULL);

	void		MouseDown(BPoint, uint32, BView*, bool);

	BRegion		Draw(BView*, float);

	void		SetValues(float, float, float, float);

	BView*		MakeConfigurationView(const BMessenger& target);
	void		Reset();
	void		SetPreviewBitmap(BBitmap*);

	const char*	ReturnHelpString();
	const char*	ReturnName();

	ManipulatorSettings*	ReturnSettings();
	void		SetSelection(Selection* new_selection);

	void		SetInterpolationMethod(interpolation_type newMethod) { method = newMethod; }

	void		UpdateSettings();
};


class ScaleCanvasManipulatorView : public WindowGUIManipulatorView {
public:
							ScaleCanvasManipulatorView(ScaleCanvasManipulator*,
								const BMessenger& target);

	virtual	void			AttachedToWindow();
	virtual	void			MessageReceived(BMessage* message);

			bool			MaintainProportions() { return maintain_proportions; }
			void			SetValues(float left, float top, float right, float bottom);
			void			GetControlValues(float& left, float& top,
								float& right, float& bottom);
private:
			void			_SetTarget(BView* view);
			void			_SetValues(float left, float top, float right, float bottom);
			BButton*		_MakeButton(const char* label, uint32 what, float coefficient);

private:
			BMessenger		fTarget;
			float			original_left;
			float			original_top;
			float			original_width;
			float			original_height;
			float			current_left;
			float			current_top;
			float			current_width;
			float			current_height;
			bool			maintain_proportions;
			interpolation_type	method;

			ScaleCanvasManipulator*	fManipulator;
			NumberControl*	left_control;
			NumberControl*	top_control;
			NumberControl*	width_control;
			NumberControl*	height_control;
			BPopUpMenu*		sample_mode_menu;
};


#endif // SCALE_CANVAS_MANIPULATOR_H
