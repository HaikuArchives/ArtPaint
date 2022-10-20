/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef MANIPULATOR_H
#define MANIPULATOR_H


#include <image.h>


class BBitmap;
class BStatusBar;
class Selection;
class ManipulatorSettings;


enum {	  // increase on API changes
	ADD_ON_API_VERSION	= 0x00000008
};


/* !
	The following enumeration is used to identify between different manipulators.
	The internal manipulators each need one constant to identify them. The
	constants are used with a method that generates proper manipulator when
	given a manipulator_type and an optional add_on_id.
*/
enum manipulator_type {
	ADD_ON_MANIPULATOR,
	CROP_MANIPULATOR,
	FREE_TRANSFORM_MANIPULATOR,
	HORIZ_FLIP_MANIPULATOR,
	ROTATION_MANIPULATOR,
	ROTATE_CW_MANIPULATOR,
	ROTATE_CCW_MANIPULATOR,
	ROTATE_CW_CANVAS_MANIPULATOR,
	ROTATE_CCW_CANVAS_MANIPULATOR,
	SCALE_MANIPULATOR,
	TEXT_MANIPULATOR,
	TRANSLATION_MANIPULATOR,
	TRANSPARENCY_MANIPULATOR,
	VERT_FLIP_MANIPULATOR
};


class Manipulator {
	friend class ManipulatorServer;

public:
									Manipulator();
	virtual							~Manipulator() {}

	virtual	BBitmap*				ManipulateBitmap(BBitmap*,
										BStatusBar*) = 0;
	virtual	ManipulatorSettings*	ReturnSettings() { return NULL; }
	virtual	const char*				ReturnName() = 0;
	virtual const char*				ReturnHelpString() { return NULL; }

	double							GetSystemClockSpeed()
										{ return fSystemClockSpeed; }
	int								GetSystemCpuCount()
										{ return fCpuCount; }
	virtual void					SetSelection(Selection* new_selection) = 0;

protected:
			BBitmap*				DuplicateBitmap(BBitmap* source,
										int32 inset = 0,
										bool acceptViews = false);

private:
			image_id				add_on_id;
			double					fSystemClockSpeed;
			int						fCpuCount;
};

#endif	// MANIPULATOR_H
