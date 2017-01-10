/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef MARBLE_H
#define MARBLE_H

#include "Manipulator.h"

class ManipulatorInformer;

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
