/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef WINDOW_GUI_MANIPULATOR_H
#define	WINDOW_GUI_MANIPULATOR_H

#include "GUIManipulator.h"

class WindowGUIManipulator : public GUIManipulator {

public:
				WindowGUIManipulator() {};
				~WindowGUIManipulator() {};

virtual	BView*	MakeConfigurationView(BMessenger*) = 0;
};



class WindowGUIManipulatorView : public BView {

public:
		WindowGUIManipulatorView(BRect);

void	AttachedToWindow();
void	MessageReceived(BMessage*);
};
#endif
