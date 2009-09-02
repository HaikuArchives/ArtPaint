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

#include "ManipulatorWindow.h"

#include "ColorPalette.h"
#include "MessageConstants.h"
#include "MessageFilters.h"
#include "SettingsServer.h"
#include "StringServer.h"


#include <Button.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <SeparatorView.h>


BList ManipulatorWindow::sfWindowList(10);
sem_id ManipulatorWindow::sfWindowListMutex = create_sem(1, "list_mutex");


ManipulatorWindow::ManipulatorWindow(BRect rect, BView* view, const char* name,
		BWindow* master, BMessenger* target)
	: BWindow(rect, name, B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_ANCHORED_ON_ACTIVATE | B_NOT_CLOSABLE
		| B_AUTO_UPDATE_SIZE_LIMITS)
	, fManipulatorView(view)
{
	window_feel feel = B_NORMAL_WINDOW_FEEL;
	if (SettingsServer* server = SettingsServer::Instance()) {
		BMessage settings;
		server->GetApplicationSettings(&settings);
		settings.FindInt32(skAddOnWindowFeel, (int32*)&feel);
	}
	SetFeel(feel);

	window_look look = B_FLOATING_WINDOW_LOOK;
	if (feel == B_NORMAL_WINDOW_FEEL)
		look = B_TITLED_WINDOW_LOOK;
	SetLook(look);

	AddToSubset(master);

	acquire_sem(sfWindowListMutex);
	sfWindowList.AddItem(this);
	release_sem(sfWindowListMutex);

	// TODO: remove this once BSeparatorView works properly
	BSeparatorView* separator = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	separator->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BMessage* okMessage = new BMessage(HS_MANIPULATOR_FINISHED);
	okMessage->AddBool("status", true);
	BButton* okButton = new BButton(StringServer::ReturnString(OK_STRING),
		okMessage);

	BMessage* cancelMessage = new BMessage(HS_MANIPULATOR_FINISHED);
	cancelMessage->AddBool("status", true);
	BButton* cancelButton =
		new BButton(StringServer::ReturnString(CANCEL_STRING), cancelMessage);

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10.0)
		.Add(fManipulatorView)
		.Add(separator)
		.AddGroup(B_HORIZONTAL, 10.0)
			.AddGlue()
			.Add(okButton)
			.Add(cancelButton)
		.End()
		.SetInsets(10.0, 10.0, 10.0, 10.0)
	);

	okButton->MakeDefault(true);
	okButton->SetTarget(*target);
	cancelButton->SetTarget(*target);

	ColorPaletteWindow::AddMasterWindow(this);

	Show();
	Lock();
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN,AppKeyFilterFunction));
	Unlock();
}



ManipulatorWindow::~ManipulatorWindow()
{
	acquire_sem(sfWindowListMutex);
	sfWindowList.RemoveItem(this);
	release_sem(sfWindowListMutex);

	// The manipulator will delete this.
	fManipulatorView->RemoveSelf();

	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skAddOnWindowFrame,
			Frame());
	}

	ColorPaletteWindow::RemoveMasterWindow(this);
}



void
ManipulatorWindow::setFeel(window_feel feel)
{
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skAddOnWindowFeel,
			int32(feel));
	}

	window_look look = B_FLOATING_WINDOW_LOOK;
	if (feel == B_NORMAL_WINDOW_FEEL)
		look = B_TITLED_WINDOW_LOOK;

	acquire_sem(sfWindowListMutex);

	for (int32 i = 0; i < sfWindowList.CountItems(); ++i) {
		BWindow *window = static_cast<BWindow*> (sfWindowList.ItemAt(i));
		window->SetFeel(feel);
		window->SetLook(look);
	}

	release_sem(sfWindowListMutex);
}
