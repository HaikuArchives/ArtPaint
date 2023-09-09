/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef EMBOSS_MANIPULATOR_H
#define EMBOSS_MANIPULATOR_H

#include "Manipulator.h"

class EmbossManipulator : public Manipulator {
	Selection*	selection;

public:
				EmbossManipulator();

	BBitmap*	ManipulateBitmap(BBitmap*, BStatusBar*);
	const char*	ReturnHelpString();
	const char*	ReturnName();
	void		SetSelection(Selection* new_selection)
					{ selection = new_selection; };
};

#endif



