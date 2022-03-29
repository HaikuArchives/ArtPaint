/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <MenuItem.h>
#include <PopUpMenu.h>


#include "PopUpList.h"


PopUpList::PopUpList(BRect frame, BBitmap *down, BBitmap *up,
		const BMessage& list, int32 messageCount, const BMessenger& target)
	: BView("pop-up list",B_WILL_DRAW)
	, pushed(down)
	, not_pushed(up)
	, current_bitmap(up)
	, fTarget(target)
{
	// Here create the menu that we control
	the_menu = new BPopUpMenu("pop-up list menu", false, false);
	for (int32 i = 0; i < messageCount; ++i) {
		BMessage* message = new BMessage;
		list.FindMessage("list", i, message);
		BMenuItem* menuItem = new BMenuItem(message->FindString("label"), message);
		the_menu->AddItem(menuItem);
	}
	the_menu->SetTargetForItems(fTarget);
}


PopUpList::PopUpList(BRect frame,BBitmap *down,BBitmap *up,BPopUpMenu *menu,BMessenger *targ)
	:	BView("pop-up list",B_WILL_DRAW)
{
	pushed = down;
	current_bitmap = not_pushed = up;
	fTarget = *targ;

	the_menu = menu;
	the_menu->SetTargetForItems(fTarget);
}


PopUpList::~PopUpList()
{
	delete the_menu;
	delete pushed;
	delete not_pushed;
}


void PopUpList::Draw(BRect area)
{
	DrawBitmap(current_bitmap, area, area);
}


void PopUpList::MouseDown(BPoint point)
{
	current_bitmap = pushed;
	Draw(Bounds());

	BMenuItem *item = the_menu->Go(ConvertToScreen(point));
	if (item != NULL) {
		fTarget.SendMessage(item->Message());
	}
	current_bitmap = not_pushed;
	Draw(Bounds());
}
