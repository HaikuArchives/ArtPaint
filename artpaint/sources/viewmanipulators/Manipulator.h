/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef	MANIPULATOR_H
#define	MANIPULATOR_H

#include <Bitmap.h>
#include <StatusBar.h>

#include "ManipulatorInformer.h"
#include "ManipulatorSettings.h"
#include "Selection.h"


/*
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
	SCALE_MANIPULATOR,
	TEXT_MANIPULATOR,
	TRANSLATION_MANIPULATOR,
	TRANSPARENCY_MANIPULATOR,
	VERT_FLIP_MANIPULATOR
};


// This must be increased whenever one of the Manipulator classes changes.
enum {
	ADD_ON_API_VERSION	= 0x00000008
};

class Manipulator {
	friend	class		ManipulatorServer;
	image_id	add_on_id;

protected:
	BBitmap*		DuplicateBitmap(BBitmap*,int32 inset=0,bool accept_views=FALSE);

public:
	Manipulator() { add_on_id = -1; }
	virtual		~Manipulator() {}

	virtual	BBitmap*				ManipulateBitmap(BBitmap*,Selection*,BStatusBar*) = 0;
	virtual	ManipulatorSettings*	ReturnSettings() { return NULL; }
	virtual	const	char*			ReturnName() = 0;
};

#endif
