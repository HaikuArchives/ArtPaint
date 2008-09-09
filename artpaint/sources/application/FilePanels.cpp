/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Button.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <stdio.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>
#include <TranslationUtils.h>
#include <View.h>
#include <Window.h>

#include "FilePanels.h"
#include "MessageConstants.h"
#include "StringServer.h"

ImageSavePanel::ImageSavePanel(entry_ref *directory, BMessenger *target,int32 save_format, BBitmap *saved_bitmap)
		: BFilePanel(B_SAVE_PANEL,target,directory,0,false,new BMessage(HS_IMAGE_SAVE_REFS))
{
	Window()->Lock();

	// here set a proper title for this window
	char text[256];
	sprintf(text,"ArtPaint: %s",StringServer::ReturnString(SAVE_IMAGE_STRING));
	Window()->SetTitle(text);

	BView *text_view = Window()->FindView("text view");
	text_view->MoveBy(0,20);

	Window()->FindView("cancel button")->MoveBy(0,20);
	Window()->FindView("default button")->MoveBy(0,20);
	BView *child;

	for (int32 i=0;i<Window()->ChildAt(0)->CountChildren();i++) {
		child = Window()->ChildAt(0)->ChildAt(i);
		if ((child->ResizingMode() & B_FOLLOW_BOTTOM) || !(child->ResizingMode() & B_FOLLOW_TOP_BOTTOM))
			child->MoveBy(0,-20);
		else if (child->ResizingMode() & B_FOLLOW_TOP_BOTTOM)
			child->ResizeBy(0,-20);
	}

	// Add a button to show the datatype's settings.
	BView *cancel_button = Window()->ChildAt(0)->FindView("cancel button");
	if (cancel_button) {
		sprintf(text,"%s",StringServer::ReturnString(SETTINGS_STRING));
		float string_width = cancel_button->StringWidth(text);

		BRect button_rect = cancel_button->Frame();
		button_rect.right = button_rect.left -10;
		button_rect.left = button_rect.right - string_width - 20;

		BButton *settings_button = new BButton(button_rect,"settings button",text,new BMessage(HS_SHOW_DATATYPE_SETTINGS),B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);

		settings_button->SetTarget(*target);

		cancel_button->Parent()->AddChild(settings_button);
	}

	// this menu sends a message to the window that requested saving
	// and tells it what format is currently chosen
	// when the panel is invoked the window can save the image
	// in the right format
	BPopUpMenu *format_menu = new BPopUpMenu("pop_up_menu");
	BMessage *model_message = new BMessage(HS_SAVE_FORMAT_CHANGED);
	BTranslationUtils::AddTranslationItems(format_menu,B_TRANSLATOR_BITMAP,model_message,NULL,NULL,NULL);
	delete model_message;

	bool type_found = FALSE;
	if (format_menu->ItemAt(0) != NULL) {
		format_menu->ItemAt(0)->SetMarked(TRUE);
		for (int32 i=0;i<format_menu->CountItems();i++) {
			BMenuItem *item = format_menu->ItemAt(i);
			if (item != NULL) {
				int32 type;
				item->Message()->FindInt32("be:type",(int32*)&type);
				item->Message()->PrintToStream();
				if (type == save_format) {
					item->SetMarked(TRUE);
					type_found = TRUE;
				}
			}
		}
	}
	if (!type_found) {
		if (format_menu->ItemAt(0) != NULL) {
			format_menu->ItemAt(0)->SetMarked(TRUE);
			if (target != NULL) {
				target->SendMessage(new BMessage(*format_menu->ItemAt(0)->Message()));
			}
		}
	}
	format_menu->SetTargetForItems(*target);
	sprintf(text,"%s:",StringServer::ReturnString(SAVE_FORMAT_STRING));
	BMenuField *menu_field = new BMenuField(BRect(text_view->Frame().LeftTop() + BPoint(0,-25),text_view->Frame().RightTop() + BPoint(200,-5)),"a menu field",text,format_menu,B_FOLLOW_LEFT|B_FOLLOW_BOTTOM);
	menu_field->SetDivider(menu_field->StringWidth(text) + 5);

	Window()->ChildAt(0)->AddChild(menu_field);

	Window()->Unlock();
}

ImageSavePanel::~ImageSavePanel()
{
}
