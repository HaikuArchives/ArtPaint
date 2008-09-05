/* 

	Filename:	DispersionAddOn.h
	Contents:	Dispersion add-on declarations.	
	Author:		Heikki Suhonen
	
*/




#ifndef DISPERSION_ADD_ON_H
#define DISPERSION_ADD_ON_H

#include "Manipulator.h"

#define MAX_DISPERSION_Y	8
#define	MAX_DISPERSION_X	8

class DispersionManipulator : public Manipulator {
public:
			DispersionManipulator(BBitmap*);
			~DispersionManipulator();
			
BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);

char*		ReturnName() { return "Dispersion"; }
};

#endif



