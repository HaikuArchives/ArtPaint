/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "DatatypeSetupWindow.h"

#include "MessageFilters.h"


#include <AppDefs.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <String.h>
#include <StringView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Windows"


DatatypeSetupWindow* DatatypeSetupWindow::fDatatypeSetupWindow = NULL;

DatatypeSetupWindow::DatatypeSetupWindow()
	: BWindow(BRect(100.0, 120.0, 120.0, 120.0)," datatype setup",
		B_FLOATING_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
	SetLayout(layout);

	layout->AddView(fRootView = new BView("", 0, new BGroupLayout(B_VERTICAL)));
	layout->SetInsets(10.0, 10.0, 10.0, 10.0);
	layout->View()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	if (Lock()) {
		AddCommonFilter(new BMessageFilter(B_KEY_DOWN, AppKeyFilterFunction));
		Unlock();
	}

	fDatatypeSetupWindow = this;
}


DatatypeSetupWindow::~DatatypeSetupWindow()
{
	fDatatypeSetupWindow = NULL;
}


void
DatatypeSetupWindow::ShowWindow(translator_id translatorId)
{
	if (fDatatypeSetupWindow) {
		if (fDatatypeSetupWindow->IsHidden())
			fDatatypeSetupWindow->Show();

		if (!fDatatypeSetupWindow->IsActive())
			fDatatypeSetupWindow->Activate(true);
	} else {
		fDatatypeSetupWindow = new DatatypeSetupWindow();
		fDatatypeSetupWindow->Show();
	}

	// Also change the handler
	ChangeHandler(translatorId);
}


void
DatatypeSetupWindow::ChangeHandler(translator_id translatorId)
{
	if (fDatatypeSetupWindow && fDatatypeSetupWindow->Lock()) {
		fDatatypeSetupWindow->_ChangeHandler(translatorId);
		fDatatypeSetupWindow->Unlock();
	}
}


void
DatatypeSetupWindow::_ChangeHandler(translator_id translatorId)
{
	BView* view = fRootView->ChildAt(0);
	if (view) {
		view->RemoveSelf();
		delete view;
	}

	BRect rect;
	view = NULL;
	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (roster->MakeConfigurationView(translatorId, NULL, &view, &rect) == B_OK) {
		if (view)
			view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	} else {
		view = new BStringView("no options",
			B_TRANSLATE("No options"));
	}
	fRootView->AddChild(view);

	int32 version;
	const char* name;
	const char* info;
	BString windowTitle("Datatype Setup: ");	// TODO: translation
	if (roster->GetTranslatorInfo(translatorId, &name, &info,&version) == B_OK)
		windowTitle.Append(name);
	else
		windowTitle.Append("No translator.");
	fDatatypeSetupWindow->SetTitle(windowTitle.String());
}
