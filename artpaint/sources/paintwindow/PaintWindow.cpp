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

#include "PaintWindow.h"

#include "AboutWindow.h"
#include "BackgroundView.h"
#include "BrushStoreWindow.h"
#include "Controls.h"
#include "ColorPalette.h"
#include "DatatypeSetupWindow.h"
#include "FileIdentificationStrings.h"
#include "FilePanels.h"
#include "GlobalSetupWindow.h"
#include "HSStack.h"
#include "Image.h"
#include "ImageView.h"
#include "Layer.h"
#include "LayerWindow.h"
#include "Manipulator.h"
#include "ManipulatorServer.h"
#include "MessageConstants.h"
#include "MessageFilters.h"
#include "PaintApplication.h"
#include "PaintWindowMenuItem.h"
#include "ProjectFileFunctions.h"
#include "PopUpList.h"
#include "Settings.h"
#include "StatusView.h"
#include "SymbolImageServer.h"
#include "ToolSetupWindow.h"
#include "ToolSelectionWindow.h"
#include "UtilityClasses.h"
#include "VersionConstants.h"
#include "ViewSetupWindow.h"


#include <Alert.h>
#include <BitmapStream.h>
#include <Button.h>
#include <Clipboard.h>
#include <Entry.h>
#include <MenuBar.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <SupportDefs.h>
#include <TranslatorRoster.h>


#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// initialize the static variable
BList PaintWindow::sgPaintWindowList(10);
int32 PaintWindow::sgPaintWindowCount = 0;
int32 PaintWindow::sgUntitledWindowNumber = 1;


// these constants are for the internal communication of the PaintWindow-class
#define	HS_SHOW_VIEW_SETUP_WINDOW		'SvsW'
#define HS_SHOW_GLOBAL_SETUP_WINDOW		'SgsW'
#define	HS_SAVE_IMAGE_INTO_RESOURCES	'Sirc'
#define	HS_SAVE_IMAGE_AS_CURSOR			'Siac'
#define	HS_RECENT_IMAGE_SIZE			'Rsis'
#define	HS_SHOW_ABOUT_WINDOW			'Sabw'


struct menu_item {
	string_id label;
	uint32 what;
	char shortcut;
	uint32 modifiers;
	BHandler* target;
	string_id help;
};


PaintWindow::PaintWindow(const char* name, BRect frame, uint32 views,
		const window_settings *setup)
	: BWindow(frame, name, B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_WILL_ACCEPT_FIRST_CLICK | B_NOT_ANCHORED_ON_ACTIVATE)
	, fSettings(NULL)
	, fImageView(NULL)
	, fBackground(NULL)
	, fVerticalScrollbar(NULL)
	, fHorizontalScrollbar(NULL)
	, fMenubar(NULL)
	, fStatusView(NULL)
	, fContainerBox(NULL)
	, fSetSizeButton(NULL)
	, fWidthNumberControl(NULL)
	, fHeightNumberControl(NULL)
	, fImageSavePanel(NULL)
	, fProjectSavePanel(NULL)
	, fCurrentHandler(0)
	, fAdditionalWidth(0.0)
	, fAdditionalHeight(0.0)
{
	sgPaintWindowCount++;
	SetSizeLimits(400, 10000, 300, 10000);

	// Fit the window to screen.
	BRect new_frame = FitRectToScreen(frame);
	if (new_frame != frame) {
		MoveTo(new_frame.LeftTop());
		ResizeTo(new_frame.Width(),new_frame.Height());
	}

	if (views == 0)
		views = setup->views;

	// Record the settings.
	fSettings = new window_settings(setup);

	if ((views & HS_MENU_BAR) != 0) {
		// the menubar should be opened
		openMenuBar();
		fAdditionalHeight += fMenubar->Bounds().Height() + 1.0;
	}

	// here the scrollbars are opened and placed along window sides
	// vertical bar is opened to begin under the menubar

	float right = Bounds().right + 1;		// have to add one, otherwise scrollbars
	float bottom = Bounds().bottom + 1;	// would be 1 pixel away from edges (why ????)
	float top = 0;


	if (fMenubar != NULL)	// here add the height of menubar if it is opened
		top += fMenubar->Bounds().Height();

	// Create the scrollbars, and disable them.
	fHorizontalScrollbar = new BScrollBar(BRect(0.0,
		bottom - B_H_SCROLL_BAR_HEIGHT, right - B_V_SCROLL_BAR_WIDTH, bottom),
		"horizontal", NULL, 0, 0, B_HORIZONTAL);
	fVerticalScrollbar = new BScrollBar(BRect(right - B_V_SCROLL_BAR_WIDTH,
		top, right, bottom - B_H_SCROLL_BAR_HEIGHT), "vertical", NULL, 0, 0,
		B_VERTICAL);
	fHorizontalScrollbar->SetRange(0,0);
	fVerticalScrollbar->SetRange(0,0);

	// add the scroll bars to window
	AddChild(fHorizontalScrollbar);
	AddChild(fVerticalScrollbar);
	fHorizontalScrollbar->SetSteps(8,32);
	fVerticalScrollbar->SetSteps(8,32);

	// update the free area limits
	bottom -= (B_H_SCROLL_BAR_HEIGHT + 1);
	right -= (B_V_SCROLL_BAR_WIDTH + 1);
	top += 1;			// start other views one pixel down from menubar


	// increase the additional width and height variables
	fAdditionalWidth += B_V_SCROLL_BAR_WIDTH - 1;
	fAdditionalHeight += B_H_SCROLL_BAR_HEIGHT - 1;

	// The status-view is not optional. It contains sometimes also buttons that
	// can cause some actions to be taken.
	if ((views & HS_STATUS_VIEW) != 0) {	// the statusbar should be opened
		// Create the status-view and make it display nothing
		fStatusView = new StatusView(BRect(BPoint(0,bottom),BPoint(right,bottom)));
		fStatusView->DisplayNothing();

		// place the statusbar along bottom of window on top of scrollbar
		// and also record size that it occupies

		// here add the statusview to window's hierarchy
		AddChild(fStatusView);

		// update the bottom value to be 1 pixel above status_view
		// status_view is resized in window FrameResized() function
		bottom -= fStatusView->Bounds().Height() + 1.0;

		// update the fAdditionalHeight variable
		fAdditionalHeight += fStatusView->Bounds().Height() + 1.0;
	}

	// make the background view (the backround for image)
	fBackground = new BackgroundView(BRect(0,top,right,bottom));
	AddChild(fBackground);

	if ((views & HS_SIZING_VIEW) != 0x0000)  {
		// we need to show the creata canvas area
		const char* widthLabel = _StringForId(WIDTH_STRING);
		const char* heightLabel = _StringForId(HEIGHT_STRING);

		BFont font;
		const char* tmpLabel = widthLabel;
		if (font.StringWidth(heightLabel) > font.StringWidth(widthLabel))
			tmpLabel = heightLabel;
		float divider = font.StringWidth(tmpLabel) + 5.0;

		BRect rect(10.0, 10.0, 110.0, 10.0);
		fWidthNumberControl = new NumberControl(rect, "width_number_control",
			tmpLabel, "", NULL);
		fWidthNumberControl->ResizeToPreferred();
		fWidthNumberControl->SetDivider(divider);
		fWidthNumberControl->SetLabel(widthLabel);
		fWidthNumberControl->TextView()->SetMaxBytes(4);

		rect.OffsetBy(0.0, fWidthNumberControl->Bounds().Height() + 5.0);
		fHeightNumberControl = new NumberControl(rect, "height_number_control",
			tmpLabel, "", NULL);
		fHeightNumberControl->ResizeToPreferred();
		fHeightNumberControl->SetDivider(divider);
		fHeightNumberControl->SetLabel(heightLabel);
		fHeightNumberControl->TextView()->SetMaxBytes(4);

		rect.OffsetBy(0.0, fWidthNumberControl->Bounds().Height() + 10.0);
		fSetSizeButton = new BButton(rect, "set_size_button",
			_StringForId(CREATE_CANVAS_STRING), new BMessage(HS_IMAGE_SIZE_SET));
		fSetSizeButton->SetTarget(this);
		fSetSizeButton->MakeDefault(true);
		fSetSizeButton->ResizeToPreferred();

		char label[256];
		BMessage* message_list[RECENT_LIST_LENGTH];
		global_settings* settings = ((PaintApplication*)be_app)->GlobalSettings();
		for (int32 i = 0; i < RECENT_LIST_LENGTH; ++i) {
			const int32 width = settings->recent_image_width_list[i];
			const int32 height = settings->recent_image_height_list[i];

			message_list[i] = new BMessage(HS_RECENT_IMAGE_SIZE);
			message_list[i]->AddInt32("width", width);
			message_list[i]->AddInt32("height", height);

			sprintf(label,"%ld x %ld", width, height);
			message_list[i]->AddString("label", label);
		}

		float left = fHeightNumberControl->Frame().right + 5.0;
		float top = (fHeightNumberControl->Frame().bottom -
			fWidthNumberControl->Frame().top) / 2.0;

		int32 w, h;
		PopUpList* popUpList = new PopUpList(BRect(left, top, left + 9, top + 19),
			SymbolImageServer::ReturnSymbolAsBitmap(POP_UP_LIST_PUSHED, w, h),
			SymbolImageServer::ReturnSymbolAsBitmap(POP_UP_LIST, w, h),
			message_list, RECENT_LIST_LENGTH, new BMessenger(NULL, this));

		BMenu* standardSize = new BMenu(_StringForId(STANDARD_SIZES_STRING));

		BMessage* message = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width", 320);
		message->AddInt32("height", 256);
		standardSize->AddItem(new BMenuItem("320 x 256", message));

		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width", 640);
		message->AddInt32("height", 400);
		standardSize->AddItem(new BMenuItem("640 x 400", message));

		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width", 640);
		message->AddInt32("height", 480);
		standardSize->AddItem(new BMenuItem("640 x 480", message));

		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width", 800);
		message->AddInt32("height", 600);
		standardSize->AddItem(new BMenuItem("800 x 600", message));

		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width", 1024);
		message->AddInt32("height", 768);
		standardSize->AddItem(new BMenuItem("1024 x 768", message));

		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width", 1152);
		message->AddInt32("height", 900);
		standardSize->AddItem(new BMenuItem("1152 x 900", message));

		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width", 1280);
		message->AddInt32("height", 1024);
		standardSize->AddItem(new BMenuItem("1280 x 1024", message));

		message  = new BMessage(HS_RECENT_IMAGE_SIZE);
		message->AddInt32("width", 1600);
		message->AddInt32("height", 1200);
		standardSize->AddItem(new BMenuItem("1600 x 1200", message));

		popUpList->ReturnMenu()->AddItem(standardSize, 0);
		popUpList->ReturnMenu()->AddItem(new BSeparatorItem(), 1);

		float width = max_c(fSetSizeButton->Frame().right,
			popUpList->Frame().right) + 10.0;
		float height = fSetSizeButton->Frame().bottom + 10.0;

		rect = fBackground->Bounds();
		fContainerBox = new BBox(BRect(rect.right - width, rect.bottom - height,
			rect.right, rect.bottom - 1.0), "container_for_controls",
			B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		fBackground->AddChild(fContainerBox);
		fContainerBox->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		fContainerBox->AddChild(fWidthNumberControl);
		fContainerBox->AddChild(fHeightNumberControl);
		fContainerBox->AddChild(fSetSizeButton);
		fContainerBox->AddChild(popUpList);

		rect = fContainerBox->Bounds();
		BRect frame = fSetSizeButton->Frame();
		fSetSizeButton->MoveTo((rect.right - frame.Width()) / 2.0, frame.top);

		BMessage msg(HS_TOOL_HELP_MESSAGE);
		msg.AddString("message", _StringForId(SELECT_CANVAS_SIZE_STRING));
		PostMessage(&msg, this);
	}


	// finally inform the app that new window has been created
	BMessage message(HS_PAINT_WINDOW_OPENED);
	message.AddPointer("window", (void*)this);
	be_app->PostMessage(&message, be_app);

	// resize so that all things are properly updated
	ResizeBy(1,0);
	ResizeBy(-1,0);
	// show the window to user
	Show();

	// Add ourselves to the sgPaintWindowList
	sgPaintWindowList.AddItem(this);

	Lock();
	// Handle the window-activation with this common filter.
	BMessageFilter *activation_filter = new BMessageFilter(B_ANY_DELIVERY,
		B_ANY_SOURCE, B_MOUSE_DOWN, window_activation_filter);
	AddCommonFilter(activation_filter);
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN, AppKeyFilterFunction));
	Unlock();

	fUserFrame = Frame();
}


PaintWindow*
PaintWindow::CreatePaintWindow(BBitmap* bitmap, const char* file_name,
	int32 type, const entry_ref& ref, translator_id outTranslator)
{
	window_settings default_window_settings =
		((PaintApplication*)be_app)->GlobalSettings()->default_window_settings;

	uint32 flags = HS_MENU_BAR | HS_STATUS_VIEW | HS_HELP_VIEW ;
	BString title;
	if (file_name == NULL) {
		// sprintf(title,"%s - %d",_StringForId(UNTITLED_STRING),
		//		sgUntitledWindowNumber);
		flags = flags | HS_SIZING_VIEW;
		title = _StringForId(EMPTY_PAINT_WINDOW_STRING);
	} else {
		title = file_name;
	}

	PaintWindow* paintWindow = new PaintWindow(title.String(),
		default_window_settings.frame_rect, flags, &(default_window_settings));

	if (bitmap != NULL) {
		// some of these might also come from attributes
		window_settings* settings = paintWindow->Settings();

		settings->file_type = type;
		settings->zoom_level = default_window_settings.zoom_level;
		settings->view_position = default_window_settings.view_position;

		BNode node(&ref);
		BNodeInfo nodeInfo(&node);
		nodeInfo.GetType(paintWindow->Settings()->file_mime);

		paintWindow->SetImageEntry(BEntry(&ref, true));

// This is done elsewhere to make it possible to read the attributes
// after creating the image-view.
//		// make the window read it's attributes
//		paintWindow->ReadAttributes(node);

		// Open an imageview for the window and add a layer to it.
		paintWindow->OpenImageView(int32(bitmap->Bounds().Width() + 1),
			int32(bitmap->Bounds().Height() + 1));
		paintWindow->ReturnImageView()->ReturnImage()->InsertLayer(bitmap);
		paintWindow->AddImageView();
	}

	paintWindow->fCurrentHandler = outTranslator;

	// Change the zoom-level to be correct
	paintWindow->Lock();
	paintWindow->displayMag(paintWindow->Settings()->zoom_level);
	paintWindow->Unlock();

	return paintWindow;
}


PaintWindow::~PaintWindow()
{
	if (fImageView != NULL) {
		// if we have a BEntry for this image, we should
		// write some attributes to that file
		if (fImageEntry.InitCheck() == B_OK) {
			// call a function that writes attributes to a node
			BNode a_node(&fImageEntry);
			writeAttributes(a_node);
		}
		if (fProjectEntry.InitCheck() == B_OK) {
			BNode a_node(&fProjectEntry);
			writeAttributes(a_node);
		}

		fImageView->RemoveSelf();
		delete fImageView;
	}

	delete fSettings;
	// Decrement the window-count by 1.
	sgPaintWindowCount--;

	// Remove ourselves from the sgPaintWindowList.
	sgPaintWindowList.RemoveItem(this);
}


void
PaintWindow::FrameResized(float, float)
{
	// Store the new frame to settings.
	fSettings->frame_rect = Frame();
}


void
PaintWindow::FrameMoved(BPoint)
{
	// Store the new frame to settings.
	fSettings->frame_rect = Frame();
}


void
PaintWindow::MenusBeginning()
{
	BWindow::MenusBeginning();

	BMenuItem *item = fMenubar->FindItem(_StringForId(PASTE_AS_NEW_LAYER_STRING));
	if (item != NULL) {
		be_clipboard->Lock();
		if (be_clipboard->Data()->HasMessage("image/bitmap"))
			item->SetEnabled(true);
		else
			item->SetEnabled(false);
		be_clipboard->Unlock();
	}

	item = fMenubar->FindItem(_StringForId(PASTE_AS_NEW_PROJECT_STRING));
	if (item != NULL) {
		be_clipboard->Lock();
		if (be_clipboard->Data()->HasMessage("image/bitmap"))
			item->SetEnabled(true);
		else
			item->SetEnabled(false);
		be_clipboard->Unlock();
	}

	if (fImageView != NULL) {
		item = fMenubar->FindItem(HS_CLEAR_SELECTION);
		if (fImageView->GetSelection()->IsEmpty())
			item->SetEnabled(false);
		else
			item->SetEnabled(true);

		item = fMenubar->FindItem(HS_INVERT_SELECTION);
		if (fImageView->GetSelection()->IsEmpty())
			item->SetEnabled(false);
		else
			item->SetEnabled(true);

		item = fMenubar->FindItem(HS_GROW_SELECTION);
		if (fImageView->GetSelection()->IsEmpty())
			item->SetEnabled(false);
		else
			item->SetEnabled(true);

		item = fMenubar->FindItem(HS_SHRINK_SELECTION);
		if (fImageView->GetSelection()->IsEmpty())
			item->SetEnabled(false);
		else
			item->SetEnabled(true);
	}

	if ((fImageEntry.InitCheck() == B_OK) && (fCurrentHandler != 0)) {
		BMenuItem *item = fMenubar->FindItem(HS_SAVE_IMAGE);
		if (item) item->SetEnabled(true);
	}
	else {
		BMenuItem *item = fMenubar->FindItem(HS_SAVE_IMAGE);
		if (item) item->SetEnabled(false);
	}

	if (fProjectEntry.InitCheck() == B_OK) {
		BMenuItem *item = fMenubar->FindItem(HS_SAVE_PROJECT);
		if (item) item->SetEnabled(true);
	}
	else {
		BMenuItem *item = fMenubar->FindItem(HS_SAVE_PROJECT);
		if (item) item->SetEnabled(false);
	}

	SetHelpString("",HS_TEMPORARY_HELP_MESSAGE);
}


void
PaintWindow::MenusEnded()
{
	SetHelpString(NULL, HS_TOOL_HELP_MESSAGE);
}


void
PaintWindow::MessageReceived(BMessage *message)
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
		// this comes from fMenubar->"Window"->"Resize Window to Fit" and informs us that
		// we should fit the window to display exactly the image
		case HS_RESIZE_WINDOW_TO_FIT:
			// use a private function to do resizing
			resizeToFit();
			break;

		// This comes from the recent image-size pop-up-list.
		case HS_RECENT_IMAGE_SIZE:
			fWidthNumberControl->SetValue(message->FindInt32("width"));
			fHeightNumberControl->SetValue(message->FindInt32("height"));
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
				width = atoi(fWidthNumberControl->Text());
				height = atoi(fHeightNumberControl->Text());

				try {
					// Open the image view
					OpenImageView(width,height);
					// Add a layer to it.
					fImageView->ReturnImage()->InsertLayer();
					image_created = true;
				}
				catch (std::bad_alloc) {
					image_created = false;
					delete fImageView;
					fImageView = NULL;
					BAlert *memory_alert = new BAlert("memory_alert",_StringForId(CANNOT_CREATE_IMAGE_STRING),_StringForId(OK_STRING),NULL,NULL,B_WIDTH_AS_USUAL,B_WARNING_ALERT);
					memory_alert->Go();
				}
				if (image_created == true) {
					// Record the window's frame and also put the new size to the most recently used
					// list.
					((PaintApplication*)be_app)->GlobalSettings()->default_window_settings.frame_rect = Frame();
					// Only record the new size to the list if it does not already contain the
					// selected size.
					global_settings* settings = ((PaintApplication*)be_app)->GlobalSettings();
					int32 *widths = settings->recent_image_width_list;
					int32 *heights = settings->recent_image_height_list;
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
					fContainerBox->RemoveSelf();
					delete fContainerBox;

					// NULL this because it will be used later to check if we
					// are still resizing
					fContainerBox = NULL;
					fSetSizeButton = NULL;

					// Add the view to view hierarchy.
					AddImageView();
				}
				break;
			}

	// this is the case where mouse has been pressed down in the background-view
		// this comes from backgrounview, we should then put the actual imageview
		// to follow the mouse
		case HS_MOUSE_DOWN_IN_BACKGROUNDVIEW:
			if ((fImageView != NULL) && (fImageView->Window() != NULL)) {
				fImageView->MouseDown(message->FindPoint("point"));
			}
			break;



		// this comes from fMenubar->"Window"->"Show Layer Window" and tells us to show the
		// layer window
		case HS_SHOW_LAYER_WINDOW:
			LayerWindow::showLayerWindow();
			if (fImageView != NULL) {
				LayerWindow::ActiveWindowChanged(this,fImageView->ReturnImage()->LayerList(),fImageView->ReturnImage()->ReturnThumbnailImage());
			}
			else {
				LayerWindow::ActiveWindowChanged(this);
			}
			break;

	// this shows us a view setup window
		// this comes from fMenubar->"Window"->"Window Settings…" and tells us to show the
		// window for setting the parameters of this window and it's views
		case HS_SHOW_VIEW_SETUP_WINDOW:
			ViewSetupWindow::showViewSetupWindow(this,fImageView);
			break;

		// this comes from fMenubar->"Window"->"Global Settings…"
		case HS_SHOW_GLOBAL_SETUP_WINDOW:
			GlobalSetupWindow::showGlobalSetupWindow();
			break;

	// this tells us that user wants to open a save-panel for saving the image in a new file-name
		// This comes from fMenubar->"File"->"Save Image As…" and we should show a save-panel
		case HS_SHOW_IMAGE_SAVE_PANEL:
			if (fImageSavePanel == NULL) {
				entry_ref *ref = new entry_ref();
				if (fImageEntry.InitCheck() != B_OK) {
					// This might actually fail if the user has removed the directory.
					if (path.SetTo(((PaintApplication*)be_app)->GlobalSettings()->image_save_path) != B_OK) {
						PaintApplication::HomeDirectory(path);
					}
				}
				else {
					fImageEntry.GetPath(&path);
					// get the dir that the file lives in
					path.GetParent(&path);
				}
				get_ref_for_path(path.Path(),ref);
				BMessenger window(this);
				fImageSavePanel = new ImageSavePanel(ref, &window,
					fSettings->file_type,
					fImageView->ReturnImage()->ReturnThumbnailImage());
				delete ref;
			}
			fImageSavePanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
			set_filepanel_strings(fImageSavePanel);
			if (!fImageSavePanel->IsShowing())
				fImageSavePanel->Show();
			break;

		// This comes from fMenubar->"File"->"Save Project As…"
		case HS_SHOW_PROJECT_SAVE_PANEL:
			if (fProjectSavePanel == NULL) {
				entry_ref *ref = new entry_ref();
				if (fProjectEntry.InitCheck() != B_OK) {
					path.SetTo(((PaintApplication*)be_app)->GlobalSettings()->project_save_path);
				}
				else {
					fProjectEntry.GetPath(&path);
					// get the dir that the file lives in
					path.GetParent(&path);
				}
				get_ref_for_path(path.Path(),ref);
				BMessenger window(this);
				fProjectSavePanel = new BFilePanel(B_SAVE_PANEL, &window, ref,
					0, false, new BMessage(HS_PROJECT_SAVE_REFS));
				delete ref;
			}
			fProjectSavePanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
			char string[256];
			sprintf(string,"ArtPaint: %s",_StringForId(SAVE_PROJECT_STRING));
			fProjectSavePanel->Window()->SetTitle(string);
			set_filepanel_strings(fProjectSavePanel);
			if (!fProjectSavePanel->IsShowing())
				fProjectSavePanel->Show();
			break;

		// This comes from fMenubar->"Window"->"Show Color Window". We should open the color window.
		case HS_SHOW_COLOR_WINDOW:
			ColorPaletteWindow::showPaletteWindow(false);
			break;

		// This comes from fMenubar->"Window"->"Show Tool Window". We should open the tool window.
		case HS_SHOW_TOOL_WINDOW:
			ToolSelectionWindow::showWindow();
			break;

		// This comes from fMenubar->"Window"->"Show Tool Setup Window". We should open the tool window.
		case HS_SHOW_TOOL_SETUP_WINDOW:
			ToolSetupWindow::showWindow(((PaintApplication*)be_app)->GlobalSettings()->setup_window_tool);
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
			delete fImageSavePanel;
			fImageSavePanel = NULL;
			if (fImageView != NULL) {
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
			delete fProjectSavePanel;
			fProjectSavePanel = NULL;
			if (fImageView != NULL) {
				thread_id save_thread = spawn_thread(PaintWindow::save_project,"save project",B_NORMAL_PRIORITY,(void*)this);
				resume_thread(save_thread);
				BMessage *sendable_message = new BMessage(*message);
				send_data(save_thread,0,(void*)&sendable_message,sizeof(BMessage*));
			}
			break;

		case HS_SAVE_IMAGE:
			// We make a message containing the file name and ref for its dir.
			fImageEntry.GetParent(&parent_entry);
			fImageEntry.GetName(a_name);
			parent_entry.GetPath(&a_path);
			get_ref_for_path(a_path.Path(),&a_save_ref);
			a_save_message = new BMessage(HS_IMAGE_SAVE_REFS);
			a_save_message->AddRef("directory",&a_save_ref);
			a_save_message->AddString("name",a_name);
			PostMessage(a_save_message,this);
			delete a_save_message;
			break;

		case HS_SAVE_PROJECT:
			if (fProjectEntry.InitCheck() == B_OK) {
				// We make a message containing the file name and ref for its dir.
				fProjectEntry.GetParent(&parent_entry);
				fProjectEntry.GetName(a_name);
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
				fSettings->file_type = message->FindInt32("be:type");
				fCurrentHandler = message->FindInt32("be:translator");
				DatatypeSetupWindow::ChangeHandler(fCurrentHandler);

				BTranslatorRoster *roster = BTranslatorRoster::Default();
				int32 num_formats;
				const translation_format *formats = NULL;
				if (roster->GetOutputFormats(fCurrentHandler,&formats,&num_formats) == B_OK) {
					for (int32 i=0;i<num_formats;i++) {
						if (formats[i].type == fSettings->file_type) {
							strcpy(fSettings->file_mime,formats[i].MIME);
						}
					}
				}
				break;
			}

		// This comes from image-save-panel's setting-button and tells us to show the datatype-
		// setup-window.
		case HS_SHOW_DATATYPE_SETTINGS:
			DatatypeSetupWindow::showWindow(fCurrentHandler);
			break;

		// this might come from a lot of places, but it is assumed that it comes from the
		// image save panel
		case B_CANCEL:
			// cancel might come from somewhere else than just fImageSavePanel
			// we should check that somewhow
			if ((fImageSavePanel != NULL) && (fImageSavePanel->IsShowing() == false)) {
				delete fImageSavePanel;
				fImageSavePanel = NULL;
			}
			else if ((fProjectSavePanel != NULL) && (fProjectSavePanel->IsShowing() == false)) {
				delete fProjectSavePanel;
				fProjectSavePanel = NULL;
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


bool
PaintWindow::QuitRequested()
{
	// here we should ask the user if changes to picture should be saved
	// we also tell the application that the number of paint windows
	// has decreased by one
	if (fImageView) {
		if (fImageView->Quit() == false)
			return false;

		if (fImageSavePanel != NULL)
			delete fImageSavePanel;
	}

	LayerWindow::ActiveWindowChanged(NULL);

	if (sgPaintWindowCount <= 1)
		be_app->PostMessage(B_QUIT_REQUESTED);

	return true;
}


void
PaintWindow::WindowActivated(bool active)
{
	if (active == true) {
		if (fImageView != NULL) {
			LayerWindow::ActiveWindowChanged(this,
				fImageView->ReturnImage()->LayerList(),
				fImageView->ReturnImage()->ReturnThumbnailImage());
		} else {
			LayerWindow::ActiveWindowChanged(this);
		}
	}
}


void
PaintWindow::WorkspaceActivated(int32, bool active)
{
	if (active) {
		if (fImageView != NULL) {
			if (BScreen(this).ColorSpace() == B_CMAP8) {
//				printf("B_CMAP8\n");
				fImageView->SetDisplayMode(DITHERED_8_BIT_DISPLAY_MODE);
			} else {
//				printf("not B_CMAP8\n");
				fImageView->SetDisplayMode(FULL_RGB_DISPLAY_MODE);
			}
		}
	}
}


void
PaintWindow::Zoom(BPoint leftTop, float width, float height)
{
	bool resizedToFit = true;

	if (Frame() != getPreferredSize())
		resizedToFit = false;

	if (resizedToFit) {
		MoveTo(fUserFrame.LeftTop());
		ResizeTo(fUserFrame.Width(), fUserFrame.Height());
	} else {
		resizeToFit();
	}
}


void
PaintWindow::DisplayCoordinates(BPoint point, BPoint reference, bool useReference)
{
	// here we set the proper view to display the coordinates

	// set the coords string with sprintf
//	sprintf(coords,"X: %.0f  Y: %.0f",point.x,point.y);

	fStatusView->SetCoordinates(point, reference, useReference);

	if (fSetSizeButton != NULL) {
		// if the window is in resize mode display dimensions here too
		fWidthNumberControl->SetValue(int32(point.x));
		fHeightNumberControl->SetValue(int32(point.y));
	}
}


void
PaintWindow::displayMag(float mag)
{
	fStatusView->SetMagnifyingScale(mag);
}


void
PaintWindow::SetHelpString(const char *string,int32 type)
{
	static char tool_help_string[256];
	if ((type == HS_TOOL_HELP_MESSAGE) && (string != NULL))
		strncpy(tool_help_string, string, 255);

	if (fStatusView != NULL) {
		if (string != NULL)
			fStatusView->SetHelpMessage(string);
		else if (type == HS_TOOL_HELP_MESSAGE)
			fStatusView->SetHelpMessage(tool_help_string);
	}
}


/*!
	Some of the items have the image as their target, the targets for those
	items are set in openImageView. Remember to change image-view as target
	for added items in openImageView.
*/
bool
PaintWindow::openMenuBar()
{
	fMenubar = new BMenuBar(BRect(), "menu_bar");

	BMenu* menu = new BMenu(_StringForId(FILE_STRING));
	fMenubar->AddItem(menu);

	// the File menu
	menu_item fileMenu[] = {
		{ OPEN_IMAGE_STRING, HS_SHOW_IMAGE_OPEN_PANEL, 'O', 0, be_app, OPEN_IMAGE_HELP_STRING },
		{ SAVE_IMAGE_STRING, HS_SAVE_IMAGE, 'S', 0, this, SAVE_IMAGE_HELP_STRING },
		{ SAVE_IMAGE_AS_STRING, HS_SHOW_IMAGE_SAVE_PANEL, NULL, 0, this, SAVE_IMAGE_AS_HELP_STRING },
		{ SEPARATOR, 0, NULL, 0, NULL, SEPARATOR },	// separator
		{ NEW_PROJECT_STRING, HS_NEW_PAINT_WINDOW, 'N', 0, be_app, NEW_PROJECT_HELP_STRING },
		{ OPEN_PROJECT_STRING, HS_SHOW_PROJECT_OPEN_PANEL, 'O', B_SHIFT_KEY, be_app, OPEN_PROJECT_HELP_STRING },
		{ SAVE_PROJECT_STRING, HS_SAVE_PROJECT, 'S', B_SHIFT_KEY, this, SAVE_PROJECT_HELP_STRING },
		{ SAVE_PROJECT_AS_STRING, HS_SHOW_PROJECT_SAVE_PANEL, NULL, 0, this, SAVE_PROJECT_AS_HELP_STRING },
		{ SEPARATOR, 0, NULL, 0, NULL, SEPARATOR }	// separator
	};

	for (uint32 i = 0; i < (sizeof(fileMenu) / sizeof(menu_item)); ++i) {
		_AddMenuItems(menu, fileMenu[i].label, fileMenu[i].what,
			fileMenu[i].shortcut, fileMenu[i].modifiers, fileMenu[i].target,
			fileMenu[i].help);
	}

	BMenu* subMenu = new BMenu(_StringForId(RECENT_IMAGES_STRING));
	menu->AddItem(subMenu);
	_AddRecentMenuItems(subMenu, RECENT_IMAGES_STRING);

	subMenu = new BMenu(_StringForId(RECENT_PROJECTS_STRING));
	menu->AddItem(subMenu);
	_AddRecentMenuItems(subMenu, RECENT_PROJECTS_STRING);

	menu_item fileMenu2[] = {
		{ SEPARATOR, 0, NULL, 0, NULL, SEPARATOR },	// separator
		{ CLOSE_STRING, B_QUIT_REQUESTED, 'W', 0, this, CLOSE_HELP_STRING },
		{ QUIT_STRING, B_QUIT_REQUESTED, 'Q', 0, be_app, QUIT_HELP_STRING }
	};

	for (uint32 i = 0; i < (sizeof(fileMenu2) / sizeof(menu_item)); ++i) {
		_AddMenuItems(menu, fileMenu2[i].label, fileMenu2[i].what,
			fileMenu2[i].shortcut, fileMenu2[i].modifiers, fileMenu2[i].target,
			fileMenu2[i].help);
	}

	// menu->AddItem(new BMenuItem("Save Image Into Resources",
	//	new BMessage(HS_SAVE_IMAGE_INTO_RESOURCES)));
	// menu->AddItem(new BMenuItem("Save Layer As Cursor",
	// new BMessage(HS_SAVE_IMAGE_AS_CURSOR)));

	// the Edit menu
	menu = new BMenu(_StringForId(EDIT_STRING));
	fMenubar->AddItem(menu);

	menu_item editMenu[] = {
		{ UNDO_NOT_AVAILABLE_STRING, HS_UNDO,'Z', 0, this, UNDO_HELP_STRING },
		{ REDO_NOT_AVAILABLE_STRING, HS_REDO,'Z', B_SHIFT_KEY, this, REDO_HELP_STRING },
		{ SEPARATOR, 0, NULL, 0, NULL, SEPARATOR }	// separator
	};

	for (uint32 i = 0; i < (sizeof(editMenu) / sizeof(menu_item)); ++i) {
		_AddMenuItems(menu, editMenu[i].label, editMenu[i].what,
			editMenu[i].shortcut, editMenu[i].modifiers, editMenu[i].target,
			editMenu[i].help);
	}

	subMenu = new BMenu(_StringForId(CUT_STRING));
	menu->AddItem(subMenu);

	BMessage *a_message = new BMessage(B_CUT);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	subMenu->AddItem(new PaintWindowMenuItem(_StringForId(ACTIVE_LAYER_STRING),a_message,'X',0,this,_StringForId(LAYER_CUT_HELP_STRING)));

	a_message = new BMessage(B_CUT);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	subMenu->AddItem(new PaintWindowMenuItem(_StringForId(ALL_LAYERS_STRING),a_message,'X',B_SHIFT_KEY,this,_StringForId(ALL_LAYERS_CUT_HELP_STRING)));

	subMenu = new BMenu(_StringForId(COPY_STRING));
	menu->AddItem(subMenu);
	a_message = new BMessage(B_COPY);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	subMenu->AddItem(new PaintWindowMenuItem(_StringForId(ACTIVE_LAYER_STRING),a_message,'C',0,this,_StringForId(LAYER_COPY_HELP_STRING)));

	a_message = new BMessage(B_COPY);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	subMenu->AddItem(new PaintWindowMenuItem(_StringForId(ALL_LAYERS_STRING),a_message,'C',B_SHIFT_KEY,this,_StringForId(ALL_LAYERS_COPY_HELP_STRING)));
	menu->AddItem(new PaintWindowMenuItem(_StringForId(PASTE_AS_NEW_LAYER_STRING),new BMessage(B_PASTE),'V',0,this,_StringForId(PASTE_AS_NEW_LAYER_HELP_STRING)));
	menu->AddItem(new PaintWindowMenuItem(_StringForId(PASTE_AS_NEW_PROJECT_STRING),new BMessage(B_PASTE),'V',B_SHIFT_KEY,this,_StringForId(PASTE_AS_NEW_PROJECT_HELP_STRING)));

	menu->AddItem(new BSeparatorItem());

	menu->AddItem(new PaintWindowMenuItem(_StringForId(GROW_SELECTION_STRING),new BMessage(HS_GROW_SELECTION),'G',0,this,_StringForId(GROW_SELECTION_HELP_STRING)));
	menu->AddItem(new PaintWindowMenuItem(_StringForId(SHRINK_SELECTION_STRING),new BMessage(HS_SHRINK_SELECTION),'H',0,this,_StringForId(SHRINK_SELECTION_HELP_STRING)));
	menu->AddItem(new PaintWindowMenuItem(_StringForId(INVERT_SELECTION_STRING), new BMessage(HS_INVERT_SELECTION),(char)NULL,0,this,_StringForId(INVERT_SELECTION_HELP_STRING)));
	menu->AddItem(new PaintWindowMenuItem(_StringForId(CLEAR_SELECTION_STRING), new BMessage(HS_CLEAR_SELECTION),'D',0,this,_StringForId(CLEAR_SELECTION_HELP_STRING)));

	// The Layer menu.
	menu = new BMenu(_StringForId(LAYER_STRING));
	fMenubar->AddItem(menu);

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",ROTATION_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	menu->AddItem(new PaintWindowMenuItem(_StringForId(ROTATE_STRING),a_message,'R',0,this,_StringForId(ROTATE_LAYER_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",TRANSLATION_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	menu->AddItem(new PaintWindowMenuItem(_StringForId(TRANSLATE_STRING),a_message,'T',0,this,_StringForId(TRANSLATE_LAYER_HELP_STRING)));

//	a_message = new BMessage(HS_START_MANIPULATOR);
//	a_message->AddInt32("manipulator_type",FREE_TRANSFORM_MANIPULATOR);
//	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
//	menu->AddItem(new PaintWindowMenuItem("Free transform test",a_message,(char)NULL,0,this,"Use left shift and control to rotate and scale."));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",HORIZ_FLIP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	menu->AddItem(new PaintWindowMenuItem(_StringForId(FLIP_HORIZONTAL_STRING),a_message,B_LEFT_ARROW,0,this,_StringForId(FLIP_HORIZONTAL_LAYER_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",VERT_FLIP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	menu->AddItem(new PaintWindowMenuItem(_StringForId(FLIP_VERTICAL_STRING),a_message,B_UP_ARROW,0,this,_StringForId(FLIP_VERTICAL_LAYER_HELP_STRING)));

	menu->AddSeparatorItem();

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",TRANSPARENCY_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	menu->AddItem(new PaintWindowMenuItem(_StringForId(CHANGE_TRANSPARENCY_STRING),a_message,(char)NULL,0,this,_StringForId(CHANGE_TRANSPARENCY_HELP_STRING)));
	menu->AddItem(new PaintWindowMenuItem(_StringForId(CLEAR_LAYER_STRING),new BMessage(HS_CLEAR_LAYER),(char)NULL,0,this,_StringForId(CLEAR_LAYER_HELP_STRING)));

	menu->AddSeparatorItem();

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",TEXT_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	menu->AddItem(new PaintWindowMenuItem(_StringForId(INSERT_TEXT_STRING),a_message,'I',0,this,_StringForId(INSERT_TEXT_HELP_STRING)));
	menu->AddSeparatorItem();
	menu->AddItem(new PaintWindowMenuItem(_StringForId(SHOW_LAYER_WINDOW_STRING),new BMessage(HS_SHOW_LAYER_WINDOW),'L',0,this,_StringForId(SHOW_LAYER_WINDOW_HELP_STRING)));
	menu->AddItem(new BSeparatorItem());
	menu->AddItem(new PaintWindowMenuItem(_StringForId(ADD_LAYER_STRING),new BMessage(HS_ADD_LAYER_FRONT),'.',0,this,_StringForId(ADD_LAYER_HELP_STRING)));

	// The Canvas menu.
	menu = new BMenu(_StringForId(CANVAS_STRING));
	fMenubar->AddItem(menu);

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",ROTATION_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(_StringForId(ROTATE_STRING),a_message,'R',B_SHIFT_KEY,this,_StringForId(ROTATE_ALL_LAYERS_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",ROTATE_CW_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(_StringForId(ROTATE_CW_STRING),a_message,0,0,this,_StringForId(ROTATE_ALL_LAYERS_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",ROTATE_CCW_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(_StringForId(ROTATE_CCW_STRING),a_message,0,0,this,_StringForId(ROTATE_ALL_LAYERS_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",TRANSLATION_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(_StringForId(TRANSLATE_STRING),a_message,'T',B_SHIFT_KEY,this,_StringForId(TRANSLATE_ALL_LAYERS_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",HORIZ_FLIP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(_StringForId(FLIP_HORIZONTAL_STRING),a_message,B_LEFT_ARROW,B_SHIFT_KEY,this,_StringForId(FLIP_HORIZONTAL_ALL_LAYERS_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",VERT_FLIP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(_StringForId(FLIP_VERTICAL_STRING),a_message,B_UP_ARROW,B_SHIFT_KEY,this,_StringForId(FLIP_VERTICAL_ALL_LAYERS_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",CROP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(_StringForId(CROP_STRING),a_message,'C',B_CONTROL_KEY,this,_StringForId(CROP_HELP_STRING)));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",SCALE_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(_StringForId(SCALE_STRING),a_message,'S',B_CONTROL_KEY,this,_StringForId(SCALE_ALL_LAYERS_HELP_STRING)));

	menu->AddItem(new BSeparatorItem());
	menu->AddItem(new PaintWindowMenuItem(_StringForId(CLEAR_CANVAS_STRING), new BMessage(HS_CLEAR_CANVAS),(char)NULL,0,this,_StringForId(CLEAR_CANVAS_HELP_STRING)));


	// The Window menu,
	menu = new BMenu(_StringForId(WINDOW_STRING));
	fMenubar->AddItem(menu);

	// use + and - as shorcuts for zooming
	menu->AddItem(new PaintWindowMenuItem(_StringForId(ZOOM_IN_STRING), new BMessage(HS_ZOOM_IMAGE_IN),'+',0,this,_StringForId(ZOOM_IN_HELP_STRING)));
	menu->AddItem(new PaintWindowMenuItem(_StringForId(ZOOM_OUT_STRING), new BMessage(HS_ZOOM_IMAGE_OUT),'-',0,this,_StringForId(ZOOM_OUT_HELP_STRING)));

	subMenu = new BMenu(_StringForId(SET_ZOOM_LEVEL_STRING));
	menu->AddItem(subMenu);
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",0.25);
	subMenu->AddItem(new PaintWindowMenuItem("25%",a_message,(char)NULL,0,this,_StringForId(ZOOM_LEVEL_25_HELP_STRING)));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",0.50);
	subMenu->AddItem(new PaintWindowMenuItem("50%",a_message,(char)NULL,0,this,_StringForId(ZOOM_LEVEL_50_HELP_STRING)));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",1.0);
	subMenu->AddItem(new PaintWindowMenuItem("100%",a_message,(char)NULL,0,this,_StringForId(ZOOM_LEVEL_100_HELP_STRING)));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",2.0);
	subMenu->AddItem(new PaintWindowMenuItem("200%",a_message,(char)NULL,0,this,_StringForId(ZOOM_LEVEL_200_HELP_STRING)));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",4.0);
	subMenu->AddItem(new PaintWindowMenuItem("400%",a_message,(char)NULL,0,this,_StringForId(ZOOM_LEVEL_400_HELP_STRING)));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",8.0);
	subMenu->AddItem(new PaintWindowMenuItem("800%",a_message,(char)NULL,0,this,_StringForId(ZOOM_LEVEL_800_HELP_STRING)));

	subMenu = new BMenu(_StringForId(SET_GRID_STRING));
	menu->AddItem(subMenu);
	a_message = new BMessage(HS_GRID_ADJUSTED);
	a_message->AddPoint("origin",BPoint(0,0));
	a_message->AddInt32("unit",1);
	subMenu->AddItem(new PaintWindowMenuItem(_StringForId(OFF_STRING),a_message,(char)NULL,0,this,_StringForId(GRID_OFF_HELP_STRING)));
	a_message = new BMessage(HS_GRID_ADJUSTED);
	a_message->AddPoint("origin",BPoint(0,0));
	a_message->AddInt32("unit",2);
	subMenu->AddItem(new PaintWindowMenuItem("2x2",a_message,(char)NULL,0,this,_StringForId(GRID_2_BY_2_HELP_STRING)));
	a_message = new BMessage(HS_GRID_ADJUSTED);
	a_message->AddPoint("origin",BPoint(0,0));
	a_message->AddInt32("unit",4);
	subMenu->AddItem(new PaintWindowMenuItem("4x4",a_message,(char)NULL,0,this,_StringForId(GRID_4_BY_4_HELP_STRING)));
	a_message = new BMessage(HS_GRID_ADJUSTED);
	a_message->AddPoint("origin",BPoint(0,0));
	a_message->AddInt32("unit",8);
	subMenu->AddItem(new PaintWindowMenuItem("8x8",a_message,(char)NULL,0,this,_StringForId(GRID_8_BY_8_HELP_STRING)));
	subMenu->SetRadioMode(true);
	subMenu->ItemAt(0)->SetMarked(true);

	// use here the same shortcut as the tracker uses == Y
	menu->AddItem(new PaintWindowMenuItem(_StringForId(RESIZE_TO_FIT_STRING),new BMessage(HS_RESIZE_WINDOW_TO_FIT),'Y',0,this,_StringForId(RESIZE_TO_FIT_HELP_STRING)));
	menu->AddSeparatorItem();
	menu->AddItem(new PaintWindowMenuItem(_StringForId(SHOW_PALETTE_WINDOW_STRING),new BMessage(HS_SHOW_COLOR_WINDOW),'P',0,this,_StringForId(SHOW_PALETTE_WINDOW_HELP_STRING)));
	menu->AddItem(new PaintWindowMenuItem(_StringForId(SHOW_LAYER_WINDOW_STRING),new BMessage(HS_SHOW_LAYER_WINDOW),'L',0,this,_StringForId(SHOW_LAYER_WINDOW_HELP_STRING)));
	menu->AddItem(new PaintWindowMenuItem(_StringForId(SHOW_TOOL_WINDOW_STRING),new BMessage(HS_SHOW_TOOL_WINDOW),'K',0,this,_StringForId(SHOW_TOOL_WINDOW_HELP_STRING)));
	menu->AddItem(new PaintWindowMenuItem(_StringForId(SHOW_TOOL_SETUP_WINDOW_STRING), new BMessage(HS_SHOW_TOOL_SETUP_WINDOW),'M',0,this,_StringForId(SHOW_TOOL_SETUP_WINDOW_HELP_STRING)));
	menu->AddItem(new PaintWindowMenuItem(_StringForId(SHOW_BRUSH_WINDOW_STRING), new BMessage(HS_SHOW_BRUSH_STORE_WINDOW),'B',0,this,_StringForId(SHOW_BRUSH_WINDOW_HELP_STRING)));
	menu->AddSeparatorItem();
//	menu->AddItem(new PaintWindowMenuItem(_StringForId(NEW_PAINT_WINDOW_STRING),new BMessage(HS_NEW_PAINT_WINDOW),'N',0,this,_StringForId(NEW_PROJECT_HELP_STRING)));
//	menu->AddSeparatorItem();
//	menu->AddItem(new BMenuItem("Window settings…", new BMessage(HS_SHOW_VIEW_SETUP_WINDOW)));
	menu->AddItem(new PaintWindowMenuItem(_StringForId(SETTINGS_STRING), new BMessage(HS_SHOW_GLOBAL_SETUP_WINDOW),(char)NULL,0,this,_StringForId(SETTINGS_HELP_STRING)));

	// This will be only temporary place for add-ons. Later they will be spread
	// in the menu hierarchy according to their types.
	menu = new BMenu(_StringForId(ADD_ONS_STRING));
	thread_id add_on_adder_thread = spawn_thread(_AddAddOnsToMenu,
		"add_on_adder_thread", B_NORMAL_PRIORITY, this);
	resume_thread(add_on_adder_thread);
	fMenubar->AddItem(menu);

	// help
	menu = new BMenu(_StringForId(HELP_STRING));
	fMenubar->AddItem(menu);

	BMessage* message =  new BMessage(HS_SHOW_USER_DOCUMENTATION);
	message->AddString("document", "index.html");

	BMenuItem* item = new PaintWindowMenuItem(_StringForId(USER_MANUAL_STRING),
		message, NULL, 0, this, _StringForId(USER_MANUAL_HELP_STRING));
	item->SetTarget(be_app);
	menu->AddItem(item);

	message = new BMessage(HS_SHOW_USER_DOCUMENTATION);
	message->AddString("document", "shortcuts.html");
	item = new PaintWindowMenuItem(_StringForId(SHORTCUTS_STRING), message,
		NULL, 0, this, _StringForId(SHORTCUTS_HELP_STRING));
	item->SetTarget(be_app);
	menu->AddItem(item);

	menu->AddSeparatorItem();

	message = new BMessage(HS_SHOW_ABOUT_WINDOW);
	item = new PaintWindowMenuItem(_StringForId(ABOUT_ARTPAINT_STRING),
		message, NULL, 0, this, _StringForId(ABOUT_HELP_STRING));
	menu->AddItem(item);

	_ChangeMenuMode(NO_IMAGE_MENU);

	AddChild(fMenubar);

	return true;
}


int32
PaintWindow::_AddAddOnsToMenu(void* data)
{
	PaintWindow* paintWindow = static_cast<PaintWindow*>(data);
	if (!paintWindow)
		return B_ERROR;

	while ((ManipulatorServer::AddOnsAvailable() == false)
		|| (paintWindow->KeyMenuBar() == NULL)) {
		snooze(50000);
	}

	BMenu* addOnMenu = NULL;
	if (paintWindow->KeyMenuBar() != NULL) {
		BMenuBar* menuBar = paintWindow->KeyMenuBar();
		BMenuItem* item = menuBar->FindItem(_StringForId(ADD_ONS_STRING));
		addOnMenu = item->Submenu();
	}

	if (addOnMenu) {
		if (paintWindow->Lock()) {
			BMessenger target(paintWindow);
			image_id* addon_array = ManipulatorServer::AddOnArray();
			for (int32 i = 0; i < ManipulatorServer::AddOnCount(); ++i) {
				image_id imageId = addon_array[i];

				char* add_on_name;
				status_t errors = get_image_symbol(imageId, "name",
						B_SYMBOL_TYPE_DATA, (void**)&add_on_name);

				int32* add_on_version;
				errors |= get_image_symbol(imageId, "add_on_api_version",
						B_SYMBOL_TYPE_DATA, (void**)&add_on_version);

				const char* add_on_help;
				if (get_image_symbol(imageId, "menu_help_string",
						B_SYMBOL_TYPE_DATA, (void**)&add_on_help) != B_OK) {
					add_on_help = "Starts an add-on effect.";
				}

				if (*add_on_version == ADD_ON_API_VERSION && errors == B_OK) {
					BMessage* message = new BMessage(HS_START_MANIPULATOR);
					message->AddInt32("add_on_id", imageId);
					message->AddInt32("layers", HS_MANIPULATE_CURRENT_LAYER);
					message->AddInt32("manipulator_type", ADD_ON_MANIPULATOR);

					addOnMenu->AddItem(new PaintWindowMenuItem(add_on_name,
						message, (char)NULL, 0, paintWindow, add_on_help));
				}
			}
			addOnMenu->SetTargetForItems(target);
			paintWindow->Unlock();
		}
	}

	if (paintWindow->fImageView != NULL)
		addOnMenu->SetTargetForItems(paintWindow->fImageView);

	return B_OK;
}


status_t
PaintWindow::OpenImageView(int32 width, int32 height)
{
	// The image-view should be at most as large as backgroundview
	// but not larger than image width and height.
	Lock();	// Lock while we take background's bounds.
	fImageView = new ImageView(fBackground->Bounds(),width,height);
	Unlock();

	// return true because we were successfull
	return true;
}


status_t
PaintWindow::AddImageView()
{
	// We have to lock as this functionmight be
	// called from outside the window's thread.
	Lock();

	// put the view as target for scrollbars
	fHorizontalScrollbar->SetTarget(fImageView);
	fVerticalScrollbar->SetTarget(fImageView);
	// Change the regular help-view's message.
//	BMessage *help_message = new BMessage(HS_REGULAR_HELP_MESSAGE);
//	help_message->AddString("message",HS_DRAW_MODE_HELP_MESSAGE);
//	PostMessage(help_message,this);
//	delete help_message;

	// Change the menu-mode to enable all items.
	_ChangeMenuMode(FULL_MENU);

	// Add the view to window's view hierarchy.
	fBackground->AddChild(fImageView);

	// Adjust image's position and size.
	fImageView->adjustSize();
	fImageView->adjustPosition();
	// Update the image's scroll-bars.
	fImageView->adjustScrollBars();

	// Change image for target for certain menu-items. These cannot be changed
	// before image is added as a child to this window.
	fMenubar->FindItem(_StringForId(EDIT_STRING))->Submenu()->SetTargetForItems(fImageView);
	fMenubar->FindItem(_StringForId(PASTE_AS_NEW_PROJECT_STRING))->SetTarget(be_app);

	BMenu *menu = fMenubar->FindItem(_StringForId(EDIT_STRING))->Submenu();
	if (menu != NULL) {
		BMenu *sub_menu;
		for (int32 i=0;i<menu->CountItems();i++) {
			sub_menu = menu->SubmenuAt(i);
			if (sub_menu != NULL)
				sub_menu->SetTargetForItems(fImageView);
		}
	}

	fMenubar->FindItem(_StringForId(ADD_ONS_STRING))->Submenu()->SetTargetForItems(fImageView);
	fMenubar->FindItem(HS_ADD_LAYER_FRONT)->SetTarget(fImageView);
	fMenubar->FindItem(_StringForId(CROP_STRING))->SetTarget(fImageView);
	fMenubar->FindItem(HS_CLEAR_CANVAS)->SetTarget(fImageView);
	fMenubar->FindItem(HS_CLEAR_LAYER)->SetTarget(fImageView);
	fMenubar->FindItem(HS_ZOOM_IMAGE_IN)->SetTarget(fImageView);
	fMenubar->FindItem(HS_ZOOM_IMAGE_OUT)->SetTarget(fImageView);
	fMenubar->FindItem(_StringForId(SET_ZOOM_LEVEL_STRING))->Submenu()->SetTargetForItems(fImageView);
	fMenubar->FindItem(_StringForId(SET_GRID_STRING))->Submenu()->SetTargetForItems(fImageView);
	HSStack<BMenu*> menu_stack(100);

	for (int32 i=0;i<fMenubar->CountItems();i++) {
		if (fMenubar->ItemAt(i)->Submenu() != NULL)
			menu_stack.push(fMenubar->ItemAt(i)->Submenu());
	}

	// Change the image as target for all menu-items that have HS_START_MANIPULATOR
	// as their message's what constant.
	menu = menu_stack.pop();
	while (menu != NULL) {
		for (int32 i=0;i<menu->CountItems();i++) {
			if (menu->ItemAt(i)->Command() == HS_START_MANIPULATOR) {
				menu->ItemAt(i)->SetTarget(fImageView);
			}
			if (menu->ItemAt(i)->Submenu() != NULL) {
				menu_stack.push(menu->ItemAt(i)->Submenu());
			}
		}
		menu = menu_stack.pop();
	}

	// This allows Alt-+ next to the backspace key to work (the menu item shortcut only works
	// with the + key on the numeric keypad)
	AddShortcut('=', B_COMMAND_KEY, new BMessage(HS_ZOOM_IMAGE_IN), fImageView);

	// change the name of the image and the project in the image-view
	if (fProjectEntry.InitCheck() == B_OK) {
		char name[B_PATH_NAME_LENGTH];
		fProjectEntry.GetName(name);
		fImageView->SetProjectName(name);
	}
	else {
		char title[256];
		sprintf(title,"%s - %ld",_StringForId(UNTITLED_STRING),
			sgUntitledWindowNumber++);
		fImageView->SetProjectName(title);
	}
	if (fImageEntry.InitCheck() == B_OK) {
		char name[B_PATH_NAME_LENGTH];
		fImageEntry.GetName(name);
		fImageView->SetImageName(name);
	}
	fStatusView->DisplayToolsAndColors();
	// Finally unlock the window.
	Unlock();

	// We can update ourselves to the layer-window.
	LayerWindow::ActiveWindowChanged(this,fImageView->ReturnImage()->LayerList(),fImageView->ReturnImage()->ReturnThumbnailImage());

	return B_OK;
}


void
PaintWindow::resizeToFit()
{
	// here we check if the window fits to screen when resized
	// if not then we will make the window as big as will fit on screen

	// we will get screens dimensions and then use min() function to decide
	// what will be new window dimensions


	// Store the user-frame
	if (Frame() != getPreferredSize())
		fUserFrame = Frame();

	// here we should also take magify_scale into account
	BRect screen_bounds;
	{
		screen_bounds = BScreen(this).Frame();
	}
	screen_bounds.OffsetTo(0,0);

	if (!fImageView) {
		screen_bounds.InsetBy(7.5, 7.5);
		ResizeTo(screen_bounds.right, screen_bounds.bottom - 20.0);
		MoveTo(screen_bounds.LeftTop() + BPoint(-5.0, 15.0));
		return;
	}

	// we have to subtract a little from screen dimensions to leave some room
	// around window
	float width = min_c(screen_bounds.Width() - 30,
		fImageView->getMagScale() * fImageView->ReturnImage()->Width() +
		fAdditionalWidth);
	float height = min_c(screen_bounds.Height() - 30,
		fImageView->getMagScale()*fImageView->ReturnImage()->Height() +
		fAdditionalHeight);
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


BRect
PaintWindow::getPreferredSize()
{
	// here we should also take magify_scale into account
	BRect screen_bounds;
	{
		screen_bounds = BScreen(this).Frame();
	}
	screen_bounds.OffsetTo(0,0);

	if (!fImageView)
		return screen_bounds;

	// we have to subtract a little from screen dimensions to leave some room
	// around window
	float width = min_c(screen_bounds.Width() - 30.0,
		fImageView->getMagScale() * fImageView->ReturnImage()->Width() +
		fAdditionalWidth);
	float height = min_c(screen_bounds.Height() - 30.0,
		fImageView->getMagScale() * fImageView->ReturnImage()->Height() +
		fAdditionalHeight);
	width = ceil(width);
	height = ceil(height);

	BPoint left_top;
	left_top.x = min_c(max_c(5,Frame().LeftTop().x),screen_bounds.Width()-width);
	left_top.y = min_c(max_c(5,Frame().LeftTop().y),screen_bounds.Height()-height);

	left_top.x = floor(left_top.x);
	left_top.y = floor(left_top.y);

	return BRect(left_top.x, left_top.y, left_top.x + width, left_top.y + height);
}


int32
PaintWindow::save_image(void *data)
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


status_t
PaintWindow::saveImage(BMessage *message)
{
	if (fImageView->Freeze() == B_OK) {
		entry_ref ref;
		message->FindRef("directory",&ref);
		BDirectory directory = BDirectory(&ref);
		status_t err;

		// store the entry-ref
		err = fImageEntry.SetTo(&directory,message->FindString("name"),true);

		// Only one save ref is received so we do not need to loop.
		BFile file;
		if ((err = file.SetTo(&directory,message->FindString("name"),B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE)) != B_OK) {
			fImageView->UnFreeze();
			return err;
		}

		// get the applications signature
		app_info info;
		be_app->GetAppInfo(&info);

		// Create a BNodeInfo for this file and set the MIME-type and preferred app.
		BNodeInfo *nodeinfo = new BNodeInfo(&file);
		nodeinfo->SetType(fSettings->file_mime);
//		nodeinfo->SetPreferredApp(info.signature);
		delete nodeinfo;

		// here we should save some attributes with the file
		BNode *node = new BNode(&directory,message->FindString("name"));
		writeAttributes(*node);

		// here translate the data using a BitmapStream-object
		BBitmap *picture = fImageView->ReturnImage()->ReturnRenderedImage();

		printf("Picture at 0,0: 0x%8lx\n",*((uint32*)(picture->Bits())));

		BBitmapStream *image_stream = new BBitmapStream(picture);
		BTranslatorRoster *roster = BTranslatorRoster::Default();

		err = roster->Translate(image_stream,(const translator_info*)NULL,(BMessage*)NULL,&file,fSettings->file_type,B_TRANSLATOR_BITMAP);
		fImageView->UnFreeze();
		if (err == B_OK) {
			char title[B_FILE_NAME_LENGTH];
			fImageEntry.GetName(title);
			Lock();
			fImageView->ResetChangeStatistics(false,true);
			fImageView->SetImageName(title);
//			BMenuItem *item = fMenubar->FindItem(HS_SAVE_IMAGE);
//			if (item) item->SetEnabled(true);
			Unlock();
			// Also change this new path into the settings.

			BPath path;
			fImageEntry.GetPath(&path);
			((PaintApplication*)be_app)->GlobalSettings()->insert_recent_image_path(path.Path());
			path.GetParent(&path);

			if (path.Path() != NULL) {
				strcpy(((PaintApplication*)be_app)->GlobalSettings()->image_save_path,path.Path());
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


status_t
PaintWindow::saveImageIntoResources()
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
	BBitmap *buffer = fImageView->ReturnImage()->ReturnRenderedImage();
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


status_t
PaintWindow::saveImageAsCursor()
{
	BFile cursor_file;
	if (cursor_file.SetTo("/boot/home/Cursor.h",B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE) != B_OK) {
		return B_ERROR;
	}
	BBitmap *buffer = fImageView->ReturnImage()->ReturnActiveBitmap();

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


int32
PaintWindow::save_project(void *data)
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


status_t
PaintWindow::saveProject(BMessage *message)
{
	if (fImageView->Freeze() == B_OK) {
		// We open the file that the ref and the name of message tells us.
		entry_ref ref;
		message->FindRef("directory",&ref);
		BDirectory directory = BDirectory(&ref);
		status_t err;

		// store the entry-ref
		err = fProjectEntry.SetTo(&directory,message->FindString("name"),true);

		// Only one save ref is received so we do not need to loop.
		BFile file;
		if ((err = file.SetTo(&directory,message->FindString("name"),B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE)) != B_OK) {
			fImageView->UnFreeze();
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
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// Write the file-id.
		int32 id = PROJECT_FILE_ID;
		if (file.Write(&id,sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// Write the number of sections that the file contains. Currently there are
		// only two sections.
		int32 section_count = 2;
		if (file.Write(&section_count,sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// First write the dimensions section. The real dimensions of the image are written to the file.
		int32 marker = PROJECT_FILE_SECTION_START;
		if (file.Write(&marker,sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// Here write the section type.
		int32 type = PROJECT_FILE_DIMENSION_SECTION_ID;
		if (file.Write(&type,sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// Leave room for the section-length
		file.Seek(sizeof(int64),SEEK_CUR);

		// Here write the width and height.
		int32 width = (int32)fImageView->ReturnImage()->Width();
		int32 height = (int32)fImageView->ReturnImage()->Height();
		int64 written_bytes = 0;
		written_bytes = file.Write(&width,sizeof(int32));
		written_bytes += file.Write(&height,sizeof(int32));

		if (written_bytes != 2*sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		file.Seek(-written_bytes-sizeof(int64),SEEK_CUR);
		if (file.Write(&written_bytes,sizeof(int64)) != sizeof(int64)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}
		file.Seek(written_bytes,SEEK_CUR);

		marker = PROJECT_FILE_SECTION_END;
		if (file.Write(&marker,sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// Here starts the layer-section.
		marker = PROJECT_FILE_SECTION_START;
		if (file.Write(&marker,sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}
		id = PROJECT_FILE_LAYER_SECTION_ID;
		if (file.Write(&id,sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// Leave some room for the length.
		file.Seek(sizeof(int64),SEEK_CUR);

		// Here tell the image to write layers.
		written_bytes = fImageView->ReturnImage()->WriteLayers(file);

		file.Seek(-written_bytes-sizeof(int64),SEEK_CUR);
		if (file.Write(&written_bytes,sizeof(int64)) != sizeof(int64)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}
		file.Seek(written_bytes,SEEK_CUR);

		// Write the end-marker for the layer-section.
		marker = PROJECT_FILE_SECTION_END;
		if (file.Write(&marker,sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}


		// Now we are happily at the end of writing.
		fImageView->ResetChangeStatistics(true,false);
		fImageView->UnFreeze();
		char title[256];
		fProjectEntry.GetName(title);
		Lock();
		fImageView->SetProjectName(title);

		// This must come after the project's name has been set.
		LayerWindow::ActiveWindowChanged(this,fImageView->ReturnImage()->LayerList(),fImageView->ReturnImage()->ReturnThumbnailImage());
//		BMenuItem *item = fMenubar->FindItem(HS_SAVE_PROJECT);
//		if (item) item->SetEnabled(true);
		Unlock();
		// Also change this new path into the settings.
		BPath path;
		fProjectEntry.GetPath(&path);
		((PaintApplication*)be_app)->GlobalSettings()->insert_recent_project_path(path.Path());
		path.GetParent(&path);

		if (path.Path() != NULL) {
			strcpy(((PaintApplication*)be_app)->GlobalSettings()->project_save_path,path.Path());
		}
		return B_OK;
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
	if (fImageView && fImageView->LockLooper()) {
		fSettings->zoom_level = fImageView->getMagScale();
		node.WriteAttr("ArtP:zoom_level", B_FLOAT_TYPE, 0,
			&(fSettings->zoom_level), sizeof(float));

		fSettings->view_position = fImageView->LeftTop();
		node.WriteAttr("ArtP:view_position", B_POINT_TYPE, 0,
			&(fSettings->view_position), sizeof(BPoint));
		fImageView->UnlockLooper();
	}
}


void
PaintWindow::ReadAttributes(const BNode& node)
{
	if (Lock()) {
		float zoom_level;
		if (node.ReadAttr("ArtP:zoom_level", B_FLOAT_TYPE, 0, &zoom_level,
			sizeof(float)) > 0) {
			fSettings->zoom_level = zoom_level;
			if (fImageView)
				fImageView->setMagScale(zoom_level);
		}

		BPoint view_position;
		if (node.ReadAttr("ArtP:view_position" ,B_POINT_TYPE, 0, &view_position,
			sizeof(BPoint)) > 0) {
			fSettings->view_position = view_position;
			if (fImageView)
				fImageView->ScrollTo(view_position);
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


void
PaintWindow::_AddMenuItems(BMenu* menu, string_id label, uint32 what,
	char shortcut, uint32 modifiers, BHandler* target, string_id help)
{
	if (label != SEPARATOR && help != SEPARATOR) {
		BMenuItem* item = new PaintWindowMenuItem(_StringForId(label),
			new BMessage(what), shortcut, modifiers, this, _StringForId(help));
		menu->AddItem(item);
		item->SetTarget(target);
	} else {
		menu->AddSeparatorItem();
	}
}


void
PaintWindow::_AddRecentMenuItems(BMenu* menu, string_id id)
{
	BPath path;
	global_settings* settings = ((PaintApplication*)be_app)->GlobalSettings();
	for (int32 i = 0; i < RECENT_LIST_LENGTH; ++i) {
		if (id == RECENT_IMAGES_STRING)
			path.SetTo(settings->recent_image_paths[i], NULL, true);
		else if (id == RECENT_PROJECTS_STRING)
			path.SetTo(settings->recent_project_paths[i], NULL, true);

		entry_ref ref;
		if (get_ref_for_path(path.Path(), &ref) == B_OK) {
			BMessage* message = new BMessage(B_REFS_RECEIVED);
			message->AddRef("refs", &ref);

			PaintWindowMenuItem* item = new PaintWindowMenuItem(path.Leaf(),
				message, NULL, 0, this, path.Path());
			menu->AddItem(item);
			item->SetTarget(be_app);
		}
	}
}


const char*
PaintWindow::_StringForId(string_id stringId)
{
	return StringServer::ReturnString(stringId);
}


/*!
	This function changes the enability of some menu-items. The
	parameter is used as a guide to what should be enabled and not.
*/
void
PaintWindow::_ChangeMenuMode(menu_modes newMode)
{
	int32 count = fMenubar->CountItems();
	for (int32 i = 0; i < count; ++i) {
		if (BMenuItem* currentItem = fMenubar->ItemAt(i)) {
			currentItem->SetEnabled(true);
			if (BMenu* subMenu = currentItem->Submenu())
				_EnableMenuItems(subMenu);
		}
	}

	switch (newMode) {
		case NO_IMAGE_MENU: {
			// In this case we disable some additional items and then let fall
			// through to the next case that disables most of the other items.
			_DisableMenuItem(_StringForId(RESIZE_TO_FIT_STRING));
			_DisableMenuItem(_StringForId(ZOOM_IN_STRING));
			_DisableMenuItem(_StringForId(ZOOM_OUT_STRING));
			_DisableMenuItem(_StringForId(SET_ZOOM_LEVEL_STRING));
			_DisableMenuItem(_StringForId(SET_GRID_STRING));
		}	// fall through

		case MINIMAL_MENU: {
			// In this case most of the items should be disabled.
			_DisableMenuItem(_StringForId(CANVAS_STRING));
			_DisableMenuItem(_StringForId(LAYER_STRING));
			_DisableMenuItem(_StringForId(SAVE_IMAGE_AS_STRING));
			_DisableMenuItem(_StringForId(SAVE_PROJECT_AS_STRING));
			_DisableMenuItem(_StringForId(EDIT_STRING));
			_DisableMenuItem(_StringForId(ADD_ONS_STRING));
		}	// fall through

		case FULL_MENU: {
			// In this case we should not disable any menu-items.
			if ((newMode == FULL_MENU && fImageEntry.InitCheck() != B_OK)
				|| newMode != FULL_MENU) {
				if (BMenuItem* item = fMenubar->FindItem(HS_SAVE_IMAGE))
					item->SetEnabled(false);
			}

			if ((newMode == FULL_MENU && fProjectEntry.InitCheck() != B_OK)
				|| newMode != FULL_MENU) {
				if (BMenuItem* item = fMenubar->FindItem(HS_SAVE_PROJECT))
					item->SetEnabled(false);
			}
		}	break;
	}
}


void
PaintWindow::_EnableMenuItems(BMenu* menu)
{
	int32 count = menu->CountItems();
	for (int32 i = 0; i < count; ++i) {
		if (BMenuItem* currentItem = menu->ItemAt(i)) {
			currentItem->SetEnabled(true);
			if (BMenu* subMenu = currentItem->Submenu())
				_EnableMenuItems(subMenu);
		}
	}
}


void
PaintWindow::_DisableMenuItem(const char* label)
{
	BMenuItem* item = fMenubar->FindItem(label);
	if (item)
		item->SetEnabled(false);
}
