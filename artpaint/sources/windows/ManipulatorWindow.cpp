/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */

#include "ManipulatorWindow.h"

#include "ColorPalette.h"
#include "MessageConstants.h"
#include "MessageFilters.h"
#include "SettingsServer.h"
#include "StringServer.h"


#include <Button.h>


BList* ManipulatorWindow::window_list = new BList();
sem_id ManipulatorWindow::list_mutex = create_sem(1, "list_mutex");


ManipulatorWindow::ManipulatorWindow(BRect rect, BView *view, const char *name,
		BWindow *master, BMessenger *t)
	: BWindow(rect ,name, B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_ANCHORED_ON_ACTIVATE
		| B_NOT_CLOSABLE)
{
	window_feel feel = B_NORMAL_WINDOW_FEEL;
	if (SettingsServer* server = SettingsServer::Instance()) {
		BMessage settings;
		server->GetApplicationSettings(&settings);
		settings.FindInt32(skAddOnWindowFeel, (int32*)&feel);
	}

	window_look look = B_FLOATING_WINDOW_LOOK;
	if (feel == B_NORMAL_WINDOW_FEEL)
		look = B_TITLED_WINDOW_LOOK;

	SetFeel(feel);
	SetLook(look);

	AddToSubset(master);

	manipulator_view = view;

	acquire_sem(list_mutex);
	window_list->AddItem(this);
	release_sem(list_mutex);

	// Here create the background-view
	ManipulatorWindowBackgroundView* bg_view = new ManipulatorWindowBackgroundView(view->Bounds());
	bg_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Create the ok and cancel-buttons
	BButton *ok_button;
	BButton *cancel_button;

	BMessage *ok_message = new BMessage(HS_MANIPULATOR_FINISHED);
	ok_message->AddBool("status",TRUE);
	ok_button = new BButton(BRect(0,0,0,0),"ok_button",StringServer::ReturnString(OK_STRING),ok_message);
	ok_button->MakeDefault(true);
	ok_button->ResizeToPreferred();


	BMessage *cancel_message = new BMessage(HS_MANIPULATOR_FINISHED);
	cancel_message->AddBool("status",FALSE);
	cancel_button = new BButton(BRect(0,0,0,0),"cancel_button",StringServer::ReturnString(CANCEL_STRING),cancel_message);
	cancel_button->ResizeToPreferred();


	// Resize the background-view
	bg_view->ResizeBy(0,ok_button->Frame().Height()+21);
	bg_view->ResizeTo(max_c(2*ok_button->Frame().Width()+100,view->Frame().Width()),bg_view->Frame().Height());

	// Position the buttons and the view
	ok_button->MoveTo(bg_view->Bounds().right - ok_button->Frame().Width() - 10,bg_view->Bounds().bottom-ok_button->Frame().Height()-10);
	cancel_button->MoveTo(ok_button->Frame().left-cancel_button->Frame().Width()-10,ok_button->Frame().top+(ok_button->Frame().bottom - ok_button->Frame().top)/2-cancel_button->Frame().Height()/2);
	view->MoveTo((bg_view->Bounds().Width()-view->Bounds().Width())/2,0);

	ResizeTo(bg_view->Bounds().Width(),bg_view->Bounds().Height());
	bg_view->MoveTo(0,0);
	bg_view->AddChild(view);
	bg_view->AddChild(ok_button);
	bg_view->AddChild(cancel_button);

	ok_button->SetTarget(*t);
	cancel_button->SetTarget(*t);

	AddChild(bg_view);

	ColorPaletteWindow::AddMasterWindow(this);
	Show();
	Lock();
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN,AppKeyFilterFunction));
	Unlock();
}



ManipulatorWindow::~ManipulatorWindow()
{
	acquire_sem(list_mutex);
	window_list->RemoveItem(this);
	release_sem(list_mutex);
	manipulator_view->RemoveSelf();	// The manipulator will delete this.

	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skAddOnWindowFrame,
			Frame());
	}

	ColorPaletteWindow::RemoveMasterWindow(this);
}



void ManipulatorWindow::setFeel(window_feel feel)
{
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skAddOnWindowFeel,
			int32(feel));
	}

	window_look look = B_FLOATING_WINDOW_LOOK;
	if (feel == B_NORMAL_WINDOW_FEEL)
		look = B_TITLED_WINDOW_LOOK;

	acquire_sem(list_mutex);

	for (int32 i = 0; i < window_list->CountItems(); ++i) {
		BWindow *window = static_cast<BWindow*> (window_list->ItemAt(i));
		window->SetFeel(feel);
		window->SetLook(look);
	}

	release_sem(list_mutex);
}
