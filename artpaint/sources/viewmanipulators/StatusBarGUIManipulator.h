/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef STATUS_BAR_GUI_MANIPULATOR_H
#define STATUS_BAR_GUI_MANIPULATOR_H

#include "GUIManipulator.h"

class StatusBarGUIManipulator : public GUIManipulator {
public:
	StatusBarGUIManipulator() {};
	~StatusBarGUIManipulator() {};

virtual	BView*	MakeConfigurationView(float, float, BMessenger*) { return NULL; }
};

#endif
