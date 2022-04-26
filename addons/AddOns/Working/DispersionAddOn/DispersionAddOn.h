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
public:
			DispersionManipulator(BBitmap*);
			~DispersionManipulator();

BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);

const char*	ReturnName();
};

#endif



