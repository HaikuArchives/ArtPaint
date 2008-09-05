/* 

	Filename:	DispersionAddOn.h
	Contents:	Dispersion add-on declarations.	
	Author:		Heikki Suhonen
	
*/




#ifndef _POLAR_MAPPER_H
#define _POLAR_MAPPER_H

#include "Manipulator.h"

#define MAX_DISPERSION_Y	8
#define	MAX_DISPERSION_X	8

class PolarMapper : public Manipulator {
public:
			PolarMapper(BBitmap*);
			~PolarMapper();
			
BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);

char*		ReturnName() { return "LogPolar Mapper"; }
};

#endif



