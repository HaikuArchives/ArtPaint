/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef DISPERSION_ADD_ON_H
#define DISPERSION_ADD_ON_H

#include "Manipulator.h"

#define MAX_DISPERSION_Y	8
#define	MAX_DISPERSION_X	8

class DispersionManipulator : public Manipulator {
	Selection* selection;

public:
				DispersionManipulator(BBitmap*);

	BBitmap*	ManipulateBitmap(BBitmap*, BStatusBar*);
	const char*	ReturnHelpString();
	const char*	ReturnName();
	void		SetSelection(Selection* new_selection)
					{ selection = new_selection; }
};

#endif
