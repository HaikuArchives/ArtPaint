/* 

	Filename:	StatusBarGUIManipulator.h
	Contents:	StatusBarGUIManipulator-class declaration.	
	Author:		Heikki Suhonen
	
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