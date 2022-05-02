/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef _AHE_MANIPULATOR_H
#define _AHE_MANIPULATOR_H

#include "Manipulator.h"

class AHEManipulator : public Manipulator {
BBitmap		*target_bitmap;

public:
			AHEManipulator(BBitmap*);
			~AHEManipulator();

BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);
const char*	ReturnHelpString();
const char*	ReturnName();
};

#endif	// _AHE_MANIPULATOR_H



