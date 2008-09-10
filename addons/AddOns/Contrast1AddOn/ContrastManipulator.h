/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef CONTRAST_MANIPULATOR_H
#define CONTRAST_MANIPULATOR_H

#include "Manipulator.h"

class ContrastManipulator : public Manipulator {
BBitmap		*target_bitmap;

public:
			ContrastManipulator(BBitmap*);
			~ContrastManipulator();

BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);
char*		ReturnName() { return "Stretch Histogram"; }
};

#endif



