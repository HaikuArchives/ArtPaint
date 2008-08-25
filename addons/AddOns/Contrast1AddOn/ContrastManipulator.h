/* 

	Filename:	ContrastManipulator.h
	Contents:	ContrastManipulator-declarations.	
	Author:		Heikki Suhonen
	
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



