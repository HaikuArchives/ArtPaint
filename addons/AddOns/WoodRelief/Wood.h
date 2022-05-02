/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
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
		int			processor_count;

public:
			WoodManipulator();
			~WoodManipulator();

BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);
const char*	ReturnHelpString();
const char*	ReturnName();
};

#endif
