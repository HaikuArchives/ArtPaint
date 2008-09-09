/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef DATATYPE_SETUP_WINDOW_H
#define DATATYPE_SETUP_WINDOW_H

#include <TranslatorRoster.h>
#include <Window.h>

class BWindow;
class DatatypeSetupWindow : public BWindow {

static	BWindow	*setup_window;
		BView	*container;


				DatatypeSetupWindow();
				~DatatypeSetupWindow();
public:
static	void	ChangeHandler(translator_id);
static	void	showWindow(translator_id);
};

#endif
