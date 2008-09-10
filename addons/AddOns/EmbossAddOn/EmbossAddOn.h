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
public:
			EmbossManipulator();
			~EmbossManipulator();

BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);

char*		ReturnName() { return "Emboss"; }
};

#endif



