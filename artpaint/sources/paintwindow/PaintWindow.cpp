/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Alert.h>
#include <BitmapStream.h>
#include <Button.h>
#include <Clipboard.h>
#include <Entry.h>
#include <InterfaceDefs.h>
#include <MenuBar.h>
#include <new.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SupportDefs.h>
#include <TranslatorRoster.h>

#include "PaintWindow.h"
#include "ImageView.h"
#include "BackgroundView.h"
#include "MessageConstants.h"
#include "StatusView.h"
#include "UtilityClasses.h"
#include "LayerWindow.h"
#include "PaintApplication.h"
#include "FilePanels.h"
#include "ViewSetupWindow.h"
#include "Settings.h"
#include "ColorPalette.h"
#include "Controls.h"
#include "DatatypeSetupWindow.h"
#include "FileIdentificationStrings.h"
#include "VersionConstants.h"
#include "ToolSetupWindow.h"
#include "ToolSelectionWindow.h"
#include "HSStack.h"
#include "PopUpList.h"
#include "BrushStoreWindow.h"
#include "Layer.h"
#include "GlobalSetupWindow.h"
#include "MessageFilters.h"
#include "Manipulator.h"
#include "ManipulatorServer.h"
#include "ProjectFileFunctions.h"
#include "PaintWindowMenuItem.h"
#include "Image.h"
#include "StringServer.h"
#include "SymbolImageServer.h"
#include "AboutWindow.h"

// initialize the static variable
BList PaintWindow::paint_window_list(10);
int32 PaintWindow::paint_window_count = 0;
int32 PaintWindow::untitled_window_number = 1;


PaintWindow::PaintWindow(char *name,BRect frame, uint32 views,const window_settings *setup )
				: BWindow(frame,name,B_DOCUMENT_WINDOW_LOOK,B_NORMAL_WINDOW_FEEL,B_WILL_ACCEPT_FIRST_CLICK|B_NOT_ANCHORED_ON_ACTIVATE)
{
	SetSizeLimits(400, 10000, 300, 10000);

	// Fit the window to screen.
	BRect new_frame = FitRectToScreen(frame);
	if (new_frame != frame) {
		MoveTo(new_frame.LeftTop());
		ResizeTo(new_frame.Width(),new_frame.Height());
	}

	// Here we set various views to NULL so we can later test if they are opened.
	menubar = NULL;
	image_view = NULL;
	set_size_button = NULL;

	strcpy(tool_help_string,"");

	// also NULL the save-panels
	image_save_panel = NULL;
	project_save_panel = NULL;

	// Create initial entrys. Actually BEntrys should not be used to store paths
	// as they consume a file-descriptor.
	image_entry = BEntry();
	project_entry = BEntry();

	// Increment the window count by 1.
	paint_window_count++;

	if (views == 0) {
		views = setup->views;
	}

	// Record the settings.
	settings = new window_settings(setup);
	current_handler = 0;

	// reset the additional variables
	additional_width = 0;
	additional_height = 0;

	// here get the be_plain_font height
	font_height p_f_height;
	BFont plain_font;
	plain_font.GetHeight(&p_f_height);

	if ((views & HS_MENU_BAR) != 0) {	// the menubar should be opened
		openMenuBar();

		// the height of menubar can be obtained by calling menubar->Bounds().Height()
		additional_height += menubar->Bounds().Height() + 1;
	}


	// here the scrollbars are opened and placed along window sides
	// vertical bar is opened to begin under the menubar

	float right = Bounds().right + 1;		// have to add one, otherwise scrollbars
	float bottom = Bounds().bottom + 1;	// would be 1 pixel away from edges (why ????)
	float top = 0;


	if (menubar != NULL)	// here add the height of menubar if it is opened
		top += menubar->Bounds().Height();

	// Create the scrollbars, and disable them.
	horiz_scroll = new BScrollBar(BRect(0,bottom - B_H_SCROLL_BAR_HEIGHT,right-B_V_SCROLL_BAR_WIDTH,bottom),"horiz",NULL,0,0,B_HORIZONTAL);
	vert_scroll = new BScrollBar(BRect(right - B_V_SCROLL_BAR_WIDTH,top,right,bottom - B_H_SCROLL_BAR_HEIGHT),"vert",NULL,0,0,B_VERTICAL);
	horiz_scroll->SetRange(0,0);
	vert_scroll->SetRange(0,0);

	// add the scroll bars to window
	AddChild(horiz_scroll);
	AddChild(vert_scroll);
	horiz_scroll->SetSteps(8,32);
	vert_scroll->SetSteps(8,32);

	// update the free area limits
	bottom -= (B_H_SCROLL_BAR_HEIGHT + 1);
	right -= (B_V_SCROLL_BAR_WIDTH + 1);
	top += 1;			// start other views one pixel down from menubar


	// increase the additional width and height variables
	additional_width += B_V_SCROLL_BAR_WIDTH - 1;
	additional_height += B_H_SCROLL_BAR_HEIGHT - 1;

	// The status-view is not optional. It contains sometimes also buttons that
	// can cause some actions to be taken.
	if ((views & HS_STATUS_VIEW) != 0) {	// the statusbar should be opened
		// Create the status-view and make it display nothing
		status_view = new StatusView(BRect(BPoint(0,bottom),BPoint(right,bottom)));
		status_view->DisplayNothing();

		// place the statusbar along bottom of window on top of scrollbar
		// and also record size that it occupies

		// here add the statusview to window's hierarchy
		AddChild(status_view);

		// update the bottom value to be 1 pixel above status_view
		// status_view is resized in window FrameResized() function
		bottom -= (status_view->Bounds().Height() + 1);

		// update the additional_height variable
		additional_height += status_view->Bounds().Height() + 1;
	}

	// here set the window minimum size so that all views are nicely visible
	// even when window is at minimum size
	// also create the ImageView and backgroundview
	// adjust the coordinates so that no views overlap
	// later should take into account all the views that were opened

	// make the background view
	background = new BackgroundView(BRect(0,top,right,bottom));
	AddChild(background);


	if ( (views & HS_SIZING_VIEW) != 0x0000 )  {
		// here we should open the views that are used to set the image size
		// we should add them as children for background-view and put them on
		// the right side of that area

		// here set the width and height views, no message is required
		// set the label of width view also to Height so that the views
		// resize to same size
		const char *width_string = StringServer::ReturnString(WIDTH_STRING);
		const char *height_string = StringServer::ReturnString(HEIGHT_STRING);
		const char *create_canvas_string = StringServer::ReturnString(CREATE_CANVAS_STRING);
		const char *longer_string;
		if (strlen(height_string) > strlen(width_string))
			longer_string = height_string;
		else
			longer_string = width_string;

		width_view = new NumberControl(BRect(10,10,110,10),"width_view",longer_string,"",NULL);
		width_view->TextView()->SetMaxBytes(4);
		height_view = new NumberControl(BRect(10,width_view->Frame().bottom + 10,110,width_view->Frame().bottom + 10),"height_view",longer_string,"",NULL);
		height_view->TextView()->SetMaxBytes(4);
		set_size_button = new BButton(BRect(10,height_view->Frame().bottom + 10,110,height_view->Frame().bottom + 10),"set_size_button",create_canvas_string, new BMessage(HS_IMAGE_SIZE_SET));
		set_size_button->SetTarget(this);

		// resize the buttons to preferred sizes
		width_view->ResizeToPreferred();
		height_view->ResizeToPreferred();
		set_size_button->ResizeToPreferred();


		// and then change the label of width_view to Width
		width_view->SetLabel(width_string);
		height_view->SetLabel(height_string);


		// Here also create a button that controls a pop-up menu that contains the
		// most recently used sizes as items. The menu-items should post a message
		// to this window, that then changes the values to width_view and height_view.
		float pop_up_left = height_view->Frame().right+5;
		float pop_up_top = height_view->Frame().top - (height_view->Frame().top -width_view->Frame().bottom)/2 - 10;
		int32 w,h;
		BBitmap *pushed = SymbolImageServer::ReturnSymbolAsBitmap(POP_UP_LIST_PUSHED,w,h);
		BBitmap *not_pushed = SymbolImageServer::ReturnSymbolAsBitmap(POP_UP_LIST,w,h);
		BMessage* message_list[RECENT_LIST_LENGTH];
		char label[256];
		for (int32 i=0;i<RECENT_LIST_LENGTH;i++) {
			message_list[i] = new BMessage(HS_RECENT_IMAGE_SIZE);
			message_list[i]->AddInt32("width",((PaintApplication*)be_app)->Settings()->recent_image_width_list[i]);
			message_list[i]->AddInt32("height",((PaintApplication*)be_app)->Settings()->recent_image_height_list[i]);
			sprintf(label,"%ld x %ld",
				((PaintApplication*)be_app)->Settings()->recent_image_width_list[i],
				((PaintApplication*)be_app)->Settings()->recent_image_height_list[i]);
			message_list[i]->AddString("label",label);
		}
		PopUpList *pop_up_list = new PopUpList(BRect(pop_up_left,pop_up_top,pop_up_left+9,pop_up_top+19),pushed,not_pushed,message_list,RECENT_LIST_LENGTH,new BMessenger(NULL,this));
		BMenu *standard_size_menu = new BMenu(StringServer::ReturnString(STANDARD_SIZES_STRING));
		BMessage *message;
		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width",320);
		message->AddInt32("height",256);
		standard_size_menu->AddItem(new BMenuItem("320 x 256",message));

		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width",640);
		message->AddInt32("height",400);
		standard_size_menu->AddItem(new BMenuItem("640 x 400",message));

		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width",640);
		message->AddInt32("height",480);
		standard_size_menu->AddItem(new BMenuItem("640 x 480",message));

		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width",800);
		message->AddInt32("height",600);
		standard_size_menu->AddItem(new BMenuItem("800 x 600",message));

		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width",1024);
		message->AddInt32("height",768);
		standard_size_menu->AddItem(new BMenuItem("1024 x 768",message));

		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width",1152);
		message->AddInt32("height",900);
		standard_size_menu->AddItem(new BMenuItem("1152 x 900",message));

		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width",1280);
		message->AddInt32("height",1024);
		standard_size_menu->AddItem(new BMenuItem("1280 x 1024",message));

		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width",1600);
		message->AddInt32("height",1200);
		standard_size_menu->AddItem(new BMenuItem("1600 x 1200",message));


		pop_up_list->ReturnMenu()->AddItem(standard_size_menu,0);
		pop_up_list->ReturnMenu()->AddItem(new BSeparatorItem(),1);


//		container_width += 5;

		// here create the container_box that is large enough
		float container_width = max_c(set_size_button->Frame().Width() + 20,pop_up_list->Frame().right+5);
		float container_height = set_size_button->Frame().Height() + 2*height_view->Frame().Height() + 40;
		container_box = new BBox(BRect(background->Bounds().right-container_width,background->Bounds().bottom-container_height,background->Bounds().right,background->Bounds().bottom),"container for controls",B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);

		// here we should center the button and boxes horizontally

		background->AddChild(container_box);

		// Container-box tries to change its color to that of background's
		// (which should be B_TRANSPARENT_32_BIT) so we have to change it to
		// the color we want it to have before adding children to it.
		container_box->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		container_box->AddChild(width_view);
		container_box->AddChild(height_view);
		container_box->AddChild(set_size_button);
		container_box->AddChild(pop_up_list);
		BMessage *help_message = new BMessage(HS_TOOL_HELP_MESSAGE);
		help_message->AddString("message",StringServer::ReturnString(SELECT_CANVAS_SIZE_STRING));
		PostMessage(help_message,this);
		delete help_message;
		set_size_button->MakeDefault(true);
	}


	// finally inform the app that new window has been created
	BMessage *message_to_app = new BMessage(HS_PAINT_WINDOW_OPENED);
	message_to_app->AddPointer("window",(void*)this);
	be_app->PostMessage(message_to_app,be_app);
	delete message_to_app;

	// resize so that all things are properly updated
	ResizeBy(1,0);
	ResizeBy(-1,0);
	// show the window to user
	Show();

	// Add ourselves to the paint_window_list
	paint_window_list.AddItem(this);

	Lock();
	// Handle the window-activation with this common filter.
	BMessageFilter *activation_filter = new BMessageFilter(B_ANY_DELIVERY,B_ANY_SOURCE,B_MOUSE_DOWN,window_activation_filter);
	AddCommonFilter(activation_filter);
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN,AppKeyFilterFunction));
	Unlock();

	user_frame = Frame();

}


PaintWindow* PaintWindow::createPaintWindow(BBitmap *a_bitmap,char *file_name,int32 type, entry_ref ref, translator_id outTranslator)
{
	PaintWindow *a_window;

	window_settings default_window_settings = ((PaintApplication*)be_app)->Settings()->default_window_settings;

	uint32 flags = HS_MENU_BAR | HS_STATUS_VIEW | HS_HELP_VIEW ;
	char title[100];
	if (file_name == NULL) {
//		sprintf(title,"%s - %d",StringServer::ReturnString(UNTITLED_STRING),untitled_window_number);
		sprintf(title,"%s",StringServer::ReturnString(EMPTY_PAINT_WINDOW_STRING));;
		flags = flags | HS_SIZING_VIEW;
	}
	else
		sprintf(title,"%s",file_name);

	if (a_bitmap == NULL) {
		a_window = new PaintWindow(title,default_window_settings.frame_rect,flags,&(default_window_settings));
//		untitled_window_number++;
	}
	else {
		a_window = new PaintWindow(title,default_window_settings.frame_rect, flags,&(default_window_settings));

		// some of these might also come from attributes
		a_window->Settings()->zoom_level = default_window_settings.zoom_level;
		a_window->Settings()->view_position = default_window_settings.view_position;
		a_window->Settings()->file_type = type;

		BNode a_node(&ref);
		BNodeInfo a_node_info(&a_node);

		a_node_info.GetType(a_window->Settings()->file_mime);
		BEntry *entry = a_window->ImageEntry();
		*entry = BEntry(&ref,TRUE);

// This is done elsewhere to make it possible to read the attributes
// after creating the image-view.
//		// make the window read it's attributes
//		a_window->readAttributes(a_node);

		// Open an imageview for the window and add a layer to it.
		a_window->OpenImageView(int32(a_bitmap->Bounds().Width() + 1),
			int32(a_bitmap->Bounds().Height() + 1));
		a_window->ReturnImageView()->ReturnImage()->InsertLayer(a_bitmap);
		a_window->AddImageView();
	}

	a_window->current_handler = outTranslator;

	// Change the zoom-level to be correct
	a_window->Lock();
	a_window->displayMag(a_window->Settings()->zoom_level);
	a_window->Unlock();


	return a_window;
}



PaintWindow::~PaintWindow()
{
	if (image_view != NULL) {
		// if we have a BEntry for this image, we should
		// write some attributes to that file
		if (image_entry.InitCheck() == B_NO_ERROR) {
			// call a function that writes attributes to a node
			BNode a_node(&image_entry);
			writeAttributes(a_node);
		}
		if (project_entry.InitCheck() == B_NO_ERROR) {
			BNode a_node(&project_entry);
			writeAttributes(a_node);
		}

		image_view->RemoveSelf();
		delete image_view;
	}

	delete settings;
	// Decrement the window-count by 1.
	paint_window_count--;

	// Remove ourselves from the paint_window_list.
	paint_window_list.RemoveItem(this);
}


void PaintWindow::FrameResized(float, float)
{
	// Store the new frame to settings.
	settings->frame_rect = Frame();
}

void PaintWindow::FrameMoved(BPoint)
{
	// Store the new frame to settings.
	settings->frame_rect = Frame();
}



void PaintWindow::MenusBeginning()
{
	BWindow::MenusBeginning();

	BMenuItem *item = menubar->FindItem(StringServer::ReturnString(PASTE_AS_NEW_LAYER_STRING));
	if (item != NULL) {
		be_clipboard->Lock();
		if (be_clipboard->Data()->HasMessage("image/bitmap"))
			item->SetEnabled(TRUE);
		else
			item->SetEnabled(FALSE);
		be_clipboard->Unlock();
	}

	item = menubar->FindItem(StringServer::ReturnString(PASTE_AS_NEW_PROJECT_STRING));
	if (item != NULL) {
		be_clipboard->Lock();
		if (be_clipboard->Data()->HasMessage("image/bitmap"))
			item->SetEnabled(TRUE);
		else
			item->SetEnabled(FALSE);
		be_clipboard->Unlock();
	}


	if (image_view != NULL) {
		item = menubar->FindItem(HS_CLEAR_SELECTION);
		if (image_view->GetSelection()->IsEmpty())
			item->SetEnabled(FALSE);
		else
			item->SetEnabled(TRUE);

		item = menubar->FindItem(HS_INVERT_SELECTION);
		if (image_view->GetSelection()->IsEmpty())
			item->SetEnabled(FALSE);
		else
			item->SetEnabled(TRUE);

		item = menubar->FindItem(HS_GROW_SELECTION);
		if (image_view->GetSelection()->IsEmpty())
			item->SetEnabled(FALSE);
		else
			item->SetEnabled(TRUE);

		item = menubar->FindItem(HS_SHRINK_SELECTION);
		if (image_view->GetSelection()->IsEmpty())
			item->SetEnabled(FALSE);
		else
			item->SetEnabled(TRUE);
	}

	if ((image_entry.InitCheck() == B_NO_ERROR) && (current_handler != 0)) {
		BMenuItem *item = menubar->FindItem(HS_SAVE_IMAGE);
		if (item) item->SetEnabled(TRUE);
	}
	else {
		BMenuItem *item = menubar->FindItem(HS_SAVE_IMAGE);
		if (item) item->SetEnabled(FALSE);
	}

	if (project_entry.InitCheck() == B_NO_ERROR) {
		BMenuItem *item = menubar->FindItem(HS_SAVE_PROJECT);
		if (item) item->SetEnabled(TRUE);
	}
	else {
		BMenuItem *item = menubar->FindItem(HS_SAVE_PROJECT);
		if (item) item->SetEnabled(FALSE);
	}

	SetHelpString("",HS_TEMPORARY_HELP_MESSAGE);
}


void PaintWindow::MenusEnded()
{
	SetHelpString(NULL,HS_TOOL_HELP_MESSAGE);
}

void PaintWindow::MessageReceived(BMessage *message)
{
	// handles all the messages that come to PaintWindow
	// this path can be used when saving the image or project
	BPath path;
	BEntry parent_entry;
	entry_ref a_save_ref;
	BPath a_path;
	char a_name[B_FILE_NAME_LENGTH];
	BMessage *a_save_message;

	switch ( message->what ) {
		// this comes from menubar->"Window"->"Resize Window to Fit" and informs us that
		// we should fit the window to display exactly the image
		case HS_RESIZE_WINDOW_TO_FIT:
			// use a private function to do resizing
			resizeToFit();
			break;

		// This comes from the recent image-size pop-up-list.
		case HS_RECENT_IMAGE_SIZE:
			width_view->SetValue(message->FindInt32("width"));
			height_view->SetValue(message->FindInt32("height"));
			break;

		// this comes from a button and it informs that the user has decided the image size
		// and we should create a canvas
		case HS_IMAGE_SIZE_SET:
			{
				// Here we have to take the measurements from NumberControls.
				// The units might be something else than pixels. This reading and
				// conversion should be done in a separate function.
				bool image_created;
				int32 width,height;
				width = atoi(width_view->Text());
				height = atoi(height_view->Text());

				try {
					// Open the image view
					OpenImageView(width,height);
					// Add a layer to it.
					image_view->ReturnImage()->InsertLayer();
					image_created = TRUE;
				}
				catch (bad_alloc) {
					image_created = FALSE;
					delete image_view;
					image_view = FALSE;
					BAlert *memory_alert = new BAlert("memory_alert",StringServer::ReturnString(CANNOT_CREATE_IMAGE_STRING),StringServer::ReturnString(OK_STRING),NULL,NULL,B_WIDTH_AS_USUAL,B_WARNING_ALERT);
					memory_alert->Go();
				}
				if (image_created == TRUE) {
					// Record the window's frame and also put the new size to the most recently used
					// list.
					((PaintApplication*)be_app)->Settings()->default_window_settings.frame_rect = Frame();
					// Only record the new size to the list if it does not already contain the
					// selected size.
					int32 *widths = ((PaintApplication*)be_app)->Settings()->recent_image_width_list;
					int32 *heights = ((PaintApplication*)be_app)->Settings()->recent_image_height_list;
					int32 list_position = -1;
					for (int32 i=0;i<RECENT_LIST_LENGTH;i++) {
						if ((widths[i] == width) && (heights[i]==height)) {
							list_position = i;
						}
					}
					if (list_position == -1) {
						for (int32 i=RECENT_LIST_LENGTH-1;i>0;i--) {
							widths[i] = widths[i-1];
							heights[i] = heights[i-1];
						}
						widths[0] = width;
						heights[0] = height;
					}
					else {
						// Move it to the head of the list.
						for (int32 i=list_position;i>0;i--) {
							widths[i] = widths[i-1];
							heights[i] = heights[i-1];
						}
						widths[0] = width;
						heights[0] = height;
					}

					// Remove the sizing buttons.
					container_box->RemoveSelf();
					delete container_box;
					// NULL this because it will be used later to check if we are still resizing
					set_size_button = NULL;

					// Add the view to view hierarchy.
					AddImageView();
				}
				break;
			}

	// this is the case where mouse has been pressed down in the background-view
		// this comes from backgrounview, we should then put the actual imageview
		// to follow the mouse
		case HS_MOUSE_DOWN_IN_BACKGROUNDVIEW:
			if ((image_view != NULL) && (image_view->Window() != NULL)) {
				image_view->MouseDown(message->FindPoint("point"));
			}
			break;



		// this comes from menubar->"Window"->"Show Layer Window" and tells us to show the
		// layer window
		case HS_SHOW_LAYER_WINDOW:
			LayerWindow::showLayerWindow();
			if (image_view != NULL) {
				LayerWindow::ActiveWindowChanged(this,image_view->ReturnImage()->LayerList(),image_view->ReturnImage()->ReturnThumbnailImage());
			}
			else {
				LayerWindow::ActiveWindowChanged(this);
			}
			break;

	// this shows us a view setup window
		// this comes from menubar->"Window"->"Window Settings…" and tells us to show the
		// window for setting the parameters of this window and it's views
		case HS_SHOW_VIEW_SETUP_WINDOW:
			ViewSetupWindow::showViewSetupWindow(this,image_view);
			break;

		// this comes from menubar->"Window"->"Global Settings…"
		case HS_SHOW_GLOBAL_SETUP_WINDOW:
			GlobalSetupWindow::showGlobalSetupWindow();
			break;

	// this tells us that user wants to open a save-panel for saving the image in a new file-name
		// This comes from menubar->"File"->"Save Image As…" and we should show a save-panel
		case HS_SHOW_IMAGE_SAVE_PANEL:
			if (image_save_panel == NULL) {
				entry_ref *ref = new entry_ref();
				if (image_entry.InitCheck() != B_NO_ERROR) {
					// This might actually fail if the user has removed the directory.
					if (path.SetTo(((PaintApplication*)be_app)->Settings()->image_save_path) != B_NO_ERROR) {
						PaintApplication::HomeDirectory(path);
					}
				}
				else {
					image_entry.GetPath(&path);
					// get the dir that the file lives in
					path.GetParent(&path);
				}
				get_ref_for_path(path.Path(),ref);
				image_save_panel = new ImageSavePanel(ref, new BMessenger(this),settings->file_type,image_view->ReturnImage()->ReturnThumbnailImage());
				delete ref;
			}
			image_save_panel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
			set_filepanel_strings(image_save_panel);
			if (!image_save_panel->IsShowing())
				image_save_panel->Show();
			break;

		// This comes from menubar->"File"->"Save Project As…"
		case HS_SHOW_PROJECT_SAVE_PANEL:
			if (project_save_panel == NULL) {
				entry_ref *ref = new entry_ref();
				if (project_entry.InitCheck() != B_NO_ERROR) {
					path.SetTo(((PaintApplication*)be_app)->Settings()->project_save_path);
				}
				else {
					project_entry.GetPath(&path);
					// get the dir that the file lives in
					path.GetParent(&path);
				}
				get_ref_for_path(path.Path(),ref);
				project_save_panel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this),ref,0,FALSE,new BMessage(HS_PROJECT_SAVE_REFS));
				delete ref;
			}
			project_save_panel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
			char string[256];
			sprintf(string,"ArtPaint: %s",StringServer::ReturnString(SAVE_PROJECT_STRING));
			project_save_panel->Window()->SetTitle(string);
			set_filepanel_strings(project_save_panel);
			if (!project_save_panel->IsShowing())
				project_save_panel->Show();
			break;

		// This comes from menubar->"Window"->"Show Color Window". We should open the color window.
		case HS_SHOW_COLOR_WINDOW:
			ColorPaletteWindow::showPaletteWindow(FALSE);
			break;

		// This comes from menubar->"Window"->"Show Tool Window". We should open the tool window.
		case HS_SHOW_TOOL_WINDOW:
			ToolSelectionWindow::showWindow();
			break;

		// This comes from menubar->"Window"->"Show Tool Setup Window". We should open the tool window.
		case HS_SHOW_TOOL_SETUP_WINDOW:
			ToolSetupWindow::showWindow(((PaintApplication*)be_app)->Settings()->setup_window_tool);
			break;

		case HS_SHOW_BRUSH_STORE_WINDOW:
			BrushStoreWindow::showWindow();
			break;

		// this comes from the image save panel and contains the refs with which to save the image
		case HS_IMAGE_SAVE_REFS:
			// Here we call the function that saves the image.
			// We should actually call it in another thread while ensuring that it saves the
			// correct bitmap. We should also protect the bitmap from being modified while we save.
			// We should also inform the user about saving of the.
			delete image_save_panel;
			image_save_panel = NULL;
			if (image_view != NULL) {
				thread_id save_thread = spawn_thread(PaintWindow::save_image,"save image",B_NORMAL_PRIORITY,(void*)this);
				resume_thread(save_thread);
				BMessage *sendable_message = new BMessage(*message);
				send_data(save_thread,0,(void*)&sendable_message,sizeof(BMessage*));
			}
			break;

		// This comes from project save-panel and contains refs for the file to which the project
		// should be saved.
		case HS_PROJECT_SAVE_REFS:
			// We call the function that saves the project.
			delete project_save_panel;
			project_save_panel = NULL;
			if (image_view != NULL) {
				thread_id save_thread = spawn_thread(PaintWindow::save_project,"save project",B_NORMAL_PRIORITY,(void*)this);
				resume_thread(save_thread);
				BMessage *sendable_message = new BMessage(*message);
				send_data(save_thread,0,(void*)&sendable_message,sizeof(BMessage*));
			}
			break;

		case HS_SAVE_IMAGE:
			// We make a message containing the file name and ref for its dir.
			image_entry.GetParent(&parent_entry);
			image_entry.GetName(a_name);
			parent_entry.GetPath(&a_path);
			get_ref_for_path(a_path.Path(),&a_save_ref);
			a_save_message = new BMessage(HS_IMAGE_SAVE_REFS);
			a_save_message->AddRef("directory",&a_save_ref);
			a_save_message->AddString("name",a_name);
			PostMessage(a_save_message,this);
			delete a_save_message;
			break;

		case HS_SAVE_PROJECT:
			if (project_entry.InitCheck() == B_NO_ERROR) {
				// We make a message containing the file name and ref for its dir.
				project_entry.GetParent(&parent_entry);
				project_entry.GetName(a_name);
				parent_entry.GetPath(&a_path);
				get_ref_for_path(a_path.Path(),&a_save_ref);
				a_save_message = new BMessage(HS_PROJECT_SAVE_REFS);
				a_save_message->AddRef("directory",&a_save_ref);
				a_save_message->AddString("name",a_name);
				PostMessage(a_save_message,this);
				delete a_save_message;
			}
			else {
				PostMessage(HS_SHOW_PROJECT_SAVE_PANEL,this);
			}
			break;

		// this comes from the image save panel's format menu and informs that the wanted save
		// format has changed
		case HS_SAVE_FORMAT_CHANGED:
			{
				settings->file_type = message->FindInt32("be:type");
				current_handler = message->FindInt32("be:translator");
				DatatypeSetupWindow::ChangeHandler(current_handler);

				BTranslatorRoster *roster = BTranslatorRoster::Default();
				int32 num_formats;
				const translation_format *formats = NULL;
				if (roster->GetOutputFormats(current_handler,&formats,&num_formats) == B_NO_ERROR) {
					for (int32 i=0;i<num_formats;i++) {
						if (formats[i].type == settings->file_type) {
							strcpy(settings->file_mime,formats[i].MIME);
						}
					}
				}
				break;
			}

		// This comes from image-save-panel's setting-button and tells us to show the datatype-
		// setup-window.
		case HS_SHOW_DATATYPE_SETTINGS:
			DatatypeSetupWindow::showWindow(current_handler);
			break;

		// this might come from a lot of places, but it is assumed that it comes from the
		// image save panel
		case B_CANCEL:
			// cancel might come from somewhere else than just image_save_panel
			// we should check that somewhow
			if ((image_save_panel != NULL) && (image_save_panel->IsShowing() == FALSE)) {
				delete image_save_panel;
				image_save_panel = NULL;
			}
			else if ((project_save_panel != NULL) && (project_save_panel->IsShowing() == FALSE)) {
				delete project_save_panel;
				project_save_panel = NULL;
			}
			break;

		// This might come from many places and it informs us to display a message
		// in the help view.
		case HS_TEMPORARY_HELP_MESSAGE:
		case HS_TOOL_HELP_MESSAGE:
			const char *help_msg[256];
			if (message->FindString("message",help_msg) == B_OK)
				SetHelpString(*help_msg,message->what);
			else
				SetHelpString(NULL,message->what);
			break;


		// This comes from menuitem in file-menu. We should save the image into a resource-file.
		// At the moment that file will be hardcoded to ArtPaint's resources and we have to type
		// the resource ID from the shell.
		case HS_SAVE_IMAGE_INTO_RESOURCES:
			saveImageIntoResources();
			break;

		case HS_SAVE_IMAGE_AS_CURSOR:
			saveImageAsCursor();
			break;

		case HS_SHOW_ABOUT_WINDOW:
			AboutWindow::showWindow();
	// otherwise call inherited function
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

bool PaintWindow::QuitRequested()
{
	// here we should ask the user if changes to picture should be saved
	// we also tell the application that the number of paint windows
	// has decreased by one
	if (image_view != NULL) {
		if (image_view->Quit() == FALSE) {
			return FALSE;
		}
		if (image_save_panel != NULL)
			delete image_save_panel;
	}

	LayerWindow::ActiveWindowChanged(NULL);

	if (paint_window_count <= 1)
		be_app->PostMessage(B_QUIT_REQUESTED);
	return TRUE;
}

void PaintWindow::WindowActivated(bool active)
{
	if (active == TRUE) {
		if (image_view != NULL) {
			LayerWindow::ActiveWindowChanged(this,image_view->ReturnImage()->LayerList(),image_view->ReturnImage()->ReturnThumbnailImage());
		}
		else {
			LayerWindow::ActiveWindowChanged(this);
		}
	}
}


void PaintWindow::WorkspaceActivated(int32,bool active)
{
	if (active) {
		if (image_view != NULL) {
			BScreen screen(this);
			if (screen.ColorSpace() == B_CMAP8) {
//				printf("B_CMAP8\n");
				image_view->SetDisplayMode(DITHERED_8_BIT_DISPLAY_MODE);
			}
			else {
//				printf("not B_CMAP8\n");
				image_view->SetDisplayMode(FULL_RGB_DISPLAY_MODE);
			}
		}
	}
}


void PaintWindow::Zoom(BPoint lefttop,float width,float height)
{
	BRect frame = Frame();
	BRect preferred = getPreferredSize();

	bool resized_to_fit = true;

	if (frame != preferred) {
		printf("Frame is not preferred\n");
		resized_to_fit = false;
	}
	if (resized_to_fit) {
		MoveTo(user_frame.LeftTop());
		ResizeTo(user_frame.Width(),user_frame.Height());
	}
	else {
		resizeToFit();
	}
}


void PaintWindow::DisplayCoordinates(BPoint point,BPoint reference,bool use_reference)
{
	// here we set the proper view to display the coordinates

	// set the coords string with sprintf
//	sprintf(coords,"X: %.0f  Y: %.0f",point.x,point.y);

	status_view->SetCoordinates(point,reference,use_reference);

	if (set_size_button != NULL) {
		// if the window is in resize mode display dimensions here too
		width_view->SetValue(int32(point.x));
		height_view->SetValue(int32(point.y));
	}
}


void PaintWindow::displayMag(float mag)
{
	status_view->SetMagnifyingScale(mag);
}


void PaintWindow::SetHelpString(const char *string,int32 type)
{
	if ((type == HS_TOOL_HELP_MESSAGE) && (string != NULL))
		strncpy(tool_help_string,string,255);

	if (status_view != NULL) {
		if (string != NULL)
			status_view->SetHelpMessage(string);
		else if (type == HS_TOOL_HELP_MESSAGE)
			status_view->SetHelpMessage(tool_help_string);
	}
}


bool PaintWindow::openMenuBar()
{
	// Some of the items have the image as their target, the targets for those items
	// are set in openImageView.

	// Remember to change image-view as a target for added items in openImageView.


	// this points to item that is added to menubar
	BMenu *item;
	BMenu *sub_menu;
	BMessage *a_message;

	// here we place the menubar along the top edge of window
	menubar = new BMenuBar(BRect(0,0,1,1),"menu_bar");

	item = new BMenu(StringServer::ReturnString(FILE_STRING));
	menubar->AddItem(item);

	sub_menu = new BMenu(StringServer::ReturnString(RECENT_IMAGES_STRING));
	item->AddItem(sub_menu);
	for (int32 i=0;i<RECENT_LIST_LENGTH;i++) {
		char *path = ((PaintApplication*)be_app)->Settings()->recent_image_paths[i];
		if (path != NULL) {
			BPath path_object(path,NULL,TRUE);
			BEntry entry(path_object.Path(),TRUE);
			if (entry.Exists() == TRUE) {
				entry_ref ref;
				entry.GetRef(&ref);
				a_message = new BMessage(B_REFS_RECEIVED);
				a_message->AddRef("refs",&ref);
				PaintWindowMenuItem *recent_item = new PaintWindowMenuItem(path_object.Leaf(),a_message,(char)NULL,0,this,path_object.Path());
				sub_menu->AddItem(recent_item);
				recent_item->SetTarget(be_app);
			}
		}
	}
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(OPEN_IMAGE_STRING),new BMessage(HS_SHOW_IMAGE_OPEN_PANEL),'O',0,this,StringServer::ReturnString(OPEN_IMAGE_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(SAVE_IMAGE_AS_STRING),new BMessage(HS_SHOW_IMAGE_SAVE_PANEL),(char)NULL,0,this,StringServer::ReturnString(SAVE_IMAGE_AS_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(SAVE_IMAGE_STRING),new BMessage(HS_SAVE_IMAGE),'S',0,this,StringServer::ReturnString(SAVE_IMAGE_HELP_STRING)));
	item->FindItem(HS_SHOW_IMAGE_OPEN_PANEL)->SetTarget(be_app);
	item->FindItem(HS_SAVE_IMAGE)->SetEnabled(FALSE);
	item->AddSeparatorItem();
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(NEW_PROJECT_STRING),new BMessage(HS_NEW_PAINT_WINDOW),'N',0,this,StringServer::ReturnString(NEW_PROJECT_HELP_STRING)));	// This is same as window->new paint window
	sub_menu = new BMenu(StringServer::ReturnString(RECENT_PROJECTS_STRING));
	item->AddItem(sub_menu);
	for (int32 i=0;i<RECENT_LIST_LENGTH;i++) {
		char *path = ((PaintApplication*)be_app)->Settings()->recent_project_paths[i];
		if (path != NULL) {
			BPath path_object(path,NULL,TRUE);
			BEntry entry(path_object.Path(),TRUE);
			if (entry.Exists() == TRUE) {
				entry_ref ref;
				entry.GetRef(&ref);
				a_message = new BMessage(B_REFS_RECEIVED);
				a_message->AddRef("refs",&ref);
				PaintWindowMenuItem *recent_item = new PaintWindowMenuItem(path_object.Leaf(),a_message,(char)NULL,0,this,path_object.Path());
				sub_menu->AddItem(recent_item);
				recent_item->SetTarget(be_app);
			}
		}
	}

	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(OPEN_PROJECT_STRING), new BMessage(HS_SHOW_PROJECT_OPEN_PANEL),'O',B_SHIFT_KEY,this,StringServer::ReturnString(OPEN_PROJECT_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(SAVE_PROJECT_AS_STRING),new BMessage(HS_SHOW_PROJECT_SAVE_PANEL),(char)NULL,0,this,StringServer::ReturnString(SAVE_PROJECT_AS_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(SAVE_PROJECT_STRING),new BMessage(HS_SAVE_PROJECT),'S',B_SHIFT_KEY,this,StringServer::ReturnString(SAVE_PROJECT_HELP_STRING)));
	item->FindItem(HS_SAVE_PROJECT)->SetEnabled(FALSE);
	item->FindItem(HS_SHOW_PROJECT_OPEN_PANEL)->SetTarget(be_app);
	item->FindItem(HS_NEW_PAINT_WINDOW)->SetTarget(be_app);
	item->AddSeparatorItem();
	// The B_QUIT_REQUESTED is used to close the window and quit the app.
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(CLOSE_STRING),new BMessage(B_QUIT_REQUESTED),'W',0,this,StringServer::ReturnString(CLOSE_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(QUIT_STRING),new BMessage(B_QUIT_REQUESTED),'Q',0,this,StringServer::ReturnString(QUIT_HELP_STRING)));
	item->FindItem(StringServer::ReturnString(QUIT_STRING))->SetTarget(be_app);

//	item->AddItem(new BMenuItem("Save Image Into Resources", new BMessage(HS_SAVE_IMAGE_INTO_RESOURCES)));
//	item->AddItem(new BMenuItem("Save Layer As Cursor", new BMessage(HS_SAVE_IMAGE_AS_CURSOR)));

	// The Edit menu.
	item = new BMenu(StringServer::ReturnString(EDIT_STRING));
	menubar->AddItem(item),
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(UNDO_NOT_AVAILABLE_STRING),new BMessage(HS_UNDO),'Z',0,this,StringServer::ReturnString(UNDO_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(REDO_NOT_AVAILABLE_STRING),new BMessage(HS_REDO),'Z',B_SHIFT_KEY,this,StringServer::ReturnString(REDO_HELP_STRING)));
	item->AddItem(new BSeparatorItem());
	sub_menu = new BMenu(StringServer::ReturnString(CUT_STRING));
	item->AddItem(sub_menu);
	a_message = new BMessage(B_CUT);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	sub_menu->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(ACTIVE_LAYER_STRING),a_message,'X',0,this,StringServer::ReturnString(LAYER_CUT_HELP_STRING)));

	a_message = new BMessage(B_CUT);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	sub_menu->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(ALL_LAYERS_STRING),a_message,'X',B_SHIFT_KEY,this,StringServer::ReturnString(ALL_LAYERS_CUT_HELP_STRING)));

	sub_menu = new BMenu(StringServer::ReturnString(COPY_STRING));
	item->AddItem(sub_menu);
	a_message = new BMessage(B_COPY);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	sub_menu->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(ACTIVE_LAYER_STRING),a_message,'C',0,this,StringServer::ReturnString(LAYER_COPY_HELP_STRING)));

	a_message = new BMessage(B_COPY);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	sub_menu->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(ALL_LAYERS_STRING),a_message,'C',B_SHIFT_KEY,this,StringServer::ReturnString(ALL_LAYERS_COPY_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(PASTE_AS_NEW_LAYER_STRING),new BMessage(B_PASTE),'V',0,this,StringServer::ReturnString(PASTE_AS_NEW_LAYER_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(PASTE_AS_NEW_PROJECT_STRING),new BMessage(B_PASTE),'V',B_SHIFT_KEY,this,StringServer::ReturnString(PASTE_AS_NEW_PROJECT_HELP_STRING)));

	item->AddItem(new BSeparatorItem());

	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(GROW_SELECTION_STRING),new BMessage(HS_GROW_SELECTION),'G',0,this,StringServer::ReturnString(GROW_SELECTION_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(SHRINK_SELECTION_STRING),new BMessage(HS_SHRINK_SELECTION),'H',0,this,StringServer::ReturnString(SHRINK_SELECTION_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(INVERT_SELECTION_STRING), new BMessage(HS_INVERT_SELECTION),(char)NULL,0,this,StringServer::ReturnString(INVERT_SELECTION_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(CLEAR_SELECTION_STRING), new BMessage(HS_CLEAR_SELECTION),'D',0,this,StringServer::ReturnString(CLEAR_SELECTION_HELP_STRING)));

	// The Layer menu.
	item = new BMenu(StringServer::ReturnString(LAYER_STRING));
	menubar->AddItem(item);

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",ROTATION_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(ROTATE_STRING),a_message,'R',0,this,StringServer::ReturnString(ROTATE_LAYER_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",TRANSLATION_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(TRANSLATE_STRING),a_message,'T',0,this,StringServer::ReturnString(TRANSLATE_LAYER_HELP_STRING)));

//	a_message = new BMessage(HS_START_MANIPULATOR);
//	a_message->AddInt32("manipulator_type",FREE_TRANSFORM_MANIPULATOR);
//	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
//	item->AddItem(new PaintWindowMenuItem("Free transform test",a_message,(char)NULL,0,this,"Use left shift and control to rotate and scale."));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",HORIZ_FLIP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(FLIP_HORIZONTAL_STRING),a_message,B_LEFT_ARROW,0,this,StringServer::ReturnString(FLIP_HORIZONTAL_LAYER_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",VERT_FLIP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(FLIP_VERTICAL_STRING),a_message,B_UP_ARROW,0,this,StringServer::ReturnString(FLIP_VERTICAL_LAYER_HELP_STRING)));

	item->AddSeparatorItem();

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",TRANSPARENCY_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(CHANGE_TRANSPARENCY_STRING),a_message,(char)NULL,0,this,StringServer::ReturnString(CHANGE_TRANSPARENCY_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(CLEAR_LAYER_STRING),new BMessage(HS_CLEAR_LAYER),(char)NULL,0,this,StringServer::ReturnString(CLEAR_LAYER_HELP_STRING)));

	item->AddSeparatorItem();

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",TEXT_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(INSERT_TEXT_STRING),a_message,'I',0,this,StringServer::ReturnString(INSERT_TEXT_HELP_STRING)));
	item->AddSeparatorItem();
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(SHOW_LAYER_WINDOW_STRING),new BMessage(HS_SHOW_LAYER_WINDOW),'L',0,this,StringServer::ReturnString(SHOW_LAYER_WINDOW_HELP_STRING)));
	item->AddItem(new BSeparatorItem());
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(ADD_LAYER_STRING),new BMessage(HS_ADD_LAYER_FRONT),'.',0,this,StringServer::ReturnString(ADD_LAYER_HELP_STRING)));

	// The Canvas menu.
	item = new BMenu(StringServer::ReturnString(CANVAS_STRING));
	menubar->AddItem(item);

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",ROTATION_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(ROTATE_STRING),a_message,'R',B_SHIFT_KEY,this,StringServer::ReturnString(ROTATE_ALL_LAYERS_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",ROTATE_CW_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(ROTATE_CW_STRING),a_message,0,0,this,StringServer::ReturnString(ROTATE_ALL_LAYERS_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",ROTATE_CCW_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(ROTATE_CCW_STRING),a_message,0,0,this,StringServer::ReturnString(ROTATE_ALL_LAYERS_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",TRANSLATION_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(TRANSLATE_STRING),a_message,'T',B_SHIFT_KEY,this,StringServer::ReturnString(TRANSLATE_ALL_LAYERS_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",HORIZ_FLIP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(FLIP_HORIZONTAL_STRING),a_message,B_LEFT_ARROW,B_SHIFT_KEY,this,StringServer::ReturnString(FLIP_HORIZONTAL_ALL_LAYERS_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",VERT_FLIP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(FLIP_VERTICAL_STRING),a_message,B_UP_ARROW,B_SHIFT_KEY,this,StringServer::ReturnString(FLIP_VERTICAL_ALL_LAYERS_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",CROP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(CROP_STRING),a_message,'C',B_CONTROL_KEY,this,StringServer::ReturnString(CROP_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",SCALE_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(SCALE_STRING),a_message,'S',B_CONTROL_KEY,this,StringServer::ReturnString(SCALE_ALL_LAYERS_HELP_STRING)));

	item->AddItem(new BSeparatorItem());
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(CLEAR_CANVAS_STRING), new BMessage(HS_CLEAR_CANVAS),(char)NULL,0,this,StringServer::ReturnString(CLEAR_CANVAS_HELP_STRING)));


	// The Window menu,
	item = new BMenu(StringServer::ReturnString(WINDOW_STRING));
	menubar->AddItem(item);

	// use + and - as shorcuts for zooming
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(ZOOM_IN_STRING), new BMessage(HS_ZOOM_IMAGE_IN),'+',0,this,StringServer::ReturnString(ZOOM_IN_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(ZOOM_OUT_STRING), new BMessage(HS_ZOOM_IMAGE_OUT),'-',0,this,StringServer::ReturnString(ZOOM_OUT_HELP_STRING)));

	sub_menu = new BMenu(StringServer::ReturnString(SET_ZOOM_LEVEL_STRING));
	item->AddItem(sub_menu);
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",0.25);
	sub_menu->AddItem(new PaintWindowMenuItem("25%",a_message,(char)NULL,0,this,StringServer::ReturnString(ZOOM_LEVEL_25_HELP_STRING)));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",0.50);
	sub_menu->AddItem(new PaintWindowMenuItem("50%",a_message,(char)NULL,0,this,StringServer::ReturnString(ZOOM_LEVEL_50_HELP_STRING)));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",1.0);
	sub_menu->AddItem(new PaintWindowMenuItem("100%",a_message,(char)NULL,0,this,StringServer::ReturnString(ZOOM_LEVEL_100_HELP_STRING)));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",2.0);
	sub_menu->AddItem(new PaintWindowMenuItem("200%",a_message,(char)NULL,0,this,StringServer::ReturnString(ZOOM_LEVEL_200_HELP_STRING)));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",4.0);
	sub_menu->AddItem(new PaintWindowMenuItem("400%",a_message,(char)NULL,0,this,StringServer::ReturnString(ZOOM_LEVEL_400_HELP_STRING)));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",8.0);
	sub_menu->AddItem(new PaintWindowMenuItem("800%",a_message,(char)NULL,0,this,StringServer::ReturnString(ZOOM_LEVEL_800_HELP_STRING)));

	sub_menu = new BMenu(StringServer::ReturnString(SET_GRID_STRING));
	item->AddItem(sub_menu);
	a_message = new BMessage(HS_GRID_ADJUSTED);
	a_message->AddPoint("origin",BPoint(0,0));
	a_message->AddInt32("unit",1);
	sub_menu->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(OFF_STRING),a_message,(char)NULL,0,this,StringServer::ReturnString(GRID_OFF_HELP_STRING)));
	a_message = new BMessage(HS_GRID_ADJUSTED);
	a_message->AddPoint("origin",BPoint(0,0));
	a_message->AddInt32("unit",2);
	sub_menu->AddItem(new PaintWindowMenuItem("2x2",a_message,(char)NULL,0,this,StringServer::ReturnString(GRID_2_BY_2_HELP_STRING)));
	a_message = new BMessage(HS_GRID_ADJUSTED);
	a_message->AddPoint("origin",BPoint(0,0));
	a_message->AddInt32("unit",4);
	sub_menu->AddItem(new PaintWindowMenuItem("4x4",a_message,(char)NULL,0,this,StringServer::ReturnString(GRID_4_BY_4_HELP_STRING)));
	a_message = new BMessage(HS_GRID_ADJUSTED);
	a_message->AddPoint("origin",BPoint(0,0));
	a_message->AddInt32("unit",8);
	sub_menu->AddItem(new PaintWindowMenuItem("8x8",a_message,(char)NULL,0,this,StringServer::ReturnString(GRID_8_BY_8_HELP_STRING)));
	sub_menu->SetRadioMode(TRUE);
	sub_menu->ItemAt(0)->SetMarked(TRUE);

	// use here the same shortcut as the tracker uses == Y
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(RESIZE_TO_FIT_STRING),new BMessage(HS_RESIZE_WINDOW_TO_FIT),'Y',0,this,StringServer::ReturnString(RESIZE_TO_FIT_HELP_STRING)));
	item->AddSeparatorItem();
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(SHOW_PALETTE_WINDOW_STRING),new BMessage(HS_SHOW_COLOR_WINDOW),'P',0,this,StringServer::ReturnString(SHOW_PALETTE_WINDOW_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(SHOW_LAYER_WINDOW_STRING),new BMessage(HS_SHOW_LAYER_WINDOW),'L',0,this,StringServer::ReturnString(SHOW_LAYER_WINDOW_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(SHOW_TOOL_WINDOW_STRING),new BMessage(HS_SHOW_TOOL_WINDOW),'K',0,this,StringServer::ReturnString(SHOW_TOOL_WINDOW_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(SHOW_TOOL_SETUP_WINDOW_STRING), new BMessage(HS_SHOW_TOOL_SETUP_WINDOW),'M',0,this,StringServer::ReturnString(SHOW_TOOL_SETUP_WINDOW_HELP_STRING)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(SHOW_BRUSH_WINDOW_STRING), new BMessage(HS_SHOW_BRUSH_STORE_WINDOW),'B',0,this,StringServer::ReturnString(SHOW_BRUSH_WINDOW_HELP_STRING)));
	item->AddSeparatorItem();
//	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(NEW_PAINT_WINDOW_STRING),new BMessage(HS_NEW_PAINT_WINDOW),'N',0,this,StringServer::ReturnString(NEW_PROJECT_HELP_STRING)));
//	item->AddSeparatorItem();
//	item->AddItem(new BMenuItem("Window settings…", new BMessage(HS_SHOW_VIEW_SETUP_WINDOW)));
	item->AddItem(new PaintWindowMenuItem(StringServer::ReturnString(SETTINGS_STRING), new BMessage(HS_SHOW_GLOBAL_SETUP_WINDOW),(char)NULL,0,this,StringServer::ReturnString(SETTINGS_HELP_STRING)));

	// This will be only temporary place for add-ons.
	// Later they will be spread in the menu hierarchy
	// according to their types.
	item = new BMenu(StringServer::ReturnString(ADD_ONS_STRING));
	thread_id add_on_adder_thread = spawn_thread(AddAddOnsToMenu,"add_on_adder_thread",B_NORMAL_PRIORITY,this);
	resume_thread(add_on_adder_thread);
	menubar->AddItem(item);

	item = new BMenu(StringServer::ReturnString(HELP_STRING));
	menubar->AddItem(item);
	BMenuItem *an_item;
	a_message =  new BMessage(HS_SHOW_USER_DOCUMENTATION);
	a_message->AddString("document","index.html");
	an_item = new PaintWindowMenuItem(StringServer::ReturnString(USER_MANUAL_STRING),a_message,(char)NULL,0,this,StringServer::ReturnString(USER_MANUAL_HELP_STRING));
	an_item->SetTarget(be_app);
	item->AddItem(an_item);

	a_message =  new BMessage(HS_SHOW_USER_DOCUMENTATION);
	a_message->AddString("document","shortcuts.html");
	an_item = new PaintWindowMenuItem(StringServer::ReturnString(SHORTCUTS_STRING),a_message,(char)NULL,0,this,StringServer::ReturnString(SHORTCUTS_HELP_STRING));
	an_item->SetTarget(be_app);
	item->AddItem(an_item);

	item->AddItem(new BSeparatorItem());

	a_message = new BMessage(HS_SHOW_ABOUT_WINDOW);
	an_item = new PaintWindowMenuItem(StringServer::ReturnString(ABOUT_ARTPAINT_STRING),a_message,(char)NULL,0,this,StringServer::ReturnString(ABOUT_HELP_STRING));
	item->AddItem(an_item);

	ChangeMenuMode(NO_IMAGE_MENU);
	AddChild(menubar);

	// the creation of menubar succeeded...
	return TRUE;
}



int32 PaintWindow::AddAddOnsToMenu(void *data)
{
	PaintWindow *this_pointer = (PaintWindow*)data;
	if (this_pointer == NULL)
		return B_ERROR;

	while ((ManipulatorServer::AddOnsAvailable() == FALSE) || (this_pointer->KeyMenuBar() == NULL)) {
		snooze(50 * 1000);
	}

	image_id add_on_id;
	image_id *addon_array = ManipulatorServer::AddOnArray();
	BMenu *add_on_menu = NULL;
	if (this_pointer->KeyMenuBar() != NULL) {
		add_on_menu = this_pointer->KeyMenuBar()->FindItem(StringServer::ReturnString(ADD_ONS_STRING))->Submenu();
	}
	if (add_on_menu != NULL) {
		this_pointer->Lock();
		for (int32 i=0;i<ManipulatorServer::AddOnCount();i++) {
			add_on_id = addon_array[i];
			char *add_on_name;
			char *add_on_help;
			int32 *add_on_version;
			status_t errors;

			errors = get_image_symbol(add_on_id,"name",B_SYMBOL_TYPE_DATA,(void**)&add_on_name);
			errors |= get_image_symbol(add_on_id,"add_on_api_version",B_SYMBOL_TYPE_DATA,(void**)&add_on_version);
			if (get_image_symbol(add_on_id,"menu_help_string",B_SYMBOL_TYPE_DATA,(void**)&add_on_help) != B_NO_ERROR)
				add_on_help = "Starts an add-on effect.";

			BMessage *a_message;
			if ((*add_on_version == ADD_ON_API_VERSION) && (errors == B_NO_ERROR)) {
				a_message = new BMessage(HS_START_MANIPULATOR);
				a_message->AddInt32("manipulator_type",ADD_ON_MANIPULATOR);
				a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
				a_message->AddInt32("add_on_id",add_on_id);
				add_on_menu->AddItem(new PaintWindowMenuItem(add_on_name,a_message,(char)NULL,0,this_pointer,add_on_help));
			}
		}
		add_on_menu->SetTargetForItems(BMessenger(this_pointer));
		this_pointer->Unlock();
	}

	if (this_pointer->image_view != NULL) {
		add_on_menu->SetTargetForItems(this_pointer->image_view);
	}

	return B_OK;
}


status_t PaintWindow::OpenImageView(int32 width, int32 height)
{
	// The image-view should be at most as large as backgroundview
	// but not larger than image width and height.
	Lock();	// Lock while we take background's bounds.
	image_view = new ImageView(background->Bounds(),width,height);
	Unlock();

	// return TRUE because we were successfull
	return TRUE;
}



status_t PaintWindow::AddImageView()
{
	// We have to lock as this functionmight be
	// called from outside the window's thread.
	Lock();

	// put the view as target for scrollbars
	horiz_scroll->SetTarget(image_view);
	vert_scroll->SetTarget(image_view);
	// Change the regular help-view's message.
//	BMessage *help_message = new BMessage(HS_REGULAR_HELP_MESSAGE);
//	help_message->AddString("message",HS_DRAW_MODE_HELP_MESSAGE);
//	PostMessage(help_message,this);
//	delete help_message;

	// Change the menu-mode to enable all items.
	ChangeMenuMode(FULL_MENU);

	// Add the view to window's view hierarchy.
	background->AddChild(image_view);

	// Adjust image's position and size.
	image_view->adjustSize();
	image_view->adjustPosition();
	// Update the image's scroll-bars.
	image_view->adjustScrollBars();

	// Change image for target for certain menu-items. These cannot be changed
	// before image is added as a child to this window.
	menubar->FindItem(StringServer::ReturnString(EDIT_STRING))->Submenu()->SetTargetForItems(image_view);
	menubar->FindItem(StringServer::ReturnString(PASTE_AS_NEW_PROJECT_STRING))->SetTarget(be_app);

	BMenu *menu = menubar->FindItem(StringServer::ReturnString(EDIT_STRING))->Submenu();
	if (menu != NULL) {
		BMenu *sub_menu;
		for (int32 i=0;i<menu->CountItems();i++) {
			sub_menu = menu->SubmenuAt(i);
			if (sub_menu != NULL)
				sub_menu->SetTargetForItems(image_view);
		}
	}

	menubar->FindItem(StringServer::ReturnString(ADD_ONS_STRING))->Submenu()->SetTargetForItems(image_view);
	menubar->FindItem(HS_ADD_LAYER_FRONT)->SetTarget(image_view);
	menubar->FindItem(StringServer::ReturnString(CROP_STRING))->SetTarget(image_view);
	menubar->FindItem(HS_CLEAR_CANVAS)->SetTarget(image_view);
	menubar->FindItem(HS_CLEAR_LAYER)->SetTarget(image_view);
	menubar->FindItem(HS_ZOOM_IMAGE_IN)->SetTarget(image_view);
	menubar->FindItem(HS_ZOOM_IMAGE_OUT)->SetTarget(image_view);
	menubar->FindItem(StringServer::ReturnString(SET_ZOOM_LEVEL_STRING))->Submenu()->SetTargetForItems(image_view);
	menubar->FindItem(StringServer::ReturnString(SET_GRID_STRING))->Submenu()->SetTargetForItems(image_view);
	HSStack<BMenu*> menu_stack(100);

	for (int32 i=0;i<menubar->CountItems();i++) {
		if (menubar->ItemAt(i)->Submenu() != NULL)
			menu_stack.push(menubar->ItemAt(i)->Submenu());
	}

	// Change the image as target for all menu-items that have HS_START_MANIPULATOR
	// as their message's what constant.
	menu = menu_stack.pop();
	while (menu != NULL) {
		for (int32 i=0;i<menu->CountItems();i++) {
			if (menu->ItemAt(i)->Command() == HS_START_MANIPULATOR) {
				menu->ItemAt(i)->SetTarget(image_view);
			}
			if (menu->ItemAt(i)->Submenu() != NULL) {
				menu_stack.push(menu->ItemAt(i)->Submenu());
			}
		}
		menu = menu_stack.pop();
	}

	// This allows Alt-+ next to the backspace key to work (the menu item shortcut only works
	// with the + key on the numeric keypad)
	AddShortcut('=', B_COMMAND_KEY, new BMessage(HS_ZOOM_IMAGE_IN), image_view);

	// change the name of the image and the project in the image-view
	if (project_entry.InitCheck() == B_OK) {
		char name[B_PATH_NAME_LENGTH];
		project_entry.GetName(name);
		image_view->SetProjectName(name);
	}
	else {
		char title[256];
		sprintf(title,"%s - %ld",StringServer::ReturnString(UNTITLED_STRING),
			untitled_window_number++);
		image_view->SetProjectName(title);
	}
	if (image_entry.InitCheck() == B_OK) {
		char name[B_PATH_NAME_LENGTH];
		image_entry.GetName(name);
		image_view->SetImageName(name);
	}
	status_view->DisplayToolsAndColors();
	// Finally unlock the window.
	Unlock();

	// We can update ourselves to the layer-window.
	LayerWindow::ActiveWindowChanged(this,image_view->ReturnImage()->LayerList(),image_view->ReturnImage()->ReturnThumbnailImage());

	return B_OK;
}


void PaintWindow::resizeToFit()
{
	// here we check if the window fits to screen when resized
	// if not then we will make the window as big as will fit on screen

	// we will get screens dimensions and then use min() function to decide
	// what will be new window dimensions


	// Store the user-frame
	if (Frame() != getPreferredSize())
		user_frame = Frame();

	// here we should also take magify_scale into account
	BRect screen_bounds;
	{
		screen_bounds = BScreen(this).Frame();
	}
	screen_bounds.OffsetTo(0,0);

	if (!image_view) {
		screen_bounds.InsetBy(7.5, 7.5);
		ResizeTo(screen_bounds.right, screen_bounds.bottom - 20.0);
		MoveTo(screen_bounds.LeftTop() + BPoint(-5.0, 15.0));
		return;
	}

	// we have to subtract a little from screen dimensions to leave some room
	// around window
	float width = min_c(screen_bounds.Width() - 30,
		image_view->getMagScale() * image_view->ReturnImage()->Width() +
		additional_width);
	float height = min_c(screen_bounds.Height() - 30,
		image_view->getMagScale()*image_view->ReturnImage()->Height() +
		additional_height);
	width = ceil(width);
	height = ceil(height);

	BPoint left_top;
	left_top.x = min_c(max_c(5,Frame().LeftTop().x),screen_bounds.Width()-width);
	left_top.y = min_c(max_c(5,Frame().LeftTop().y),screen_bounds.Height()-height);

	left_top.x = floor(left_top.x);
	left_top.y = floor(left_top.y);

	ResizeTo(width,height);
	MoveTo(left_top);
}


BRect PaintWindow::getPreferredSize()
{
	// here we should also take magify_scale into account
	BRect screen_bounds;
	{
		screen_bounds = BScreen(this).Frame();
	}
	screen_bounds.OffsetTo(0,0);

	if (!image_view)
		return screen_bounds;

	// we have to subtract a little from screen dimensions to leave some room
	// around window
	float width = min_c(screen_bounds.Width() - 30.0,
		image_view->getMagScale() * image_view->ReturnImage()->Width() +
		additional_width);
	float height = min_c(screen_bounds.Height() - 30.0,
		image_view->getMagScale() * image_view->ReturnImage()->Height() +
		additional_height);
	width = ceil(width);
	height = ceil(height);

	BPoint left_top;
	left_top.x = min_c(max_c(5,Frame().LeftTop().x),screen_bounds.Width()-width);
	left_top.y = min_c(max_c(5,Frame().LeftTop().y),screen_bounds.Height()-height);

	left_top.x = floor(left_top.x);
	left_top.y = floor(left_top.y);

	return BRect(left_top.x, left_top.y, left_top.x + width, left_top.y + height);
}

int32 PaintWindow::save_image(void *data)
{
	PaintWindow *window = (PaintWindow*)data;
	BMessage *message = NULL;
	thread_id sender;
	receive_data(&sender,(void*)&message,sizeof(BMessage*));
	if (window != NULL) {
		status_t error = window->saveImage(message);
		delete message;
		return error;
	}
	else
		return B_ERROR;
}

status_t PaintWindow::saveImage(BMessage *message)
{
	if (image_view->Freeze() == B_NO_ERROR) {
		entry_ref ref;
		message->FindRef("directory",&ref);
		BDirectory directory = BDirectory(&ref);
		status_t err;

		// store the entry-ref
		err = image_entry.SetTo(&directory,message->FindString("name"),TRUE);

		// Only one save ref is received so we do not need to loop.
		BFile file;
		if ((err = file.SetTo(&directory,message->FindString("name"),B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE)) != B_NO_ERROR) {
			image_view->UnFreeze();
			return err;
		}

		// get the applications signature
		app_info info;
		be_app->GetAppInfo(&info);

		// Create a BNodeInfo for this file and set the MIME-type and preferred app.
		BNodeInfo *nodeinfo = new BNodeInfo(&file);
		nodeinfo->SetType(settings->file_mime);
//		nodeinfo->SetPreferredApp(info.signature);
		delete nodeinfo;

		// here we should save some attributes with the file
		BNode *node = new BNode(&directory,message->FindString("name"));
		writeAttributes(*node);

		// here translate the data using a BitmapStream-object
		BBitmap *picture = image_view->ReturnImage()->ReturnRenderedImage();

		printf("Picture at 0,0: 0x%8lx\n",*((uint32*)(picture->Bits())));

		BBitmapStream *image_stream = new BBitmapStream(picture);
		BTranslatorRoster *roster = BTranslatorRoster::Default();

		err = roster->Translate(image_stream,(const translator_info*)NULL,(BMessage*)NULL,&file,settings->file_type,B_TRANSLATOR_BITMAP);
		image_view->UnFreeze();
		if (err == B_NO_ERROR) {
			char title[B_FILE_NAME_LENGTH];
			image_entry.GetName(title);
			Lock();
			image_view->ResetChangeStatistics(false,true);
			image_view->SetImageName(title);
//			BMenuItem *item = menubar->FindItem(HS_SAVE_IMAGE);
//			if (item) item->SetEnabled(TRUE);
			Unlock();
			// Also change this new path into the settings.

			BPath path;
			image_entry.GetPath(&path);
			((PaintApplication*)be_app)->Settings()->insert_recent_image_path(path.Path());
			path.GetParent(&path);

			if (path.Path() != NULL) {
				strcpy(((PaintApplication*)be_app)->Settings()->image_save_path,path.Path());
			}
		}
		else {
			printf("error while saving\n");
			strerror(err);
		}
		return err;
	}
	else
		return B_ERROR;
}


status_t PaintWindow::saveImageIntoResources()
{
	// Find a file named ArtPaint.rsrc in the apps directory an create a resource pointing to it.
	BFile res_file;
	BResources *resources;

	if (res_file.SetTo("/boot/home/projects/ArtPaint/ArtPaint.rsrc",B_READ_WRITE) != B_OK) {
		return B_ERROR;
	}
	resources = new BResources(&res_file);

	// Read the id for the saved file.
	int32 res_id;
	printf("Please give an integer to represent the id for resource: ");
	scanf("%ld",&res_id);

	// While writing the image convert it to B_COLOR_8_BIT.
	BBitmap *buffer = image_view->ReturnImage()->ReturnRenderedImage();
	char *image_8_bit = new char[buffer->BitsLength()/4];
	uint32 *bits = (uint32*)buffer->Bits();
	int32 bits_length = buffer->BitsLength()/4;
	BScreen *screen = new BScreen();
	for (int32 i=0;i<bits_length;i++) {
		image_8_bit[i] = screen->IndexForColor(BGRAColorToRGB(*bits++));
	}
	delete screen;
	// Write the resource
	if (resources->HasResource(B_COLOR_8_BIT_TYPE,res_id))
		resources->RemoveResource(B_COLOR_8_BIT_TYPE,res_id);

	resources->AddResource(B_COLOR_8_BIT_TYPE,res_id,image_8_bit,bits_length,"ArtPaint");

	// Finally delete resources and return B_OK.
	delete resources;
	delete image_8_bit;
	return B_OK;
}


status_t PaintWindow::saveImageAsCursor()
{
	BFile cursor_file;
	if (cursor_file.SetTo("/boot/home/Cursor.h",B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE) != B_OK) {
		return B_ERROR;
	}
	BBitmap *buffer = image_view->ReturnImage()->ReturnActiveBitmap();

	int32 colors[16][2];
	int32 transparency[16][2];


	uint32 *bits = (uint32*)buffer->Bits();
	int32 bpr = buffer->BytesPerRow()/4;

	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	for (int32 y=0;y<16;y++) {
		int32 c = 0x00;
		int32 t = 0x00;
		for (int32 x=0;x<8;x++) {
			color.word = *(bits+x+y*bpr);
			c = c | ((color.bytes[2]>127?0:1) << (7-x));
			t = t | ((color.bytes[3]>127?1:0) << (7-x));
		}
		colors[y][0] = c;
		transparency[y][0] = t;
		c = 0x00;
		t = 0x00;
		for (int32 x=8;x<16;x++) {
			color.word = *(bits+x+y*bpr);
			c = c | ((color.bytes[2]>127?0:1) << (7-(x-8)));
			t = t | ((color.bytes[3]>127?1:0) << (7-(x-8)));
		}
		colors[y][1] = c;
		transparency[y][1] = t;
	}


	// Here we print the data to a string.
	char header_text[2048];
	sprintf(header_text,"const unsigned char CURSOR[] =\n\t\t{\n\t\t\t0x10, 0x01,"
		"0x01, 0x01,\n\t\t\t0x%0lx, 0x%0lx, 0x%0lx, 0x%0lx,\n\t\t\t0x%0lx, 0x%0lx,"
		"0x%0lx, 0x%0lx,\n\t\t\t0x%0lx, 0x%0lx, 0x%0lx, 0x%0lx,\n\t\t\t0x%0lx,"
		"0x%0lx, 0x%0lx, 0x%0lx,\n\t\t\t0x%0lx, 0x%0lx, 0x%0lx, 0x%0lx,\n\t\t\t"
		"0x%0lx, 0x%0lx, 0x%0lx, 0x%0lx,\n\t\t\t0x%0lx, 0x%0lx, 0x%0lx, 0x%0lx,"
		"\n\t\t\t0x%0lx, 0x%0lx, 0x%0lx, 0x%0lx,\n\t\t\t0x%0lx, 0x%0lx, 0x%0lx,"
		"0x%0lx,\n\t\t\t0x%0lx, 0x%0lx, 0x%0lx, 0x%0lx,\n\t\t\t0x%0lx, 0x%0lx,"
		"0x%0lx, 0x%0lx,\n\t\t\t0x%0lx, 0x%0lx, 0x%0lx, 0x%0lx,\n\t\t\t0x%0lx,"
		"0x%0lx, 0x%0lx, 0x%0lx,\n\t\t\t0x%0lx, 0x%0lx, 0x%0lx, 0x%0lx,\n\t\t\t"
		"0x%0lx, 0x%0lx, 0x%0lx, 0x%0lx,\n\t\t\t0x%0lx, 0x%0lx,0x%0lx, 0x%0lx,"
		"\n\t\t };", colors[0][0], colors[0][1], colors[1][0], colors[1][1],
		colors[2][0], colors[2][1], colors[3][0], colors[3][1], colors[4][0],
		colors[4][1], colors[5][0], colors[5][1], colors[6][0], colors[6][1],
		colors[7][0], colors[7][1], colors[8][0], colors[8][1], colors[9][0],
		colors[9][1], colors[10][0], colors[10][1], colors[11][0], colors[11][1],
		colors[12][0], colors[12][1], colors[13][0], colors[13][1], colors[14][0],
		colors[14][1], colors[15][0], colors[15][1],transparency[0][0],
		transparency[0][1],transparency[1][0],transparency[1][1],transparency[2][0],
		transparency[2][1],transparency[3][0],transparency[3][1],transparency[4][0],
		transparency[4][1],transparency[5][0],transparency[5][1],transparency[6][0],
		transparency[6][1],transparency[7][0],transparency[7][1],transparency[8][0],
		transparency[8][1],transparency[9][0],transparency[9][1],transparency[10][0],
		transparency[10][1],transparency[11][0],transparency[11][1],transparency[12][0],
		transparency[12][1],transparency[13][0],transparency[13][1],transparency[14][0],
		transparency[14][1],transparency[15][0],transparency[15][1]);

	cursor_file.Write(header_text,strlen(header_text));

	return B_OK;
}

int32 PaintWindow::save_project(void *data)
{
	PaintWindow *window = (PaintWindow*)data;
	BMessage *message = NULL;
	thread_id sender;
	receive_data(&sender,(void*)&message,sizeof(BMessage*));
	if (window != NULL) {
		status_t error = window->saveProject(message);
		delete message;
		return error;
	}
	else
		return B_ERROR;
}

status_t PaintWindow::saveProject(BMessage *message)
{
	if (image_view->Freeze() == B_NO_ERROR) {
		// We open the file that the ref and the name of message tells us.
		entry_ref ref;
		message->FindRef("directory",&ref);
		BDirectory directory = BDirectory(&ref);
		status_t err;

		// store the entry-ref
		err = project_entry.SetTo(&directory,message->FindString("name"),TRUE);

		// Only one save ref is received so we do not need to loop.
		BFile file;
		if ((err = file.SetTo(&directory,message->FindString("name"),B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE)) != B_NO_ERROR) {
			image_view->UnFreeze();
			return err;
		}

		// get the applications signature
		app_info info;
		be_app->GetAppInfo(&info);

		// create a BNodeInfo for this file
		BNodeInfo *nodeinfo = new BNodeInfo(&file);
		nodeinfo->SetType(HS_PROJECT_MIME_STRING);
//		nodeinfo->SetPreferredApp(info.signature);
		delete nodeinfo;

		// The project-file will be written in the endianness of the host-computer
		// First word of the file will mark the endianness. If its 0xFFFFFFFF the file is
		// little-endian if it is 0x00000000, the file is big-endian.
		int32 lendian;
		if (B_HOST_IS_LENDIAN == 1)
			lendian = 0xFFFFFFFF;
		else
			lendian = 0x00000000;

		// Write the endianness
		if (file.Write(&lendian,sizeof(int32)) != sizeof(int32)) {
			image_view->UnFreeze();
			return B_ERROR;
		}

		// Write the file-id.
		int32 id = PROJECT_FILE_ID;
		if (file.Write(&id,sizeof(int32)) != sizeof(int32)) {
			image_view->UnFreeze();
			return B_ERROR;
		}

		// Write the number of sections that the file contains. Currently there are
		// only two sections.
		int32 section_count = 2;
		if (file.Write(&section_count,sizeof(int32)) != sizeof(int32)) {
			image_view->UnFreeze();
			return B_ERROR;
		}

		// First write the dimensions section. The real dimensions of the image are written to the file.
		int32 marker = PROJECT_FILE_SECTION_START;
		if (file.Write(&marker,sizeof(int32)) != sizeof(int32)) {
			image_view->UnFreeze();
			return B_ERROR;
		}

		// Here write the section type.
		int32 type = PROJECT_FILE_DIMENSION_SECTION_ID;
		if (file.Write(&type,sizeof(int32)) != sizeof(int32)) {
			image_view->UnFreeze();
			return B_ERROR;
		}

		// Leave room for the section-length
		file.Seek(sizeof(int64),SEEK_CUR);

		// Here write the width and height.
		int32 width = (int32)image_view->ReturnImage()->Width();
		int32 height = (int32)image_view->ReturnImage()->Height();
		int64 written_bytes = 0;
		written_bytes = file.Write(&width,sizeof(int32));
		written_bytes += file.Write(&height,sizeof(int32));

		if (written_bytes != 2*sizeof(int32)) {
			image_view->UnFreeze();
			return B_ERROR;
		}

		file.Seek(-written_bytes-sizeof(int64),SEEK_CUR);
		if (file.Write(&written_bytes,sizeof(int64)) != sizeof(int64)) {
			image_view->UnFreeze();
			return B_ERROR;
		}
		file.Seek(written_bytes,SEEK_CUR);

		marker = PROJECT_FILE_SECTION_END;
		if (file.Write(&marker,sizeof(int32)) != sizeof(int32)) {
			image_view->UnFreeze();
			return B_ERROR;
		}

		// Here starts the layer-section.
		marker = PROJECT_FILE_SECTION_START;
		if (file.Write(&marker,sizeof(int32)) != sizeof(int32)) {
			image_view->UnFreeze();
			return B_ERROR;
		}
		id = PROJECT_FILE_LAYER_SECTION_ID;
		if (file.Write(&id,sizeof(int32)) != sizeof(int32)) {
			image_view->UnFreeze();
			return B_ERROR;
		}

		// Leave some room for the length.
		file.Seek(sizeof(int64),SEEK_CUR);

		// Here tell the image to write layers.
		written_bytes = image_view->ReturnImage()->WriteLayers(file);

		file.Seek(-written_bytes-sizeof(int64),SEEK_CUR);
		if (file.Write(&written_bytes,sizeof(int64)) != sizeof(int64)) {
			image_view->UnFreeze();
			return B_ERROR;
		}
		file.Seek(written_bytes,SEEK_CUR);

		// Write the end-marker for the layer-section.
		marker = PROJECT_FILE_SECTION_END;
		if (file.Write(&marker,sizeof(int32)) != sizeof(int32)) {
			image_view->UnFreeze();
			return B_ERROR;
		}


		// Now we are happily at the end of writing.
		image_view->ResetChangeStatistics(true,false);
		image_view->UnFreeze();
		char title[256];
		project_entry.GetName(title);
		Lock();
		image_view->SetProjectName(title);

		// This must come after the project's name has been set.
		LayerWindow::ActiveWindowChanged(this,image_view->ReturnImage()->LayerList(),image_view->ReturnImage()->ReturnThumbnailImage());
//		BMenuItem *item = menubar->FindItem(HS_SAVE_PROJECT);
//		if (item) item->SetEnabled(TRUE);
		Unlock();
		// Also change this new path into the settings.
		BPath path;
		project_entry.GetPath(&path);
		((PaintApplication*)be_app)->Settings()->insert_recent_project_path(path.Path());
		path.GetParent(&path);

		if (path.Path() != NULL) {
			strcpy(((PaintApplication*)be_app)->Settings()->project_save_path,path.Path());
		}
		return B_NO_ERROR;
	}
	else {
		return B_ERROR;
	}
}


void
PaintWindow::writeAttributes(BNode& node)
{
	BRect frame(Frame());
	node.WriteAttr("ArtP:frame_rect", B_RECT_TYPE, 0, &frame, sizeof(BRect));
	if (image_view && image_view->LockLooper()) {
		settings->zoom_level = image_view->getMagScale();
		node.WriteAttr("ArtP:zoom_level", B_FLOAT_TYPE, 0,
			&(settings->zoom_level), sizeof(float));

		settings->view_position = image_view->LeftTop();
		node.WriteAttr("ArtP:view_position", B_POINT_TYPE, 0,
			&(settings->view_position), sizeof(BPoint));
		image_view->UnlockLooper();
	}
}


void
PaintWindow::readAttributes(BNode &node)
{
	if (Lock()) {
		float zoom_level;
		if (node.ReadAttr("ArtP:zoom_level", B_FLOAT_TYPE, 0, &zoom_level,
			sizeof(float)) > 0) {
			settings->zoom_level = zoom_level;
			if (image_view)
				image_view->setMagScale(zoom_level);
		}

		BPoint view_position;
		if (node.ReadAttr("ArtP:view_position" ,B_POINT_TYPE, 0, &view_position,
			sizeof(BPoint)) > 0) {
			settings->view_position = view_position;
			if (image_view)
				image_view->ScrollTo(view_position);
		}

		BRect frame;
		if (node.ReadAttr("ArtP:frame_rect", B_RECT_TYPE, 0, &frame,
			sizeof(BRect)) > 0) {
			frame = FitRectToScreen(frame);
			MoveTo(frame.left, frame.top);
			ResizeTo(frame.Width(), frame.Height());
		}

		Unlock();
	}
}


void PaintWindow::ChangeMenuMode(menu_modes new_mode)
{
	// Here create a pseudo-stack for use in quasi-recursion.
	// Always increment last_item before adding item and decrement
	// it after removing an item.
	BMenu *sub_menus[50];
	int32 last_item = -1;
	BMenuItem *current_item;

	// First enable all the items in the menu.
	for (int32 i=0;i<menubar->CountItems();i++) {
		current_item = menubar->ItemAt(i);
		current_item->SetEnabled(TRUE);
		if (current_item->Submenu()) {
			last_item++;
			sub_menus[last_item] = current_item->Submenu();
			while (last_item >= 0) {
				BMenu *sub_menu = sub_menus[last_item];
				last_item--;
				for (int32 j=0;j<sub_menu->CountItems();j++) {
					current_item = sub_menu->ItemAt(j);
					current_item->SetEnabled(TRUE);
					if (current_item->Submenu()) {
						last_item++;
						sub_menus[last_item] = current_item->Submenu();
					}
				}
			}
		}
	}

	BMenuItem *to_be_disabled;
	// And then disable the items that need to be disabled.
	switch (new_mode) {

	// In this case we should not disable any menu-items.
	// Except perhaps ones that deal with some changing status
	// of the image (e.g. area selections and the clipboard). The image
	// will have functions with which to query the status.
	case FULL_MENU:
		if (image_entry.InitCheck() != B_NO_ERROR) {
			to_be_disabled = menubar->FindItem(HS_SAVE_IMAGE);
			if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);
		}
		if (project_entry.InitCheck() != B_NO_ERROR) {
			to_be_disabled = menubar->FindItem(HS_SAVE_PROJECT);
			if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);
		}
		break;

	// In this case we disable some additional items and then let fall through
	// to the next case that disables most of the other items.
	case NO_IMAGE_MENU:
		to_be_disabled = menubar->FindItem(StringServer::ReturnString(RESIZE_TO_FIT_STRING));
		if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);

		to_be_disabled = menubar->FindItem(StringServer::ReturnString(ZOOM_IN_STRING));
		if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);

		to_be_disabled = menubar->FindItem(StringServer::ReturnString(ZOOM_OUT_STRING));
		if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);

		to_be_disabled = menubar->FindItem(StringServer::ReturnString(SET_ZOOM_LEVEL_STRING));
		if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);

		to_be_disabled = menubar->FindItem(StringServer::ReturnString(SET_GRID_STRING));
		if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);

	// In this case most of the items should be disabled.
	case MINIMAL_MENU:
		to_be_disabled = menubar->FindItem(StringServer::ReturnString(CANVAS_STRING));
		if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);

		to_be_disabled = menubar->FindItem(StringServer::ReturnString(LAYER_STRING));
		if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);

		to_be_disabled = menubar->FindItem(StringServer::ReturnString(SAVE_IMAGE_AS_STRING));
		if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);

		to_be_disabled = menubar->FindItem(StringServer::ReturnString(SAVE_PROJECT_AS_STRING));
		if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);

		to_be_disabled = menubar->FindItem(StringServer::ReturnString(EDIT_STRING));
		if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);

		to_be_disabled = menubar->FindItem(StringServer::ReturnString(ADD_ONS_STRING));
		if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);

		to_be_disabled = menubar->FindItem(HS_SAVE_IMAGE);
		if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);

		to_be_disabled = menubar->FindItem(HS_SAVE_PROJECT);
		if (to_be_disabled != NULL) to_be_disabled->SetEnabled(FALSE);

		break;

	}
}




StatusView* PaintWindow::ReturnStatusView()
{
	return status_view;
}
