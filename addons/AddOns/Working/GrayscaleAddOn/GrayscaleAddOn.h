/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef GRAY_SCALE_ADD_ON_H
#define GRAY_SCALE_ADD_ON_H

#include "Manipulator.h"

class GrayscaleAddOnManipulator : public Manipulator {
public:
			GrayscaleAddOnManipulator(BBitmap*);
			~GrayscaleAddOnManipulator();

BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);
const char*	ReturnName();
};

#endif



