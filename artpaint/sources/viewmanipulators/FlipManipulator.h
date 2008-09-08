/*

	Filename:	FlipManipulator.h
	Contents:	FlipManipulator-class declarations (Horizontal and Vertical manipulator)
	Author:		Heikki Suhonen

*/


#include "Manipulator.h"

#ifndef FLIP_MANIPULATOR_H
#define	FLIP_MANIPULATOR_H

class HorizFlipManipulator : public Manipulator {

public:
			HorizFlipManipulator();
BBitmap*	ManipulateBitmap(BBitmap *original,Selection*,BStatusBar*);

const char*	ReturnName();
};

class VertFlipManipulator : public Manipulator {

public:
			VertFlipManipulator();
BBitmap*	ManipulateBitmap(BBitmap *original,Selection*,BStatusBar*);

const char*	ReturnName();
};


#endif
