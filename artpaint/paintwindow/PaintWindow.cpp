/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "PaintWindow.h"

#include "AboutWindow.h"
#include "BackgroundView.h"
#include "BrushStoreWindow.h"
#include "ColorPalette.h"
#include "DatatypeSetupWindow.h"
#include "FileIdentificationStrings.h"
#include "FilePanels.h"
#include "GlobalSetupWindow.h"
#include "Image.h"
#include "ImageView.h"
#include "Layer.h"
#include "LayerWindow.h"
#include "Manipulator.h"
#include "ManipulatorServer.h"
#include "ManipulatorInformer.h"
#include "MessageConstants.h"
#include "MessageFilters.h"
#include "NumberControl.h"
#include "PaintApplication.h"
#include "PaintWindowMenuItem.h"
#include "ProjectFileFunctions.h"
#include "PopUpList.h"
#include "ResourceServer.h"
#include "Selection.h"
#include "SettingsServer.h"
#include "StatusView.h"
#include "ToolSetupWindow.h"
#include "ToolSelectionWindow.h"
#include "UtilityClasses.h"
#include "ViewSetupWindow.h"


#include <Alert.h>
#include <BitmapStream.h>
#include <Button.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <Entry.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <SupportDefs.h>
#include <TranslatorRoster.h>
#include <private/interface/WindowInfo.h>


#include <new>
#include <stack>
#include <stdio.h>
#include <stdlib.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PaintWindow"


using ArtPaint::Interface::NumberControl;


// initialize the static variable
BList PaintWindow::sgPaintWindowList(10);
int32 PaintWindow::sgPaintWindowCount = 0;
int32 PaintWindow::sgUntitledWindowNumber = 1;


// these constants are for the internal communication of the PaintWindow-class
#define	HS_SHOW_VIEW_SETUP_WINDOW		'SvsW'
#define HS_SHOW_GLOBAL_SETUP_WINDOW		'SgsW'
#define	HS_RECENT_IMAGE_SIZE			'Rsis'
#define	HS_SHOW_ABOUT_WINDOW			'Sabw'


struct menu_item {
	BString label;
	uint32 what;
	char shortcut;
	uint32 modifiers;
	BHandler* target;
	BString help;
};


PaintWindow::PaintWindow(BRect frame, const char* name, uint32 views,
		const BMessage& settings)
	: BWindow(frame, name, B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_WILL_ACCEPT_FIRST_CLICK | B_NOT_ANCHORED_ON_ACTIVATE | B_AUTO_UPDATE_SIZE_LIMITS)
	, fSettings(settings)
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
	, fImageSizeWindow(NULL)
{
	sgPaintWindowCount++;
	SetSizeLimits(500, 10000, 400, 10000);

	// Fit the window to screen.
	BRect new_frame = FitRectToScreen(frame);
	if (new_frame != frame) {
		MoveTo(new_frame.LeftTop());
		ResizeTo(new_frame.Width(),new_frame.Height());
	}

	if (views == 0)
		fSettings.FindUInt32(skViews, &views);

	if ((views & HS_MENU_BAR) != 0)
		openMenuBar();	// the menubar should be opened

	// The status-view is not optional. It contains sometimes also buttons that
	// can cause some actions to be taken.
	if ((views & HS_STATUS_VIEW) != 0) {
		// Create the status-view and make it display nothing
		fStatusView = new StatusView();
	}

	// make the background view (the backround for image)
	fBackground = new BackgroundView(BRect(0, 0, 0, 0));

	fVerticalScrollbar = fBackground->ScrollBar(B_VERTICAL);
	fVerticalScrollbar->SetSteps(8.0, 32.0);
	fHorizontalScrollbar = fBackground->ScrollBar(B_HORIZONTAL);
	fHorizontalScrollbar->SetSteps(8.0, 32.0);

	if ((views & HS_SIZING_VIEW) != 0x0000)  {
		// we need to show the create canvas area
		const char* widthLabel = B_TRANSLATE("Width");
		const char* heightLabel = B_TRANSLATE("Height");

		BFont font;
		const char* tmpLabel = widthLabel;
		if (font.StringWidth(heightLabel) > font.StringWidth(widthLabel))
			tmpLabel = heightLabel;

		fWidthNumberControl = new NumberControl(tmpLabel, "", NULL);
		fWidthNumberControl->SetLabel(widthLabel);
		fWidthNumberControl->TextView()->SetMaxBytes(4);

		fHeightNumberControl = new NumberControl(tmpLabel, "", NULL);
		fHeightNumberControl->SetLabel(heightLabel);
		fHeightNumberControl->TextView()->SetMaxBytes(4);

		fSetSizeButton = new BButton("set_size_button",
			B_TRANSLATE("Create canvas"), new BMessage(HS_IMAGE_SIZE_SET));

		fSetSizeButton->SetTarget(this);
		fSetSizeButton->MakeDefault(true);

		SettingsServer* settings = SettingsServer::Instance();
		const ImageSizeList& list = settings->RecentImageSizes();

		BMessage messages;
		ImageSizeList::const_iterator it;
		for (it = list.begin(); it != list.end(); ++it) {
			BString label;
			label << int32((*it).width) << " x " << int32((*it).height);

			BMessage msg(HS_RECENT_IMAGE_SIZE);
			msg.AddInt32("width", (*it).width);
			msg.AddInt32("height", (*it).height);
			msg.AddString("label", label.String());

			messages.AddMessage("list", &msg);
		}

		float left = fHeightNumberControl->Frame().right + 5.0;
		float top = (fHeightNumberControl->Frame().bottom -
			fWidthNumberControl->Frame().top) / 2.0;

		BBitmap* listNormal;
		BBitmap* listPushed;
		ResourceServer* server = ResourceServer::Instance();
		server->GetBitmap(POP_UP_LIST, &listNormal);
		server->GetBitmap(POP_UP_LIST_PUSHED, &listPushed);

		PopUpList* popUpList = new PopUpList(BRect(left, top, left + 9, top + 19),
			listPushed, listNormal, messages, list.size(), this);

		BMenu* standardSize = new BMenu(B_TRANSLATE("Standard sizes"));

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

		fContainerBox = new BBox("container_for_controls");
		fContainerBox->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		fImageSizeWindow = new BWindow(BRect(0, 0, 175, 125), "Canvas Size",
			B_MODAL_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL, B_NOT_MOVABLE |
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE);

		fImageSizeWindow->AddToSubset(this);

		BGridLayout* containerLayout = BLayoutBuilder::Grid<>(
			 fImageSizeWindow, 5.0, 5.0)
			.Add(fWidthNumberControl, 0, 0, 0, 0)
			.Add(fWidthNumberControl->CreateLabelLayoutItem(), 0, 0)
			.Add(fWidthNumberControl->CreateTextViewLayoutItem(), 1, 0)
			.Add(fHeightNumberControl, 0, 1, 0, 0)
			.Add(fHeightNumberControl->CreateLabelLayoutItem(), 0, 1)
			.Add(fHeightNumberControl->CreateTextViewLayoutItem(), 1, 1)
			.Add(fSetSizeButton, 1, 2)
			.Add(popUpList, 2, 0)
			.SetInsets(10.0, 2.0, 0.0, 2.0);
		containerLayout->SetMinColumnWidth(2, 15.0);

		BMessage msg(HS_TOOL_HELP_MESSAGE);
		msg.AddString("message", B_TRANSLATE("Select the canvas size you want."));
		PostMessage(&msg, this);

		fImageSizeWindow->ResizeToPreferred();
		fImageSizeWindow->CenterIn(Frame());
		fImageSizeWindow->Activate();
		fImageSizeWindow->Show();
	}

	BGroupLayout *inner = BLayoutBuilder::Group<>(B_VERTICAL, 0)
		.Add(fBackground)
		.SetInsets(-1.0, -2.0, -2.0, -1.0);

	BGroupLayout* outer = BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(inner);

	if (fMenubar)
		outer->AddView(0, fMenubar);

	if (fStatusView) {
		outer->AddView(fStatusView);
	}

	// finally inform the app that new window has been created
	BMessage message(HS_PAINT_WINDOW_OPENED);
	message.AddPointer("window", (void*)this);
	be_app->PostMessage(&message, be_app);

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
PaintWindow::CreatePaintWindow(BBitmap* bitmap, const char* fileName,
	uint32 translatorType, const entry_ref& ref, translator_id outTranslator)
{
	BMessage tmpSettings;
	status_t status = B_ERROR;
	SettingsServer* server = SettingsServer::Instance();
	if ((status = server->GetWindowSettings(&tmpSettings)) != B_OK)
		status = server->GetDefaultWindowSettings(&tmpSettings);

	if (status == B_OK) {
		uint32 flags = HS_MENU_BAR | HS_STATUS_VIEW | HS_HELP_VIEW ;
		if (fileName == NULL) {
			flags = flags | HS_SIZING_VIEW;
			fileName = B_TRANSLATE("Empty paint window");
		}

		BRect frame;
		if (tmpSettings.FindRect(skFrame, &frame) == B_OK) {
			PaintWindow* paintWindow = new (std::nothrow) PaintWindow(frame,
				fileName, flags, tmpSettings);

			if (paintWindow) {
				BMessage* settings = paintWindow->Settings();
				paintWindow->fCurrentHandler = outTranslator;

				if (paintWindow->Lock()) {
					float zoom;
					if (settings->FindFloat(skZoom, &zoom) == B_OK)
						paintWindow->displayMag(zoom);
					paintWindow->Unlock();
				}

				if (bitmap) {
					BNode node(&ref);
					BNodeInfo nodeInfo(&node);

					char mime[B_MIME_TYPE_LENGTH];
					nodeInfo.GetType(mime);

					settings->ReplaceString(skMimeType, mime);
					settings->ReplaceUInt32(skTranslatorType, translatorType);

					paintWindow->SetImageEntry(BEntry(&ref, true));

					BRect bounds = bitmap->Bounds();
					paintWindow->OpenImageView(bounds.IntegerWidth() + 1,
						bounds.IntegerHeight() + 1);
					paintWindow->ReturnImageView()->ReturnImage()->InsertLayer(bitmap);
					paintWindow->AddImageView();
				}
			}
			return paintWindow;
		}
	}
	return NULL;
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

	// Decrement the window-count by 1.
	sgPaintWindowCount--;

	// Remove ourselves from the sgPaintWindowList.
	sgPaintWindowList.RemoveItem(this);
}


void
PaintWindow::FrameResized(float newWidth, float newHeight)
{
	fSettings.ReplaceRect(skFrame, Frame());
	if (fImageSizeWindow && fImageSizeWindow->Lock()) {
		if (!fImageSizeWindow->IsHidden())
			fImageSizeWindow->CenterIn(Frame());
		fImageSizeWindow->Unlock();
	}
}


void
PaintWindow::FrameMoved(BPoint newPosition)
{
	fSettings.ReplaceRect(skFrame, Frame());
	if (fImageSizeWindow && fImageSizeWindow->Lock()) {
		if (!fImageSizeWindow->IsHidden())
			fImageSizeWindow->CenterIn(Frame());
		fImageSizeWindow->Unlock();
	}
}


void
PaintWindow::MenusBeginning()
{
	BWindow::MenusBeginning();

	SettingsServer* server = SettingsServer::Instance();

	_AddRecentMenuItems(fRecentImages, &(server->RecentImagePaths()));
	_AddRecentMenuItems(fRecentProjects, &(server->RecentProjectPaths()));

	BMenuItem *item = fMenubar->FindItem(B_TRANSLATE("Paste as a new layer"));
	if (item != NULL) {
		be_clipboard->Lock();
		if (be_clipboard->Data()->HasMessage("image/bitmap"))
			item->SetEnabled(true);
		else
			item->SetEnabled(false);
		be_clipboard->Unlock();
	}

	item = fMenubar->FindItem(B_TRANSLATE("Paste as a new project"));
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
	switch ( message->what ) {
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED: {
			be_app_messenger.SendMessage(message);
		}	break;

		case HS_RESIZE_WINDOW_TO_FIT: {
			// this comes from fMenubar->"Window"->"Resize Window to Fit" and
			// informs us that we should fit the window to display exactly the
			// image use a private function to do resizing
			_ResizeToImage();
		}	break;

		case HS_RECENT_IMAGE_SIZE: {
			// This comes from the recent image-size pop-up-list.
			if (fImageSizeWindow && fImageSizeWindow->Lock()) {
				fWidthNumberControl->SetValue(message->FindInt32("width"));
				fHeightNumberControl->SetValue(message->FindInt32("height"));
				fImageSizeWindow->Unlock();
			}
		}	break;

		case HS_IMAGE_SIZE_SET: {
			// this comes from a button and it informs that the user has
			// decided the image size and we should create a canvas

			// Here we have to take the measurements from NumberControls.
			// The units might be something else than pixels. This reading and
			// conversion should be done in a separate function.
			int32 width, height;
			width = atoi(fWidthNumberControl->Text());
			height = atoi(fHeightNumberControl->Text());

			bool success = false;
			try {
				// Open the image view
				OpenImageView(width, height);
				// Add a layer to it.
				fImageView->ReturnImage()->InsertLayer();
				success = true;
			}
			catch (std::bad_alloc) {
				delete fImageView;
				fImageView = NULL;
				BAlert* alert = new BAlert("",
					B_TRANSLATE("Not enough free memory to create the image. Please try again with a smaller image size."),
					B_TRANSLATE("OK"), NULL, NULL,
					B_WIDTH_AS_USUAL,B_WARNING_ALERT);
				alert->Go();
			}

			if (success) {
				BMessage windowSettings;
				SettingsServer* server = SettingsServer::Instance();
				if (server->GetWindowSettings(&windowSettings) != B_OK)
					server->GetDefaultWindowSettings(&windowSettings);

				// Record the window's frame
				windowSettings.ReplaceRect(skFrame, Frame());
				server->SetWindowSettings(windowSettings);

				// record the new size to the recent list
				server->AddRecentImageSize(BSize(width, height));

				// Add the view to view hierarchy.
				AddImageView();
				fImageSizeWindow->Hide();
			}
		}	break;

		case HS_MOUSE_DOWN_IN_BACKGROUNDVIEW: {
			// this is the case where mouse has been pressed down in the
			// background-view, we should then put the actual imageview
			// to follow the mouse
			if (fImageView && fImageView->Window())
				fImageView->MouseDown(message->FindPoint("point"));
		}	break;


		case HS_SHOW_LAYER_WINDOW: {
			// this comes from fMenubar->"Window"->"Show Layer Window" and tells
			// us to show the layer window
			LayerWindow::showLayerWindow();
			if (fImageView) {
				LayerWindow::ActiveWindowChanged(this,
					fImageView->ReturnImage()->LayerList(),
					fImageView->ReturnImage()->ReturnThumbnailImage());
			} else {
				LayerWindow::ActiveWindowChanged(this);
			}
		}	break;

		case HS_SHOW_VIEW_SETUP_WINDOW: {
			// his comes from fMenubar->"Window"->"Window Settings…" and tells
			// us to show the  window for setting the parameters of this window
			// and it's views
			ViewSetupWindow::showViewSetupWindow(this,fImageView);
		}	break;

		case HS_SHOW_GLOBAL_SETUP_WINDOW: {
			// this comes from fMenubar->"Window"->"Global Settings…"
			GlobalSetupWindow::ShowGlobalSetupWindow();
		}	break;

		case HS_SHOW_IMAGE_SAVE_PANEL: {
			if (!fImageSavePanel) {
				BPath path;
				if (fImageEntry.InitCheck() != B_OK) {
					BMessage setting;
					if (SettingsServer* server = SettingsServer::Instance())
						server->GetApplicationSettings(&setting);

					// Might fail if the user has removed the directory.
					if (path.SetTo(setting.FindString(skImageSavePath)) != B_OK)
						PaintApplication::HomeDirectory(path);
				} else {
					fImageEntry.GetPath(&path);
					path.GetParent(&path);
				}

				entry_ref ref;
				get_ref_for_path(path.Path(), &ref);

				uint32 translatorType;
				fSettings.FindUInt32(skTranslatorType, &translatorType);

				BMessenger panelTarget(this);
				BMessage message(HS_IMAGE_SAVE_REFS);
				fImageSavePanel = new ImageSavePanel(ref, panelTarget,
					message, translatorType);
					//fImageView->ReturnImage()->ReturnThumbnailImage());
			}

			fImageSavePanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
			set_filepanel_strings(fImageSavePanel);
			if (!fImageSavePanel->IsShowing())
				fImageSavePanel->Show();
		}	break;

		case HS_SHOW_PROJECT_SAVE_PANEL: {
			// This comes from fMenubar->"File"->"Save Project As…"
			if (!fProjectSavePanel) {
				BPath path;
				if (fProjectEntry.InitCheck() != B_OK) {
					BMessage settings;
					SettingsServer* server = SettingsServer::Instance();
					if (server->GetApplicationSettings(&settings) == B_OK) {
						BString tmp = settings.FindString(skProjectSavePath);
						if (path.SetTo(tmp.String()) != B_OK)
							PaintApplication::HomeDirectory(path);
					}
				} else {
					fProjectEntry.GetPath(&path);
					path.GetParent(&path);
				}

				entry_ref ref;
				get_ref_for_path(path.Path(), &ref);

				BMessenger window(this);
				BMessage msg(HS_PROJECT_SAVE_REFS);
				msg.AddInt32("TryAgain", message->GetInt32("TryAgain", false));
				msg.AddInt32("quitAll", message->GetInt32("quitAll", false));
				fProjectSavePanel = new BFilePanel(B_SAVE_PANEL, &window, &ref,
					0, false, &msg);
			}

			fProjectSavePanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
			char string[256];
			sprintf(string, "ArtPaint: %s", B_TRANSLATE("Save project"));
			fProjectSavePanel->Window()->SetTitle(string);
			set_filepanel_strings(fProjectSavePanel);
			if (!fProjectSavePanel->IsShowing())
				fProjectSavePanel->Show();
		}	break;

		case HS_SHOW_COLOR_WINDOW: {
			// This comes from fMenubar->"Window"->"Show Color Window".
			// We should open the color window.
			ColorPaletteWindow::showPaletteWindow(); // TODO: was (false)
		}	break;

		case HS_SHOW_TOOL_WINDOW: {
			// This comes from fMenubar->"Window"->"Show Tool Window". We should
			// open the tool window.
			ToolSelectionWindow::showWindow();
		}	break;

		case HS_SHOW_TOOL_SETUP_WINDOW: {
			if (SettingsServer* server = SettingsServer::Instance()) {
				BMessage settings;
				server->GetApplicationSettings(&settings);
				ToolSetupWindow::ShowToolSetupWindow(settings.FindInt32(skTool));
			}
		}	break;

		case HS_SHOW_BRUSH_STORE_WINDOW: {
			BrushStoreWindow::showWindow();
		}	break;

		case HS_IMAGE_SAVE_REFS: {
			if (fImageSavePanel)
				fImageSavePanel->Hide();
			// Here we call the function that saves the image. We actually call
			// it in another thread while ensuring that it saves the correct
			// bitmap. We should also protect the bitmap from being modified
			// while we save. We should also inform the user about saving.
			if (fImageView) {
				thread_id threadId = spawn_thread(PaintWindow::save_image,
					"save image", B_NORMAL_PRIORITY, (void*)this);
				if (threadId >= 0) {
					BMessage* data = new BMessage(*message);
					send_data(threadId, 0, (void*)&data, sizeof(BMessage*));
					resume_thread(threadId);
				}
			}
		}	break;

		case HS_PROJECT_SAVE_REFS: {
			printf("Saving refs\n");
			if (fProjectSavePanel)
				fProjectSavePanel->Hide();
			if (fImageView) {
				// We call the function that saves the project.
				thread_id threadId = spawn_thread(PaintWindow::save_project,
					"save project", B_NORMAL_PRIORITY, (void*)this);
				if (threadId >= 0) {
					BMessage* data = new BMessage(*message);
					send_data(threadId, 0, (void*)&data, sizeof(BMessage*));
					resume_thread(threadId);
				}
			}
		}	break;

		case HS_SAVE_IMAGE: {
			// We make a message containing the file name and ref for its dir.
			BEntry parent;
			fImageEntry.GetParent(&parent);

			BPath path;
			parent.GetPath(&path);

			entry_ref ref;
			get_ref_for_path(path.Path(), &ref);

			char name[B_FILE_NAME_LENGTH];
			fImageEntry.GetName(name);

			BMessage msg(HS_IMAGE_SAVE_REFS);
			msg.AddString("name", name);
			msg.AddRef("directory", &ref);
			PostMessage(&msg, this);
		}	break;

		case HS_SAVE_PROJECT: {
			// Create a message containing file-name and ref for its dir.
			if (fProjectEntry.InitCheck() == B_OK) {
				BEntry parent;
				fProjectEntry.GetParent(&parent);

				BPath path;
				parent.GetPath(&path);

				entry_ref ref;
				get_ref_for_path(path.Path(), &ref);

				char name[B_FILE_NAME_LENGTH];
				fProjectEntry.GetName(name);

				BMessage msg(HS_PROJECT_SAVE_REFS);
				msg.AddString("name", name);
				msg.AddRef("directory", &ref);
				msg.AddInt32("TryAgain", message->GetInt32("TryAgain", false));
				msg.AddInt32("quitAll", message->GetInt32("quitAll", false));
				PostMessage(&msg, this);
			} else {
				BMessage msg(HS_SHOW_PROJECT_SAVE_PANEL);
				msg.AddInt32("TryAgain", message->GetInt32("TryAgain", false));
				msg.AddInt32("quitAll", message->GetInt32("quitAll", false));
				PostMessage(&msg, this);
			}
		}	break;

		case HS_SAVE_FORMAT_CHANGED: {
			// this comes from the image save panel's format menu and informs
			// that the wanted save format has changed
			fCurrentHandler = message->FindInt32("be:translator");
			DatatypeSetupWindow::ChangeHandler(fCurrentHandler);

			uint32 translatorType = message->FindInt32("be:type");
			fSettings.ReplaceUInt32(skTranslatorType, translatorType);

			int32 count;
			const translation_format* formats = NULL;
			BTranslatorRoster* roster = BTranslatorRoster::Default();
			if (roster->GetOutputFormats(fCurrentHandler, &formats, &count) == B_OK) {
				for (int32 i = 0; i < count; ++i) {
					if (formats[i].type == translatorType)
						fSettings.ReplaceString(skMimeType, BString(formats[i].MIME));
				}
			}
		}	break;

		case HS_SHOW_DATATYPE_SETTINGS: {
			// This comes from image-save-panel's setting-button and tells us to
			// show the datatype-setup-window.
			DatatypeSetupWindow::ShowWindow(fCurrentHandler);
		}	break;

		case B_CANCEL: {
			if (fImageSavePanel && fImageSavePanel->IsShowing())
					fImageSavePanel->Hide();

			if (fProjectSavePanel && fProjectSavePanel->IsShowing())
					fProjectSavePanel->Hide();
		}	break;

		case HS_TOOL_HELP_MESSAGE: {
		case HS_TEMPORARY_HELP_MESSAGE:
			// This might come from many places and it informs us to display a
			// message in the help view.
			static BString helpMessage;
			message->FindString("message", &helpMessage);
			SetHelpString(helpMessage.String(), message->what);
		}	break;

		case HS_SHOW_ABOUT_WINDOW: {
			AboutWindow::showWindow();
		}	break;

		default: {
			BWindow::MessageReceived(message);
		}	break;
	}
}


bool
PaintWindow::QuitRequested()
{
	// here we should ask the user if changes to picture should be saved we also
	// tell the application that the number of paint windows has decreased by one
	if (fImageView) {
		if (fImageView->Quit() == false)
			return false;

		if (fImageSavePanel)
			delete fImageSavePanel;

		if (fProjectSavePanel)
			delete fProjectSavePanel;
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
	if (fImageView) {
		if (Image* image = fImageView->ReturnImage()) {
			if (Frame() == _PreferredSize(image)) {
				MoveTo(fUserFrame.LeftTop());
				printf("here: %f x %f\n", fUserFrame.Width(), fUserFrame.Height());
				ResizeTo(fUserFrame.Width(), fUserFrame.Height());
			} else {
				_ResizeToImage();
			}
			return;
		}
	}

	BWindow::Zoom(leftTop, width, height);
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
		if (fImageSizeWindow && fImageSizeWindow->Lock()) {
			fWidthNumberControl->SetValue(int32(point.x));
			fHeightNumberControl->SetValue(int32(point.y));
			fImageSizeWindow->Unlock();
		}
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
	fMenubar = new BMenuBar("menu_bar");

	BMenu* menu = new BMenu(B_TRANSLATE("File"));
	fMenubar->AddItem(menu);

	// the File menu
	menu_item fileMenu[] = {
		{ B_TRANSLATE("Open image…"), HS_SHOW_IMAGE_OPEN_PANEL,
			'O', 0, be_app,
			B_TRANSLATE("Opens an image from the disk.") },
		{ B_TRANSLATE("Save image"), HS_SAVE_IMAGE,
			'S', 0, this,
			B_TRANSLATE("Saves the image to the disk with the current name.") },
		{ B_TRANSLATE("Save image as…"), HS_SHOW_IMAGE_SAVE_PANEL,
		 	0, 0, this,
		 	B_TRANSLATE("Saves the image to disk.") },
		{ "SEPARATOR", 0, 0, 0, NULL, "SEPARATOR" },	// separator
		{ B_TRANSLATE("New project…"), HS_NEW_PAINT_WINDOW,
			'N', 0, be_app,
			B_TRANSLATE("Creates a new empty paint window.") },
		{ B_TRANSLATE("Open project…"), HS_SHOW_PROJECT_OPEN_PANEL,
		 	'O', B_SHIFT_KEY, be_app,
		 	B_TRANSLATE("Opens a project from the disk.") },
		{ B_TRANSLATE("Save project"), HS_SAVE_PROJECT,
			'S', B_SHIFT_KEY, this,
			B_TRANSLATE("Saves the project to the disk with the current name.") },
		{ B_TRANSLATE("Save project as…"), HS_SHOW_PROJECT_SAVE_PANEL,
			0, 0, this,
			B_TRANSLATE("Saves the project to the disk.") },
		{ "SEPARATOR", 0, 0, 0, NULL, "SEPARATOR" }	// separator
	};

	for (uint32 i = 0; i < (sizeof(fileMenu) / sizeof(menu_item)); ++i) {
		_AddMenuItems(menu, fileMenu[i].label, fileMenu[i].what,
			fileMenu[i].shortcut, fileMenu[i].modifiers, fileMenu[i].target,
			fileMenu[i].help);
	}

	fRecentImages = new BMenu(B_TRANSLATE("Recent images"));
	menu->AddItem(fRecentImages);

	fRecentProjects = new BMenu(B_TRANSLATE("Recent projects"));
	menu->AddItem(fRecentProjects);

	menu_item fileMenu2[] = {
		{ "SEPARATOR", 0, 0, 0, NULL, "SEPARATOR" },	// separator
		{ B_TRANSLATE("Close"), B_QUIT_REQUESTED,
			'W', 0, this,
			B_TRANSLATE("Closes the current window.") },
		{ B_TRANSLATE("Quit"), B_QUIT_REQUESTED,
			'Q', 0, be_app,
			B_TRANSLATE("Quits the whole program.") }
	};

	for (uint32 i = 0; i < (sizeof(fileMenu2) / sizeof(menu_item)); ++i) {
		_AddMenuItems(menu, fileMenu2[i].label, fileMenu2[i].what,
			fileMenu2[i].shortcut, fileMenu2[i].modifiers, fileMenu2[i].target,
			fileMenu2[i].help);
	}

	// the Edit menu
	menu = new BMenu(B_TRANSLATE("Edit"));
	fMenubar->AddItem(menu);

	menu_item editMenu[] = {
		{ B_TRANSLATE("Undo not available"), HS_UNDO,
			'Z', 0, this,
			B_TRANSLATE("Undos the previous action.") },
		{ B_TRANSLATE("Redo not available"), HS_REDO,
			'Z', B_SHIFT_KEY, this,
			B_TRANSLATE("Redos the action that was last undone.") },
		{ "SEPARATOR", 0, 0, 0, NULL, "SEPARATOR" }	// separator
	};

	for (uint32 i = 0; i < (sizeof(editMenu) / sizeof(menu_item)); ++i) {
		_AddMenuItems(menu, editMenu[i].label, editMenu[i].what,
			editMenu[i].shortcut, editMenu[i].modifiers, editMenu[i].target,
			editMenu[i].help);
	}

	BMenu* subMenu = new BMenu(B_TRANSLATE("Cut"));
	menu->AddItem(subMenu);

	BMessage *a_message = new BMessage(B_CUT);
	a_message->AddInt32("layers", HS_MANIPULATE_CURRENT_LAYER);
	subMenu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Active layer"),
		a_message, 'X', 0, this,
		B_TRANSLATE("Copies the image to the clipboard and clears all layers.")));

	a_message = new BMessage(B_CUT);
	a_message->AddInt32("layers", HS_MANIPULATE_ALL_LAYERS);
	subMenu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("All layers"),
		a_message, 'X', B_SHIFT_KEY, this,
		B_TRANSLATE("Copies the image to the clipboard and clears all layers.")));

	subMenu = new BMenu(B_TRANSLATE("Copy"));
	menu->AddItem(subMenu);
	a_message = new BMessage(B_COPY);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	subMenu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Active layer"),
		a_message, 'C', 0, this,
		B_TRANSLATE("Copies the active layer to the clipboard.")));

	a_message = new BMessage(B_COPY);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	subMenu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("All layers"),
		a_message, 'C', B_SHIFT_KEY, this,
		B_TRANSLATE("Copies the image to the clipboard.")));
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Paste as a new layer"),
		new BMessage(B_PASTE), 'V', 0, this,
		B_TRANSLATE("Pastes previously copied layer from the clipboard as a new layer.")));
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Paste as a new project"),
		new BMessage(B_PASTE), 'V', B_SHIFT_KEY, this,
		B_TRANSLATE("Pastes clipboard as a new project.")));

	menu->AddItem(new BSeparatorItem());

	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Grow selection"),
		new BMessage(HS_GROW_SELECTION), 'G', 0, this,
		B_TRANSLATE("Grows the selection a little bit in every direction.")));
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Shrink selection"),
		new BMessage(HS_SHRINK_SELECTION), 'H', 0, this,
		B_TRANSLATE("Shrinks the selection a little bit from every direction.")));
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Invert selection"),
		new BMessage(HS_INVERT_SELECTION), 0, 0, this,
		B_TRANSLATE("Inverts the selction so that selected becomes non-selected and vice versa.")));
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Clear selection"),
		new BMessage(HS_CLEAR_SELECTION), 'D', 0, this,
		B_TRANSLATE("Clears the selection so that everything is selected.")));

	// The Layer menu.
	menu = new BMenu(B_TRANSLATE("Layer"));
	fMenubar->AddItem(menu);

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",ROTATION_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Rotate…"),
		a_message, 'R', 0, this,
		B_TRANSLATE("Starts rotating the active layer.")));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",TRANSLATION_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Translate…"),
		a_message, 'T', 0, this,
		B_TRANSLATE("Starts moving the active layer.")));

//	a_message = new BMessage(HS_START_MANIPULATOR);
//	a_message->AddInt32("manipulator_type",FREE_TRANSFORM_MANIPULATOR);
//	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
//	menu->AddItem(new PaintWindowMenuItem("Free transform test",a_message,0,0,this,"Use left shift and control to rotate and scale."));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",HORIZ_FLIP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Flip horizontally"),
		a_message, B_LEFT_ARROW, 0, this,
		B_TRANSLATE("Flips the active layer horizontally.")));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",VERT_FLIP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Flip vertically"),
		a_message, B_UP_ARROW, 0, this,
		B_TRANSLATE("Flips the active layer vertically.")));

	menu->AddSeparatorItem();

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",TRANSPARENCY_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Change transparency…"),
		a_message, 0, 0, this,
		B_TRANSLATE("Starts changing the transparency of active layer.")));
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Clear layer"),
		new BMessage(HS_CLEAR_LAYER), 0, 0, this,
		B_TRANSLATE("Clears the active layer.")));

	menu->AddSeparatorItem();

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",TEXT_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_CURRENT_LAYER);
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Insert text…"),
		a_message, 'I', 0, this,
		B_TRANSLATE("Inserts text into the active layer. Same as the text tool.")));
	menu->AddSeparatorItem();
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Show layer window"),
		new BMessage(HS_SHOW_LAYER_WINDOW), 'L', 0, this,
		B_TRANSLATE("Brings up the layer window.")));
	menu->AddItem(new BSeparatorItem());
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Add layer"),
		new BMessage(HS_ADD_LAYER_FRONT), '.', 0, this,
		B_TRANSLATE("Adds a layer to the top of this image.")));

	// The Canvas menu.
	menu = new BMenu(B_TRANSLATE("Canvas"));
	fMenubar->AddItem(menu);

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",ROTATION_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Rotate…"),
		a_message, 'R', B_SHIFT_KEY, this,
		B_TRANSLATE("Starts rotating all layers.")));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",ROTATE_CW_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Rotate +90°"),
		a_message, 0, 0, this,
		B_TRANSLATE("Rotates all layers 90° clockwise.")));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",ROTATE_CCW_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Rotate -90°"),
		a_message, 0, 0, this,
		B_TRANSLATE("Rotates all layers 90° counter-clockwise.")));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",TRANSLATION_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Translate…"),
		a_message, 'T', B_SHIFT_KEY, this,
		B_TRANSLATE("Starts moving all layers.")));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",HORIZ_FLIP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Flip horizontally"),
		a_message, B_LEFT_ARROW, B_SHIFT_KEY, this,
		B_TRANSLATE("Flips all layers horizontally.")));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",VERT_FLIP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Flip vertically"),
		a_message, B_UP_ARROW, B_SHIFT_KEY, this,
		B_TRANSLATE("Flips all layers vertically.")));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",CROP_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Crop…"),
		a_message, 'C', B_CONTROL_KEY, this,
		B_TRANSLATE("Starts cropping the image.")));

	a_message = new BMessage(HS_START_MANIPULATOR);
	a_message->AddInt32("manipulator_type",SCALE_MANIPULATOR);
	a_message->AddInt32("layers",HS_MANIPULATE_ALL_LAYERS);
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Scale…"),
		a_message, 'S', B_CONTROL_KEY, this,
		B_TRANSLATE("Starts scaling the image.")));

	menu->AddItem(new BSeparatorItem());
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Clear canvas"),
		new BMessage(HS_CLEAR_CANVAS), 0, 0, this,
		B_TRANSLATE("Clears all layers.")));


	// The Window menu,
	menu = new BMenu(B_TRANSLATE("Window"));
	fMenubar->AddItem(menu);

	// use + and - as shorcuts for zooming
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Zoom in"),
		new BMessage(HS_ZOOM_IMAGE_IN), '+', 0, this,
		B_TRANSLATE("Zooms into the image.")));
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Zoom out"),
		new BMessage(HS_ZOOM_IMAGE_OUT), '-', 0, this,
		B_TRANSLATE("Zooms out from the image.")));

	subMenu = new BMenu(B_TRANSLATE("Set zoom level"));
	menu->AddItem(subMenu);
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",0.25);
	subMenu->AddItem(new PaintWindowMenuItem("25%",
		a_message, 0, 0, this,
		B_TRANSLATE("Sets the zoom level to 25%.")));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",0.50);
	subMenu->AddItem(new PaintWindowMenuItem("50%",
		a_message, 0, 0, this,
		B_TRANSLATE("Sets the zoom level to 50%.")));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",1.0);
	subMenu->AddItem(new PaintWindowMenuItem("100%",
		a_message, 0, 0, this,
		B_TRANSLATE("Sets the zoom level to 100%.")));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",2.0);
	subMenu->AddItem(new PaintWindowMenuItem("200%",
		a_message, 0, 0, this,
		B_TRANSLATE("Sets the zoom level to 200%.")));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",4.0);
	subMenu->AddItem(new PaintWindowMenuItem("400%",
		a_message, 0, 0, this,
		B_TRANSLATE("Sets the zoom level to 400%.")));
	a_message = new BMessage(HS_SET_MAGNIFYING_SCALE);
	a_message->AddFloat("magnifying_scale",8.0);
	subMenu->AddItem(new PaintWindowMenuItem("800%",
		a_message, 0, 0, this,
		B_TRANSLATE("Sets the zoom level to 800%.")));

	subMenu = new BMenu(B_TRANSLATE("Set grid"));
	menu->AddItem(subMenu);
	a_message = new BMessage(HS_GRID_ADJUSTED);
	a_message->AddPoint("origin",BPoint(0,0));
	a_message->AddInt32("unit",1);
	subMenu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Off"),
		a_message, 0, 0, this,
		B_TRANSLATE("Turns the grid off.")));
	a_message = new BMessage(HS_GRID_ADJUSTED);
	a_message->AddPoint("origin",BPoint(0,0));
	a_message->AddInt32("unit",2);
	subMenu->AddItem(new PaintWindowMenuItem("2x2",
		a_message, 0, 0, this,
		B_TRANSLATE("Sets the grid to 2 by 2 pixels.")));
	a_message = new BMessage(HS_GRID_ADJUSTED);
	a_message->AddPoint("origin",BPoint(0,0));
	a_message->AddInt32("unit",4);
	subMenu->AddItem(new PaintWindowMenuItem("4x4",
		a_message, 0, 0, this,
		B_TRANSLATE("Sets the grid to 4 by 4 pixels.")));
	a_message = new BMessage(HS_GRID_ADJUSTED);
	a_message->AddPoint("origin",BPoint(0,0));
	a_message->AddInt32("unit",8);
	subMenu->AddItem(new PaintWindowMenuItem("8x8",
		a_message, 0, 0, this,
		B_TRANSLATE("Sets the grid to 8 by 8 pixels.")));
	subMenu->SetRadioMode(true);
	subMenu->ItemAt(0)->SetMarked(true);

	// use here the same shortcut as the tracker uses == Y
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Resize to fit"),
		new BMessage(HS_RESIZE_WINDOW_TO_FIT), 'Y', 0, this,
		B_TRANSLATE("Resizes the window to proper size for the image and the screen.")));
	menu->AddSeparatorItem();
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Show palette window"),
		new BMessage(HS_SHOW_COLOR_WINDOW), 'P', 0, this,
		B_TRANSLATE("Brings up the palette window.")));
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Show layer window"),
		new BMessage(HS_SHOW_LAYER_WINDOW), 'L', 0, this,
		B_TRANSLATE("Brings up the layer window.")));
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Show tool window"),
		new BMessage(HS_SHOW_TOOL_WINDOW), 'K', 0, this,
		B_TRANSLATE("Brings up the tool selection window.")));
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Show tool setup window"),
		new BMessage(HS_SHOW_TOOL_SETUP_WINDOW), 'M', 0, this,
		B_TRANSLATE("Brings up the tool setup window.")));
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Show brush window"),
		new BMessage(HS_SHOW_BRUSH_STORE_WINDOW), 'B', 0, this,
		B_TRANSLATE("Brings up the brush window.")));
	menu->AddSeparatorItem();
//	menu->AddItem(new PaintWindowMenuItem(_StringForId(NEW_PAINT_WINDOW_STRING),new BMessage(HS_NEW_PAINT_WINDOW),'N',0,this,_StringForId(NEW_PROJECT_HELP_STRING)));
//	menu->AddSeparatorItem();
//	menu->AddItem(new BMenuItem("Window settings…", new BMessage(HS_SHOW_VIEW_SETUP_WINDOW)));
	menu->AddItem(new PaintWindowMenuItem(B_TRANSLATE("Settings…"),
		new BMessage(HS_SHOW_GLOBAL_SETUP_WINDOW), 0, 0, this,
		B_TRANSLATE("Brings up the settings window.")));

	// This will be only temporary place for add-ons. Later they will be spread
	// in the menu hierarchy according to their types.
	menu = new BMenu(B_TRANSLATE("Add-ons"));
	thread_id add_on_adder_thread = spawn_thread(_AddAddOnsToMenu,
		"add_on_adder_thread", B_NORMAL_PRIORITY, this);
	resume_thread(add_on_adder_thread);
	fMenubar->AddItem(menu);

	// help
	menu = new BMenu(B_TRANSLATE("Help"));
	fMenubar->AddItem(menu);

	BMessage* message =  new BMessage(HS_SHOW_USER_DOCUMENTATION);
	message->AddString("document", "index.html");

	BMenuItem* item = new PaintWindowMenuItem(B_TRANSLATE("User manual…"),
		message, 0, 0, this,
		B_TRANSLATE("Opens the main documentation for ArtPaint."));
	item->SetTarget(be_app);
	menu->AddItem(item);

	message = new BMessage(HS_SHOW_USER_DOCUMENTATION);
	message->AddString("document", "shortcuts.html");
	item = new PaintWindowMenuItem(B_TRANSLATE("Shortcuts…"),
		message, 0, 0, this,
		B_TRANSLATE("Opens a page that describes ArtPaint's keyboard shortcuts."));
	item->SetTarget(be_app);
	menu->AddItem(item);

	menu->AddSeparatorItem();

	message = new BMessage(HS_SHOW_ABOUT_WINDOW);
	item = new PaintWindowMenuItem(B_TRANSLATE("About ArtPaint"),
		message, 0, 0, this,
		B_TRANSLATE("Shows a window that has information about the program."));
	menu->AddItem(item);

	_ChangeMenuMode(NO_IMAGE_MENU);

	return true;
}


int32
PaintWindow::_AddAddOnsToMenu(void* data)
{
	PaintWindow* paintWindow = static_cast<PaintWindow*> (data);
	if (!paintWindow)
		return B_ERROR;

	ManipulatorServer* server = ManipulatorServer::Instance();
	if (!server)
		return B_ERROR;

	while (!server->AddOnsLoaded() || !paintWindow->KeyMenuBar())
		snooze(50000);

	if (BMenuBar* menuBar = paintWindow->KeyMenuBar()) {
		BMenuItem* item = menuBar->FindItem(B_TRANSLATE("Add-ons"));
		if (BMenu* addOnMenu = item->Submenu()) {
			if (paintWindow->Lock()) {
				ImageList::const_iterator it;
				const ImageList& imageList = server->AddOnImageList();
				for (it = imageList.begin(); it != imageList.end(); ++it) {
					int32* add_on_version;
					status_t status = get_image_symbol(*it, "add_on_api_version",
						B_SYMBOL_TYPE_DATA, (void**)&add_on_version);

					if (status != B_OK)
						continue;

					if (*add_on_version == ADD_ON_API_VERSION) {
						Manipulator* (*Instantiate)(BBitmap*, ManipulatorInformer*);
						status_t status = get_image_symbol(*it, "instantiate_add_on",
							B_SYMBOL_TYPE_TEXT, (void**)&Instantiate);
						if (status == B_OK) {
							Manipulator *manipulator = Instantiate(NULL, NULL);

							if (manipulator != NULL) {
								BMessage* message = new BMessage(HS_START_MANIPULATOR);
								message->AddInt32("image_id", *it);
								message->AddInt32("layers", HS_MANIPULATE_CURRENT_LAYER);
								message->AddInt32("manipulator_type", ADD_ON_MANIPULATOR);

								addOnMenu->AddItem(new PaintWindowMenuItem(
									manipulator->ReturnName(),
									message, 0, 0, paintWindow,
									manipulator->ReturnHelpString()));

								delete manipulator;
								manipulator = NULL;
							}
						}

					}
				}
				addOnMenu->SetTargetForItems(paintWindow);
				paintWindow->Unlock();
			}

			if (paintWindow->fImageView)
				addOnMenu->SetTargetForItems(paintWindow->fImageView);
			return B_OK;
		}
	}
	return B_ERROR;
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
	fBackground->SetTarget(fImageView);
	// Change the regular help-view's message.
//	BMessage *help_message = new BMessage(HS_REGULAR_HELP_MESSAGE);
//	help_message->AddString("message",HS_DRAW_MODE_HELP_MESSAGE);
//	PostMessage(help_message,this);
//	delete help_message;

	// Change the menu-mode to enable all items.
	_ChangeMenuMode(FULL_MENU);

	// Adjust image's position and size.
	fImageView->adjustSize();
	fImageView->adjustPosition();
	// Update the image's scroll-bars.
	fImageView->adjustScrollBars();

	// Change image for target for certain menu-items. These cannot be changed
	// before image is added as a child to this window.
	fMenubar->FindItem(B_TRANSLATE("Edit"))->Submenu()->SetTargetForItems(fImageView);
	fMenubar->FindItem(B_TRANSLATE("Paste as a new project"))->SetTarget(be_app);

	BMenu *menu = fMenubar->FindItem(B_TRANSLATE("Edit"))->Submenu();
	if (menu != NULL) {
		BMenu *sub_menu;
		for (int32 i=0;i<menu->CountItems();i++) {
			sub_menu = menu->SubmenuAt(i);
			if (sub_menu != NULL)
				sub_menu->SetTargetForItems(fImageView);
		}
	}

	fMenubar->FindItem(B_TRANSLATE("Add-ons"))->Submenu()->SetTargetForItems(fImageView);
	fMenubar->FindItem(HS_ADD_LAYER_FRONT)->SetTarget(fImageView);
	fMenubar->FindItem(B_TRANSLATE("Crop…"))->SetTarget(fImageView);
	fMenubar->FindItem(HS_CLEAR_CANVAS)->SetTarget(fImageView);
	fMenubar->FindItem(HS_CLEAR_LAYER)->SetTarget(fImageView);
	fMenubar->FindItem(HS_ZOOM_IMAGE_IN)->SetTarget(fImageView);
	fMenubar->FindItem(HS_ZOOM_IMAGE_OUT)->SetTarget(fImageView);
	fMenubar->FindItem(B_TRANSLATE("Set zoom level"))->Submenu()->SetTargetForItems(fImageView);
	fMenubar->FindItem(B_TRANSLATE("Set grid"))->Submenu()->SetTargetForItems(fImageView);

	std::stack<BMenu*> menus;
	menus.push(static_cast<BMenu*> (NULL));
	for (int32 i = 0; i < fMenubar->CountItems(); ++i) {
		if (BMenu* subMenu = fMenubar->ItemAt(i)->Submenu())
			menus.push(subMenu);
	}

	// Change the image as target for all menu-items that have HS_START_MANIPULATOR
	// as their message's what constant.
	menu = menus.top();
	menus.pop();
	while (menu != NULL) {
		for (int32 i = 0; i < menu->CountItems(); ++i) {
			if (menu->ItemAt(i)->Command() == HS_START_MANIPULATOR)
				menu->ItemAt(i)->SetTarget(fImageView);

			if (menu->ItemAt(i)->Submenu() != NULL)
				menus.push(menu->ItemAt(i)->Submenu());
		}
		menu = menus.top();
		menus.pop();
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
		BString format("%s - %ld");
		BString title;
		title.SetToFormat(format, B_TRANSLATE("Untitled"),
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
	_ResizeToImage();
	Unlock();

	// We can update ourselves to the layer-window.
	LayerWindow::ActiveWindowChanged(this, fImageView->ReturnImage()->LayerList(),
		fImageView->ReturnImage()->ReturnThumbnailImage());

	return B_OK;
}


void
PaintWindow::_ResizeToImage()
{
	if (!fImageView)
		return;

	if (Image* image = fImageView->ReturnImage()) {
		BRect frame = _PreferredSize(image);

		if (Frame() != frame)
			fUserFrame = Frame();

		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}
}


BRect
PaintWindow::_PreferredSize(Image* image) const
{
	int32* tokens = 0;
	int32 tokenCount = 0;
	status_t status = BPrivate::get_window_order(current_workspace(), &tokens,
		&tokenCount);

	float tabHeight = 21.0;
	float borderSize = 5.0;
	if (status == B_OK && tokens && tokenCount > 0) {
		for (int32 i = 0; i < tokenCount; ++i) {
			if (client_window_info* windowInfo = get_window_info(tokens[i])) {
				if (!windowInfo->is_mini && !windowInfo->show_hide_level > 0) {
					tabHeight = windowInfo->tab_height;
					borderSize = windowInfo->border_size;
					free(windowInfo);
					break;
				}
				free(windowInfo);
			}
		}
		free(tokens);
	}

	BRect screenFrame = BScreen().Frame().OffsetToCopy(B_ORIGIN);
	screenFrame.top += tabHeight;
	screenFrame.InsetBy(borderSize, borderSize);

	BRect rect = Frame();
	const float scale = fImageView->getMagScale();
	const float width = (scale * image->Width());
	if (screenFrame.Width() < width) {
		rect.left = borderSize;
		rect.right = rect.left + screenFrame.Width();
	} else {
		rect.right = rect.left + width;
	}

	const float height = (scale * image->Height());
	if (screenFrame.Height() < height) {
		rect.top = tabHeight + borderSize;
		rect.bottom = rect.top + screenFrame.Height();
	} else {
		rect.bottom = rect.top + height;
	}

	return rect;
}


int32
PaintWindow::save_image(void* data)
{
	int32 status = B_ERROR;
		printf("Error: %s\n", strerror(status));
	if (PaintWindow *window = static_cast<PaintWindow*>(data)) {
		thread_id sender;
		BMessage *message = NULL;
		receive_data(&sender, (void*)&message, sizeof(BMessage*));

		status = window->_SaveImage(message);

		if (status == B_OK) {
			if (message->GetInt32("TryAgain", false)) {
				if (message->GetInt32("quitAll", false))
					be_app_messenger.SendMessage(B_QUIT_REQUESTED);
				else
					BMessenger(window).SendMessage(B_QUIT_REQUESTED);
			}
		}
		delete message;
	}
	return status;
}


status_t
PaintWindow::_SaveImage(BMessage *message)
{
	status_t status = B_ERROR;
	if (fImageView->Freeze() == B_OK) {
		BString name;
		entry_ref ref;
		if (message->FindString("name", &name) != B_OK
			|| message->FindRef("directory", &ref) != B_OK)
			return status;

		BDirectory directory(&ref);
		// store the entry-ref
		status = fImageEntry.SetTo(&directory, name.String(), true);

		// Only one save ref is received so we do not need to loop.
		BFile file;
		if ((status = file.SetTo(&directory, name.String(), B_WRITE_ONLY |
			B_CREATE_FILE | B_ERASE_FILE)) != B_OK) {
			fImageView->UnFreeze();
			return status;
		}

		// Create a BNodeInfo for this file and set the MIME-type and preferred
		// app. Get and set the app signature, not sure why it's commented out.
		BNodeInfo nodeInfo(&file);
		nodeInfo.SetType(fSettings.FindString(skMimeType));

		// app_info info;
		// be_app->GetAppInfo(&info);
		// nodeinfo.SetPreferredApp(info.signature);

		// here we should save some attributes with the file
		BNode node(&directory, message->FindString("name"));
		writeAttributes(node);

		// here translate the data using a BitmapStream-object
		BBitmap* bitmap = fImageView->ReturnImage()->ReturnRenderedImage();
		printf("Bitmap at 0,0: 0x%8lx\n",*((uint32*)(bitmap->Bits())));

		// TODO: check if we leak here
		BBitmapStream* bitmapStream = new BBitmapStream(bitmap);
		BTranslatorRoster* roster = BTranslatorRoster::Default();

		uint32 translatorType;
		fSettings.FindUInt32(skTranslatorType, &translatorType);
		status = roster->Translate(bitmapStream, (const translator_info*)NULL,
			(BMessage*)NULL, &file, translatorType, B_TRANSLATOR_BITMAP);
		fImageView->UnFreeze();

		if (status == B_OK) {
			char title[B_FILE_NAME_LENGTH];
			fImageEntry.GetName(title);
			if (Lock()) {
				fImageView->ResetChangeStatistics(false, true);
				fImageView->SetImageName(title);

				// BMenuItem *item = fMenubar->FindItem(HS_SAVE_IMAGE);
				// if (item) item->SetEnabled(true);

				Unlock();
			}

			BPath path;
			fImageEntry.GetPath(&path);

			// Also change this new path into the settings.
			SettingsServer* server = SettingsServer::Instance();
			server->AddRecentImagePath(path.Path());

			path.GetParent(&path);
			if (path.Path() != NULL) {
				server->SetValue(SettingsServer::Application, skImageSavePath,
					path.Path());
			}
		} else {
			printf("Error while saving: %s\n", strerror(status));
		}
	}

	return status;
}


int32
PaintWindow::save_project(void* data)
{
	status_t status = B_ERROR;
	if (PaintWindow* window = static_cast<PaintWindow*>(data)) {
		thread_id sender;
		BMessage* message = NULL;
		receive_data(&sender, (void*)&message, sizeof(BMessage*));

		status = window->_SaveProject(message);

		if (status == B_OK) {
			if (message->GetInt32("TryAgain", 0)) {
				if (message->GetInt32("quitAll", 0))
					be_app_messenger.SendMessage(B_QUIT_REQUESTED);
				else
					BMessenger(window).SendMessage(B_QUIT_REQUESTED);
			}
		}
		delete message;
	}
	return status;
}


status_t
PaintWindow::_SaveProject(BMessage *message)
{
	if (fImageView->Freeze() == B_OK) {
		BString name;
		entry_ref ref;
		if (message->FindString("name", &name) != B_OK
			|| message->FindRef("directory", &ref) != B_OK)
			return B_ERROR;

		BDirectory directory(&ref);
		// store the entry-ref
		fProjectEntry.SetTo(&directory, name.String(), true);

		// Only one save ref is received so we do not need to loop.
		BFile file;
		if (file.SetTo(&directory, name.String(), B_WRITE_ONLY |
			B_CREATE_FILE | B_ERASE_FILE) != B_OK) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// Create a BNodeInfo for this file and set the MIME-type and preferred
		// app. Get and set the app signature, not sure why it's commented out.
		BNodeInfo nodeInfo(&file);
		nodeInfo.SetType(HS_PROJECT_MIME_STRING);

		// app_info info;
		// be_app->GetAppInfo(&info);
		// nodeinfo.SetPreferredApp(info.signature);

		// The project-file will be written in the endianness of the host.
		// First word of the file will mark the endianness. If its 0xFFFFFFFF
		// the file is little-endian if it is 0x00000000, the file is big-endian.
		int32 endianness = 0xFFFFFFFF;
		if (B_HOST_IS_BENDIAN == 1)
			endianness = 0x00000000;

		// Write the endianness
		if (file.Write(&endianness, sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// Write the file-id.
		int32 id = PROJECT_FILE_ID;
		if (file.Write(&id, sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// Write the number of sections that the file contains. Currently there
		// are only two sections.
		int32 sectionCount = 2;
		if (file.Write(&sectionCount, sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// First write the dimensions section. The real dimensions of the image
		// are written to the file.
		int32 marker = PROJECT_FILE_SECTION_START;
		if (file.Write(&marker, sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// Here write the section type.
		int32 type = PROJECT_FILE_DIMENSION_SECTION_ID;
		if (file.Write(&type, sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// Leave room for the section-length
		file.Seek(sizeof(int64), SEEK_CUR);

		// Here write the width and height.
		int32 width = int32(fImageView->ReturnImage()->Width());
		int32 height = int32(fImageView->ReturnImage()->Height());

		int64 bytesWritten = 0;
		bytesWritten = file.Write(&width, sizeof(int32));
		bytesWritten += file.Write(&height, sizeof(int32));

		if (bytesWritten != 2 * sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		file.Seek(-bytesWritten - sizeof(int64), SEEK_CUR);
		if (file.Write(&bytesWritten, sizeof(int64)) != sizeof(int64)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}
		file.Seek(bytesWritten, SEEK_CUR);

		marker = PROJECT_FILE_SECTION_END;
		if (file.Write(&marker, sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// Here starts the layer-section.
		marker = PROJECT_FILE_SECTION_START;
		if (file.Write(&marker, sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		id = PROJECT_FILE_LAYER_SECTION_ID;
		if (file.Write(&id, sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// Leave some room for the length.
		file.Seek(sizeof(int64),SEEK_CUR);

		// Here tell the image to write layers.
		bytesWritten = fImageView->ReturnImage()->WriteLayers(file);

		file.Seek(-bytesWritten - sizeof(int64), SEEK_CUR);
		if (file.Write(&bytesWritten, sizeof(int64)) != sizeof(int64)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}
		file.Seek(bytesWritten, SEEK_CUR);

		// Write the end-marker for the layer-section.
		marker = PROJECT_FILE_SECTION_END;
		if (file.Write(&marker, sizeof(int32)) != sizeof(int32)) {
			fImageView->UnFreeze();
			return B_ERROR;
		}

		// Now we are happily at the end of writing.
		fImageView->ResetChangeStatistics(true, false);
		fImageView->UnFreeze();

		char title[256];
		fProjectEntry.GetName(title);
		if (Lock()) {
			fImageView->SetProjectName(title);

			// This must come after the project's name has been set.
			LayerWindow::ActiveWindowChanged(this,
				fImageView->ReturnImage()->LayerList(),
				fImageView->ReturnImage()->ReturnThumbnailImage());

			// BMenuItem *item = fMenubar->FindItem(HS_SAVE_PROJECT);
			// if (item) item->SetEnabled(true);

			Unlock();
		}

		BPath path;
		fProjectEntry.GetPath(&path);

		// Also change this new path into the settings.
		SettingsServer* server = SettingsServer::Instance();
		server->AddRecentProjectPath(path.Path());

		path.GetParent(&path);
		if (path.Path() != NULL) {
			server->SetValue(SettingsServer::Application, skProjectSavePath,
				path.Path());
		}
		return B_OK;
	}
	return B_ERROR;
}


void
PaintWindow::writeAttributes(BNode& node)
{
	float zoom = 1;
	BPoint point(0.0, 0.0);

	if (fImageView && fImageView->LockLooper()) {
		point = fImageView->LeftTop();
		zoom = fImageView->getMagScale();
		fImageView->UnlockLooper();
	}

	fSettings.ReplaceFloat(skZoom, zoom);
	fSettings.ReplacePoint(skPosition, point);

	BRect frame = Frame();
	node.WriteAttr("ArtP:zoom_level", B_FLOAT_TYPE, 0, &zoom, sizeof(float));
	node.WriteAttr("ArtP:frame_rect", B_RECT_TYPE, 0, &frame, sizeof(BRect));
	node.WriteAttr("ArtP:view_position", B_POINT_TYPE, 0, &point, sizeof(BPoint));
}


void
PaintWindow::ReadAttributes(const BNode& node)
{
	if (Lock()) {
		float zoom;
		if (node.ReadAttr("ArtP:zoom_level", B_FLOAT_TYPE, 0, &zoom,
			sizeof(float)) == sizeof(float)) {
			if (fImageView)
				fImageView->setMagScale(zoom);
			fSettings.ReplaceFloat(skZoom, zoom);
		}

		BPoint position;
		if (node.ReadAttr("ArtP:view_position" ,B_POINT_TYPE, 0, &position,
			sizeof(BPoint)) == sizeof(BPoint)) {
			if (fImageView)
				fImageView->ScrollTo(position);
			fSettings.ReplacePoint(skPosition, position);
		}

		BRect frame;
		if (node.ReadAttr("ArtP:frame_rect", B_RECT_TYPE, 0, &frame,
			sizeof(BRect)) == sizeof(BRect)) {
			frame = FitRectToScreen(frame);
			MoveTo(frame.left, frame.top);
			fSettings.ReplaceRect(skFrame, frame);
			ResizeTo(frame.Width(), frame.Height());
		}

		Unlock();
	}
}


void
PaintWindow::_AddMenuItems(BMenu* menu, BString label, uint32 what,
	char shortcut, uint32 modifiers, BHandler* target, BString help)
{
	if (label != "SEPARATOR" && help != "SEPARATOR") {
		BMenuItem* item = new PaintWindowMenuItem(label,
			new BMessage(what), shortcut, modifiers, this, help);
		menu->AddItem(item);
		item->SetTarget(target);
	} else {
		menu->AddSeparatorItem();
	}
}


void
PaintWindow::_AddRecentMenuItems(BMenu* menu, const StringList* list)
{
	menu->RemoveItems(0, menu->CountItems(), true);

	BPath path;
	StringList::const_iterator it;
	for (it = list->begin(); it != list->end(); ++it) {
		entry_ref ref;
		path.SetTo((*it).String(), NULL, true);
		if (get_ref_for_path(path.Path(), &ref) == B_OK) {
			BMessage* message = new BMessage(B_REFS_RECEIVED);
			message->AddRef("refs", &ref);

			PaintWindowMenuItem* item = new PaintWindowMenuItem(path.Leaf(),
				message, 0, 0, this, path.Path());
			menu->AddItem(item);
			item->SetTarget(be_app);
		}
	}
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
			_DisableMenuItem(B_TRANSLATE("Resize to fit"));
			_DisableMenuItem(B_TRANSLATE("Zoom in"));
			_DisableMenuItem(B_TRANSLATE("Zoom out"));
			_DisableMenuItem(B_TRANSLATE("Set zoom level"));
			_DisableMenuItem(B_TRANSLATE("Set grid"));
		}	// fall through

		case MINIMAL_MENU: {
			// In this case most of the items should be disabled.
			_DisableMenuItem(B_TRANSLATE("Canvas"));
			_DisableMenuItem(B_TRANSLATE("Layer"));
			_DisableMenuItem(B_TRANSLATE("Save image as…"));
			_DisableMenuItem(B_TRANSLATE("Save project as…"));
			_DisableMenuItem(B_TRANSLATE("Edit"));
			_DisableMenuItem(B_TRANSLATE("Add-ons"));
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
