/* 

	Filename:	NegativeAddOn.h
	Contents:	A header for an add-on for making a negative of the image.	
	Author:		Heikki Suhonen
	
*/




#ifndef NEGATIVE_ADD_ON_H
#define NEGATIVE_ADD_ON_H

#include "Manipulator.h"

class NegativeAddOnManipulator : public Manipulator {
public:
			NegativeAddOnManipulator(BBitmap*);
			~NegativeAddOnManipulator();
			
BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);
char*		ReturnName() { return "Negative"; }
};

#endif



