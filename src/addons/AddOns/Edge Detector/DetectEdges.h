/* 

	Filename:	DetectEdges.h
	Contents:	Declaration for a manipulator that enhances edges in the image.	
	Author:		Heikki Suhonen
	
*/




#ifndef DETECT_EDGES_H
#define DETECT_EDGES_H

#include "Manipulator.h"

class DetectEdgesManipulator : public Manipulator {
		BBitmap		*source_bitmap;
		BBitmap		*target_bitmap;
		BStatusBar	*progress_bar;
		Selection	*the_selection;

static	int32		thread_entry(void*);
		int32		thread_function(int32);

public:
			DetectEdgesManipulator();
			~DetectEdgesManipulator();
			
BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);
char*		ReturnName();
};
#endif
