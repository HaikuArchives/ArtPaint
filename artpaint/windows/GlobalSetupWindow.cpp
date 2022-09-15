/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *		Humdinger <humdingerb@gmail.com>
 *
 */

#include "GlobalSetupWindow.h"

#include "BitmapUtilities.h"
#include "BrushStoreWindow.h"
#include "Cursors.h"
#include "ColorPalette.h"
#include "LayerWindow.h"
#include "ManipulatorWindow.h"
#include "NumberControl.h"
#include "NumberSliderControl.h"
#include "PaintWindow.h"
#include "SettingsServer.h"
#include "ToolSelectionWindow.h"
#include "ToolSetupWindow.h"
#include "UndoQueue.h"
#include "UtilityClasses.h"


#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <Control.h>
#include <LayoutBuilder.h>
#include <RadioButton.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>
#include <TabView.h>


#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Windows"


using ArtPaint::Interface::NumberControl;
using ArtPaint::Interface::NumberSliderControl;


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

const uint32 kBgColorsChanged					= '_bgc';
const uint32 kBgGridSizeChanged					= '_bgg';
const uint32 kBgRevertToDefaults     			= '_bgd';

const uint32 kDrop								= '_drp';


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

	BStringView* label = new BStringView("label", B_TRANSLATE("Keep in front"));
	label->SetFont(be_bold_font);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(label)
		.AddGrid(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(fEffects, 0, 0)
			.Add(fBrush, 0, 1)
			.Add(fColor, 0, 2)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(B_USE_DEFAULT_SPACING), 1, 0, 1, 3)
			.Add(fLayer, 2, 0)
			.Add(fSelection, 2, 1)
			.Add(fSetup, 2, 2)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, 0, 0)
		.End()
		.AddGlue()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
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
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.AddGroup (B_VERTICAL, B_USE_SMALL_SPACING)
			.Add(fDisabledUndo =
					new BRadioButton(B_TRANSLATE("Off"),
					new BMessage(kSetUndoDisabled)))
			.Add(fUnlimitedUndo =
					new BRadioButton(B_TRANSLATE("Unlimited"),
					new BMessage(kSetUnlimitedUndo)))
			.Add(fAdjustableUndo =
					new BRadioButton(B_TRANSLATE("Adjustable"),
					new BMessage(kSetAdjustableUndo)))
		.End()
		.AddGroup (B_HORIZONTAL)
			.AddStrut(B_USE_DEFAULT_SPACING)
			.Add(fAdjustableUndoInput = new NumberControl(B_TRANSLATE("Undo steps:"), "20",
					new BMessage(kUndoDepthAdjusted)))
			.AddGlue()
		.End()
		.AddGlue()
		.SetInsets(B_USE_BIG_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

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


// #pragma mark -- TransparencyControlView


class GlobalSetupWindow::TransparencyControlView : public BView {
public:
						TransparencyControlView();
	virtual				~TransparencyControlView() {}

	virtual	void		AllAttached();
	virtual	void		MessageReceived(BMessage* message);

			void		ApplyChanges();
			void		SetColor(PreviewPane* which, uint32 color);

private:
			void		_SetDefaults();
			void		_Update();

private:
			NumberSliderControl*	fGridSizeControl;
			BBitmap*				fPreview;
			PreviewPane*			fBgContainer;
			ColorSwatch*			fColorSwatch1;
			ColorSwatch*			fColorSwatch2;
			BButton*				fDefaultsButton;

			uint32		fColor1;
			uint32		fColor2;
			int32		fGridSize;
};


GlobalSetupWindow::TransparencyControlView::TransparencyControlView()
	: BView("background control view", 0)
{
	BMessage* message = new BMessage(kBgGridSizeChanged);
	fGridSizeControl = new NumberSliderControl(B_TRANSLATE("Grid size:"),
								"0", message, 4, 50, false, true);
	fGridSizeControl->Slider()->SetExplicitMinSize(BSize(
		StringWidth("SLIDERSLIDERSLIDERSLIDER"), B_SIZE_UNSET));
	fGridSizeControl->TextControl()->SetWidthInBytes(7);

	float size = be_plain_font->Size() / 12.0f;
	BRect frame(0, 0, size * 250, size * 100);

	fBgContainer = new PreviewPane(frame);
	BRect frameSwatch = (0, 0, size * 24, size * 24);

	fColorSwatch1 = new ColorSwatch(frameSwatch, "color1");
	fColorSwatch1->SetToolTip(B_TRANSLATE("Drop a color from the Colors window"));
	fColorSwatch2 = new ColorSwatch(frameSwatch, "color2");
	fColorSwatch2->SetToolTip(B_TRANSLATE("Drop a color from the Colors window"));

	BStringView* labelColors = new BStringView("color1label",
		B_TRANSLATE("Colors:"));

	fDefaultsButton = new BButton(B_TRANSLATE("Defaults"),
		new BMessage(kBgRevertToDefaults));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fBgContainer)
		.AddGrid(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fGridSizeControl, 0, 0, 0, 0)
			.Add(fGridSizeControl->LabelLayoutItem(), 0, 0)
			.Add(fGridSizeControl->TextViewLayoutItem(), 1, 0, 2)
			.Add(fGridSizeControl->Slider(), 3, 0)
			.Add(labelColors, 0, 1)
			.Add(fColorSwatch1, 1, 1)
			.Add(fColorSwatch2, 2, 1)
		.End()
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGlue()
			.Add(fDefaultsButton)
		.End()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

	_SetDefaults();

	if (SettingsServer* server = SettingsServer::Instance()) {
		 BMessage settings;
		 server->GetApplicationSettings(&settings);

		fGridSize = settings.GetInt32(skBgGridSize, fGridSize);
		fColor1 = settings.GetUInt32(skBgColor1, fColor1);
		fColor2 = settings.GetUInt32(skBgColor2, fColor2);
	}

	fGridSize = max_c(4, fGridSize);
}


void
GlobalSetupWindow::TransparencyControlView::AllAttached()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	fGridSizeControl->SetValue(fGridSize);

	fColorSwatch1->SetColor(fColor1);
	fColorSwatch2->SetColor(fColor2);

	fGridSizeControl->SetTarget(this);
	fColorSwatch1->SetTarget(this);
	fColorSwatch2->SetTarget(this);
	fDefaultsButton->SetTarget(this);

	_Update();
}


void
GlobalSetupWindow::TransparencyControlView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kBgGridSizeChanged: {
			fGridSize = fGridSizeControl->Value();
			_Update();
		} break;
		case kBgColorsChanged: {
			const char* name;
			if (message->FindString("name", &name) == B_OK) {
				uint32 new_color;
				if (message->FindUInt32("color", &new_color) == B_OK) {
					if (strcmp(name, "color1") == 0)
						fColor1 = new_color;
					else
						fColor2 = new_color;

					_Update();
				}
			}
		} break;
		case kBgRevertToDefaults: {
			_SetDefaults();
			fGridSizeControl->SetValue(fGridSize);
			_Update();
		} break;
		default: {
			BView::MessageReceived(message);
		}	break;
	}
}


void
GlobalSetupWindow::TransparencyControlView::ApplyChanges()
{
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skBgGridSize,
			fGridSize);
		server->SetValue(SettingsServer::Application, skBgColor1,
			fColor1);
		server->SetValue(SettingsServer::Application, skBgColor2,
			fColor2);
	}

	PaintWindow::Redraw();
}


void
GlobalSetupWindow::TransparencyControlView::_Update()
{
	fColorSwatch1->SetColor(fColor1);
	fColorSwatch2->SetColor(fColor2);
	BitmapUtilities::CheckerBitmap(fBgContainer->previewBitmap(), fColor1, fColor2, fGridSize);
	fBgContainer->Redraw();
}


void
GlobalSetupWindow::TransparencyControlView::_SetDefaults()
{
	rgb_color color1, color2;
	color1.red = color1.green = color1.blue = 0xBB;
	color2.red = color2.green = color2.blue = 0x99;
	color1.alpha = color2.alpha = 0xFF;
	fColor1 = RGBColorToBGRA(color1);
	fColor2 = RGBColorToBGRA(color2);
	fGridSize = 20;
}


PreviewPane::PreviewPane(BRect frame)
	: BView(frame, "previewpane", B_FOLLOW_NONE, B_WILL_DRAW)
	, fPreviewBitmap(NULL)
{
	SetExplicitMinSize(BSize(frame.Width(), frame.Height()));
	SetExplicitMaxSize(BSize(frame.Width(), frame.Height()));

	frame.InsetBy(1.0, 1.0);
	fPreviewBitmap = new BBitmap(BRect(0.0, 0.0, frame.Width(),
		frame.Height()), B_RGBA32);
}


PreviewPane::~PreviewPane()
{
	delete fPreviewBitmap;
}


void
PreviewPane::Draw(BRect updateRect)
{
	BView::Draw(updateRect);

	DrawBitmap(fPreviewBitmap, BPoint(1.0, 1.0));

	SetHighColor(0, 0, 0, 255);
	StrokeRect(Bounds());
}


void PreviewPane::MessageReceived(BMessage* message)
{
	switch(message->what) {
		default:
			BView::MessageReceived(message);
	}
}


ColorSwatch::ColorSwatch(BRect frame, const char* name)
	: BControl(frame, "colorswatch", name, new BMessage(kBgColorsChanged),
			B_FOLLOW_NONE, B_WILL_DRAW)
	, fSwatchBitmap(NULL)
	, fColor(0)
{
	SetExplicitMinSize(BSize(frame.Width(), frame.Height()));
	SetExplicitMaxSize(BSize(frame.Width(), frame.Height()));

	Message()->AddString("name", name);
	Message()->AddUInt32("color", fColor);

	frame.InsetBy(1.0, 1.0);
	fSwatchBitmap = new BBitmap(BRect(0.0, 0.0, frame.Width(),
		frame.Height()), B_RGBA32);
}


ColorSwatch::~ColorSwatch()
{
	delete fSwatchBitmap;
}


void
ColorSwatch::Draw(BRect updateRect)
{
	BControl::Draw(updateRect);

	SetHighColor(0, 0, 0, 255);
	StrokeRect(Bounds());

	BitmapUtilities::ClearBitmap(fSwatchBitmap, fColor);
	DrawBitmap(fSwatchBitmap, BPoint(1.0, 1.0));
}


void ColorSwatch::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case B_PASTE: {
			if (message->WasDropped()) {
				BPoint dropPoint = ConvertFromScreen(message->DropPoint());
				ssize_t size;
				const void* data;
				if (message->FindData("RGBColor", B_RGB_COLOR_TYPE, &data,
					&size) == B_OK && size == sizeof(rgb_color)) {
					rgb_color new_color;
					memcpy((void*)(&new_color), data, size);
					new_color.alpha = 0xFF;
					SetColor(RGBColorToBGRA(new_color));
					Message()->ReplaceUInt32("color", fColor);
					Draw(Bounds());
					Invoke();
				}
			}
		} break;
		default:
			BControl::MessageReceived(message);
	}
}


void
ColorSwatch::SetColor(uint32 color)
{
	fColor = color;
	Draw(Bounds());
}


void
ColorSwatch::MouseDown(BPoint point)
{
	rgb_color color = BGRAColorToRGB(fColor);
	ColorPaletteWindow::showPaletteWindow();
	ColorPaletteWindow::ChangePaletteColor(color);
}


// #pragma mark -- MiscControlView


class GlobalSetupWindow::MiscControlView : public BView {
public:
						MiscControlView();
	virtual				~MiscControlView() {}

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


GlobalSetupWindow::MiscControlView::MiscControlView()
	: BView("general control view", 0)
	, fCursorMode(TOOL_CURSOR_MODE)
	, fShutdownMode(B_CONTROL_ON)
{
	BStringView* labelCursor = new BStringView("labelCursor", B_TRANSLATE("Cursor"));
	labelCursor->SetFont(be_bold_font);

	BStringView* labelApp = new BStringView("labelApp", B_TRANSLATE("Application"));
	labelApp->SetFont(be_bold_font);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(labelCursor)
		.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
			.Add(fToolCursor =
				new BRadioButton(B_TRANSLATE("Tool cursor"),
				new BMessage(kCrossHairCursorMode)))
			.Add(fCrossHairCursor =
				new BRadioButton(B_TRANSLATE("Cross-hair cursor"),
				new BMessage(kToolCursorMode)))
			.SetInsets(B_USE_DEFAULT_SPACING, 0, 0, 0)
		.End()
		.Add(BSpaceLayoutItem::CreateVerticalStrut(B_USE_DEFAULT_SPACING))
		.Add(labelApp)
		.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
			.Add(fConfirmShutdown =
				new BCheckBox(B_TRANSLATE("Confirm quitting"),
				new BMessage(kConfirmShutdownChanged)))
			.SetInsets(B_USE_DEFAULT_SPACING, 0, 0, 0)
		.End()
		.AddGlue()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

	fToolCursor->SetValue(B_CONTROL_ON);

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
GlobalSetupWindow::MiscControlView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	fToolCursor->SetTarget(this);
	fCrossHairCursor->SetTarget(this);
	fConfirmShutdown->SetTarget(this);
}


void
GlobalSetupWindow::MiscControlView::MessageReceived(BMessage* message)
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
GlobalSetupWindow::MiscControlView::ApplyChanges()
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
		B_TRANSLATE("Settings"),
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE |
		B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
{
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
	fTransparencyControlView = new TransparencyControlView;
	fTabView->AddTab(fTransparencyControlView, tab);
	tab->SetLabel(B_TRANSLATE("Transparency"));

	tab = new BTab();
	fMiscControlView = new MiscControlView;
	fTabView->AddTab(fMiscControlView, tab);
	tab->SetLabel(B_TRANSLATE("Miscellaneous"));

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(fTabView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(new BButton(B_TRANSLATE("Cancel"),
				new BMessage(kCloseAndDiscardSettings)))
			.Add(new BButton(B_TRANSLATE("OK"),
				new BMessage(kCloseAndApplySettings)))
			.AddGlue()
		.End()
		.SetInsets(-2, B_USE_WINDOW_INSETS, -2, B_USE_WINDOW_INSETS);

	int32 activeTab = 0;
	if (SettingsServer* server = SettingsServer::Instance()) {
		BMessage settings;
		server->GetApplicationSettings(&settings);
		settings.FindInt32(skSettingsWindowTab, &activeTab);
	}
	fTabView->Select(activeTab);

	float width = 0;
	for (int i = 0; i < fTabView->CountTabs(); i++)
		width += fTabView->TabFrame(i).Width();

	fTabView->SetExplicitMinSize(BSize(width + 2 * be_plain_font->Size(), B_SIZE_UNSET));
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

			if (fMiscControlView)
				fMiscControlView->ApplyChanges();

			if (fTransparencyControlView)
				fTransparencyControlView->ApplyChanges();

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
	BRect frame(100, 100, 300, 300);
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
