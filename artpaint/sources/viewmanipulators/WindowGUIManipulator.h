/*

	Filename:	WindowGUIManipulator.h
	Contents:	WindowGUIManipulator-class declaration
	Author:		Heikki Suhonen

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
