/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef TEMPLATE_H
#define TEMPLATE_H

#include "Manipulator.h"

class $MANIPULATOR_NAME : public Manipulator {
public:
			$MANIPULATOR_NAME();
			~$MANIPULATOR_NAME();

BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);
const char*	ReturnName();
};
#endif
