/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef	ROTATION_MANIPULATOR_H
#define	ROTATION_MANIPULATOR_H

#include "Cursors.h"
#include "ManipulatorSettings.h"
#include "WindowGUIManipulator.h"

#include <Messenger.h>

class BTextControl;
class HSPolygon;
class RotationManipulatorConfigurationView;


class RotationManipulatorSettings : public ManipulatorSettings {
public:
	RotationManipulatorSettings()
		: ManipulatorSettings() { angle = 0; origo = BPoint(0,0); }

	RotationManipulatorSettings(RotationManipulatorSettings *s)
		: ManipulatorSettings() { angle = s->angle; origo = s->origo; }


	float	angle;
	BPoint	origo;
};


/* !
	This manipulator handles angles. The supported angle value are between
	-180˚ and +180. Negative angles are clockwise and positive counterclockwise.
*/
class RotationManipulator: public WindowGUIManipulator {
	BBitmap*	ManipulateBitmap(BBitmap* b, BStatusBar* stb)
	{ return WindowGUIManipulator::ManipulateBitmap(b, stb); }

	BBitmap* 	copy_of_the_preview_bitmap;
	BBitmap* 	preview_bitmap;

	float		previous_angle;
	float		starting_angle;
	BPoint		previous_origo;
	int32		last_calculated_resolution;
	int32		lowest_available_quality;
	int32		highest_available_quality;

	RotationManipulatorSettings*			settings;

	HSPolygon*								view_polygon;

	RotationManipulatorConfigurationView*	config_view;

	bool		move_origo;

	Selection*	selection;

public:
	RotationManipulator(BBitmap*);
	~RotationManipulator();

	void		SetPreviewBitmap(BBitmap*);

	BRegion		Draw(BView*, float);
	void		MouseDown(BPoint, uint32, BView*, bool);

	BBitmap*	ManipulateBitmap(ManipulatorSettings*, BBitmap*, BStatusBar*);
	int32		PreviewBitmap(bool full_quality = FALSE,
					BRegion* updated_region = NULL);
	void		Reset();

	const char*	ReturnHelpString();
	const char*	ReturnName();

	BView*					MakeConfigurationView(const BMessenger& target);

	ManipulatorSettings*	ReturnSettings() { return new RotationManipulatorSettings(settings); }

	const	void*			ManipulatorCursor() { return HS_ROTATION_CURSOR; }

	void		SetAngle(float);

	void		SetSelection(Selection* new_selection);
};


class RotationManipulatorConfigurationView : public WindowGUIManipulatorView {
public:
								RotationManipulatorConfigurationView(
									RotationManipulator* manipulator,
									const BMessenger& target);
	virtual						~RotationManipulatorConfigurationView() {}

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			void				SetAngle(float angle);
			void				SetTarget(const BMessenger& target);

private:
		BMessenger				fTarget;

		BTextControl*			fTextControl;
		RotationManipulator*	fManipulator;
};

#endif	// ROTATION_MANIPULATOR_H
