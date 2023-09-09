/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef POP_UP_LIST_H
#define POP_UP_LIST_H

#include <View.h>
#include <PopUpMenu.h>

// This class uses a BView to control a pop-up menu. The invoked menu-item will
// then send a message to the desired target. The messages for menu-items are taken from
// constructor's message-list. The message should also contain a string named
// 'label'. That will be used as the label for menuitem.
class PopUpList : public BView {
		BBitmap*	pushed;
		BBitmap*	not_pushed;

		BBitmap*	current_bitmap;
		BMessenger	fTarget;
		BPopUpMenu*	the_menu;

public:
					PopUpList(BRect,BBitmap*, BBitmap*, const BMessage& list, int32 count,
						const BMessenger& target);
					PopUpList(BRect,BBitmap*, BBitmap*, BPopUpMenu*, BMessenger*);
					~PopUpList();

		void		Draw(BRect);
		void		MouseDown(BPoint);

		BMenu*		ReturnMenu() { return the_menu; }
};


#endif // POP_UP_LIST_H
