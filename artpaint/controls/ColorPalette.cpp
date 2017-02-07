/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Alert.h>
#include <Bitmap.h>
#include <FilePanel.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <Path.h>
#include <PictureButton.h>
#include <Resources.h>
#include <Roster.h>
#include <stdlib.h>
#include <string.h>
#include <TextControl.h>

#include "FloaterManager.h"
#include "ColorPalette.h"
#include "MessageConstants.h"
#include "SettingsServer.h"
#include "FileIdentificationStrings.h"
#include "StatusView.h"
#include "RGBControl.h"
#include "CMYControl.h"
#include "HSVControl.h"
#include "YIQControl.h"
#include "YUVControl.h"
#include "MessageFilters.h"
#include "UtilityClasses.h"
#include "StringServer.h"
#include "ResourceServer.h"
#include "Patterns.h"
#include "PaintApplication.h"
#include "PaletteWindowClient.h"
#include "FilePanels.h"


// Initialize the static variable to NULL.
ColorPaletteWindow* ColorPaletteWindow::palette_window = NULL;
BList* ColorPaletteWindow::master_window_list = new BList();
BList* ColorPaletteWindow::palette_window_clients = new BList();


ColorPaletteWindow::ColorPaletteWindow(BRect frame, int32 mode)
	: BWindow(frame, StringServer::ReturnString(PALETTE_WINDOW_NAME_STRING),
		B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE |
		B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK | B_AVOID_FRONT)
	, open_panel(NULL)
	, save_panel(NULL)
{
	// here we just record the mode that user has requested
	// for displaying the controls (eg. RGBA, HSV, something else)
	selector_mode = mode;

	color_control = NULL;
	color_slider = NULL;

	window_feel feel = B_NORMAL_WINDOW_FEEL;
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skPaletteWindowVisible,
			true);

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
	// call some function that initializes the views depending on the mode
	if (!openControlViews(mode)) {
		// if for some reason view opening did not succeed we delete all of them
		// that were created
		deleteControlViews(mode);
	} else {
		// we can assume that everything went OK show ourselves on screen
		Show();
	}

	if (Lock()) {
		AddCommonFilter(new BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE,
			B_MOUSE_DOWN, window_activation_filter));
		AddCommonFilter(new BMessageFilter(B_KEY_DOWN,AppKeyFilterFunction));
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
		server->SetValue(SettingsServer::Application, skPaletteWindowFrame,
			Frame());
		server->SetValue(SettingsServer::Application, skPaletteWindowVisible,
			false);
		server->SetValue(SettingsServer::Application, skPaletteColorMode,
			selector_mode);
	}

	FloaterManager::RemoveFloater(this);
	palette_window = NULL;
}


void ColorPaletteWindow::MessageReceived(BMessage *message)
{
	// this path is used when getting the applications path to save
	// the palettes. It must be defined here instead of inside the
	// switch-clause
	BPath path;
	BMessage *color_message;
	rgb_color color;
	int32 buttons;

	switch (message->what) {

	// this comes from the HSColorControl object and indicates that it's value has changed
	case HS_COLOR_CONTROL_INVOKED:
		// send message to each container
		// message should contain the index of that color in color-set
		// this should only be done if the color_container is in edit-mode
		// and set the current color in the set for that color too
		color_message = new BMessage(HS_COLOR_CHANGED);
		color_message->AddInt32("index",ColorSet::currentSet()->currentColorIndex());
		// For some reason the alpha channel needs to be forced to 255
		color = color_control->ValueAsColor();
		color.alpha = 255;
		ColorSet::currentSet()->setCurrentColor(color);
		ColorContainer::sendMessageToAllContainers(color_message);
		SelectedColorsView::sendMessageToAll(color_message);

		// also change the color for mousebutton that was used
		message->FindInt32("buttons",&buttons);
		if (buttons & B_PRIMARY_MOUSE_BUTTON)
			((PaintApplication*)be_app)->SetColor(color, TRUE);
		else
			((PaintApplication*)be_app)->SetColor(color, FALSE);

		InformClients(ColorSet::currentSet()->currentColor());
		break;

	// This comes from the RGBControl-object and indicates that it's value has changed.
	// This might also come from CMYControl, YIQControl ...
	case HS_RGB_CONTROL_INVOKED:
		// send message to each container
		// message should contain the index of that color in color-set
		// this should only be done if the color_container is in edit-mode
		// and set the current color in the set for that color too
		color_message = new BMessage(HS_COLOR_CHANGED);
		color_message->AddInt32("index",ColorSet::currentSet()->currentColorIndex());
		ColorSet::currentSet()->setCurrentColor(color_slider->ValueAsColor());
		ColorContainer::sendMessageToAllContainers(color_message);
		SelectedColorsView::sendMessageToAll(color_message);

		// also change the color for mousebutton that was used
		message->FindInt32("buttons",&buttons);
		if (buttons & B_PRIMARY_MOUSE_BUTTON)
			((PaintApplication*)be_app)->SetColor(color_slider->ValueAsColor(),TRUE);
		else
			((PaintApplication*)be_app)->SetColor(color_slider->ValueAsColor(),FALSE);

		InformClients(ColorSet::currentSet()->currentColor());
		break;

	// this comes from the menubar->"Set"->"New Palette"->"N" and indicates that a new
	// palette-set should be created, the "colors"-parameter tells us how many colors the
	// new set has
	case HS_NEW_PALETTE_CREATED:
		new ColorSet(message->FindInt32("colors"));
		// inform the color-containers about change
		ColorContainer::sendMessageToAllContainers(new BMessage(HS_PALETTE_CHANGED));
		// here change the name view to display sets name
		if (ColorSet::numberOfSets() > 1) {
			menu_bar->FindItem(HS_DELETE_PALETTE)->SetEnabled(TRUE);
		}
		break;

	// this comes from the menubar->"Set"->"Delete Current Set" and indicates that the current
	// set should be deleted
	case HS_DELETE_PALETTE:
		// check if there are any more sets left
		if (ColorSet::numberOfSets() >= 2) {
			// first delete ourselves
			delete ColorSet::currentSet();
			// inform the color-containers about change
			ColorContainer::sendMessageToAllContainers(new BMessage(HS_PALETTE_CHANGED));
			// here change the name view to display sets name
			if (ColorSet::numberOfSets() <= 1) {
				menu_bar->FindItem(HS_DELETE_PALETTE)->SetEnabled(FALSE);
			}
		}
		else {
			(new BAlert("","Cannot delete the only set.","OK"))->Go();
		}
		break;

	// this comes from a button that is named "next set button", the button is in this window
	// the message indicates that we should change the colorcontainers to display next color set
	case HS_NEXT_PALETTE:
		// change the entry in palette list
		ColorSet::moveToNextSet();
		// inform all color-containers about palette-change
		ColorContainer::sendMessageToAllContainers(new BMessage(HS_PALETTE_CHANGED));
		break;

	// this comes from a button that is named "previous set button", the button is in this window
	// the message indicates that we should change the colorcontainers to display previous color set
	case HS_PREVIOUS_PALETTE:
		// change the entry in palette list
		ColorSet::moveToPrevSet();
		// inform all color-containers about palette-change
		ColorContainer::sendMessageToAllContainers(new BMessage(HS_PALETTE_CHANGED));
		break;

	// this is sent from ColorContainer::MouseDown and it's purpose is to
	// let us change the corresponding color to the color-controller also
	case HS_PALETTE_SELECTION_CHANGED:
		// update the color controller to display this new color
		// only should do it if container is in edit mode
		if (color_control != NULL)
			color_control->SetValue(ColorSet::currentSet()->currentColor());
		if (color_slider != NULL)
			color_slider->SetValue(ColorSet::currentSet()->currentColor());
		InformClients(ColorSet::currentSet()->currentColor());
		break;

	// this comes from the menubar->"Set"->"Open Set" and indicates that
	// a file panel should be opened for the purpose of loading new set
	case HS_SHOW_PALETTE_OPEN_PANEL:
		// here we must open the file-panel for loading
		// here get the path for palette-files, should be made relative to apps directory
		// or maybe remember the directory from last use

		if (open_panel == NULL) {
			// here we get the home directory of the application
			PaintApplication::HomeDirectory(path);

			// force normalization of the path to check validity
			if (path.Append("Palette sets/",TRUE) != B_OK) {
				PaintApplication::HomeDirectory(path);
			}

			entry_ref ref;
			get_ref_for_path(path.Path(), &ref);

			BMessenger target(this);
			BMessage message(HS_PALETTE_OPEN_REFS);
			open_panel = new BFilePanel(B_OPEN_PANEL, &target, &ref,
				B_FILE_NODE, true, &message);
		}
		char string[256];
		sprintf(string,"ArtPaint: %s",StringServer::ReturnString(OPEN_COLOR_SET_STRING));
		open_panel->Window()->SetTitle(string);
		set_filepanel_strings(open_panel);
		open_panel->Show();
		break;

	// this comes from the menubar->"Set"->"Save Set" and indicates that
	// a file panel should be opened for the purpose of saving current set
	case HS_SHOW_PALETTE_SAVE_PANEL:
		// here we must open the file-panel for saving
		if (save_panel == NULL) {
			// get the home directory of the app
			PaintApplication::HomeDirectory(path);

			// force normalization of the path to check validity
			if (path.Append("Palette sets/",TRUE) != B_OK) {
				PaintApplication::HomeDirectory(path);
			}
			// convert it to entry_ref
			entry_ref ref;
			get_ref_for_path(path.Path(), &ref);

			BMessenger target(this);
			BMessage message(HS_PALETTE_SAVE_REFS);
			save_panel = new BFilePanel(B_SAVE_PANEL, &target, &ref, 0, false,
				&message);
		}
		save_panel->SetSaveText(ColorSet::currentSet()->getName());
		sprintf(string,"ArtPaint: %s",StringServer::ReturnString(SAVE_COLOR_SET_STRING));
		save_panel->Window()->SetTitle(string);
		set_filepanel_strings(save_panel);
		save_panel->Show();
		break;

	// this comes from the open_panel or the applocation-object and includes refs for
	// the palette-files that should be loaded
	case HS_PALETTE_OPEN_REFS:
		handlePaletteLoad(message);

		// inform all color containers that palette has changed
		ColorContainer::sendMessageToAllContainers(new BMessage(HS_PALETTE_CHANGED));
		if (ColorSet::numberOfSets() > 1) {
			menu_bar->FindItem(HS_DELETE_PALETTE)->SetEnabled(TRUE);
		}
		break;

	// this comes from the save_panel and indicates that current set should be saved
	// to the file that the "refs" indicate
	case HS_PALETTE_SAVE_REFS:
		handlePaletteSave(message);
		break;

	// this comes from the menubar->"Mode"->"RGB-Mode" and indicates that
	// the color selector should be changed to a RGBControl, this is used
	// also for other purposes than just a message-constant
	case HS_RGB_COLOR_MODE:
		if (selector_mode != HS_RGB_COLOR_MODE) {
			deleteControlViews(selector_mode);
			selector_mode = HS_RGB_COLOR_MODE;
			openControlViews(HS_RGB_COLOR_MODE);
		}
		break;

	// this comes from the menubar->"Mode"->"CMY-Mode" and indicates that
	// the color selector should be changed to a RGBControl, this is used
	// also for other purposes than just a message-constant
	case HS_CMY_COLOR_MODE:
		if (selector_mode != HS_CMY_COLOR_MODE) {
			deleteControlViews(selector_mode);
			selector_mode = HS_CMY_COLOR_MODE;
			openControlViews(HS_CMY_COLOR_MODE);
		}
		break;

	// this comes from the menubar->"Mode"->"YIQ-Mode" and indicates that
	// the color selector should be changed to a RGBControl, this is used
	// also for other purposes than just a message-constant
	case HS_YIQ_COLOR_MODE:
		if (selector_mode != HS_YIQ_COLOR_MODE) {
			deleteControlViews(selector_mode);
			selector_mode = HS_YIQ_COLOR_MODE;
			openControlViews(HS_YIQ_COLOR_MODE);
		}
		break;

	// this comes from the menubar->"Mode"->"YUV-Mode" and indicates that
	// the color selector should be changed to a RGBControl, this is used
	// also for other purposes than just a message-constant
	case HS_YUV_COLOR_MODE:
		if (selector_mode != HS_YUV_COLOR_MODE) {
			deleteControlViews(selector_mode);
			selector_mode = HS_YUV_COLOR_MODE;
			openControlViews(HS_YUV_COLOR_MODE);
		}
		break;

	case HS_HSV_COLOR_MODE:
		if (selector_mode != HS_HSV_COLOR_MODE) {
			deleteControlViews(selector_mode);
			selector_mode = HS_HSV_COLOR_MODE;
			openControlViews(HS_HSV_COLOR_MODE);
		}
		break;


	// this comes from the menubar->"Mode"->"Simple-Mode" and indicates that
	// the color selector should be changed to a HSColorControl, this is used
	// also for other purposes than just a message-constant
	case HS_SIMPLE_COLOR_MODE:
		if (selector_mode != HS_SIMPLE_COLOR_MODE) {
			deleteControlViews(selector_mode);
			selector_mode = HS_SIMPLE_COLOR_MODE;
			openControlViews(HS_SIMPLE_COLOR_MODE);
		}
		break;

	default:
		BWindow::MessageReceived(message);
		break;
	}
}

bool ColorPaletteWindow::QuitRequested()
{
	// We might do something useful here.
	return TRUE;
}

bool ColorPaletteWindow::openControlViews(int32 mode)
{
	// first we open the views that are common to all modes of color palette
	// at least the menubar and the color-set container and controls that can change
	// the color-set or its name

	// these variables are used to position the views correctly
	float top , left;

	// update the top to be under menu_bar
	top = menu_bar->Frame().Height() + 1;

	box1 = new BBox(BRect(0,top,0,top));
	box1->SetBorder(B_PLAIN_BORDER);
	// here open the color_container
	color_container = new ColorContainer(BRect(0,3,63,66),ColorSet::currentSet()->sizeOfSet());
	color_container->SetDraggingEnabled(TRUE);
	box1->AddChild(color_container);

	// update the top to be under container in the box1 coordinates that is
	top = color_container->Frame().bottom + 4;

	// Here add the buttons that control the color-set.
	ResourceServer* server = ResourceServer::Instance();
	if (server) {
		BPicture arrow_pushed;
		BPicture arrow_not_pushed;

		server->GetPicture(LEFT_ARROW, &arrow_not_pushed);
		server->GetPicture(LEFT_ARROW_PUSHED, &arrow_pushed);

		previous_set = new BPictureButton(BRect(3, top, 11, top + 12),
			"left_arrow", &arrow_not_pushed, &arrow_pushed,
			new BMessage(HS_PREVIOUS_PALETTE));
		box1->AddChild(previous_set);
		previous_set->SetTarget(this);
		previous_set->ResizeToPreferred();

		server->GetPicture(RIGHT_ARROW, &arrow_not_pushed);
		server->GetPicture(RIGHT_ARROW_PUSHED, &arrow_pushed);

		next_set = new BPictureButton(previous_set->Frame(), "right arrow",
			&arrow_not_pushed, &arrow_pushed, new BMessage(HS_NEXT_PALETTE));
		box1->AddChild(next_set);
		next_set->SetTarget(this);
		next_set->ResizeToPreferred();
		next_set->MoveBy(next_set->Frame().Width() + 3,0);
	}

	// here resize the box1 to appropriate size
	box1->ResizeTo(max_c(next_set->Frame().right + 20,color_container->Frame().Width()+6),next_set->Frame().bottom+3);

	// here center the views horizontally
	color_container->MoveBy((box1->Frame().Width() - color_container->Frame().Width())/2,0);

	AddChild(box1);
	left = box1->Frame().right + 1;
	top = menu_bar->Frame().Height() + 1;

	// this will be assigned to RGBControl or similar object
	BMessage *invocation_message;
	// here we open the views that show color controls e.g. RGB- or HSV-controls
	switch (mode) {

	// in this case just open a HSColorControl and color-set container
		case HS_SIMPLE_COLOR_MODE:
			box2 = new BBox(BRect(box1->Frame().right+1,top,box1->Frame().right+10,box1->Frame().bottom));
			color_control = new HSColorControl(BPoint(5,0),B_CELLS_32x8,8,"");
			color_control->SetTarget(this);
			box2->ResizeTo(color_control->Frame().Width()+10,box2->Frame().Height());

			ResizeTo(box2->Frame().right,max_c(box1->Frame().Height() + top-1,color_control->Frame().Height() + top));
			box2->AddChild(color_control);
			AddChild(box2);
			// here center the color control vertically
			color_control->MoveBy(0,(box2->Frame().Height() - color_control->Frame().Height())/2);
			break;

	// in this case open an RGBControl
		case HS_RGB_COLOR_MODE: case HS_CMY_COLOR_MODE: case HS_YIQ_COLOR_MODE: case HS_YUV_COLOR_MODE:
		case HS_HSV_COLOR_MODE:
			box2 = new BBox(BRect(box1->Frame().right+1,top,box1->Frame().right+10,box1->Frame().bottom));

			if (mode == HS_RGB_COLOR_MODE) {
				rgb_color c = {0,0,0,255};
				color_slider = new RGBControl(BPoint(5,0),c);
			}
			else if (mode == HS_CMY_COLOR_MODE) {
				rgb_color c = {255,255,255,255};
				color_slider = new CMYControl(BPoint(5,0),c);
			}
			else if (mode == HS_YIQ_COLOR_MODE) {
				rgb_color c = {255,255,255,255};
				color_slider = new YIQControl(BPoint(5,0),c);
			}
			else if (mode == HS_YUV_COLOR_MODE) {
				rgb_color c = {255,255,255,255};
				color_slider = new YUVControl(BPoint(5,0),c);
			}
			else if (mode == HS_HSV_COLOR_MODE) {
				rgb_color c = {255,255,255,255};
				color_slider = new HSVControl(BPoint(5,0),c);
			}
			box2->AddChild(color_slider);
			AddChild(box2);

			color_slider->SetTarget(this);
			box2->ResizeTo(color_slider->Frame().Width()+10,box2->Frame().Height());
			ResizeTo(box2->Frame().right,max_c(box1->Frame().Height() + top-1,color_slider->Frame().Height() + top));

			// here center the color control vertically
			color_slider->MoveBy(0,(box2->Frame().Height() - color_slider->Frame().Height())/2);
			invocation_message = new BMessage(HS_RGB_CONTROL_INVOKED);
			invocation_message->AddInt32("buttons",0);
			color_slider->SetMessage(invocation_message);
			break;

		default:
			return TRUE;
	}
	box2->SetBorder(B_PLAIN_BORDER);

	// Update the color-controllers and slider's values
	if (color_control != NULL)
		color_control->SetValue(ColorSet::currentSet()->currentColor());
	if (color_slider != NULL)
		color_slider->SetValue(ColorSet::currentSet()->currentColor());

	return TRUE;
}



void ColorPaletteWindow::deleteControlViews(int32)
{
	// here we delete all views that are not NULL
	box1->RemoveSelf();
	delete box1;

	box2->RemoveSelf();
	delete box2;

// NULL all the controls
	color_control = NULL;
	color_slider = NULL;
}


void ColorPaletteWindow::openMenuBar()
{
	BMenu *menu;
	BMenu *sub_menu;
	BMenuItem *menu_item;

	menu_bar = new BMenuBar(BRect(0,0,0,0),"menu bar");
	menu = new BMenu(StringServer::ReturnString(COLOR_SET_STRING));
	menu_bar->AddItem(menu);

	sub_menu = new BMenu(StringServer::ReturnString(NEW_COLOR_SET_STRING));

	// in this loop we add possible palette sizes to menu
	BMessage *msg;
	char item_title[20] = "";
	for (int32 i = 3; i <= 6; i++) {
		sprintf(item_title, "%ld %s",((int32)pow(2, i)),
			StringServer::ReturnString(COLORS_STRING));
		msg = new BMessage(HS_NEW_PALETTE_CREATED);
		msg->AddInt32("colors", ((int32)pow(2, i)));
		menu_item = new BMenuItem(item_title, msg);
		sub_menu->AddItem(menu_item);
	}
	// the palette window will handle all things that concern making or loading new palette
	sub_menu->SetTargetForItems(this);
	menu->AddItem(sub_menu);

	menu_item = new BMenuItem(StringServer::ReturnString(DELETE_CURRENT_SET_STRING),new BMessage(HS_DELETE_PALETTE));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	if (ColorSet::numberOfSets() <= 1) {
		menu_item->SetEnabled(FALSE);
	}

	menu->AddItem(new BSeparatorItem());
	menu_item  = new BMenuItem(StringServer::ReturnString(OPEN_COLOR_SET_STRING),new BMessage(HS_SHOW_PALETTE_OPEN_PANEL));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	menu_item  = new BMenuItem(StringServer::ReturnString(SAVE_COLOR_SET_STRING),new BMessage(HS_SHOW_PALETTE_SAVE_PANEL));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);

	menu = new BMenu(StringServer::ReturnString(COLOR_MODEL_STRING));
	menu_bar->AddItem(menu);

	char string[256];
	sprintf(string,"RGB");
	menu_item = new BMenuItem(string,new BMessage(HS_RGB_COLOR_MODE));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	sprintf(string,"CMY");
	menu_item = new BMenuItem(string,new BMessage(HS_CMY_COLOR_MODE));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	sprintf(string,"HSV");
	menu_item = new BMenuItem(string,new BMessage(HS_HSV_COLOR_MODE));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	sprintf(string,"YIQ");
	menu_item = new BMenuItem(string,new BMessage(HS_YIQ_COLOR_MODE));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	sprintf(string,"YUV");
	menu_item = new BMenuItem(string,new BMessage(HS_YUV_COLOR_MODE));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	sprintf(string,"BeOS");
	menu_item = new BMenuItem(string,new BMessage(HS_SIMPLE_COLOR_MODE));
	menu_item->SetTarget(this);
	menu->AddItem(menu_item);
	menu->SetRadioMode(TRUE);
	menu->FindItem(selector_mode)->SetMarked(TRUE);

	AddChild(menu_bar);
}

void ColorPaletteWindow::handlePaletteLoad(BMessage *message)
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
	if ( type != B_REF_TYPE )
		return;

	for ( long i = --count; i >= 0; i-- ) {
		if ( message->FindRef("refs", i, &ref) == B_OK ) {
			BFile file;
			if ( file.SetTo(&ref, B_READ_ONLY) == B_OK ) {
				// the file was succesfully opened
				// here we check that it is actually a palette file
				// and read it if it is and also generate a new palette to
				// list of palettes
				if ((bytes_read = file.Read(file_type,strlen(HS_PALETTE_ID_STRING))) < 0) {
					// here some error reading file has happened
					(new BAlert("","Error reading file","OK"))->Go();
				}
				else {
					// the read was succesfull, terminate the id string and compare
					file_type[bytes_read] = '\0';
					if (strcmp(file_type,HS_PALETTE_ID_STRING) != 0) {
						// this was not a palette file
						(new BAlert("","Not a palette file","OK"))->Go();
					}
					else {
						// this was palette file, read the rest of it
						int32 palette_size;
						if ((bytes_read = file.Read(&palette_size,sizeof(int32))) != sizeof(int32)) {
							// here some error reading file has happened
							(new BAlert("","File structure corrupted","OK"))->Go();
						}
						else {
							// create the palette and read the palette colors
							ColorSet *loaded_set = new ColorSet(palette_size);
							rgb_color loaded_color;

							// here is the loop that reads palette entries from file
							for (int32 i = 0; i < palette_size; i++) {
								file.Read(&loaded_color,sizeof(rgb_color));
								loaded_set->setColor(i,loaded_color);
							}
							// this array holds the palette-name
							char palette_name[256];
							// read as many bytes as there is left to be the palette name
							bytes_read = file.Read(palette_name,255);
							palette_name[bytes_read] = '\0';
							loaded_set->setName(palette_name);
						}

					}
				}
			}
		}
	}
}


void ColorPaletteWindow::handlePaletteSave(BMessage *message)
{
	uint32 type;
	int32 count;
	entry_ref ref;

	// we can probably assume that the ref is actually a file
	message->GetInfo("directory", &type, &count);
	if ( type != B_REF_TYPE ) {
		return;
	}
	// here take a pointer to current color-set, so that if current set is changed
	// this will still save the same set
	ColorSet *color_set = ColorSet::currentSet();

	for ( long i = --count; i >= 0; i-- ) {
		if ( message->FindRef("directory", i, &ref) == B_OK ) {
			BFile file;
			BDirectory directory = BDirectory(&ref);
			if ( file.SetTo(&directory,message->FindString("name",i), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE) == B_OK ) {
				// here we write the current color-set to file
				// first set the file's type and other metadata
				// get the applications signature
				app_info info;
				be_app->GetAppInfo(&info);

				BNodeInfo node(&file);
				node.SetType(HS_PALETTE_MIME_STRING);
				node.SetPreferredApp(info.signature);

				if (file.Write(HS_PALETTE_ID_STRING,strlen(HS_PALETTE_ID_STRING)) < 0) {
					// error happened
					(new BAlert("","Cannot write to file","OK"))->Go();
				}
				else {
					// write the rest of the file
					int32 size = color_set->sizeOfSet();
					file.Write(&size,sizeof(int32));

					// this loop writes the color entries
					for (int32 i=0;i<size;i++) {
						rgb_color c = color_set->colorAt(i);
						file.Write(&c,sizeof(rgb_color));
					}

					// finally write the name of set
					file.Write(color_set->getName(),strlen(color_set->getName()));
				}
			}
		}
	}
}


void
ColorPaletteWindow::showPaletteWindow(BMessage *msg)
{
	if (palette_window == NULL) {
		BRect frame(300, 100, 400, 200);
		color_window_modes mode = HS_RGB_COLOR_MODE;

		if (SettingsServer* server = SettingsServer::Instance()) {
			BMessage settings;
			server->GetApplicationSettings(&settings);
			settings.FindRect(skPaletteWindowFrame, &frame);
			settings.FindInt32(skPaletteColorMode, (int32*)&mode);
		}

		ColorPaletteWindow* window = new ColorPaletteWindow(frame, mode);
		for (int32 i = 0; i < master_window_list->CountItems(); ++i)
			((BWindow*)master_window_list->ItemAt(i))->AddToSubset(window);
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

		palette_window->Unlock();
	}
}


void
ColorPaletteWindow::setFeel(window_feel feel)
{
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skPaletteWindowFeel,
			int32(feel));
	}

	if (palette_window) {
		window_look look = B_FLOATING_WINDOW_LOOK;
		if (feel == B_NORMAL_WINDOW_FEEL)
			look = B_TITLED_WINDOW_LOOK;

		palette_window->SetFeel(feel);
		palette_window->SetLook(look);
	}
}


void
ColorPaletteWindow::AddMasterWindow(BWindow *window)
{
	master_window_list->AddItem(window);
	if (palette_window)
		window->AddToSubset(palette_window);
}


void
ColorPaletteWindow::RemoveMasterWindow(BWindow *window)
{
	master_window_list->RemoveItem(window);
}



void
ColorPaletteWindow::AddPaletteWindowClient(PaletteWindowClient *client)
{
	if (!palette_window_clients->HasItem(client))
		palette_window_clients->AddItem(client);
}


void
ColorPaletteWindow::RemovePaletteWindowClient(PaletteWindowClient *client)
{
	palette_window_clients->RemoveItem(client);
}


void
ColorPaletteWindow::InformClients(const rgb_color& c)
{
	for (int32 i = 0; i < palette_window_clients->CountItems(); ++i) {
		PaletteWindowClient* client =
			static_cast<PaletteWindowClient*>(palette_window_clients->ItemAt(i));
		if (client)
			client->PaletteColorChanged(c);
	}
}


// here is the definition of HSColorControl-class
HSColorControl::HSColorControl(BPoint location, color_control_layout matrix,
		float cellSide, const char *name)
	: BColorControl(location, matrix, cellSide, name)
{
	// Set the message here so that using the text boxes before the sliders will work
	BMessage *message = new BMessage(HS_COLOR_CONTROL_INVOKED);
	message->AddInt32("buttons", B_PRIMARY_MOUSE_BUTTON);
	SetMessage(message);
}

void HSColorControl::MouseDown(BPoint location)
{
	// here we take the button that was pressed
	int32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	// set the invokers message to contain the mouse-button that was last pressed
	BMessage *message = new BMessage(HS_COLOR_CONTROL_INVOKED);
	message->AddInt32("buttons",buttons);
	SetMessage(message);

	// call the inherited MouseDown-function
	BColorControl::MouseDown(location);
}




// here starts the definitions for ColorContainer class

// here define the variable that points to list of color containers
BList* ColorContainer::container_list = new BList();


ColorContainer::ColorContainer(BRect frame, int32 amount_of_colors, uint32 resizingMode, bool highlight,bool add_arrows)
					: BView(frame,"color container",resizingMode,B_WILL_DRAW)
{
	// here initialize the important variables

	highlight_selected = highlight;
	left_arrow = NULL;
	right_arrow = NULL;
	contains_arrows = FALSE;

	setUpContainer(frame,amount_of_colors,add_arrows);

	// add this container to the list
	container_list->AddItem(this);

	dragging_enabled = FALSE;
}



ColorContainer::~ColorContainer()
{
	// here remove ourselves from container list
	container_list->RemoveItem(this);
}


void ColorContainer::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());

	if (left_arrow != NULL)
		left_arrow->SetTarget(this);
	if (right_arrow != NULL)
		right_arrow->SetTarget(this);
}

void ColorContainer::Draw(BRect)
{
	// here we draw the colors with FillRect
	// we get the colors from palette that is held somewhere
	// every instance of this class should also draw whenever
	// a palette entry changes, how should we achieve that ????

	BRect rect;

	for (int32 i=0;i<color_count;i++) {

		rect = colorBounds(i);
		SetHighAndLowColors(ColorSet::currentSet()->colorAt(i));
		FillRect(rect,HS_2X2_BLOCKS);
	}

	if (highlight_selected) {
		// also draw the rectangle around selected color if required
		SetHighColor(255,255,255,255);
		StrokeRect(colorBounds(ColorSet::currentSet()->currentColorIndex()));
	}
}



void ColorContainer::MouseDown(BPoint point)
{
	// here we highlight the color that is under the cursor
	// and when we have selected the color we inform some
	// third parties about that
	uint32 buttons = 0;
	uint32 original_button;

	GetMouse(&point, &buttons, true);
	int32 index,prev_index = ColorSet::currentSet()->currentColorIndex();

	// first draw the rectangle around the newly selected color
	index = pointIndex(point);
	if (index != -1) {
		// only of course if some color was actually selected
		SetHighColor(255,255,255);
		StrokeRect(colorBounds(index));
	}

	original_button = buttons;

	if (dragging_enabled == FALSE) {
		while ( buttons ) {
			index = pointIndex(point);
			if ((index != -1) && (index != prev_index)) {
				// point is over some new color-entry
				if (prev_index != -1) {
					// re-draw the previous rectangle
					SetHighAndLowColors(ColorSet::currentSet()->colorAt(prev_index));
					FillRect(colorBounds(prev_index),HS_2X2_BLOCKS);
				}
				// then draw rect around new color
				SetHighColor(255,255,255);
				StrokeRect(colorBounds(index));
			}
			else if ((index == -1) && (prev_index != -1)) {
				// we fill the previous rectangle
				SetHighColor(ColorSet::currentSet()->colorAt(prev_index));
				FillRect(colorBounds(prev_index));
			}

			prev_index = index;
			snooze(20 * 1000);
			GetMouse(&point, &buttons, true);
		}
	}
	else if (index >= 0) {
		float distance = 0;
		BPoint original_point = point;
		while ((distance < 6) && (buttons)) {
			GetMouse(&point,&buttons);
			snooze(20 * 1000);
			distance = fabs(original_point.x-point.x) + fabs(original_point.y-point.y);
		}

		if ((distance >= 6) && buttons) {
			// Start a drag'n'drop session.
			BBitmap *dragged_map = new BBitmap(BRect(0,0,15,15),B_RGB32,TRUE);
			BView *dragger_view = new BView(BRect(0,0,15,15),"dragger_view",B_FOLLOW_NONE,B_WILL_DRAW);
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
			dragger_message.AddData("RGBColor",B_RGB_COLOR_TYPE,&c,sizeof(rgb_color));
			DragMessage(&dragger_message,dragged_map,B_OP_ALPHA,BPoint(7,7));
//			dragged = TRUE;
			index = ColorSet::currentSet()->currentColorIndex();	// The active color did not change.
		}
	}

	if (index != -1) {
		// here we should make the current_color_index in active set to be index
		ColorSet::currentSet()->setCurrentColorIndex(index);

		// here we should inform the app that a color has been changed
		if (original_button & B_PRIMARY_MOUSE_BUTTON)
			((PaintApplication*)be_app)->SetColor(ColorSet::currentSet()->currentColor(),TRUE);
		else
			((PaintApplication*)be_app)->SetColor(ColorSet::currentSet()->currentColor(),FALSE);

		// we must also inform all other containers that the selection changed
		// so that they can highlight the proper rectangle, this will be done
		// with messages
		sendMessageToAllContainers(new BMessage(HS_PALETTE_SELECTION_CHANGED));

		// also inform the selected colors' views
		SelectedColorsView::sendMessageToAll(new BMessage(HS_COLOR_CHANGED));



		// give the window the information that selection has changed
		rgb_color c = ColorSet::currentSet()->currentColor();
		ColorPaletteWindow::ChangePaletteColor(c);
		Window()->PostMessage(HS_PALETTE_SELECTION_CHANGED,Window());
	}
	else {
		SetHighColor(255,255,255);
		StrokeRect(colorBounds(ColorSet::currentSet()->currentColorIndex()));
	}

	if (!highlight_selected) {
		// clear the highlight from selected color
		SetHighAndLowColors(ColorSet::currentSet()->currentColor());
		FillRect(colorBounds(ColorSet::currentSet()->currentColorIndex()),HS_2X2_BLOCKS);
	}

}



void ColorContainer::MouseMoved(BPoint,uint32 transit,const BMessage*)
{
	// These are posted to the window in case that the window contains
	// a help view.
	if ((transit == B_ENTERED_VIEW) && (Window()->IsActive())) {
		BMessage *help_message = new BMessage(HS_TEMPORARY_HELP_MESSAGE);
		help_message->AddString("message",StringServer::ReturnString(CLICK_TO_SELECT_COLOR_STRING));
		Window()->PostMessage(help_message);
		delete help_message;
	}
	if (transit == B_EXITED_VIEW) {
		Window()->PostMessage(HS_TOOL_HELP_MESSAGE);
	}
}


void ColorContainer::MessageReceived(BMessage *message)
{
	switch (message->what) {

	// this message comes from ColorContainer::sendMessageToAllContainers and that function
	// is called in ColorWindow's MessageReceived, it's purpose is to inform
	case HS_PALETTE_CHANGED:
		// here call the function that fits the view to
		// accommodate the new palette. 0 colors instructs the function
		// to look the amount from ColorSet.
		setUpContainer(Bounds(),0,FALSE);
		Draw(Bounds());
		break;

	case HS_NEXT_PALETTE:
		ColorSet::moveToNextSet();
		ColorContainer::sendMessageToAllContainers(new BMessage(HS_PALETTE_CHANGED));
		break;
	case HS_PREVIOUS_PALETTE:
		ColorSet::moveToPrevSet();
		ColorContainer::sendMessageToAllContainers(new BMessage(HS_PALETTE_CHANGED));
		break;

	// this message comes from ColorContainer::sendMessageToAllContainers and that function
	// is called in ColorWindow's MessageReceived, it informs us that one of the colors in the
	// set has been changed, this constant is used for the same purpose in a slight different context
	// the changed color is at "index" int32 data member in the message
	case HS_COLOR_CHANGED:
		// the colorset has been updated, we will draw using colorChanged-function
		colorChanged(message->FindInt32("index"));
		break;

	case B_PASTE:
		if (message->WasDropped()) {
			// Here we see on to which button it was dropped and then
			// try to extract a color from the message
			rgb_color *color;
			ssize_t color_size;
			if (message->FindData("RGBColor",B_RGB_COLOR_TYPE,(const void**)&color,&color_size) == B_OK) {
				BPoint drop_point = message->DropPoint();
				drop_point = ConvertFromScreen(drop_point);
				int32 index = pointIndex(drop_point);
				if (index >= 0) {
					ColorSet::currentSet()->setCurrentColorIndex(index);
					ColorSet::currentSet()->setCurrentColor(*color);
					BMessage a_message(HS_COLOR_CHANGED);
					a_message.AddInt32("index",index);
					ColorContainer::sendMessageToAllContainers(&a_message);
					a_message.what = HS_PALETTE_SELECTION_CHANGED;
					ColorContainer::sendMessageToAllContainers(&a_message);
					Window()->PostMessage(HS_PALETTE_SELECTION_CHANGED,Window());
				}
			}
		}
		break;
	// this message comes from ColorContainer::sendMessageToAllContainers and that function
	// is called in ColorContainer::MouseDown, it informs us that the selected color in the
	// set has changed
	case HS_PALETTE_SELECTION_CHANGED:
		// the selected color of palette has changed
		// if we highlight it then draw completely
		// because we don't know what was the previous color
		if (highlight_selected)
			Draw(Bounds());
		break;
	default:
		BView::MessageReceived(message);
		break;
	}
}

void ColorContainer::setUpContainer(BRect frame, int32 number_of_colors,bool add_arrows)
{
	// This gets stuck in an infinite loop if the height of the frame
	// is negative.
	horiz_c_size = 3;
	vert_c_size = 3;
	horiz_gutter = 1;
	vert_gutter = 1;
	color_count = number_of_colors;
	row_count = 1;

	if (color_count == 0) {
		// here we take the new color-count from current color-set
		color_count = ColorSet::currentSet()->sizeOfSet();
	}

	// first count how many rows are to be used
	while ((row_count<=color_count/row_count) && (row_count*vert_c_size<=frame.Height()))
		row_count *= 2;
	row_count /= 2;

	// then increase the row height to maximum
	while ((row_count*(vert_c_size + vert_gutter) - vert_gutter)<=frame.Height())
		vert_c_size++;
	vert_c_size--;

	// then increase the width to maximum
	float maximum_width = frame.Width();
	if (left_arrow != NULL) {
		maximum_width -= (left_arrow->Frame().right + 2);
	}
	while ((color_count/row_count*(horiz_c_size+horiz_gutter) - horiz_gutter) <= maximum_width)
		horiz_c_size++;
	horiz_c_size--;

	ResourceServer* server = ResourceServer::Instance();
	if (add_arrows && server) {
		BPicture arrow_pushed;
		BPicture arrow_not_pushed;

		server->GetPicture(LEFT_ARROW, &arrow_not_pushed);
		server->GetPicture(LEFT_ARROW_PUSHED, &arrow_pushed);

		left_arrow = new BPictureButton(BRect(0, 0, 8, 12), "left_arrow",
			&arrow_not_pushed, &arrow_pushed, new BMessage(HS_PREVIOUS_PALETTE));
		AddChild(left_arrow);
		left_arrow->ResizeToPreferred();
		left_arrow->MoveTo(BPoint(2, 2));

		server->GetPicture(RIGHT_ARROW, &arrow_not_pushed);
		server->GetPicture(RIGHT_ARROW_PUSHED, &arrow_pushed);

		right_arrow = new BPictureButton(BRect(0, 0, 8, 12), "right_arrow",
			&arrow_not_pushed, &arrow_pushed, new BMessage(HS_NEXT_PALETTE));
		AddChild(right_arrow);
		right_arrow->ResizeToPreferred();
		right_arrow->MoveTo(BPoint(2, left_arrow->Frame().bottom + 3));

		contains_arrows = true;
	}

	// here resize the view to just fit the colors
	ResizeTo((color_count/row_count*(horiz_c_size+horiz_gutter) - horiz_gutter),(row_count*(vert_c_size + vert_gutter) - vert_gutter));

	if (contains_arrows) {
		ResizeBy(left_arrow->Frame().right+2,0);
	}
}


void ColorContainer::SetHighAndLowColors(const rgb_color &c)
{
	rgb_color low = c;
	rgb_color high = c;

	float coeff = c.alpha / 255.0;
	low.red = (uint8)(coeff*c.red);
	low.green = (uint8)(coeff*c.green);
	low.blue = (uint8)(coeff*c.blue);
	low.alpha = 255;

	high.red = (uint8)(coeff*c.red + (1-coeff)*255);
	high.green = (uint8)(coeff*c.green + (1-coeff)*255);
	high.blue = (uint8)(coeff*c.blue + (1-coeff)*255);
	high.alpha = 255;

	SetHighColor(high);
	SetLowColor(low);
}

BRect ColorContainer::colorBounds(int32 index)
{
	// this function calculates the rectangle for the
	// palette entry at index that is to be drawn on screen
	BRect rect;
	int32 row=1,column=1;
	for (int32 i=0;i<index;i++) {
		if (column >= color_count/row_count) {
			column = 1;
			row++;
		}
		else
			column++;
	}
	rect = BRect((column-1)*(horiz_c_size + horiz_gutter),(row-1)*(vert_c_size + vert_gutter),column*(horiz_c_size + horiz_gutter) - horiz_gutter,row*(vert_c_size + vert_gutter) - vert_gutter);
	if ((contains_arrows == TRUE) && (left_arrow != NULL)) {
		rect.OffsetBy(left_arrow->Frame().right + 2,0);
	}
	return rect;
}


int32 ColorContainer::pointIndex(BPoint point)
{
	// this function returns which palette entry is at point in the view
	// or -1 if no entry is to found there
	// at the moment just calculate usin rect Contains-function
	// later should be implemented more elegantly

	for (int32 i=0;i<color_count;i++) {
		if (colorBounds(i).Contains(point))
			return i;
	}

	// none of the rectangles contained point
	return -1;
}

void ColorContainer::colorChanged(int32 color_index)
{
	// draw the new color
	rgb_color c = ColorSet::currentSet()->colorAt(color_index);
	rgb_color low = c;
	float coeff = c.alpha / 255.0;
	low.red = (uint8)(c.red*coeff);
	low.green = (uint8)(c.green*coeff);
	low.blue = (uint8)(c.blue*coeff);
	low.alpha = 255;

	c.red = (uint8)(c.red * coeff + (1-coeff)*255);
	c.green = (uint8)(c.green * coeff + (1-coeff)*255);
	c.blue = (uint8)(c.blue * coeff + (1-coeff)*255);
	c.alpha = 255;

	SetHighColor(c);
	SetLowColor(low);
	FillRect(colorBounds(color_index),HS_2X2_BLOCKS);

	// also draw the white rect around the color that is currently selected
	// could be some other that was changed
	if (highlight_selected) {
		SetHighColor(255,255,255);
		StrokeRect(colorBounds(ColorSet::currentSet()->currentColorIndex()));
	}
	// and we will Sync()
	Sync();
}


void ColorContainer::sendMessageToAllContainers(BMessage *message)
{
	// here go through the list of color containers
	// and post message to each of them

	for (int32 i=0;i<container_list->CountItems();i++) {
		((ColorContainer*)container_list->ItemAt(i))->Window()->PostMessage(message,(ColorContainer*)container_list->ItemAt(i));
	}
}


// here begins the definition of ColorSet-class
// first initialize the static variables
BList* ColorSet::color_set_list = new BList();
int32 ColorSet::current_set_index = 0;


ColorSet::ColorSet(int32 amount_of_colors, ColorSet *copy_this_palette)
{
	// first check that the amount of colors is reasonable
	amount_of_colors = min_c(amount_of_colors,256);
	amount_of_colors = max_c(amount_of_colors,1);

	// first allocate memory for the new palette
	palette = new rgb_color[amount_of_colors];

	// allocate memory for name, 100 chars should be long enough
	name = new char[100];

	if (copy_this_palette == NULL) {
		// if no palette was given to copy from make a default palette
		for (int32 i=0;i<amount_of_colors;i++) {
			palette[i].red = i*256/amount_of_colors;
			palette[i].green = i*256/amount_of_colors;
			palette[i].blue = i*256/amount_of_colors;
			palette[i].alpha = 255;
		}
	}
	else {
		// here we copy that palette
		int32 source_size = copy_this_palette->sizeOfSet();
		for (int i=0;i<amount_of_colors;i++) {
			palette[i] = copy_this_palette->colorAt(i % source_size);
		}
	}
	// store the color count
	color_count = amount_of_colors;

	// create a default name
	strcpy(name,"default palette");

	// put the current color to 0
	current_color_index = 0;

	color_set_list->AddItem(this);
}

ColorSet::~ColorSet()
{
	// remove ourselves from the color_set_list
	color_set_list->RemoveItem(this);
	current_set_index = min_c(current_set_index,color_set_list->CountItems()-1);
}


rgb_color ColorSet::colorAt(int32 index)
{
	// check that the index is correct
	if ((index>=0)&&(index<color_count)) {
		return palette[index];
	}
	else {
		// otherwise return the first color
		return palette[0];
	}
}


void ColorSet::setColor(int32 index, rgb_color color)
{
	// check that the index is correct
	if ((index>=0)&&(index<color_count)) {
		palette[index] = color;
	}

}


void ColorSet::setName(const char *set_name)
{
	strcpy(name,set_name);
}


char* ColorSet::getName()
{
	return name;
}


status_t ColorSet::readSets(BFile &file)
{
	int32 number_of_sets;

	if (file.Read(&number_of_sets,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	for (int32 i=0;i<number_of_sets;i++) {
		int32 size_of_set;
		if (file.Read(&size_of_set,sizeof(int32)) != sizeof(int32))
			return B_ERROR;

		ColorSet *new_set = new ColorSet(size_of_set);

		int32 name_length;
		char name[255];
		file.Read(&name_length,sizeof(int32));
		file.Read(name,name_length);
		name[name_length] = '\0';

		new_set->setName(name);

		for (int32 b=0;b<size_of_set;b++) {
			rgb_color c;
			if (file.Read(&c,sizeof(rgb_color)) != sizeof(rgb_color))
				return B_ERROR;

			new_set->palette[b] = c;
		}
	}

	return B_OK;
}


status_t ColorSet::writeSets(BFile &file)
{
	int32 written_value = color_set_list->CountItems();

	if (file.Write(&written_value,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	for (int32 i=0;i<color_set_list->CountItems();i++) {
		written_value = ((ColorSet*)color_set_list->ItemAt(i))->sizeOfSet();
		if (file.Write(&written_value,sizeof(int32)) != sizeof(int32))
			return B_ERROR;

		written_value = strlen(((ColorSet*)color_set_list->ItemAt(i))->getName());
		if (file.Write(&written_value,sizeof(int32)) != sizeof(int32))
			return B_ERROR;

		written_value = file.Write(((ColorSet*)color_set_list->ItemAt(i))->getName(),
			strlen(((ColorSet*)color_set_list->ItemAt(i))->getName()));
		if (uint32(written_value) != strlen(((ColorSet*)color_set_list->ItemAt(i))->getName()))
			return B_ERROR;

		// here write the palette entries
		for (int32 b=0;b<((ColorSet*)color_set_list->ItemAt(i))->sizeOfSet();b++) {
			rgb_color c = ((ColorSet*)color_set_list->ItemAt(i))->palette[b];
			if (file.Write(&c,sizeof(rgb_color)) != sizeof(rgb_color))
				return B_ERROR;
		}
	}


	return B_OK;
}
