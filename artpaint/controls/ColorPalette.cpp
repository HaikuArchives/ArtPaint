/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "ColorPalette.h"

#include "BitmapUtilities.h"
#include "FileIdentificationStrings.h"
#include "FilePanels.h"
#include "FloaterManager.h"
#include "MessageConstants.h"
#include "MessageFilters.h"
#include "PaintApplication.h"
#include "PaletteWindowClient.h"
#include "Patterns.h"
#include "ResourceServer.h"
#include "SettingsServer.h"
#include "StatusView.h"
#include "UtilityClasses.h"


#include <Alert.h>
#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <FilePanel.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <Path.h>
#include <PictureButton.h>
#include <Resources.h>
#include <Roster.h>
#include <SpaceLayoutItem.h>
#include <TextControl.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ColorPalette"


// Initialize the static variable to NULL.
ColorPaletteWindow* ColorPaletteWindow::palette_window = NULL;
BList* ColorPaletteWindow::master_window_list = new BList();
BList* ColorPaletteWindow::palette_window_clients = new BList();


status_t
HexStringToUInt32(BString hexColor, uint32& color_word)
{
	hexColor.ReplaceAll("#", "");
	hexColor.ToLower();
	union color_conversion color;
	status_t valid_color = B_ERROR;

	if (hexColor.Length() == 3 || hexColor.Length() == 4) {
		BString byteStr;

		for (int i = 0; i < 3; i++) {
			char digit = hexColor.ByteAt(i);
			uint32 byte = 0;
			byteStr.SetToFormat("%c%c", digit, digit);
			byteStr.ScanWithFormat("%X", &byte);
			color.bytes[2 - i] = (uint8)byte;
		}
		if (hexColor.Length() == 4) {
			char digit = hexColor.ByteAt(3);
			uint32 byte = 0;
			byteStr.SetToFormat("%c%c", digit, digit);
			byteStr.ScanWithFormat("%X", &byte);
			color.bytes[3] = (uint8)byte;
		} else
			color.bytes[3] = (uint8)0xFF;

		valid_color = B_OK;
		color_word = color.word;
	} else if (hexColor.Length() == 6 || hexColor.Length() == 8) {
		BString byteStr;

		for (int i = 0; i < 3; ++i) {
			int j = i * 2;
			char digit1 = hexColor.ByteAt(j);
			char digit2 = hexColor.ByteAt(j + 1);
			uint32 byte = 0;
			byteStr.SetToFormat("%c%c", digit1, digit2);
			byteStr.ScanWithFormat("%X", &byte);
			color.bytes[2 - i] = (uint8)byte;
		}
		if (hexColor.Length() == 8) {
			char digit1 = hexColor.ByteAt(6);
			char digit2 = hexColor.ByteAt(7);
			uint32 byte = 0;
			byteStr.SetToFormat("%c%c", digit1, digit2);
			byteStr.ScanWithFormat("%X", &byte);
			color.bytes[3] = (uint8)byte;
		} else
			color.bytes[3] = 0xFF;

		valid_color = B_OK;
		color_word = color.word;
	}

	return valid_color;
}


ColorPaletteWindow::ColorPaletteWindow(BRect frame, int32 mode)
	:
	BWindow(frame, B_TRANSLATE("Colors"), B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_V_RESIZABLE | B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK | B_AVOID_FRONT
			| B_AUTO_UPDATE_SIZE_LIMITS),
	open_panel(NULL),
	save_panel(NULL)
{
	// here we just record the mode that user has requested
	// for displaying the controls (eg. RGBA, HSV, something else)
	selector_mode = mode;

	color_control = NULL;
	color_slider = NULL;

	window_feel feel = B_NORMAL_WINDOW_FEEL;
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skPaletteWindowVisible, true);

		BMessage settings;
		if (server->GetApplicationSettings(&settings) == B_OK)
			settings.FindInt32(skPaletteWindowFeel, (int32*)&feel);
	}
	SetFeel(feel);

	window_look look = B_FLOATING_WINDOW_LOOK;
	if (feel == B_NORMAL_WINDOW_FEEL)
		look = B_TITLED_WINDOW_LOOK;
	SetLook(look);

	openMenuBar();

	BBox* container_box = new BBox("container-box");
	container_box->SetBorder(B_PLAIN_BORDER);

	color_container = new ColorContainer(BRect(0, 3, 63, 66), ColorSet::currentSet()->sizeOfSet());
	color_container->SetDraggingEnabled(TRUE);

	BFont font;

	// Here add the buttons that control the color-set.
	previous_set = new BButton("\xe2\xaf\x87", new BMessage(HS_PREVIOUS_PALETTE));
	previous_set->SetExplicitMaxSize(BSize(font.StringWidth("xx\xe2\xaf\x87X"), B_SIZE_UNSET));
	previous_set->SetExplicitMinSize(BSize(font.StringWidth("xx\xe2\xaf\x87X"), B_SIZE_UNSET));
	previous_set->SetTarget(this);
	previous_set->SetToolTip(B_TRANSLATE_COMMENT("Previous color set", "In Color Palette window"));
	previous_set->SetEnabled(FALSE);

	next_set = new BButton("\xe2\xaf\x88", new BMessage(HS_NEXT_PALETTE));
	next_set->SetExplicitMaxSize(BSize(font.StringWidth("xx\xe2\xaf\x88X"), B_SIZE_UNSET));
	next_set->SetExplicitMinSize(BSize(font.StringWidth("xx\xe2\xaf\x88X"), B_SIZE_UNSET));
	next_set->SetTarget(this);
	next_set->SetToolTip(B_TRANSLATE_COMMENT("Next color set", "In Color Palette window"));
	next_set->SetEnabled(FALSE);

	BGridLayout* colorSetGrid = BLayoutBuilder::Grid<>(container_box, 2, B_USE_SMALL_SPACING)
		.Add(color_container, 0, 0, 1, 3)
		.Add(previous_set, 1, 0)
		.Add(next_set, 1, 2)
		.SetInsets(3, 3, 8, 3);

	colorSetGrid->SetMinColumnWidth(0, font.StringWidth("PALETTEPALETTE"));

	BMessage* message = new BMessage(HS_RGB_CONTROL_INVOKED);
	message->AddInt32("buttons", 0);

	rgb_color c = {255, 255, 255, 255};

	color_control = new HSColorControl(BPoint(0, 0), B_CELLS_32x8, 8, "");
	color_control->SetTarget(this);
	rgbSlider = new RGBColorControl(c);
	rgbSlider->SetMessage(message);
	rgbSlider->SetTarget(this);
	cmySlider = new CMYColorControl(c);
	cmySlider->SetMessage(message);
	cmySlider->SetTarget(this);
	labSlider = new LABColorControl(c);
	labSlider->SetMessage(message);
	labSlider->SetTarget(this);
	hsvSlider = new HSVColorControl(c);
	hsvSlider->SetMessage(message);
	hsvSlider->SetTarget(this);

	sliderLayout = BLayoutBuilder::Cards<>()
		.Add(rgbSlider)
		.Add(cmySlider)
		.Add(labSlider)
		.Add(hsvSlider)
		.Add(color_control);

	rgbSlider->SetExplicitMinSize(BSize(font.StringWidth("XX12345SLIDERSLIDER"), B_SIZE_UNSET));
	cmySlider->SetExplicitMinSize(BSize(font.StringWidth("XX12345SLIDERSLIDER"), B_SIZE_UNSET));
	labSlider->SetExplicitMinSize(BSize(font.StringWidth("XX12345SLIDERSLIDER"), B_SIZE_UNSET));
	hsvSlider->SetExplicitMinSize(BSize(font.StringWidth("XX12345SLIDERSLIDER"), B_SIZE_UNSET));
	color_control->SetExplicitMinSize(BSize(font.StringWidth("XX12345SLIDERSLIDER"), B_SIZE_UNSET));

	colorPreview = new ColorChip("chippy");
	colorPreview->SetColor(RGBColorToBGRA(c));
	colorPreview->SetExplicitMinSize(BSize(font.StringWidth("#dddddddd#"), B_SIZE_UNSET));
	colorPreview->SetExplicitMaxSize(BSize(font.StringWidth("#dddddddd#"), B_SIZE_UNSET));
	hexColorField = new BTextControl("", "hex-color", new BMessage(HEX_COLOR_EDITED));
	for (uint32 i = 0; i < 256; ++i)
		hexColorField->TextView()->DisallowChar(i);
	hexColorField->TextView()->AllowChar('0');
	hexColorField->TextView()->AllowChar('1');
	hexColorField->TextView()->AllowChar('2');
	hexColorField->TextView()->AllowChar('3');
	hexColorField->TextView()->AllowChar('4');
	hexColorField->TextView()->AllowChar('5');
	hexColorField->TextView()->AllowChar('6');
	hexColorField->TextView()->AllowChar('7');
	hexColorField->TextView()->AllowChar('8');
	hexColorField->TextView()->AllowChar('9');
	hexColorField->TextView()->AllowChar('A');
	hexColorField->TextView()->AllowChar('B');
	hexColorField->TextView()->AllowChar('C');
	hexColorField->TextView()->AllowChar('D');
	hexColorField->TextView()->AllowChar('E');
	hexColorField->TextView()->AllowChar('F');
	hexColorField->TextView()->AllowChar('a');
	hexColorField->TextView()->AllowChar('b');
	hexColorField->TextView()->AllowChar('c');
	hexColorField->TextView()->AllowChar('d');
	hexColorField->TextView()->AllowChar('e');
	hexColorField->TextView()->AllowChar('f');
	hexColorField->TextView()->AllowChar('#');
	hexColorField->TextView()->SetMaxBytes(9);
	hexColorField->SetTarget(this);

	BGridLayout* colorLayout = BLayoutBuilder::Grid<>(5, 3)
		.Add(container_box, 0, 0)
		.Add(sliderLayout, 1, 0)
		.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING, 2, 0)
			.Add(colorPreview)
			.Add(hexColorField)
		.End()
		.SetInsets(5, 5, 5, 5);

	BGroupLayout* mainLayout
		= BLayoutBuilder::Group<>(this, B_VERTICAL, 0).Add(menu_bar).Add(colorLayout);

	// call some function that initializes the views depending on the mode
	openControlViews(mode);
	if (ColorSet::currentSetIndex() < ColorSet::numberOfSets() - 1)
		next_set->SetEnabled(TRUE);
	if (ColorSet::currentSetIndex() > 0)
		previous_set->SetEnabled(TRUE);

	Show();

	if (Lock()) {
		AddCommonFilter(new BMessageFilter(
			B_ANY_DELIVERY, B_ANY_SOURCE, B_MOUSE_DOWN, window_activation_filter));
		AddCommonFilter(new BMessageFilter(B_KEY_DOWN, AppKeyFilterFunction));
		Unlock();
	}

	palette_window = this;
	FloaterManager::AddFloater(this);
}


ColorPaletteWindow::~ColorPaletteWindow()
{
	delete open_panel;
	delete save_panel;

	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skPaletteWindowFrame, Frame());
		server->SetValue(SettingsServer::Application, skPaletteWindowVisible, false);
		server->SetValue(SettingsServer::Application, skPaletteColorMode, selector_mode);
		server->SetValue(SettingsServer::Application, skPaletteSelectedColor,
			ColorSet::currentSet()->currentColorIndex());
	}

	previous_set->RemoveSelf();
	next_set->RemoveSelf();
	color_container->RemoveSelf();
	hexColorField->RemoveSelf();
	colorPreview->RemoveSelf();
	sliderLayout->RemoveSelf();
	rgbSlider->RemoveSelf();
	labSlider->RemoveSelf();
	hsvSlider->RemoveSelf();
	cmySlider->RemoveSelf();
	color_control->RemoveSelf();
	delete color_control;

	color_control = NULL;
	color_slider = NULL;
	delete sliderLayout;

	FloaterManager::RemoveFloater(this);
	palette_window = NULL;
}


void
ColorPaletteWindow::MessageReceived(BMessage* message)
{
	// this path is used when getting the applications path to save
	// the palettes. It must be defined here instead of inside the
	// switch-clause
	BPath path;
	BMessage* color_message;
	rgb_color color;
	int32 buttons;

	switch (message->what) {

		// this comes from the HSColorControl object and indicates that it's value has changed
		case HS_COLOR_CONTROL_INVOKED:
		{
			// send message to each container
			// message should contain the index of that color in color-set
			// this should only be done if the color_container is in edit-mode
			// and set the current color in the set for that color too
			color_message = new BMessage(HS_COLOR_CHANGED);
			color_message->AddInt32("index", ColorSet::currentSet()->currentColorIndex());
			// For some reason the alpha channel needs to be forced to 255
			color = color_control->ValueAsColor();
			color.alpha = 255;
			ColorSet::currentSet()->setCurrentColor(color);
			ColorContainer::sendMessageToAllContainers(color_message);
			SelectedColorsView::sendMessageToAll(color_message);

			// also change the color for mousebutton that was used
			message->FindInt32("buttons", &buttons);
			if (buttons & B_PRIMARY_MOUSE_BUTTON)
				((PaintApplication*)be_app)->SetColor(color, TRUE);
			else
				((PaintApplication*)be_app)->SetColor(color, FALSE);

			InformClients(ColorSet::currentSet()->currentColor());
			colorPreview->SetColor(RGBColorToBGRA(color));
			SetHexColor(color);
		} break;
		// This comes from the RGBControl-object and indicates that it's value has changed.
		// This might also come from CMYControl, YIQControl ...
		case HS_RGB_CONTROL_INVOKED:
		{
			// send message to each container
			// message should contain the index of that color in color-set
			// this should only be done if the color_container is in edit-mode
			// and set the current color in the set for that color too
			color_message = new BMessage(HS_COLOR_CHANGED);
			color_message->AddInt32("index", ColorSet::currentSet()->currentColorIndex());
			ColorSet::currentSet()->setCurrentColor(color_slider->ValueAsColor());
			colorPreview->SetColor(color_slider->Value());
			ColorContainer::sendMessageToAllContainers(color_message);
			SelectedColorsView::sendMessageToAll(color_message);

			// also change the color for mousebutton that was used
			message->FindInt32("buttons", &buttons);
			if (buttons & B_PRIMARY_MOUSE_BUTTON)
				((PaintApplication*)be_app)->SetColor(color_slider->ValueAsColor(), TRUE);
			else
				((PaintApplication*)be_app)->SetColor(color_slider->ValueAsColor(), FALSE);

			rgb_color current = ColorSet::currentSet()->currentColor();
			InformClients(current);
			SetHexColor(current);
		} break;
		case COLOR_CHIP_INVOKED:
		{
			rgb_color* color;
			ssize_t color_size;
			if (message->FindData("RGBColor", B_RGB_COLOR_TYPE,
				(const void**)&color, &color_size) == B_OK) {
				if (color_control != NULL)
					color_control->SetValue(*color);

				if (color_slider != NULL)
					color_slider->SetValue(*color);

				ColorSet::currentSet()->setCurrentColor(*color);
				color_message = new BMessage(HS_COLOR_CHANGED);
				color_message->AddInt32("index", ColorSet::currentSet()->currentColorIndex());
				ColorContainer::sendMessageToAllContainers(color_message);
				SelectedColorsView::sendMessageToAll(color_message);

				// also change the color for mousebutton that was used
				message->FindInt32("buttons", &buttons);
				if (buttons & B_PRIMARY_MOUSE_BUTTON)
					((PaintApplication*)be_app)->SetColor(*color, TRUE);
				else
					((PaintApplication*)be_app)->SetColor(*color, FALSE);

				InformClients(ColorSet::currentSet()->currentColor());
				SetHexColor(*color);
			}
		} break;
		// this comes from the menubar->"Set"->"New Palette"->"N" and indicates that a new
		// palette-set should be created, the "colors"-parameter tells us how many colors the
		// new set has
		case HS_NEW_PALETTE_CREATED:
		{
			new ColorSet(message->FindInt32("colors"));
			// inform the color-containers about change
			ColorContainer::sendMessageToAllContainers(new BMessage(HS_PALETTE_CHANGED));
			// here change the name view to display sets name
			int32 numSets = ColorSet::numberOfSets();
			if (numSets > 1) {
				menu_bar->FindItem(HS_DELETE_PALETTE)->SetEnabled(TRUE);
				previous_set->SetEnabled(TRUE);
			}
			ColorSet::moveToSet(numSets - 1);
			next_set->SetEnabled(FALSE);
		} break;
		// this comes from the menubar->"Set"->"Delete Current Set" and indicates that the current
		// set should be deleted
		case HS_DELETE_PALETTE:
		{
			// check if there are any more sets left
			if (ColorSet::numberOfSets() >= 2) {
				// first delete ourselves
				delete ColorSet::currentSet();
				// inform the color-containers about change
				ColorContainer::sendMessageToAllContainers(new BMessage(HS_PALETTE_CHANGED));
				// here change the name view to display sets name
				if (ColorSet::numberOfSets() <= 1) {
					menu_bar->FindItem(HS_DELETE_PALETTE)->SetEnabled(FALSE);
					previous_set->SetEnabled(FALSE);
					next_set->SetEnabled(FALSE);
				}
			} else {
				(new BAlert("", B_TRANSLATE("Cannot delete the only color set."),
					B_TRANSLATE("OK")))->Go();
			}
		} break;
		// this comes from a button that is named "next set button", the button is in this window
		// the message indicates that we should change the colorcontainers to display next color set
		case HS_NEXT_PALETTE:
		{
			// change the entry in palette list
			ColorSet::moveToNextSet();
			// inform all color-containers about palette-change
			ColorContainer::sendMessageToAllContainers(new BMessage(HS_PALETTE_CHANGED));
			if (ColorSet::currentSetIndex() == ColorSet::numberOfSets() - 1)
				next_set->SetEnabled(FALSE);
			else
				next_set->SetEnabled(TRUE);

			if (ColorSet::numberOfSets() > 1)
				previous_set->SetEnabled(TRUE);
		} break;
		// this comes from a button that is named "previous set button", the button is in this
		// window the message indicates that we should change the colorcontainers to display
		// previous color set
		case HS_PREVIOUS_PALETTE:
		{
			// change the entry in palette list
			ColorSet::moveToPrevSet();
			// inform all color-containers about palette-change
			ColorContainer::sendMessageToAllContainers(new BMessage(HS_PALETTE_CHANGED));
			if (ColorSet::currentSetIndex() == 0)
				previous_set->SetEnabled(FALSE);
			else
				previous_set->SetEnabled(TRUE);

			if (ColorSet::numberOfSets() > 1)
				next_set->SetEnabled(TRUE);
			else
				next_set->SetEnabled(FALSE);
		} break;
		// this is sent from ColorContainer::MouseDown and it's purpose is to
		// let us change the corresponding color to the color-controller also
		case HS_PALETTE_SELECTION_CHANGED:
		{
			// update the color controller to display this new color
			// only should do it if container is in edit mode
			rgb_color current = ColorSet::currentSet()->currentColor();
			if (color_control != NULL)
				color_control->SetValue(current);

			if (color_slider != NULL)
				color_slider->SetValue(current);

			InformClients(current);
			colorPreview->SetColor(RGBColorToBGRA(current));
			SetHexColor(current);
		} break;
		// this comes from the menubar->"Set"->"Open Set" and indicates that
		// a file panel should be opened for the purpose of loading new set
		case HS_SHOW_PALETTE_OPEN_PANEL:
		{
			// here we must open the file-panel for loading
			// here get the path for palette-files, should be made relative to apps directory
			// or maybe remember the directory from last use

			if (open_panel == NULL) {
				// here we get the home directory of the application
				PaintApplication::HomeDirectory(path);

				// force normalization of the path to check validity
				if (path.Append("Palette sets/", TRUE) != B_OK)
					PaintApplication::HomeDirectory(path);

				entry_ref ref;
				get_ref_for_path(path.Path(), &ref);

				BMessenger target(this);
				BMessage message(HS_PALETTE_OPEN_REFS);
				open_panel
					= new BFilePanel(B_OPEN_PANEL, &target, &ref, B_FILE_NODE, true, &message);
			}
			open_panel->Window()->SetTitle(B_TRANSLATE("ArtPaint: Open color set" B_UTF8_ELLIPSIS));
			set_filepanel_strings(open_panel);
			open_panel->Show();
		} break;
		// this comes from the menubar->"Set"->"Save Set" and indicates that
		// a file panel should be opened for the purpose of saving current set
		case HS_SHOW_PALETTE_SAVE_PANEL:
		{
			// here we must open the file-panel for saving
			if (save_panel == NULL) {
				// get the home directory of the app
				PaintApplication::HomeDirectory(path);

				// force normalization of the path to check validity
				if (path.Append("Palette sets/", TRUE) != B_OK)
					PaintApplication::HomeDirectory(path);

				// convert it to entry_ref
				entry_ref ref;
				get_ref_for_path(path.Path(), &ref);

				BMessenger target(this);
				BMessage message(HS_PALETTE_SAVE_REFS);
				save_panel = new BFilePanel(B_SAVE_PANEL, &target, &ref, 0, false, &message);
			}
			save_panel->SetSaveText(ColorSet::currentSet()->getName());
			save_panel->Window()->SetTitle(B_TRANSLATE("ArtPaint: Open color set" B_UTF8_ELLIPSIS));
			set_filepanel_strings(save_panel);
			save_panel->Show();
		} break;
		// this comes from the open_panel or the applocation-object and includes refs for
		// the palette-files that should be loaded
		case HS_PALETTE_OPEN_REFS:
		{
			handlePaletteLoad(message);

			// inform all color containers that palette has changed
			ColorContainer::sendMessageToAllContainers(new BMessage(HS_PALETTE_CHANGED));
			if (ColorSet::numberOfSets() > 1)
				menu_bar->FindItem(HS_DELETE_PALETTE)->SetEnabled(TRUE);

			if (ColorSet::currentSetIndex() == ColorSet::numberOfSets() - 1)
				next_set->SetEnabled(FALSE);
			else
				next_set->SetEnabled(TRUE);

			if (ColorSet::currentSetIndex() > 1)
				previous_set->SetEnabled(TRUE);
			else
				previous_set->SetEnabled(FALSE);
		} break;
		// this comes from the save_panel and indicates that current set should be saved
		// to the file that the "refs" indicate
		case HS_PALETTE_SAVE_REFS:
		{
			handlePaletteSave(message);
		} break;
		// this comes from the menubar->"Mode"->"RGB-Mode" and indicates that
		// the color selector should be changed to a RGBControl, this is used
		// also for other purposes than just a message-constant
		case HS_RGB_COLOR_MODE:
		{
			if (selector_mode != HS_RGB_COLOR_MODE) {
				selector_mode = HS_RGB_COLOR_MODE;
				openControlViews(HS_RGB_COLOR_MODE);
			}
		} break;
		// this comes from the menubar->"Mode"->"CMY-Mode" and indicates that
		// the color selector should be changed to a RGBControl, this is used
		// also for other purposes than just a message-constant
		case HS_CMY_COLOR_MODE:
		{
			if (selector_mode != HS_CMY_COLOR_MODE) {
				selector_mode = HS_CMY_COLOR_MODE;
				openControlViews(HS_CMY_COLOR_MODE);
			}
		} break;
		// this comes from the menubar->"Mode"->"YIQ-Mode" and indicates that
		// the color selector should be changed to a RGBControl, this is used
		// also for other purposes than just a message-constant
		case HS_YIQ_COLOR_MODE:
		{
			if (selector_mode != HS_YIQ_COLOR_MODE) {
				selector_mode = HS_YIQ_COLOR_MODE;
				openControlViews(HS_YIQ_COLOR_MODE);
			}
		} break;
		// this comes from the menubar->"Mode"->"LAB-Mode" and indicates that
		// the color selector should be changed to a RGBControl, this is used
		// also for other purposes than just a message-constant
		case HS_LAB_COLOR_MODE:
		{
			if (selector_mode != HS_LAB_COLOR_MODE) {
				selector_mode = HS_LAB_COLOR_MODE;
				openControlViews(HS_LAB_COLOR_MODE);
			}
		} break;
		case HS_HSV_COLOR_MODE:
		{
			if (selector_mode != HS_HSV_COLOR_MODE) {
				selector_mode = HS_HSV_COLOR_MODE;
				openControlViews(HS_HSV_COLOR_MODE);
			}
		} break;
		// this comes from the menubar->"Mode"->"Simple-Mode" and indicates that
		// the color selector should be changed to a HSColorControl, this is used
		// also for other purposes than just a message-constant
		case HS_SIMPLE_COLOR_MODE:
		{
			if (selector_mode != HS_SIMPLE_COLOR_MODE) {
				selector_mode = HS_SIMPLE_COLOR_MODE;
				openControlViews(HS_SIMPLE_COLOR_MODE);
			}
		} break;
		case B_PASTE:
		{
			if (message->WasDropped()) {
				rgb_color* color;
				ssize_t color_size;
				if (message->FindData(
					"RGBColor", B_RGB_COLOR_TYPE, (const void**)&color, &color_size) == B_OK) {

					if (color_control != NULL)
						color_control->SetValue(*color);
					if (color_slider != NULL)
						color_slider->SetValue(*color);

					ColorSet::currentSet()->setCurrentColor(*color);
					color_message = new BMessage(HS_COLOR_CHANGED);
					color_message->AddInt32("index", ColorSet::currentSet()->currentColorIndex());
					int32 buttons;
					if (message->FindInt32("buttons", &buttons) == B_OK)
						color_message->ReplaceInt32("buttons", buttons);
					else
						color_message->ReplaceInt32("buttons", 0);

					if (buttons > 1)
						((PaintApplication*)be_app)->SetColor(*color, FALSE);
					else
						((PaintApplication*)be_app)->SetColor(*color, TRUE);

					colorPreview->SetColor(RGBColorToBGRA(*color));

					ColorContainer::sendMessageToAllContainers(color_message);
					SelectedColorsView::sendMessageToAll(color_message);

					InformClients(ColorSet::currentSet()->currentColor());
					SetHexColor(*color);
				}
			}
		} break;
		case B_SIMPLE_DATA:
		case B_MIME_DATA:
		{
			if (message->WasDropped()) {
				const char* colorr;
				ssize_t color_size;
				if (message->FindData(
						"text/plain", B_MIME_TYPE, (const void**)&colorr, &color_size) == B_OK) {
					char colorStr[color_size + 1];
					for (int i = 0; i < color_size; ++i)
						colorStr[i] = colorr[i];
					colorStr[color_size] = '\0';
					hexColorField->SetText(colorStr);
					color_message = new BMessage(HEX_COLOR_EDITED);
					int32 buttons;
					if (message->FindInt32("buttons", &buttons) == B_OK)
						color_message->ReplaceInt32("buttons", buttons);
					else
						color_message->ReplaceInt32("buttons", 0);

					PostMessage(color_message);
				}
			}
		} break;
		case HEX_COLOR_EDITED:
		{
			BString hexColor = hexColorField->Text();

			uint32 color_word;
			if (HexStringToUInt32(hexColor, color_word) == B_OK) {
				rgb_color newColor = BGRAColorToRGB(color_word);
				if (color_control != NULL)
					color_control->SetValue(newColor);
				if (color_slider != NULL)
					color_slider->SetValue(newColor);

				colorPreview->SetColor(color_word);
				int32 index;
				if (message->FindInt32("index", &index) != B_OK)
					index = ColorSet::currentSet()->currentColorIndex();

				ColorSet::currentSet()->setCurrentColor(newColor);
				color_message = new BMessage(HS_COLOR_CHANGED);
				color_message->AddInt32("index", index);
				int32 buttons;
				if (message->FindInt32("buttons", &buttons) == B_OK)
					color_message->ReplaceInt32("buttons", buttons);
				else
					color_message->ReplaceInt32("buttons", 0);

				if (buttons > 1)
					((PaintApplication*)be_app)->SetColor(newColor, FALSE);
				else
					((PaintApplication*)be_app)->SetColor(newColor, TRUE);

				ColorContainer::sendMessageToAllContainers(color_message);
				SelectedColorsView::sendMessageToAll(color_message);

				InformClients(ColorSet::currentSet()->currentColor());
				SetHexColor(newColor);
			} else
				SetHexColor(ColorSet::currentSet()->currentColor());
		} break;
		default:
			BWindow::MessageReceived(message);
	}
}


bool
ColorPaletteWindow::QuitRequested()
{
	// We might do something useful here.
	return TRUE;
}


bool
ColorPaletteWindow::openControlViews(int32 mode)
{
	// here we open the views that show color controls e.g. RGB- or HSV-controls
	switch (mode) {

		// in this case just open a HSColorControl and color-set container
		case HS_SIMPLE_COLOR_MODE:
		{
			sliderLayout->SetVisibleItem((int32)4);
		} break;
		// in this case open an RGBControl
		case HS_RGB_COLOR_MODE:
		{
			sliderLayout->SetVisibleItem((int32)0);
			color_slider = rgbSlider;
		} break;
		case HS_CMY_COLOR_MODE:
		{
			sliderLayout->SetVisibleItem((int32)1);
			color_slider = cmySlider;
		} break;
		case HS_LAB_COLOR_MODE:
		{
			sliderLayout->SetVisibleItem((int32)2);
			color_slider = labSlider;
		} break;
		case HS_HSV_COLOR_MODE:
		{
			sliderLayout->SetVisibleItem((int32)3);
			color_slider = hsvSlider;
		} break;
		default:
			return TRUE;
	}

	// Update the color-controllers and slider's values
	rgb_color current = ColorSet::currentSet()->currentColor();
	if (color_control != NULL)
		color_control->SetValue(current);

	if (color_slider != NULL)
		color_slider->SetValue(current);

	colorPreview->SetColor(RGBColorToBGRA(current));

	SetHexColor(current);

	return TRUE;
}


void
ColorPaletteWindow::openMenuBar()
{
	BMenu* menu;
	BMenu* sub_menu;
	BMenuItem* menu_item;

	menu_bar = new BMenuBar("menu bar");
	menu = new BMenu(B_TRANSLATE("Color set"));
	menu_bar->AddItem(menu);

	sub_menu = new BMenu(B_TRANSLATE("New color set"));

	// in this loop we add possible palette sizes to menu
	BMessage* msg;
	BString item_title;
	for (int32 i = 3; i <= 6; i++) {
		item_title.SetToFormat(B_TRANSLATE_COMMENT("%" B_PRId32 " colors",
			"The number of colors in the palette, only translate 'colors'."),
			((int32)pow(2, i)));
		msg = new BMessage(HS_NEW_PALETTE_CREATED);
		msg->AddInt32("colors", ((int32)pow(2, i)));
		menu_item = new BMenuItem(item_title, msg);
		sub_menu->AddItem(menu_item);
	}
	// the palette window will handle all things that concern making
	// or loading new palette
	sub_menu->SetTargetForItems(this);
	menu->AddItem(sub_menu);

	menu_item = new BMenuItem(B_TRANSLATE("Delete current set"), new BMessage(HS_DELETE_PALETTE));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	if (ColorSet::numberOfSets() <= 1)
		menu_item->SetEnabled(FALSE);

	menu->AddItem(new BSeparatorItem());
	menu_item = new BMenuItem(
		B_TRANSLATE("Open color set" B_UTF8_ELLIPSIS), new BMessage(HS_SHOW_PALETTE_OPEN_PANEL));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	menu_item
		= new BMenuItem(B_TRANSLATE("Save color set"), new BMessage(HS_SHOW_PALETTE_SAVE_PANEL));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);

	menu = new BMenu(B_TRANSLATE("Color model"));
	menu_bar->AddItem(menu);

	menu_item = new BMenuItem("RGB", new BMessage(HS_RGB_COLOR_MODE));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	menu_item = new BMenuItem("CMYK", new BMessage(HS_CMY_COLOR_MODE));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	menu_item = new BMenuItem("HSV", new BMessage(HS_HSV_COLOR_MODE));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	menu_item = new BMenuItem("CIELAB", new BMessage(HS_LAB_COLOR_MODE));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	menu_item = new BMenuItem("BeOS", new BMessage(HS_SIMPLE_COLOR_MODE));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	menu->SetRadioMode(TRUE);
	menu->FindItem(selector_mode)->SetMarked(TRUE);
}


void
ColorPaletteWindow::handlePaletteLoad(BMessage* message)
{
	// here check for file type and possibly load it to memory
	uint32 type;
	int32 count;
	entry_ref ref;

	// this holds the bytes that were read
	ssize_t bytes_read;

	// this will hold the identification string, they are not longer than 256 chars
	char file_type[256];

	// we can probably assume that the ref is actually a file
	message->GetInfo("refs", &type, &count);
	if (type != B_REF_TYPE)
		return;

	for (long i = --count; i >= 0; i--) {
		if (message->FindRef("refs", i, &ref) == B_OK) {
			BFile file;
			if (file.SetTo(&ref, B_READ_ONLY) == B_OK) {
				// the file was succesfully opened
				// here we check that it is actually a palette file
				// and read it if it is and also generate a new palette to
				// list of palettes
				if ((bytes_read = file.Read(file_type, strlen(HS_PALETTE_ID_STRING))) < 0) {
					// here some error reading file has happened
					(new BAlert("", B_TRANSLATE("Error reading file"), B_TRANSLATE("OK")))->Go();
				} else {
					// the read was succesfull, terminate the id string and compare
					file_type[bytes_read] = '\0';
					if (strcmp(file_type, HS_PALETTE_ID_STRING) != 0) {
						// this was not a palette file
						(new BAlert("", B_TRANSLATE("Not a color set file"), B_TRANSLATE("OK")))
							->Go();
					} else {
						// this was palette file, read the rest of it
						int32 palette_size;
						if ((bytes_read = file.Read(&palette_size, sizeof(int32)))
							!= sizeof(int32)) {
							// here some error reading file has happened
							(new BAlert(
								 "", B_TRANSLATE("File structure corrupted"), B_TRANSLATE("OK")))
								->Go();
						} else {
							// create the palette and read the palette colors
							ColorSet* loaded_set = new ColorSet(palette_size);
							rgb_color loaded_color;

							// here is the loop that reads palette entries from file
							for (int32 i = 0; i < palette_size; i++) {
								file.Read(&loaded_color, sizeof(rgb_color));
								loaded_set->setColor(i, loaded_color);
							}
							// this array holds the palette-name
							char palette_name[256];
							// read as many bytes as there is left to be the palette name
							bytes_read = file.Read(palette_name, 255);
							palette_name[bytes_read] = '\0';
							loaded_set->setName(palette_name);
						}
					}
				}
			}
		}
	}
}


void
ColorPaletteWindow::handlePaletteSave(BMessage* message)
{
	uint32 type;
	int32 count;
	entry_ref ref;

	// we can probably assume that the ref is actually a file
	message->GetInfo("directory", &type, &count);
	if (type != B_REF_TYPE)
		return;

	// here take a pointer to current color-set, so that if current set is changed
	// this will still save the same set
	ColorSet* color_set = ColorSet::currentSet();

	for (long i = --count; i >= 0; i--) {
		if (message->FindRef("directory", i, &ref) == B_OK) {
			BFile file;
			BDirectory directory = BDirectory(&ref);
			if (file.SetTo(&directory, message->FindString("name", i),
					B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE) == B_OK) {
				// here we write the current color-set to file
				// first set the file's type and other metadata
				// get the applications signature
				app_info info;
				be_app->GetAppInfo(&info);

				BNodeInfo node(&file);
				node.SetType(HS_PALETTE_MIME_STRING);
				node.SetPreferredApp(info.signature);

				if (file.Write(HS_PALETTE_ID_STRING, strlen(HS_PALETTE_ID_STRING)) < 0) {
					// error happened
					(new BAlert("", B_TRANSLATE("Cannot write to file"), B_TRANSLATE("OK")))->Go();
				} else {
					// write the rest of the file
					int32 size = color_set->sizeOfSet();
					file.Write(&size, sizeof(int32));

					// this loop writes the color entries
					for (int32 i = 0; i < size; i++) {
						rgb_color c = color_set->colorAt(i);
						file.Write(&c, sizeof(rgb_color));
					}

					// finally write the name of set
					file.Write(color_set->getName(), strlen(color_set->getName()));
				}
			}
		}
	}
}


void
ColorPaletteWindow::showPaletteWindow(BMessage* msg)
{
	if (palette_window == NULL) {
		BRect frame(300, 100, 400, 200);
		color_window_modes mode = HS_RGB_COLOR_MODE;

		if (SettingsServer* server = SettingsServer::Instance()) {
			BMessage settings;
			server->GetApplicationSettings(&settings);
			settings.FindRect(skPaletteWindowFrame, &frame);
			settings.FindInt32(skPaletteColorMode, (int32*)&mode);
			int32 color_index;
			settings.FindInt32(skPaletteSelectedColor, (int32*)&color_index);
			ColorSet::currentSet()->setCurrentColorIndex(color_index);
		}

		palette_window = new ColorPaletteWindow(frame, mode);
		for (int32 i = 0; i < master_window_list->CountItems(); ++i)
			((BWindow*)master_window_list->ItemAt(i))->AddToSubset(palette_window);
	} else {
		if (palette_window->Lock()) {
			palette_window->SetWorkspaces(B_CURRENT_WORKSPACE);
			if (palette_window->IsHidden())
				palette_window->Show();

			if (!palette_window->IsActive())
				palette_window->Activate(TRUE);

			palette_window->Unlock();
		}
	}

	if (palette_window->Lock()) {
		BRect palette_window_frame = palette_window->Frame();
		palette_window_frame = FitRectToScreen(palette_window_frame);
		palette_window->MoveTo(palette_window_frame.LeftTop());
		palette_window->Unlock();
	}

	// If we got a message we should try to use it for loading a palette.
	if (msg)
		palette_window->handlePaletteLoad(msg);
}


void
ColorPaletteWindow::ChangePaletteColor(rgb_color& c)
{
	if (palette_window != NULL) {
		palette_window->Lock();

		if (palette_window->color_control != NULL)
			palette_window->color_control->SetValue(c);
		if (palette_window->color_slider != NULL)
			palette_window->color_slider->SetValue(c);
		if (palette_window->colorPreview != NULL)
			palette_window->colorPreview->SetColor(RGBColorToBGRA(c));
		if (palette_window->hexColorField != NULL)
			palette_window->SetHexColor(c);

		palette_window->Unlock();
	}
}


void
ColorPaletteWindow::setFeel(window_feel feel)
{
	if (SettingsServer* server = SettingsServer::Instance())
		server->SetValue(SettingsServer::Application, skPaletteWindowFeel, int32(feel));

	if (palette_window) {
		window_look look = B_FLOATING_WINDOW_LOOK;
		if (feel == B_NORMAL_WINDOW_FEEL)
			look = B_TITLED_WINDOW_LOOK;

		palette_window->SetFeel(feel);
		palette_window->SetLook(look);
	}
}


void
ColorPaletteWindow::AddMasterWindow(BWindow* window)
{
	master_window_list->AddItem(window);
	if (palette_window)
		window->AddToSubset(palette_window);
}


void
ColorPaletteWindow::RemoveMasterWindow(BWindow* window)
{
	master_window_list->RemoveItem(window);
}


void
ColorPaletteWindow::AddPaletteWindowClient(PaletteWindowClient* client)
{
	if (!palette_window_clients->HasItem(client))
		palette_window_clients->AddItem(client);
}


void
ColorPaletteWindow::RemovePaletteWindowClient(PaletteWindowClient* client)
{
	palette_window_clients->RemoveItem(client);
}


void
ColorPaletteWindow::InformClients(const rgb_color& c)
{
	for (int32 i = 0; i < palette_window_clients->CountItems(); ++i) {
		PaletteWindowClient* client
			= static_cast<PaletteWindowClient*>(palette_window_clients->ItemAt(i));
		if (client)
			client->PaletteColorChanged(c);
	}
}


void
ColorPaletteWindow::SetHexColor(const rgb_color c)
{
	BString hexColor;
	hexColor.SetToFormat("#%02x%02x%02x%02x", c.red, c.green, c.blue, c.alpha);
	hexColorField->SetText(hexColor);
}


// here is the definition of HSColorControl-class
HSColorControl::HSColorControl(
	BPoint location, color_control_layout matrix, float cellSide, const char* name)
	:
	BColorControl(location, matrix, cellSide, name)
{
	// Set the message here so that using the text boxes before the sliders will work
	BMessage* message = new BMessage(HS_COLOR_CONTROL_INVOKED);
	message->AddInt32("buttons", B_PRIMARY_MOUSE_BUTTON);
	SetMessage(message);
}


void
HSColorControl::MouseDown(BPoint location)
{
	// here we take the button that was pressed
	int32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	// set the invokers message to contain the mouse-button that was last pressed
	BMessage* message = new BMessage(HS_COLOR_CONTROL_INVOKED);
	message->AddInt32("buttons", buttons);
	SetMessage(message);

	// call the inherited MouseDown-function
	BColorControl::MouseDown(location);
}


// here starts the definitions for ColorContainer class

// here define the variable that points to list of color containers
BList* ColorContainer::container_list = new BList();


ColorContainer::ColorContainer(
	BRect frame, int32 amount_of_colors, uint32 resizingMode, bool highlight, bool add_arrows)
	:
	BView("color container", B_FRAME_EVENTS | B_WILL_DRAW)
{
	// here initialize the important variables
	highlight_selected = highlight;
	left_arrow = NULL;
	right_arrow = NULL;
	contains_arrows = FALSE;

	setUpContainer(frame, amount_of_colors, add_arrows);

	// add this container to the list
	container_list->AddItem(this);

	dragging_enabled = FALSE;
}


ColorContainer::~ColorContainer()
{
	// here remove ourselves from container list
	container_list->RemoveItem(this);
}


void
ColorContainer::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());

	if (left_arrow != NULL)
		left_arrow->SetTarget(this);
	if (right_arrow != NULL)
		right_arrow->SetTarget(this);
}


void
ColorContainer::Draw(BRect drawRect)
{
	// here we draw the colors with FillRect
	// we get the colors from palette that is held somewhere
	// every instance of this class should also draw whenever
	// a palette entry changes, how should we achieve that ????

	FillRect(drawRect);

	BRect rect;

	for (int32 i = 0; i < color_count; i++) {
		rect = colorBounds(i);
		SetHighAndLowColors(ColorSet::currentSet()->colorAt(i));
		FillRect(rect, HS_2X2_BLOCKS);
	}

	if (highlight_selected) {
		// also draw the rectangle around selected color if required
		SetHighColor(255, 255, 255, 255);
		StrokeRect(colorBounds(ColorSet::currentSet()->currentColorIndex()));
	}
}


void
ColorContainer::MouseDown(BPoint point)
{
	// here we highlight the color that is under the cursor
	// and when we have selected the color we inform some
	// third parties about that
	uint32 buttons = 0;
	uint32 original_button;

	GetMouse(&point, &buttons, true);
	int32 index, prev_index = ColorSet::currentSet()->currentColorIndex();

	// first draw the rectangle around the newly selected color
	index = pointIndex(point);
	if (index != -1) {
		// only of course if some color was actually selected
		SetHighColor(255, 255, 255);
		StrokeRect(colorBounds(index));
	}

	original_button = buttons;

	if (dragging_enabled == FALSE) {
		while (buttons) {
			index = pointIndex(point);
			if ((index != -1) && (index != prev_index)) {
				// point is over some new color-entry
				if (prev_index != -1) {
					// re-draw the previous rectangle
					SetHighAndLowColors(ColorSet::currentSet()->colorAt(prev_index));
					FillRect(colorBounds(prev_index), HS_2X2_BLOCKS);
				}
				// then draw rect around new color
				SetHighColor(255, 255, 255);
				StrokeRect(colorBounds(index));
			} else if ((index == -1) && (prev_index != -1)) {
				// we fill the previous rectangle
				SetHighColor(ColorSet::currentSet()->colorAt(prev_index));
				FillRect(colorBounds(prev_index));
			}

			prev_index = index;
			snooze(20 * 1000);
			GetMouse(&point, &buttons, true);
		}
	} else if (index >= 0) {
		float distance = 0;
		BPoint original_point = point;
		while ((distance < 6) && (buttons)) {
			GetMouse(&point, &buttons);
			snooze(20 * 1000);
			distance = fabs(original_point.x - point.x) + fabs(original_point.y - point.y);
		}

		if ((distance >= 6) && buttons) {
			// Start a drag'n'drop session.
			BBitmap* dragged_map = new BBitmap(BRect(0, 0, 15, 15), B_RGB32, TRUE);
			BView* dragger_view
				= new BView(BRect(0, 0, 15, 15), "dragger_view", B_FOLLOW_NONE, B_WILL_DRAW);
			rgb_color c = ColorSet::currentSet()->colorAt(index);
			dragged_map->AddChild(dragger_view);
			dragged_map->Lock();
			float alpha = c.alpha;
			c.alpha = 127;
			dragger_view->SetHighColor(c);
			c.alpha = (uint8)alpha;
			dragger_view->FillRect(dragger_view->Bounds());
			dragger_view->Sync();
			dragged_map->Unlock();
			BMessage dragger_message(B_PASTE);
			dragger_message.AddData("RGBColor", B_RGB_COLOR_TYPE, &c, sizeof(rgb_color));
			BString hexColor;
			hexColor.SetToFormat("#%02x%02x%02x%02x", c.red, c.green, c.blue, c.alpha);
			dragger_message.AddData(
				"text/plain", B_MIME_TYPE, hexColor.String(), hexColor.Length());
			DragMessage(&dragger_message, dragged_map, B_OP_ALPHA, BPoint(7, 7));
			index = ColorSet::currentSet()->currentColorIndex(); // The active color did not change.
		}
	}

	if (index != -1) {
		// here we should make the current_color_index in active set to be index
		ColorSet::currentSet()->setCurrentColorIndex(index);

		// here we should inform the app that a color has been changed
		if (original_button & B_PRIMARY_MOUSE_BUTTON)
			((PaintApplication*)be_app)->SetColor(ColorSet::currentSet()->currentColor(), TRUE);
		else
			((PaintApplication*)be_app)->SetColor(ColorSet::currentSet()->currentColor(), FALSE);

		// we must also inform all other containers that the selection changed
		// so that they can highlight the proper rectangle, this will be done
		// with messages
		sendMessageToAllContainers(new BMessage(HS_PALETTE_SELECTION_CHANGED));

		// also inform the selected colors' views
		SelectedColorsView::sendMessageToAll(new BMessage(HS_COLOR_CHANGED));

		// give the window the information that selection has changed
		rgb_color c = ColorSet::currentSet()->currentColor();
		ColorPaletteWindow::ChangePaletteColor(c);
		Window()->PostMessage(HS_PALETTE_SELECTION_CHANGED, Window());
	} else {
		SetHighColor(255, 255, 255);
		StrokeRect(colorBounds(ColorSet::currentSet()->currentColorIndex()));
	}

	if (!highlight_selected) {
		// clear the highlight from selected color
		SetHighAndLowColors(ColorSet::currentSet()->currentColor());
		FillRect(colorBounds(ColorSet::currentSet()->currentColorIndex()), HS_2X2_BLOCKS);
	}
}


void
ColorContainer::MouseMoved(BPoint, uint32 transit, const BMessage*)
{
	// These are posted to the window in case that the window contains
	// a help view.
	if ((transit == B_ENTERED_VIEW) && (Window()->IsActive())) {
		BMessage* help_message = new BMessage(HS_TEMPORARY_HELP_MESSAGE);
		help_message->AddString("message", B_TRANSLATE("Click to select a painting color."));
		Window()->PostMessage(help_message);
		delete help_message;
	}
	if (transit == B_EXITED_VIEW)
		Window()->PostMessage(HS_TOOL_HELP_MESSAGE);
}


void
ColorContainer::MessageReceived(BMessage* message)
{
	switch (message->what) {

		// this message comes from ColorContainer::sendMessageToAllContainers and that function
		// is called in ColorWindow's MessageReceived, it's purpose is to inform
		case HS_PALETTE_CHANGED:
		{
			// here call the function that fits the view to
			// accommodate the new palette. 0 colors instructs the function
			// to look the amount from ColorSet.
			setUpContainer(Bounds(), 0, FALSE);
			Draw(Bounds());
		} break;
		case HS_NEXT_PALETTE:
		{
			ColorSet::moveToNextSet();
			BMessage paletteChanged(HS_PALETTE_CHANGED);
			ColorContainer::sendMessageToAllContainers(&paletteChanged);
		} break;
		case HS_PREVIOUS_PALETTE:
		{
			ColorSet::moveToPrevSet();
			BMessage paletteChanged(HS_PALETTE_CHANGED);
			ColorContainer::sendMessageToAllContainers(&paletteChanged);
		} break;
		// this message comes from ColorContainer::sendMessageToAllContainers and that function
		// is called in ColorWindow's MessageReceived, it informs us that one of the colors in the
		// set has been changed, this constant is used for the same purpose in a slight different
		// context the changed color is at "index" int32 data member in the message
		case HS_COLOR_CHANGED:
		{
			// the colorset has been updated, we will draw using colorChanged-function
			colorChanged(message->FindInt32("index"));
		} break;
		case B_SIMPLE_DATA:
		case B_MIME_DATA:
		{
			if (message->WasDropped()) {
				const char* colorr;
				ssize_t color_size;
				if (message->FindData(
						"text/plain", B_MIME_TYPE, (const void**)&colorr, &color_size) == B_OK) {
					char colorStr[color_size + 1];
					for (int i = 0; i < color_size; ++i)
						colorStr[i] = colorr[i];
					colorStr[color_size] = '\0';
					uint32 color_word;
					if (HexStringToUInt32(colorStr, color_word) == B_OK) {
						BMessage color_message(HS_COLOR_CHANGED);

						int32 buttons;
						if (message->FindInt32("buttons", &buttons) == B_OK)
							color_message.AddInt32("buttons", buttons);
						else
							color_message.AddInt32("buttons", 0);

						BPoint drop_point = message->DropPoint();
						drop_point = ConvertFromScreen(drop_point);
						int32 index = pointIndex(drop_point);
						if (index >= 0) {
							color_message.AddInt32("index", index);
							ColorSet::currentSet()->setCurrentColorIndex(index);
							ColorSet::currentSet()->setCurrentColor(BGRAColorToRGB(color_word));
							ColorContainer::sendMessageToAllContainers(&color_message);
							color_message.what = HS_PALETTE_SELECTION_CHANGED;
							ColorContainer::sendMessageToAllContainers(&color_message);
							Window()->PostMessage(HS_PALETTE_SELECTION_CHANGED, Window());
						}
					}
				}
			}
		} break;
		case B_PASTE:
		{
			if (message->WasDropped()) {
				// Here we see on to which button it was dropped and then
				// try to extract a color from the message
				rgb_color* color;
				ssize_t color_size;
				if (message->FindData(
						"RGBColor", B_RGB_COLOR_TYPE, (const void**)&color, &color_size) == B_OK) {
					BPoint drop_point = message->DropPoint();
					drop_point = ConvertFromScreen(drop_point);
					int32 index = pointIndex(drop_point);
					if (index >= 0) {
						ColorSet::currentSet()->setCurrentColorIndex(index);
						ColorSet::currentSet()->setCurrentColor(*color);
						BMessage a_message(HS_COLOR_CHANGED);
						a_message.AddInt32("index", index);
						ColorContainer::sendMessageToAllContainers(&a_message);
						a_message.what = HS_PALETTE_SELECTION_CHANGED;
						ColorContainer::sendMessageToAllContainers(&a_message);
						Window()->PostMessage(HS_PALETTE_SELECTION_CHANGED, Window());
					}
				}
			}
		} break;
		// this message comes from ColorContainer::sendMessageToAllContainers and that function
		// is called in ColorContainer::MouseDown, it informs us that the selected color in the
		// set has changed
		case HS_PALETTE_SELECTION_CHANGED:
		{
			// the selected color of palette has changed
			// if we highlight it then draw completely
			// because we don't know what was the previous color
			if (highlight_selected)
				Draw(Bounds());
		} break;
		default:
			BView::MessageReceived(message);
	}
}


void
ColorContainer::setUpContainer(BRect frame, int32 number_of_colors, bool add_arrows)
{
	// This gets stuck in an infinite loop if the height of the frame
	// is negative.
	horiz_c_size = 3;
	vert_c_size = 3;
	color_count = number_of_colors;
	row_count = 1;

	if (color_count == 0) {
		// here we take the new color-count from current color-set
		color_count = ColorSet::currentSet()->sizeOfSet();
	}

	int32 bound = sqrt(color_count);

	for (int i = bound; i > 0; --i) {
		if (color_count % i == 0) {
			row_count = i;
			break;
		}
	}

	if (row_count == 0)
		row_count = 1;

	column_count = color_count / row_count;

	vert_c_size = ceil(Bounds().Height() / row_count);
	horiz_c_size = ceil(Bounds().Width() / column_count);

	ResourceServer* server = ResourceServer::Instance();
	if (add_arrows && server) {
		BPicture arrow_pushed;
		BPicture arrow_not_pushed;

		server->GetPicture(LEFT_ARROW, &arrow_not_pushed);
		server->GetPicture(LEFT_ARROW_PUSHED, &arrow_pushed);

		left_arrow = new BPictureButton(BRect(0, 0, 8, 12), "left_arrow", &arrow_not_pushed,
			&arrow_pushed, new BMessage(HS_PREVIOUS_PALETTE));
		AddChild(left_arrow);
		left_arrow->ResizeToPreferred();
		left_arrow->MoveTo(BPoint(2, 2));

		server->GetPicture(RIGHT_ARROW, &arrow_not_pushed);
		server->GetPicture(RIGHT_ARROW_PUSHED, &arrow_pushed);

		right_arrow = new BPictureButton(BRect(0, 0, 8, 12), "right_arrow", &arrow_not_pushed,
			&arrow_pushed, new BMessage(HS_NEXT_PALETTE));
		AddChild(right_arrow);
		right_arrow->ResizeToPreferred();
		right_arrow->MoveTo(BPoint(2, left_arrow->Frame().bottom + 3));

		contains_arrows = true;
	}

	if (contains_arrows)
		ResizeBy(left_arrow->Frame().right + 2, 0);
}


void
ColorContainer::SetHighAndLowColors(const rgb_color& c)
{
	rgb_color low = c;
	rgb_color high = c;

	float coeff = c.alpha / 255.0;
	low.red = (uint8)(coeff * c.red);
	low.green = (uint8)(coeff * c.green);
	low.blue = (uint8)(coeff * c.blue);
	low.alpha = 255;

	high.red = (uint8)(coeff * c.red + (1 - coeff) * 255);
	high.green = (uint8)(coeff * c.green + (1 - coeff) * 255);
	high.blue = (uint8)(coeff * c.blue + (1 - coeff) * 255);
	high.alpha = 255;

	SetHighColor(high);
	SetLowColor(low);
}


BRect
ColorContainer::colorBounds(int32 index)
{
	// this function calculates the rectangle for the
	// palette entry at index that is to be drawn on screen
	BRect rect;

	int32 row = 1, column = 1;

	for (int32 i = 0; i < index; i++) {
		if (column >= color_count / row_count) {
			column = 1;
			row++;
		} else
			column++;
	}

	horiz_c_size = ceil(Frame().Width() / column_count);
	vert_c_size = ceil(Frame().Height() / row_count);

	float top = (row - 1) * vert_c_size;
	float left = (column - 1) * horiz_c_size;

	rect = BRect(left, top, left + horiz_c_size, top + vert_c_size);

	if ((contains_arrows == TRUE) && (left_arrow != NULL))
		rect.OffsetBy(left_arrow->Frame().right + 2, 0);

	return rect;
}


int32
ColorContainer::pointIndex(BPoint point)
{
	// this function returns which palette entry is at point in the view
	// or -1 if no entry is to found there
	// at the moment just calculate usin rect Contains-function
	// later should be implemented more elegantly

	for (int32 i = 0; i < color_count; i++) {
		if (colorBounds(i).Contains(point))
			return i;
	}

	// none of the rectangles contained point
	return -1;
}


void
ColorContainer::colorChanged(int32 color_index)
{
	// draw the new color
	rgb_color c = ColorSet::currentSet()->colorAt(color_index);

	SetHighAndLowColors(c);
	FillRect(colorBounds(color_index), HS_2X2_BLOCKS);

	// also draw the white rect around the color that is currently selected
	// could be some other that was changed
	if (highlight_selected) {
		SetHighColor(255, 255, 255);
		StrokeRect(colorBounds(ColorSet::currentSet()->currentColorIndex()));
	}
	// and we will Sync()
	Sync();
}


void
ColorContainer::FrameResized(float width, float height)
{
	Draw(Bounds());
	BView::FrameResized(width, height);
}


void
ColorContainer::sendMessageToAllContainers(BMessage* message)
{
	// here go through the list of color containers
	// and post message to each of them

	for (int32 i = 0; i < container_list->CountItems(); i++) {
		((ColorContainer*)container_list->ItemAt(i))
			->Window()
			->PostMessage(message, (ColorContainer*)container_list->ItemAt(i));
	}
}


// here begins the definition of ColorSet-class
// first initialize the static variables
BList* ColorSet::color_set_list = new BList();
int32 ColorSet::current_set_index = 0;


ColorSet::ColorSet(int32 amount_of_colors, ColorSet* copy_this_palette)
{
	// first check that the amount of colors is reasonable
	amount_of_colors = min_c(amount_of_colors, 256);
	amount_of_colors = max_c(amount_of_colors, 1);

	// first allocate memory for the new palette
	palette = new rgb_color[amount_of_colors];

	// allocate memory for name, 100 chars should be long enough
	name = new char[100];

	if (copy_this_palette == NULL) {
		// if no palette was given to copy from make a default palette
		for (int32 i = 0; i < amount_of_colors; i++) {
			palette[i].red = i * 256 / amount_of_colors;
			palette[i].green = i * 256 / amount_of_colors;
			palette[i].blue = i * 256 / amount_of_colors;
			palette[i].alpha = 255;
		}
	} else {
		// here we copy that palette
		int32 source_size = copy_this_palette->sizeOfSet();
		for (int i = 0; i < amount_of_colors; i++) {
			palette[i] = copy_this_palette->colorAt(i % source_size);
		}
	}
	// store the color count
	color_count = amount_of_colors;

	// create a default name
	strcpy(name, "default palette");

	// put the current color to 0
	current_color_index = 0;

	color_set_list->AddItem(this);
}


ColorSet::~ColorSet()
{
	// remove ourselves from the color_set_list
	color_set_list->RemoveItem(this);
	current_set_index = min_c(current_set_index, color_set_list->CountItems() - 1);
}


rgb_color
ColorSet::colorAt(int32 index)
{
	// check that the index is correct
	if ((index >= 0) && (index < color_count))
		return palette[index];
	else {
		// otherwise return the first color
		return palette[0];
	}
}


void
ColorSet::setColor(int32 index, rgb_color color)
{
	// check that the index is correct
	if (index >= 0 && index < color_count)
		palette[index] = color;
}


void
ColorSet::setName(const char* set_name)
{
	strcpy(name, set_name);
}


char*
ColorSet::getName()
{
	return name;
}


status_t
ColorSet::readSets(BFile& file)
{
	int32 number_of_sets;

	if (file.Read(&number_of_sets, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	for (int32 i = 0; i < number_of_sets; i++) {
		int32 size_of_set;
		if (file.Read(&size_of_set, sizeof(int32)) != sizeof(int32))
			return B_ERROR;

		ColorSet* new_set = new ColorSet(size_of_set);

		int32 name_length;
		char name[255];
		file.Read(&name_length, sizeof(int32));
		file.Read(name, name_length);
		name[name_length] = '\0';

		new_set->setName(name);

		for (int32 b = 0; b < size_of_set; b++) {
			rgb_color c;
			if (file.Read(&c, sizeof(rgb_color)) != sizeof(rgb_color)) {
				delete new_set;
				return B_ERROR;
			}

			new_set->palette[b] = c;
		}
	}

	return B_OK;
}


status_t
ColorSet::writeSets(BFile& file)
{
	int32 written_value = color_set_list->CountItems();

	if (file.Write(&written_value, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	for (int32 i = 0; i < color_set_list->CountItems(); i++) {
		written_value = ((ColorSet*)color_set_list->ItemAt(i))->sizeOfSet();
		if (file.Write(&written_value, sizeof(int32)) != sizeof(int32))
			return B_ERROR;

		written_value = strlen(((ColorSet*)color_set_list->ItemAt(i))->getName());
		if (file.Write(&written_value, sizeof(int32)) != sizeof(int32))
			return B_ERROR;

		written_value = file.Write(((ColorSet*)color_set_list->ItemAt(i))->getName(),
			strlen(((ColorSet*)color_set_list->ItemAt(i))->getName()));
		if (uint32(written_value) != strlen(((ColorSet*)color_set_list->ItemAt(i))->getName()))
			return B_ERROR;

		// here write the palette entries
		for (int32 b = 0; b < ((ColorSet*)color_set_list->ItemAt(i))->sizeOfSet(); b++) {
			rgb_color c = ((ColorSet*)color_set_list->ItemAt(i))->palette[b];
			if (file.Write(&c, sizeof(rgb_color)) != sizeof(rgb_color))
				return B_ERROR;
		}
	}

	return B_OK;
}


ColorChip::ColorChip(const char* name)
	:
	BControl("colorchip", name, new BMessage(COLOR_CHIP_INVOKED), B_WILL_DRAW),
	fChipBitmap(NULL),
	fColor(0)
{
	fColor = 0x7fff7f7f;
	fChipBitmap = new BBitmap(BRect(0.0, 0.0, 15.0, 15.0), B_RGBA32);
	rgb_color color = BGRAColorToRGB(fColor);

	Message()->AddData("RGBColor", B_RGB_COLOR_TYPE, &color, sizeof(rgb_color));
	Message()->AddInt32("buttons", 0);
}


ColorChip::~ColorChip()
{
	delete fChipBitmap;
}


void
ColorChip::Draw(BRect updateRect)
{
	uint32 color1, color2;
	rgb_color rgb1, rgb2;
	rgb1.red = rgb1.green = rgb1.blue = 0xBB;
	rgb2.red = rgb2.green = rgb2.blue = 0x99;
	rgb1.alpha = rgb2.alpha = 0xFF;
	color1 = RGBColorToBGRA(rgb1);
	color2 = RGBColorToBGRA(rgb2);

	if (SettingsServer* server = SettingsServer::Instance()) {
		BMessage settings;
		server->GetApplicationSettings(&settings);

		color1 = settings.GetUInt32(skBgColor1, color1);
		color2 = settings.GetUInt32(skBgColor2, color2);
	}

	BitmapUtilities::CheckerBitmap(fChipBitmap, color1, color2, 8);
	BBitmap* tmp = new BBitmap(fChipBitmap);
	BitmapUtilities::ClearBitmap(tmp, fColor);
	BitmapUtilities::CompositeBitmapOnSource(fChipBitmap, fChipBitmap, tmp, tmp->Bounds());
	DrawTiledBitmap(fChipBitmap, updateRect, updateRect.LeftTop());

	SetHighColor(0, 0, 0, 255);
	StrokeRect(Bounds());

	BControl::Draw(updateRect);
}


void
ColorChip::SetColor(uint32 color)
{
	fColor = color;
	Draw(Bounds());
}


void
ColorChip::MouseDown(BPoint point)
{
	BBitmap* dragged_map = new BBitmap(BRect(0, 0, 15, 15), B_RGB32, TRUE);
	BView* dragger_view
		= new BView(BRect(0, 0, 15, 15), "dragger_view", B_FOLLOW_NONE, B_WILL_DRAW);
	rgb_color c = BGRAColorToRGB(fColor);
	dragged_map->AddChild(dragger_view);
	dragged_map->Lock();
	dragger_view->SetHighColor(c);
	dragger_view->FillRect(dragger_view->Bounds());
	dragger_view->Sync();
	dragged_map->Unlock();
	BMessage dragger_message(B_PASTE);
	dragger_message.AddData("RGBColor", B_RGB_COLOR_TYPE, &c, sizeof(rgb_color));
	BString hexColor;
	hexColor.SetToFormat("#%02x%02x%02x%02x", c.red, c.green, c.blue, c.alpha);
	dragger_message.AddData("text/plain", B_MIME_TYPE, hexColor.String(), hexColor.Length());
	DragMessage(&dragger_message, dragged_map, BPoint(7, 7));
}
