/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "FilePanels.h"

#include "MessageConstants.h"
#include "StringServer.h"

#include <Button.h>
#include <Catalog.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <String.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>
#include <TranslationUtils.h>
#include <View.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FilePanels"


ImageSavePanel::ImageSavePanel(const entry_ref& startDir, BMessenger& target,
		BMessage& message, int32 saveFormat, BBitmap* savedBitmap)
	: BFilePanel(B_SAVE_PANEL, &target, &startDir, 0, false, &message)
{
	if (Window()->Lock()) {
		BString title = "ArtPaint: ";
		title.Append(B_TRANSLATE("Save image"));
		Window()->SetTitle(title.String());

		BView* textView = Window()->FindView("text view");
		textView->MoveBy(0.0, 20.0);
		float width = textView->Bounds().Width() + 10.0;

		BView* root = Window()->ChildAt(0);
		BView* tmp = root->FindView("default button");
		tmp->MoveBy(0.0, 20.0);
		width += tmp->Bounds().Width() + 10.0;

		BView* cancelButton = root->FindView("cancel button");
		cancelButton->MoveBy(0.0, 20.0);
		width += cancelButton->Bounds().Width() + 10.0;

		int32 count = root->CountChildren();
		for (int32 i = 0; i < count; ++i) {
			BView* child = root->ChildAt(i);
			if ((child->ResizingMode() & B_FOLLOW_BOTTOM)
				|| !(child->ResizingMode() & B_FOLLOW_TOP_BOTTOM))
				child->MoveBy(0.0, -20.0);
			else if (child->ResizingMode() & B_FOLLOW_TOP_BOTTOM)
				child->ResizeBy(0.0, -20.0);
		}

		if (cancelButton) {
			BButton* settingsButton = new BButton(cancelButton->Frame(),
				"settings button", B_TRANSLATE("Settings…"),
				new BMessage(HS_SHOW_DATATYPE_SETTINGS), B_FOLLOW_RIGHT |
				B_FOLLOW_BOTTOM);
			root->AddChild(settingsButton);

			settingsButton->SetTarget(target);
			settingsButton->ResizeToPreferred();
			settingsButton->MoveBy(-(settingsButton->Bounds().Width() + 10.0),
				0.0);
			width += settingsButton->Bounds().Width() + 10.0;
		}
		Window()->ResizeTo(width + 20.0, Window()->Bounds().Height());

		// this menu sends a message to the window that requested saving
		// and tells it if the user changed the format to save the target

		// TODO: translation
		BPopUpMenu* formatMenu = new BPopUpMenu("Choose Format");
		BMessage message(HS_SAVE_FORMAT_CHANGED);
		BTranslationUtils::AddTranslationItems(formatMenu, B_TRANSLATOR_BITMAP,
			&message, NULL, NULL, NULL);

		int32 type;
		bool typeFound = false;
		count = formatMenu->CountItems();
		for (int32 i = 0; i < count; ++i) {
			if (BMenuItem* item = formatMenu->ItemAt(i)) {
				item->Message()->FindInt32("be:type", (int32*)&type);
				if (type == saveFormat) {
					typeFound = true;
					item->SetMarked(true);
					break;
				}
			}
		}

		if (!typeFound) {
			if (BMenuItem* item = formatMenu->ItemAt(0)) {
				item->SetMarked(true);
				target.SendMessage(item->Message());
			}
		}
		formatMenu->SetTargetForItems(target);

		const char* string = B_TRANSLATE("Save format");
		BMenuField* menuField = new BMenuField(BRect(textView->Frame().LeftTop() +
			BPoint(0.0, -25.0), textView->Frame().RightTop() + BPoint(200, -5)),
			"menu field", string, formatMenu, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
		root->AddChild(menuField);
		menuField->SetDivider(menuField->StringWidth(string) + 5.0);

		Window()->Unlock();
	}
}


ImageSavePanel::~ImageSavePanel()
{
}


status_t
set_filepanel_strings(BFilePanel *panel)
{
	if (!panel)
		return B_ERROR;

	BWindow* window = panel->Window();
	if (!window)
		return B_ERROR;

	if (window->Lock()) {
		BButton* button =
				dynamic_cast<BButton*>(window->FindView("default button"));

		if (button) {
			if (strcmp(button->Label(), "Save") == 0)
				button->SetLabel(B_TRANSLATE("Save"));

			if (strcmp(button->Label(), "Open") == 0)
				button->SetLabel(B_TRANSLATE("Open"));
		}

		button = dynamic_cast<BButton*>(window->FindView("cancel button"));
		if (button) {
			if (strcmp(button->Label(), "Cancel") == 0)
				button->SetLabel(B_TRANSLATE("Cancel"));
		}
		window->Unlock();
	}
	return B_OK;
}
