/* 

	Filename:	AddOnTemplate.h
	Contents:	A template header for the ArtPaint add-ons.	
	Author:		Heikki Suhonen
	
*/




#ifndef CONTRAST_MANIPULATOR_H
#define CONTRAST_MANIPULATOR_H

#include "BasicManipulator.h"

class ContrastManipulator : public BasicManipulator {
BBitmap		*target_bitmap;

public:
			ContrastManipulator(BView*,manipulator_data&);
			~ContrastManipulator();
			
BBitmap*	ManipulateBitmap(BBitmap*,BView*,float);
};

#endif



