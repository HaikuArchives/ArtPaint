/*

	Filename:	FlipManipulator.h
	Contents:	FlipManipulator-class declarations (Horizontal and Vertical manipulator)
	Author:		Heikki Suhonen

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
