/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef CONTRAST_MANIPULATOR_H
#define CONTRAST_MANIPULATOR_H

#include "Manipulator.h"

class ContrastManipulator : public Manipulator {
BBitmap		*target_bitmap;
Selection*	selection;

public:
			ContrastManipulator(BBitmap*);
			~ContrastManipulator();

BBitmap*	ManipulateBitmap(BBitmap*, BStatusBar*);
const char*	ReturnHelpString();
const char*	ReturnName();
void		SetSelection(Selection* new_selection)
				{ selection = new_selection; };
};

#endif



