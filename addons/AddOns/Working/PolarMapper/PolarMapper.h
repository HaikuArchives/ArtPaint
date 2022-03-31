/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef _POLAR_MAPPER_H
#define _POLAR_MAPPER_H

#include "Manipulator.h"

#define MAX_DISPERSION_Y	8
#define	MAX_DISPERSION_X	8

class PolarMapper : public Manipulator {
public:
			PolarMapper(BBitmap*);
			~PolarMapper();

BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);

const char*	ReturnName() { return "Polar mapper"; }
};

#endif



