/* 

	Filename:	Template.h
	Contents:	A template header for the ArtPaint add-ons.	
	Author:	
	
*/




#ifndef TEMPLATE_H
#define TEMPLATE_H

#include "Manipulator.h"

class $MANIPULATOR_NAME : public Manipulator {
public:
			$MANIPULATOR_NAME();
			~$MANIPULATOR_NAME();
			
BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);
char*		ReturnName();
};
#endif
