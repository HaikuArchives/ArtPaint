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
#ifndef HS_PICTURE_BUTTON_H
#define HS_PICTURE_BUTTON_H

#include <PictureButton.h>
#include <String.h>


class HSPictureButton : public BPictureButton {
public:
						HSPictureButton(BRect rect, BPicture* off,
							BPicture* on, BMessage* message,
							const char* helpMessage = NULL,
							const char* longHelpMessage = NULL,
							uint32 behavior = B_ONE_STATE_BUTTON,
							uint32 mode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
							uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
						HSPictureButton(BRect rect, BBitmap* off,
							BBitmap* on, BMessage* message,
							const char* helpMessage = NULL,
							const char* longHelpMessage = NULL,
							uint32 behavior = B_ONE_STATE_BUTTON,
							uint32 mode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
							uint32 flags = B_WILL_DRAW | B_NAVIGABLE);

	virtual	void		MouseDown(BPoint location);
	virtual	void		MouseMoved(BPoint point, uint32 transit, const BMessage* message);

private:
			BString		fLongHelpMessage;
};

#endif // HS_PICTURE_BUTTON_H
