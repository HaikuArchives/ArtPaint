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
#ifndef DATATYPE_SETUP_WINDOW_H
#define DATATYPE_SETUP_WINDOW_H

#include <TranslatorRoster.h>
#include <Window.h>


class DatatypeSetupWindow : public BWindow {
public:
	static	void					ShowWindow(translator_id translatorId);
	static	void					ChangeHandler(translator_id translatorId);

private:
									DatatypeSetupWindow();
	virtual							~DatatypeSetupWindow();

			void					_ChangeHandler(translator_id translatorId);

private:
			BView*					fRootView;
	static	DatatypeSetupWindow*	fDatatypeSetupWindow;
};

#endif
