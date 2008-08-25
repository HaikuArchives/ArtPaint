/* 

	Filename:	GrayScaleAddOn.h
	Contents:	A header for the grayscale add-on.	
	Author:		Heikki Suhonen
	
*/




#ifndef GRAY_SCALE_ADD_ON_H
#define GRAY_SCALE_ADD_ON_H

#include "Manipulator.h"

class GrayscaleAddOnManipulator : public Manipulator {
public:
			GrayscaleAddOnManipulator(BBitmap*);
			~GrayscaleAddOnManipulator();
			
BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);
char*		ReturnName() { return "Grayscale"; }
};

#endif



