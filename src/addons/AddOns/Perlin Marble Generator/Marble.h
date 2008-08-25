/* 

	Filename:	Marble.h
	Contents:	Declaration for a manipulator that generates marble pattern.	
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

static	int32		thread_entry(void*);
		int32		thread_function(int32);

		uint32		marble_color(float);

public:
			MarbleManipulator();
			~MarbleManipulator();
			
BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);
char*		ReturnName();
};
#endif
