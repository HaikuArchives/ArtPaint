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
#ifndef MANIPULATOR_WINDOW_H
#define MANIPULATOR_WINDOW_H

#include <Window.h>


class ManipulatorWindow : public BWindow {
public:
								ManipulatorWindow(BRect rect,
									BView* manipulatorView, const char* name,
									BWindow* master, const BMessenger& target);
	virtual						~ManipulatorWindow();

	static	void				setFeel(window_feel);

private:
	static	BList				sfWindowList;
	static	sem_id				sfWindowListMutex;

			BView*				fManipulatorView;
};

#endif	// MANIPULATOR_WINDOW_H

