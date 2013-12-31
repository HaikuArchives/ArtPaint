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

#include "PaintWindowMenuItem.h"

#include "MessageConstants.h"
#include "PaintWindow.h"


PaintWindowMenuItem::PaintWindowMenuItem(const char* label, BMessage* message,
		char shortcut, uint32 modifiers, PaintWindow* window, const char* help)
	: BMenuItem(label, message, shortcut, modifiers)
	, fHelpMessage(help)
	, fPaintWindow(window)
{
}


PaintWindowMenuItem::~PaintWindowMenuItem()
{
}


void PaintWindowMenuItem::Highlight(bool highlighted)
{
	BMenuItem::Highlight(highlighted);

	if (fPaintWindow && fPaintWindow->Lock()) {
		if (highlighted) {
			fPaintWindow->SetHelpString(fHelpMessage.String(),
				HS_TEMPORARY_HELP_MESSAGE);
		} else {
			fPaintWindow->SetHelpString("", HS_TEMPORARY_HELP_MESSAGE);
		}
		fPaintWindow->Unlock();
	}
}
