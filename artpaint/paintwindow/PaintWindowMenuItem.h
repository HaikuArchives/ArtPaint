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
#ifndef PAINT_WINDOW_MENU_ITEM_H
#define PAINT_WINDOW_MENU_ITEM_H

#include <MenuItem.h>
#include <String.h>

class PaintWindow;


class PaintWindowMenuItem : public BMenuItem {
public:
						PaintWindowMenuItem(const char* label, BMessage* message,
							char shortcut = 0, uint32 modifiers = 0,
							PaintWindow* pw = NULL, const char* help = NULL);
	virtual				~PaintWindowMenuItem();

	virtual	void		Highlight(bool highlighted);

private:
		BString			fHelpMessage;
		PaintWindow*	fPaintWindow;
};

#endif
