/* 

	Filename:	EmbossAddOn.h
	Contents:	Emboss add-on declarations.	
	Author:		Heikki Suhonen
	
*/




#ifndef EMBOSS_MANIPULATOR_H
#define EMBOSS_MANIPULATOR_H

#include "Manipulator.h"

class EmbossManipulator : public Manipulator {
public:
			EmbossManipulator();
			~EmbossManipulator();
			
BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);

char*		ReturnName() { return "Emboss"; }
};

#endif



