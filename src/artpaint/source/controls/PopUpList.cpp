/* 

	Filename:	PopUpList.cpp
	Contents:	PopUpList-class definitions.	
	Author:		Heikki Suhonen
	
*/

#include <Bitmap.h>
#include <MenuItem.h>
#include <PopUpMenu.h>


#include "PopUpList.h"


PopUpList::PopUpList(BRect frame,BBitmap *down,BBitmap *up,BMessage **message_list,int32 message_count,BMessenger *targ)
	:	BView(frame,"pop-up list",B_FOLLOW_LEFT|B_FOLLOW_TOP,B_WILL_DRAW)
{
	pushed = down;
	current_bitmap = not_pushed = up;
	target = targ;
	
	// Here create the menu that we control	
	the_menu = new BPopUpMenu("pop-up list menu",FALSE,FALSE);
	for (int32 i=0;i<message_count;i++) {
		BMenuItem *menu_item;
		menu_item = new BMenuItem(message_list[i]->FindString("label"),message_list[i]);
		the_menu->AddItem(menu_item);
	}
	the_menu->SetTargetForItems(*target);
}


PopUpList::PopUpList(BRect frame,BBitmap *down,BBitmap *up,BPopUpMenu *menu,BMessenger *targ)
	:	BView(frame,"pop-up list",B_FOLLOW_LEFT|B_FOLLOW_TOP,B_WILL_DRAW)
{
	pushed = down;
	current_bitmap = not_pushed = up;
	target = targ;
	
	the_menu = menu;
	the_menu->SetTargetForItems(*target);
}


PopUpList::~PopUpList()
{
	delete the_menu;
	delete pushed;
	delete not_pushed;
	delete target;
}

void PopUpList::Draw(BRect area)
{
	DrawBitmap(current_bitmap,area,area);
}


void PopUpList::MouseDown(BPoint point)
{
	current_bitmap = pushed;
	Draw(Bounds());

	BMenuItem *item = the_menu->Go(ConvertToScreen(point));
	if (item != NULL) {
		target->SendMessage(item->Message());	
	}
	current_bitmap = not_pushed;
	Draw(Bounds());
}