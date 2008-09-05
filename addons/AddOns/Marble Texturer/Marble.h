/* 

	Filename:	Marble.h
	Contents:	Declaration for a manipulator that creates a marble-like pattern.	
	Author:		Heikki Suhonen
	
*/




#ifndef MARBLE_H
#define MARBLE_H

#include "Manipulator.h"

class MarbleManipulator : public Manipulator {
		BBitmap		*source_bitmap;
		BBitmap		*target_bitmap;
		BStatusBar	*progress_bar;
		Selection	*the_selection;

		BBitmap		*spare_copy_bitmap;

		ManipulatorInformer	*informer;
		
static	int32		thread_entry(void*);
		int32		thread_function(int32);

		float		marble_amount(float);
		
public:
			MarbleManipulator(ManipulatorInformer*);
			~MarbleManipulator();
			
BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);
char*		ReturnName();
};

#endif
