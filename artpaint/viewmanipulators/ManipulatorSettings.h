/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
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
