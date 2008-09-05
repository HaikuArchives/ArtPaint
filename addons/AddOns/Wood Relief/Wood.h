/* 

	Filename:	Wood.h
	Contents:	Declaration for a manipulator that enhances edges in the image.	
	Author:		Heikki Suhonen
	
*/




#ifndef WOOD_H
#define WOOD_H

#include "Manipulator.h"

class WoodManipulator : public Manipulator {
		BBitmap		*source_bitmap;
		BBitmap		*target_bitmap;
		BStatusBar	*progress_bar;
		Selection	*the_selection;

		BBitmap		*spare_copy_bitmap;

static	int32		thread_entry(void*);
		int32		thread_function(int32);

public:
			WoodManipulator();
			~WoodManipulator();
			
BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);
char*		ReturnName();
};

#endif
