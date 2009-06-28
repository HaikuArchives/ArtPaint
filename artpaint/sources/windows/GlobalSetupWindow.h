/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef GLOBAL_SETUP_WINDOW_H
#define GLOBAL_SETUP_WINDOW_H


#include <Window.h>


class BTabView;


class GlobalSetupWindow : public BWindow {
public:
	static	void					ShowGlobalSetupWindow();
	static	void					CloseGlobalSetupWindow();

	virtual void					Show();
	virtual	void					MessageReceived(BMessage* message);

private:
									GlobalSetupWindow(const BPoint& leftTop);
	virtual							~GlobalSetupWindow();

private:
	class	WindowFeelView;
			WindowFeelView*			fWindowFeelView;

	class	UndoControlView;
			UndoControlView*		fUndoControlView;

	class	LanguageControlView;
			LanguageControlView*	fLanguageControlView;

	class	GeneralControlView;
			GeneralControlView*		fGeneralControlView;

			BTabView*				fTabView;
	static	GlobalSetupWindow*		fSetupWindow;
};


#endif
