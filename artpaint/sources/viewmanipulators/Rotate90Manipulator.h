/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "Manipulator.h"

#ifndef ROTATE_90_MANIPULATOR_H
#define	ROTATE_90_MANIPULATOR_H

class Rotate90ClockwiseManipulator : public Manipulator {

public:
			Rotate90ClockwiseManipulator();
BBitmap*	ManipulateBitmap(BBitmap *original,Selection*,BStatusBar*);

const char*	ReturnName();

private:
};

class Rotate90CounterclockwiseManipulator : public Manipulator {

public:
			Rotate90CounterclockwiseManipulator();
BBitmap*	ManipulateBitmap(BBitmap *original,Selection*,BStatusBar*);

const char*	ReturnName();
};


#endif
