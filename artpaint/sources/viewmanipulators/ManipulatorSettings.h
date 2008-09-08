/*

	Filename:	ManipulatorSettings.h
	Contents:	ManipulatorSettings-class declaration
	Author:		Heikki Suhonen

*/
#ifndef MANIPULATOR_SETTINGS_H
#define MANIPULATOR_SETTINGS_H


class ManipulatorSettings {
public:
							ManipulatorSettings() {}
	virtual					~ManipulatorSettings() {}

private:
	// This is here to make dynamic casting possible.
	virtual	void 			method_1() {};
};


#endif
