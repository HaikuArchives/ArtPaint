/* 

	Filename:	OilAddOn.h
	Contents:	Oil add-on declarations.	
	Author:		Heikki Suhonen
	
*/




#ifndef OIL_ADD_ON_H
#define OIL_ADD_ON_H

#include "Manipulator.h"


class OilManipulator : public Manipulator {
public:
			OilManipulator(BBitmap*);
			~OilManipulator();
			
BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);

char*		ReturnName() { return "Oil"; }
};

#endif



