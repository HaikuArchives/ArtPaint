/* 

	Filename:	EnhanceEdges.h
	Contents:	Declaration for a manipulator that enhances edges in the image.	
	Author:		Heikki Suhonen
	
*/




#ifndef ENHANCE_EDGES_H
#define ENHANCE_EDGES_H

#include "Manipulator.h"

class FadeManipulator : public Manipulator {
		BBitmap		*source_bitmap;
		BBitmap		*target_bitmap;
		BStatusBar	*progress_bar;
		Selection	*the_selection;

static	int32		thread_entry(void*);
		int32		thread_function(int32);

public:
			FadeManipulator();
			~FadeManipulator();
			
	BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);
char*		ReturnName();
};
#endif
