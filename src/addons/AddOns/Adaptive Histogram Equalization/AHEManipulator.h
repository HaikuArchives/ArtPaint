/* 

	Filename:	AHEManipulator.h
	Contents:	AHEManipulator-declarations.	
	Author:		Heikki Suhonen
	
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
char*		ReturnName() { return "Adaptive Histogram Equalization"; }
};

#endif	// _AHE_MANIPULATOR_H



