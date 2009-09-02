/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef WINDOW_GUI_MANIPULATOR_H
#define	WINDOW_GUI_MANIPULATOR_H

#include "GUIManipulator.h"


#include <View.h>


class WindowGUIManipulator : public GUIManipulator {
public:
								WindowGUIManipulator() {}
								~WindowGUIManipulator() {}

	virtual	BView*				MakeConfigurationView(BMessenger* target) = 0;
};


class WindowGUIManipulatorView : public BView {
public:
								WindowGUIManipulatorView();
								WindowGUIManipulatorView(BRect rect);

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);
};

#endif	// WINDOW_GUI_MANIPULATOR_H
