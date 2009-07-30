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

#include "ToolSetupWindow.h"

#include "DrawingTools.h"
#include "FloaterManager.h"
#include "MessageFilters.h"
#include "SettingsServer.h"
#include "StringServer.h"
#include "ToolManager.h"
#include "UtilityClasses.h"


#include <GroupLayout.h>
#include <StringView.h>


ToolSetupWindow* ToolSetupWindow::fToolSetupWindow = NULL;


ToolSetupWindow::ToolSetupWindow(BRect frame)
	: BWindow(frame, StringServer::ReturnString(TOOL_SETUP_STRING),
		B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE |
		B_NOT_V_RESIZABLE | B_WILL_ACCEPT_FIRST_CLICK | B_AVOID_FRONT |
		B_AUTO_UPDATE_SIZE_LIMITS)
	, fCurrentTool(-1)
{
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
	SetLayout(layout);

	layout->AddView(fContainer = new BView("", 0, new BGroupLayout(B_VERTICAL)));
	layout->SetInsets(10.0, 10.0, 10.0, 10.0);
	layout->View()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	window_feel feel = B_NORMAL_WINDOW_FEEL;
	SettingsServer* server = SettingsServer::Instance();
	if (server) {
		BMessage settings;
		server->GetApplicationSettings(&settings);
		settings.FindInt32(skToolSetupWindowFeel, (int32*)&feel);
	}
	SetWindowFeel(feel);

	if (Lock()) {
		AddCommonFilter(new BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE,
			B_MOUSE_DOWN, window_activation_filter));
		AddCommonFilter(new BMessageFilter(B_KEY_DOWN, AppKeyFilterFunction));
		Unlock();
	}

	Show();

	fToolSetupWindow = this;
	FloaterManager::AddFloater(this);

	if (server) {
		server->SetValue(SettingsServer::Application, skToolSetupWindowVisible,
			true);
	}
}


ToolSetupWindow::~ToolSetupWindow()
{
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skToolSetupWindowVisible,
			false);
		server->SetValue(SettingsServer::Application, skToolSetupWindowFrame,
			Frame());
	}

	fToolSetupWindow = NULL;
	FloaterManager::RemoveFloater(this);
}


void
ToolSetupWindow::ShowToolSetupWindow(int32 toolType)
{
	if (fToolSetupWindow == NULL) {
		BRect frame(70, 31, 300, 72);
		if (SettingsServer* server = SettingsServer::Instance()) {
			BMessage settings;
			server->GetApplicationSettings(&settings);
			settings.FindRect(skToolSetupWindowFrame, &frame);
		}
		new ToolSetupWindow(frame);
	}

	if (fToolSetupWindow->Lock()) {
		fToolSetupWindow->SetWorkspaces(B_CURRENT_WORKSPACE);
		if (fToolSetupWindow->IsHidden())
			fToolSetupWindow->Show();

		if (!fToolSetupWindow->IsActive() && !fToolSetupWindow->IsFront())
			fToolSetupWindow->Activate(true);

		fToolSetupWindow->Unlock();
	}

	BRect newRect = FitRectToScreen(fToolSetupWindow->Frame());
	fToolSetupWindow->MoveTo(newRect.LeftTop());

	if (toolType == 0)
		toolType = FREE_LINE_TOOL;

	if (fToolSetupWindow->Lock()) {
		fToolSetupWindow->_UpdateConfigurationView(toolType);
		fToolSetupWindow->Unlock();
	}
}


void
ToolSetupWindow::SetWindowFeel(window_feel feel)
{
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skToolSetupWindowFeel,
			int32(feel));
	}

	if (fToolSetupWindow) {
		fToolSetupWindow->SetFeel(feel);
		fToolSetupWindow->SetLook((feel == B_NORMAL_WINDOW_FEEL
			? B_TITLED_WINDOW_LOOK : B_FLOATING_WINDOW_LOOK));
	}
}


void
ToolSetupWindow::CurrentToolChanged(int32 newTool)
{
	if (fToolSetupWindow && fToolSetupWindow->Lock()) {
		fToolSetupWindow->_UpdateConfigurationView(newTool);
		fToolSetupWindow->Unlock();
	}
}


void ToolSetupWindow::_UpdateConfigurationView(int32 newTool)
{
	if (newTool != fCurrentTool) {
		fCurrentTool = newTool;

		// Remove previous tool config views.
		while (BView* oldConfigView = fContainer->ChildAt(0)) {
			oldConfigView->RemoveSelf();
			delete oldConfigView;
		}

		BView* configView = tool_manager->ReturnConfigurationView(fCurrentTool);
		if (configView == NULL) {
			// TODO: translation
			configView = new BStringView("", "No configuration available.");
		}
		fContainer->AddChild(configView);
	}
}
