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

#include "GlobalSetupWindow.h"

#include "BrushStoreWindow.h"
#include "Cursors.h"
#include "ColorPalette.h"
#include "LayerWindow.h"
#include "ManipulatorWindow.h"
#include "NumberControl.h"
#include "SettingsServer.h"
#include "ToolSelectionWindow.h"
#include "ToolSetupWindow.h"
#include "UndoQueue.h"
#include "UtilityClasses.h"


#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <GroupLayout.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>
#include <TabView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Windows"


using ArtPaint::Interface::NumberControl;


const uint32 kCloseAndApplySettings				= 'caas';
const uint32 kCloseAndDiscardSettings			= 'cads';

const uint32 kColorWindowFeelChanged			= '_col';
const uint32 kToolSelectionWindowFeelChanged	= '_sel';
const uint32 kToolSetupWindowFeelChanged		= '_set';
const uint32 kLayerWindowFeelChanged			= '_lay';
const uint32 kBrushWindowFeelChanged			= '_bru';
const uint32 kEffectsWindowFeelChanged			= '_eff';

const uint32 kSetUndoDisabled					= '_sud';
const uint32 kSetUnlimitedUndo					= '_suu';
const uint32 kSetAdjustableUndo					= '_sau';
const uint32 kUndoDepthAdjusted					= '_uda';

const uint32 kToolCursorMode					= '_too';
const uint32 kCrossHairCursorMode				= '_cro';
const uint32 kConfirmShutdownChanged			= '_con';

GlobalSetupWindow* GlobalSetupWindow::fSetupWindow = NULL;


class GlobalSetupWindow::WindowFeelView : public BView {
public:
						WindowFeelView();
	virtual				~WindowFeelView() {}

	virtual	void		AttachedToWindow();
	virtual	void		MessageReceived(BMessage* message);

			void		ApplyChanges();

private:
		window_feel		fToolWindowFeel;
		window_feel		fToolSetupWindowFeel;
		window_feel		fPaletteWindowFeel;
		window_feel		fBrushWindowFeel;
		window_feel		fLayerWindowFeel;
		window_feel		fAddOnWindowFeel;

		BCheckBox*		fColor;
		BCheckBox*		fSelection;
		BCheckBox*		fSetup;
		BCheckBox*		fLayer;
		BCheckBox*		fBrush;
		BCheckBox*		fEffects;
};


GlobalSetupWindow::WindowFeelView::WindowFeelView()
	:	BView("window feel view", 0)
	, fToolWindowFeel(B_NORMAL_WINDOW_FEEL)
	, fToolSetupWindowFeel(B_NORMAL_WINDOW_FEEL)
	, fPaletteWindowFeel(B_NORMAL_WINDOW_FEEL)
	, fBrushWindowFeel(B_NORMAL_WINDOW_FEEL)
	, fLayerWindowFeel(B_NORMAL_WINDOW_FEEL)
	, fAddOnWindowFeel(B_NORMAL_WINDOW_FEEL)
{
	if (SettingsServer* server = SettingsServer::Instance()) {
		BMessage settings;
		server->GetApplicationSettings(&settings);

		settings.FindInt32(skLayerWindowFeel, (int32*)&fLayerWindowFeel);
		settings.FindInt32(skToolSetupWindowFeel, (int32*)&fToolSetupWindowFeel);
		settings.FindInt32(skSelectToolWindowFeel, (int32*)&fToolWindowFeel);
		settings.FindInt32(skPaletteWindowFeel, (int32*)&fPaletteWindowFeel);
		settings.FindInt32(skBrushWindowFeel, (int32*)&fBrushWindowFeel);
		settings.FindInt32(skAddOnWindowFeel, (int32*)&fAddOnWindowFeel);
	}

	fColor = new BCheckBox(B_TRANSLATE("Colors"),
		new BMessage(kColorWindowFeelChanged));
	fColor->SetValue(fPaletteWindowFeel == B_FLOATING_APP_WINDOW_FEEL);

	fSelection =
		new BCheckBox(B_TRANSLATE("Tools"),
		new BMessage(kToolSelectionWindowFeelChanged));
	fSelection->SetValue(fToolWindowFeel == B_FLOATING_APP_WINDOW_FEEL);

	fSetup = new BCheckBox(B_TRANSLATE("Tool setup"),
		new BMessage(kToolSetupWindowFeelChanged));
	fSetup->SetValue(fToolSetupWindowFeel == B_FLOATING_APP_WINDOW_FEEL);

	fLayer = new BCheckBox(B_TRANSLATE("Layers"),
		new BMessage(kLayerWindowFeelChanged));
	fLayer->SetValue(fLayerWindowFeel == B_FLOATING_APP_WINDOW_FEEL);

	fBrush = new BCheckBox(B_TRANSLATE("Brushes"),
		new BMessage(kBrushWindowFeelChanged));
	fBrush->SetValue(fBrushWindowFeel == B_FLOATING_APP_WINDOW_FEEL);

	fEffects = new BCheckBox(B_TRANSLATE("Add-ons"),
		new BMessage(kEffectsWindowFeelChanged));
	fEffects->SetValue(fAddOnWindowFeel == B_FLOATING_SUBSET_WINDOW_FEEL);

	BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
	SetLayout(layout);

	BBox* box = new BBox(B_NO_BORDER, BGridLayoutBuilder(10.0, 5.0)
		.Add(fColor, 0, 0)
		.Add(fSelection, 0, 1)
		.Add(fSetup, 0, 2)
		.Add(fLayer, 1, 0)
		.Add(fBrush, 1, 1)
		.Add(fEffects, 1, 2)
		.Add(BSpaceLayoutItem::CreateGlue(), 2, 0)
		.SetInsets(20.0, 5.0, 10.0, 10.0)
		.View()
	);
	box->SetLabel(B_TRANSLATE("Keep in front"));

	layout->AddView(box);
	layout->AddItem(BSpaceLayoutItem::CreateGlue());
	layout->SetInsets(10.0, 10.0, 10.0, 10.0);
}


void
GlobalSetupWindow::WindowFeelView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	fColor->SetTarget(this);
	fSelection->SetTarget(this);
	fSetup->SetTarget(this);
	fLayer->SetTarget(this);
	fBrush->SetTarget(this);
	fEffects->SetTarget(this);
}


void
GlobalSetupWindow::WindowFeelView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kColorWindowFeelChanged: {
			if (fColor->Value() == B_CONTROL_OFF)
				fPaletteWindowFeel = B_NORMAL_WINDOW_FEEL;
			else
				fPaletteWindowFeel = B_FLOATING_APP_WINDOW_FEEL;
		}	break;

		case kToolSelectionWindowFeelChanged: {
			if (fSelection->Value() == B_CONTROL_OFF)
				fToolWindowFeel = B_NORMAL_WINDOW_FEEL;
			else
				fToolWindowFeel = B_FLOATING_APP_WINDOW_FEEL;
		}	break;

		case kToolSetupWindowFeelChanged: {
			if (fSetup->Value() == B_CONTROL_OFF)
				fToolSetupWindowFeel = B_NORMAL_WINDOW_FEEL;
			else
				fToolSetupWindowFeel = B_FLOATING_APP_WINDOW_FEEL;
		}	break;

		case kLayerWindowFeelChanged: {
			if (fLayer->Value() == B_CONTROL_OFF)
				fLayerWindowFeel = B_NORMAL_WINDOW_FEEL;
			else
				fLayerWindowFeel = B_FLOATING_APP_WINDOW_FEEL;
		}	break;

		case kBrushWindowFeelChanged: {
			if (fBrush->Value() == B_CONTROL_OFF)
				fBrushWindowFeel = B_NORMAL_WINDOW_FEEL;
			else
				fBrushWindowFeel = B_FLOATING_APP_WINDOW_FEEL;
		}	break;

		case kEffectsWindowFeelChanged: {
			if (fEffects->Value() == B_CONTROL_OFF)
				fAddOnWindowFeel = B_NORMAL_WINDOW_FEEL;
			else
				fAddOnWindowFeel = B_FLOATING_SUBSET_WINDOW_FEEL;
		}	break;

		default: {
			BView::MessageReceived(message);
		}	break;
	}
}


void
GlobalSetupWindow::WindowFeelView::ApplyChanges()
{
	ToolSelectionWindow::setFeel(fToolWindowFeel);
	ToolSetupWindow::SetWindowFeel(fToolSetupWindowFeel);
	ColorPaletteWindow::setFeel(fPaletteWindowFeel);
	BrushStoreWindow::setFeel(fBrushWindowFeel);
	LayerWindow::setFeel(fLayerWindowFeel);
	ManipulatorWindow::setFeel(fAddOnWindowFeel);
}


// #pragma mark -- UndoControlView


class GlobalSetupWindow::UndoControlView : public BView {
public:
						UndoControlView();
	virtual				~UndoControlView() {}

	virtual	void		AttachedToWindow();
	virtual	void		MessageReceived(BMessage* message);

			void		ApplyChanges();

private:
			void		_Update(int32 undoDepth, bool enableInput);

private:
		int32			fUndoDepth;
		BRadioButton*	fUnlimitedUndo;
		BRadioButton*	fAdjustableUndo;
		BRadioButton*	fDisabledUndo;
		NumberControl*	fAdjustableUndoInput;
};


GlobalSetupWindow::UndoControlView::UndoControlView()
	: BView("undo control view", 0)
	, fUndoDepth(UndoQueue::ReturnDepth())
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 5.0)
		.Add(fDisabledUndo =
				new BRadioButton(B_TRANSLATE("Off"),
				new BMessage(kSetUndoDisabled)))
		.Add(fUnlimitedUndo =
				new BRadioButton(B_TRANSLATE("Unlimited"),
				new BMessage(kSetUnlimitedUndo)))
		.Add(fAdjustableUndo =
				new BRadioButton(B_TRANSLATE("Adjustable"),
				new BMessage(kSetAdjustableUndo)))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.AddStrut(15.0)
			.Add(fAdjustableUndoInput = new NumberControl(B_TRANSLATE("Undo steps:"), "20",
					new BMessage(kUndoDepthAdjusted)))
			.AddGlue())
		.AddGlue()
		.SetInsets(20.0, 20.0, 10.0, 10.0)
	);

	fAdjustableUndoInput->SetValue(0);
	fAdjustableUndoInput->SetEnabled(false);

	if (UndoQueue::ReturnDepth() == 0)
		fDisabledUndo->SetValue(B_CONTROL_ON);

	if (UndoQueue::ReturnDepth() == INFINITE_QUEUE_DEPTH)
		fUnlimitedUndo->SetValue(B_CONTROL_ON);

	if (UndoQueue::ReturnDepth() > 0) {
		fAdjustableUndoInput->SetEnabled(true);
		fAdjustableUndo->SetValue(B_CONTROL_ON);
		fAdjustableUndoInput->SetValue(fUndoDepth);
	}
}


void
GlobalSetupWindow::UndoControlView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	fDisabledUndo->SetTarget(this);
	fUnlimitedUndo->SetTarget(this);
	fAdjustableUndo->SetTarget(this);
	fAdjustableUndoInput->SetTarget(this);
}


void
GlobalSetupWindow::UndoControlView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kSetUndoDisabled: {
			_Update(0, false);
		}	break;

		case kSetUnlimitedUndo: {
			_Update(INFINITE_QUEUE_DEPTH, false);
		}	break;

		case kSetAdjustableUndo: {
			_Update(fAdjustableUndoInput->Value(), true);
		}	break;

		case kUndoDepthAdjusted: {
			int32 value = min_c(fAdjustableUndoInput->Value(), 100);
			fAdjustableUndoInput->SetValue(value);
			fUndoDepth = value;
		}	break;

		default: {
			BView::MessageReceived(message);
		}	break;
	}
}


void
GlobalSetupWindow::UndoControlView::ApplyChanges()
{
	UndoQueue::SetQueueDepth(fUndoDepth);
}


void
GlobalSetupWindow::UndoControlView::_Update(int32 undoDepth, bool enableInput)
{
	fUndoDepth = undoDepth;
	fAdjustableUndoInput->SetEnabled(enableInput);
}


// #pragma mark -- GeneralControlView


class GlobalSetupWindow::GeneralControlView : public BView {
public:
						GeneralControlView();
	virtual				~GeneralControlView() {}

	virtual	void		AttachedToWindow();
	virtual	void		MessageReceived(BMessage* message);

			void		ApplyChanges();

private:
		int32			fCursorMode;
		int32			fShutdownMode;

		BRadioButton*	fToolCursor;
		BRadioButton*	fCrossHairCursor;
		BCheckBox*		fConfirmShutdown;
};


GlobalSetupWindow::GeneralControlView::GeneralControlView()
	: BView("general control view", 0)
	, fCursorMode(TOOL_CURSOR_MODE)
	, fShutdownMode(B_CONTROL_ON)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	BBox* box = new BBox(B_NO_BORDER, BGroupLayoutBuilder(B_VERTICAL, 5.0)
		.Add(fToolCursor =
			new BRadioButton(B_TRANSLATE("Tool cursor"),
			new BMessage(kCrossHairCursorMode)))
		.Add(fCrossHairCursor =
			new BRadioButton(B_TRANSLATE("Cross-hair cursor"),
			new BMessage(kToolCursorMode)))
		.SetInsets(20.0, 5.0, 10.0, 0.0)
		.TopView()
	);
	fToolCursor->SetValue(B_CONTROL_ON);
	box->SetLabel(B_TRANSLATE("Cursor"));

	BBox* box2 = new BBox(B_NO_BORDER, BGroupLayoutBuilder(B_VERTICAL)
		.Add(fConfirmShutdown =
				new BCheckBox(B_TRANSLATE("Confirm quitting"),
				new BMessage(kConfirmShutdownChanged)))
		.SetInsets(20.0, 5.0, 10.0, 0.0)
		.TopView()
	);
	box2->SetLabel("Application");

	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(box)
		.AddStrut(15.0)
		.Add(box2)
		.AddGlue()
		.SetInsets(10.0, 10.0, 10.0, 10.0)
		.TopView()
	);

	if (SettingsServer* server = SettingsServer::Instance()) {
		BMessage settings;
		server->GetApplicationSettings(&settings);
		settings.FindInt32(skCursorMode, &fCursorMode);
		settings.FindInt32(skQuitConfirmMode, &fShutdownMode);
	}

	if (fCursorMode == CROSS_HAIR_CURSOR_MODE)
		fCrossHairCursor->SetValue(B_CONTROL_ON);

	fConfirmShutdown->SetValue(fShutdownMode);
}


void
GlobalSetupWindow::GeneralControlView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	fToolCursor->SetTarget(this);
	fCrossHairCursor->SetTarget(this);
	fConfirmShutdown->SetTarget(this);
}


void
GlobalSetupWindow::GeneralControlView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kToolCursorMode: {
			fCursorMode = TOOL_CURSOR_MODE;
		}	break;

		case kCrossHairCursorMode: {
			fCursorMode = CROSS_HAIR_CURSOR_MODE;
		}	break;

		case kConfirmShutdownChanged: {
			fShutdownMode = fConfirmShutdown->Value();
		}	break;

		default: {
			BView::MessageReceived(message);
		}	break;
	}
}


void
GlobalSetupWindow::GeneralControlView::ApplyChanges()
{
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skQuitConfirmMode,
			fShutdownMode);
		server->SetValue(SettingsServer::Application, skCursorMode, fCursorMode);
	}
}


// #pragma mark -- GlobalSetupWindow


GlobalSetupWindow::GlobalSetupWindow(const BPoint& leftTop)
	: BWindow(BRect(leftTop, BSize(10.0, 10.0)),
		B_TRANSLATE("Global settings"),
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE |
		B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
{
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL, 10.0);
	SetLayout(layout);

	fTabView = new BTabView("tab view", B_WIDTH_FROM_LABEL);

	BTab* tab = new BTab();
	fWindowFeelView = new WindowFeelView;
	fTabView->AddTab(fWindowFeelView, tab);
	tab->SetLabel(B_TRANSLATE("Windows"));

	tab = new BTab();
	fUndoControlView = new UndoControlView;
	fTabView->AddTab(fUndoControlView, tab);
	tab->SetLabel(B_TRANSLATE("Undo"));

	tab = new BTab();
	fGeneralControlView = new GeneralControlView;
	fTabView->AddTab(fGeneralControlView, tab);
	tab->SetLabel(B_TRANSLATE("Miscellaneous"));

	layout->AddView(fTabView);
	layout->AddView(BGroupLayoutBuilder(B_HORIZONTAL, 10.0)
		.AddGlue()
		.Add(new BButton(B_TRANSLATE("Cancel"),
			new BMessage(kCloseAndDiscardSettings)))
		.Add(new BButton(B_TRANSLATE("OK"),
			new BMessage(kCloseAndApplySettings)))
		.TopView()
	);
	layout->SetInsets(10.0, 10.0, 10.0, 10.0);
	layout->View()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	int32 activeTab = 0;
	if (SettingsServer* server = SettingsServer::Instance()) {
		BMessage settings;
		server->GetApplicationSettings(&settings);
		settings.FindInt32(skSettingsWindowTab, &activeTab);
	}
	fTabView->Select(activeTab);
}


GlobalSetupWindow::~GlobalSetupWindow()
{
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skSettingsWindowFrame,
			Frame());
		server->SetValue(SettingsServer::Application, skSettingsWindowTab,
			fTabView->Selection());
	}
	fSetupWindow = NULL;
}


void
GlobalSetupWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kCloseAndApplySettings: {
			if (fWindowFeelView)
				fWindowFeelView->ApplyChanges();

			if (fUndoControlView)
				fUndoControlView->ApplyChanges();

			if (fGeneralControlView)
				fGeneralControlView->ApplyChanges();

		// fall through
		case kCloseAndDiscardSettings:
			Quit();
		}	break;

		default: {
			BWindow::MessageReceived(message);
		}	break;
	}
}

void
GlobalSetupWindow::ShowGlobalSetupWindow()
{
	BRect frame(100, 100, 350, 300);
	if (SettingsServer* server = SettingsServer::Instance()) {
		BMessage settings;
		server->GetApplicationSettings(&settings);
		settings.FindRect(skSettingsWindowFrame, &frame);
	}

	if (fSetupWindow == NULL)
		fSetupWindow = new GlobalSetupWindow(frame.LeftTop());

	if (fSetupWindow->IsHidden())
		fSetupWindow->Show();

	if (!fSetupWindow->IsActive())
		fSetupWindow->Activate(true);
}


void
GlobalSetupWindow::CloseGlobalSetupWindow()
{
	if (fSetupWindow->Lock())
		fSetupWindow->Quit();
}
