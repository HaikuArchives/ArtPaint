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

#include "PaintApplication.h"

#include "BitmapUtilities.h"
#include "BrushStoreWindow.h"
#include "ColorPalette.h"
#include "FileIdentificationStrings.h"
#include "FilePanels.h"
#include "FloaterManager.h"
#include "Image.h"
#include "ImageView.h"
#include "LayerWindow.h"
#include "ManipulatorServer.h"
#include "MessageConstants.h"
#include "PaintWindow.h"
#include "ProjectFileFunctions.h"
#include "RefFilters.h"
#include "ResourceServer.h"
#include "Settings.h"
#include "StringServer.h"
#include "ToolManager.h"
#include "ToolSelectionWindow.h"
#include "ToolSetupWindow.h"
#include "UndoQueue.h"


#include <Alert.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Clipboard.h>
#include <Directory.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <Window.h>


#include <stdio.h>
#include <string.h>


PaintApplication::PaintApplication()
	: BApplication("application/x-vnd.artpaint")
	, fImageOpenPanel(NULL)
	, fProjectOpenPanel(NULL)
	, fGlobalSettings(NULL)
{
	ResourceServer::Instantiate();

	// Some of the things in this function depend on the previously initialized
	// things, so the order may be important. This should be fixed in future.

	// create the settings
	fGlobalSettings = new global_settings();
	_ReadPreferences();

	// Set the language
	StringServer::SetLanguage(languages(fGlobalSettings->language));

	// Set the tool
	tool_manager->ChangeTool(fGlobalSettings->primary_tool);

	// Set the undo-queue to right depth
	UndoQueue::SetQueueDepth(fGlobalSettings->undo_queue_depth);

	// Read the add-ons. They will be read in another thread by the manipulator
	// server. This should be the last thing to read so that it does not
	// interfere with other reading.
	ManipulatorServer::ReadAddOns();
}


PaintApplication::~PaintApplication()
{
	if (fImageOpenPanel) {
		delete fImageOpenPanel->RefFilter();
		delete fImageOpenPanel;
	}

	delete fProjectOpenPanel;

	_WritePreferences();
	delete fGlobalSettings;

	ResourceServer::DestroyServer();
	ToolManager::DestroyToolManager();
}


void
PaintApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case HS_NEW_PAINT_WINDOW: {
			// issued from paint-window's menubar->"Window"->"New Paint Window"
			PaintWindow::CreatePaintWindow();
		}	break;

		case HS_SHOW_IMAGE_OPEN_PANEL: {
			// issued from paint-window's menubar->"File"->"Open"->"Open Image…"
			BMessage filePanelMessage(B_REFS_RECEIVED);
			if (fImageOpenPanel == NULL) {
				entry_ref ref;
				get_ref_for_path(fGlobalSettings->image_open_path, &ref);
				filePanelMessage.AddBool("from_filepanel", true);

				BMessenger app(this);
				fImageOpenPanel = new BFilePanel(B_OPEN_PANEL, &app, &ref,
					B_FILE_NODE, true, NULL, new ImageFilter());
			}

			fImageOpenPanel->SetMessage(&filePanelMessage);
			fImageOpenPanel->Window()->SetTitle(BString("ArtPaint: ")
				.Append(StringServer::ReturnString(OPEN_IMAGE_STRING)).String());
			fImageOpenPanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);

			set_filepanel_strings(fImageOpenPanel);
			fImageOpenPanel->Show();
		}	break;

		case HS_SHOW_PROJECT_OPEN_PANEL: {
			BMessage filePanelMessage(B_REFS_RECEIVED);
			if (fProjectOpenPanel == NULL) {
				entry_ref ref;
				get_ref_for_path(fGlobalSettings->project_open_path, &ref);
				filePanelMessage.AddBool("from_filepanel", true);

				BMessenger app(this);
				fProjectOpenPanel = new BFilePanel(B_OPEN_PANEL, &app, &ref,
					B_FILE_NODE);
			}

			fProjectOpenPanel->SetMessage(&filePanelMessage);
			fProjectOpenPanel->Window()->SetTitle(BString("ArtPaint: ")
				.Append(StringServer::ReturnString(OPEN_PROJECT_STRING)).String());
			fProjectOpenPanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);

			set_filepanel_strings(fProjectOpenPanel);
			fProjectOpenPanel->Show();
		}	break;

		case HS_SHOW_USER_DOCUMENTATION: {
			// issued from paint-window's menubar->"Help"->"User Documentation"
			BRoster roster;
			entry_ref mimeHandler;
			if (roster.FindApp("text/html", &mimeHandler) == B_OK) {
				BPath homePath;
				PaintApplication::HomeDirectory(homePath);

				// Force normalization of the path to check validity.
				if (homePath.Append("Documentation/", true) == B_OK) {
					const char* documentName;
					message->FindString("document", &documentName);
					if (homePath.Append(documentName, true) == B_OK) {
						BString url = "file://";
						url.Append(homePath.Path());
						const char* argv = url.String();
						roster.Launch(&mimeHandler, 1, &argv);
					}
				}
			}
		}	break;

		case B_PASTE: {
			if (be_clipboard->Lock()) {
				BMessage* data = be_clipboard->Data();
				if (data) {
					BMessage message;
					if (data->FindMessage("image/bitmap", &message) == B_OK) {
						BBitmap* pastedBitmap = new BBitmap(&message);
						if (pastedBitmap && pastedBitmap->IsValid()) {
							char name[] = "Clip 1";
							PaintWindow::CreatePaintWindow(pastedBitmap, name);
						}
					}
				}
				be_clipboard->Unlock();
			}
		}	break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


bool
PaintApplication::QuitRequested()
{
	// Here we must collect information about the window's that are still open
	// because they will be closed in BApplication::QuitRequested().
	bool layer_window_visible = fGlobalSettings->layer_window_visible;
	bool tool_setup_window_visible = fGlobalSettings->tool_setup_window_visible;
	bool tool_select_window_visible = fGlobalSettings->tool_select_window_visible;
	bool palette_window_visible = fGlobalSettings->palette_window_visible;
	bool brush_window_visible = fGlobalSettings->brush_window_visible;

	if (BApplication::QuitRequested()) {
		fGlobalSettings->layer_window_visible = layer_window_visible;
		fGlobalSettings->tool_setup_window_visible = tool_setup_window_visible;
		fGlobalSettings->tool_select_window_visible = tool_select_window_visible;
		fGlobalSettings->palette_window_visible = palette_window_visible;
		fGlobalSettings->brush_window_visible = brush_window_visible;
		return true;
	}
	return false;
}


void
PaintApplication::ReadyToRun()
{
	// Open here the ToolSelectionWindow
	if (fGlobalSettings->tool_select_window_visible)
		ToolSelectionWindow::showWindow();

	// Open here the ToolSetupWindow
	if (fGlobalSettings->tool_setup_window_visible)
		ToolSetupWindow::ShowToolSetupWindow(fGlobalSettings->setup_window_tool);

	// Test here the brush store window
	if (fGlobalSettings->brush_window_visible) {
		BrushStoreWindow* brush_window = new BrushStoreWindow();
		brush_window->Show();
	}

	if (fGlobalSettings->palette_window_visible)
		ColorPaletteWindow::showPaletteWindow(false);

	if (fGlobalSettings->layer_window_visible)
		LayerWindow::showLayerWindow();

	// Here we will open a PaintWindow if no image was loaded on startup. This
	// should be the last window opened so that it will be the active window.
	if (PaintWindow::CountPaintWindows() == 0)
		PaintWindow::CreatePaintWindow();
}


void
PaintApplication::RefsReceived(BMessage* message)
{
	// here we will determine which type of file was opened
	// and then initiate the right function for opening it
	// the files will be checked for their mime-types and some
	// also additionally for their file-identification strings
	uint32 type;
	int32 count;
	message->GetInfo("refs", &type, &count);
	if (type != B_REF_TYPE)
		return;

	for (int32 i = --count; i >= 0;  --i) {
		entry_ref ref;
		if (message->FindRef("refs", i, &ref) != B_OK)
			continue;

		BFile file;
		if (file.SetTo(&ref, B_READ_ONLY) != B_OK )
			continue;

		char mimeType[B_MIME_TYPE_LENGTH];
		memset(mimeType, 0, B_MIME_TYPE_LENGTH);
		BNodeInfo(&file).GetType(mimeType);

		BPath path;
		BEntry(&ref).GetPath(&path);

		// here compare the mime-type to all possible mime types for the
		// types we have created, type strings are defined in
		// FileIdentificationStrings.h
		if ((strcmp(mimeType, HS_PALETTE_MIME_STRING) == 0)
			|| (strcmp(mimeType, _OLD_HS_PALETTE_MIME_STRING) == 0)) {
			// Call the static showPaletteWindow-function. Giving it an
			// argument containing refs makes it also load a palette.
			BMessage message(HS_PALETTE_OPEN_REFS);
			message.AddRef("refs",&ref);
			ColorPaletteWindow::showPaletteWindow(&message);
		}
		else if ((strcmp(mimeType, HS_PROJECT_MIME_STRING) == 0)
			|| (strcmp(mimeType, _OLD_HS_PROJECT_MIME_STRING) == 0)) {
			fGlobalSettings->insert_recent_project_path(path.Path());
			_StorePath(message, ref, fGlobalSettings->project_open_path);
			if (_ReadProject(file, ref) != B_OK)
				fGlobalSettings->fRecentProjectPaths.remove(path.Path());
		}
		else if (strncmp(mimeType, "image/", 6) == 0 || strcmp(mimeType, "") == 0) {
			// The file was not one of ArtPaint's file types. Perhaps it is
			// an image-file. Try to read it using the Translation-kit.
			BBitmap* bitmap = BTranslationUtils::GetBitmapFile(path.Path());
			if (bitmap) {
				// The returned bitmap might be in 8-bit format. If that is
				// the case, we should convert to 32-bit.
				bitmap = BitmapUtilities::ConvertColorSpace(bitmap, B_RGBA32);
				BitmapUtilities::FixMissingAlpha(bitmap);

				translator_info orgInfo;
				translator_info testInfo;

				BTranslatorRoster* roster = BTranslatorRoster::Default();
				roster->Identify(&file, NULL, &orgInfo);

				// Check if a reverse translation can be done.
				BBitmapStream imageBuffer(bitmap);
				status_t status = roster->Identify(&imageBuffer, NULL,
					&testInfo, 0, NULL, orgInfo.type);
				imageBuffer.DetachBitmap(&bitmap);

				if (status != B_OK) { // Reverse translation is not possible
					orgInfo.type = 0;
					testInfo.translator = 0;
				}

				fGlobalSettings->insert_recent_image_path(path.Path());
				_StorePath(message, ref, fGlobalSettings->image_open_path);

				PaintWindow* window = PaintWindow::CreatePaintWindow(bitmap,
					ref.name, orgInfo.type, ref, testInfo.translator);
				window->ReadAttributes(file);
			} else {
				char text[255];
				sprintf(text,
					StringServer::ReturnString(UNSUPPORTED_FILE_TYPE_STRING),
					ref.name);
				_ShowAlert(text);
			}
		} else {
			char text[255];
			sprintf(text,
				StringServer::ReturnString(UNSUPPORTED_FILE_TYPE_STRING),
				ref.name);
			_ShowAlert(text);
		}
	}
}


// These functions get and set color for particular button. The ability to have
// different colors for each button is removed and thus only foreground and
// background-color can be defined.
rgb_color
PaintApplication::Color(bool foreground) const
{
	// here we return the tool that corresponds to button
	if (foreground)
		return fGlobalSettings->primary_color;
	return fGlobalSettings->secondary_color;
}


void
PaintApplication::SetColor(rgb_color color, bool foreground)
{
	if (foreground)
		fGlobalSettings->primary_color = color;
	else
		fGlobalSettings->secondary_color = color;
}


void
PaintApplication::_ReadPreferences()
{
	bool createDefaultTools = true;
	bool createDefaultColorset = true;

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("ArtPaint");
		BDirectory settingsDir(path.Path());

		HomeDirectory(path);
		path.Append("settings");
		BDirectory spareDir(path.Path());

		bool spareDirExists = spareDir.InitCheck() == B_OK;
		if (settingsDir.InitCheck() == B_OK || spareDirExists) {
			BEntry entry;
			status_t status = settingsDir.FindEntry("brushes", &entry, true);
			if ((status != B_OK) && spareDirExists)
				status = spareDir.FindEntry("brushes", &entry, true);

			if (status == B_OK) {
				BFile brushes(&entry, B_READ_ONLY);
				status = brushes.InitCheck();
				if (status == B_OK)
					BrushStoreWindow::readBrushes(brushes);
			}

			if (status != B_OK)
				;// We might create some default brushes.

			status = settingsDir.FindEntry("main_preferences",&entry, true);
			if ((status != B_OK) && spareDirExists)
				status = spareDir.FindEntry("main_preferences",&entry, true);

			if (status == B_OK) {
				BFile mainPreferences(&entry, B_READ_ONLY);
				status = mainPreferences.InitCheck();
				if (status == B_OK)
					status = fGlobalSettings->read_from_file(mainPreferences);
			}

			if (status != B_OK)
				;// Settings have the default values.

			// Here set the language for the StringServer
			StringServer::SetLanguage(languages(fGlobalSettings->language));

			// Create a tool-manager object. Depends on the language being set.
			ToolManager::CreateToolManager();

			status = settingsDir.FindEntry("tool_preferences", &entry, true);
			if ((status != B_OK) && spareDirExists)
				status = spareDir.FindEntry("tool_preferences", &entry, true);

			if (status == B_OK) {
				BFile tools(&entry, B_READ_ONLY);
				if (tools.InitCheck() == B_OK) {
					if (tool_manager->ReadToolSettings(tools) == B_OK)
						createDefaultTools = false;
				}
			}

			status = settingsDir.FindEntry("color_preferences", &entry, true);
			if ((status != B_OK) && spareDirExists)
				status = spareDir.FindEntry("color_preferences", &entry, true);

			if (status == B_OK) {
				BFile colors(&entry, B_READ_ONLY);
				if (colors.InitCheck() == B_OK) {
					if (ColorSet::readSets(colors) == B_OK)
						createDefaultColorset = false;
				}
			}
		}
	}

	if (createDefaultColorset) {
		// We might look into apps directory and palette directory for some
		// colorsets (TODO: this looks starnge, maybe implement static init)
		new ColorSet(16);
	}

	// Create a tool-manager object.
	if (createDefaultTools)
		ToolManager::CreateToolManager();
}


void
PaintApplication::_WritePreferences()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		BDirectory settingsDir(path.Path());
		status_t status = settingsDir.CreateDirectory("./ArtPaint", &settingsDir);
		if (status == B_FILE_EXISTS)
			status = settingsDir.SetTo(&settingsDir, "ArtPaint/");

		if (status == B_OK) {
			// Here we create several preferences files. One for each internal
			// manipulator, main preferences file and brushes and palettes
			// files. Later we might add even more files for example a file for
			// each tool. Now all the tool-settings are stored in one file.
			// Actually the manipulator preference files will be created by
			// ManipulatorServer.
			BFile brushes;
			if (settingsDir.CreateFile("brushes", &brushes, false) == B_OK)
				BrushStoreWindow::writeBrushes(brushes);

			BFile mainPreferences;
			if (settingsDir.CreateFile("main_preferences",
				&mainPreferences, false) == B_OK) {
					fGlobalSettings->write_to_file(mainPreferences);
			}

			BFile tools;
			if (settingsDir.CreateFile("tool_preferences", &tools, false) == B_OK)
				tool_manager->WriteToolSettings(tools);

			BFile colors;
			if (settingsDir.CreateFile("color_preferences", &colors, false) == B_OK)
				ColorSet::writeSets(colors);
		} else {
			fprintf(stderr, "Could not write preferences.\n");
		}
	}
}


status_t
PaintApplication::_ReadProject(BFile& file, entry_ref& ref)
{
	// We should read the project file and open a new window. Read the first
	// four bytes from the file to see if it is really a project file.
	int32 lendian;
	if (file.Read(&lendian, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	bool isLittleEndian = true;
	if (lendian == 0x00000000) {
		isLittleEndian = false;
	} else if (uint32(lendian) == 0xFFFFFFFF) {
		isLittleEndian = true;
	} else {
		return B_ERROR;
	}

	int32 fileId;
	if (file.Read(&fileId, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	if (isLittleEndian)
		fileId = B_LENDIAN_TO_HOST_INT32(fileId);
	else
		fileId = B_BENDIAN_TO_HOST_INT32(fileId);

	file.Seek(0,SEEK_SET);
	if (fileId != PROJECT_FILE_ID)
		return _ReadProjectOldStyle(file,ref);

	// This is the new way of reading a structured project file. The possibility
	// to read old project files is still maintained through ReadProjectOldStyle.
	int64 length = FindProjectFileSection(file, PROJECT_FILE_DIMENSION_SECTION_ID);
	if (length != (2 * sizeof(int32)))
		return B_ERROR;

	int32 width;
	if (file.Read(&width, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	int32 height;
	if (file.Read(&height, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	if (isLittleEndian) {
		width = B_LENDIAN_TO_HOST_INT32(width);
		height = B_LENDIAN_TO_HOST_INT32(height);
	} else {
		width = B_BENDIAN_TO_HOST_INT32(width);
		height = B_BENDIAN_TO_HOST_INT32(height);
	}

	// Create a paint-window using the width and height
	PaintWindow* paintWindow = PaintWindow::CreatePaintWindow(NULL, ref.name);

	paintWindow->OpenImageView(width, height);
	// Then read the layer-data. Rewind the file and put the image-view to read
	// the data.
	ImageView* image_view = paintWindow->ReturnImageView();
	image_view->ReturnImage()->ReadLayers(file);

	// This must be before the image-view is added
	paintWindow->SetProjectEntry(BEntry(&ref, true));
	paintWindow->AddImageView();

	// As last thing read the attributes from the file
	paintWindow->ReadAttributes(file);

	return B_OK;
}


status_t
PaintApplication::_ReadProjectOldStyle(BFile& file, entry_ref& ref)
{
// This old version of file reading will be copied to the conversion utility.
	// The structure of a project file is following.
	// 	1.	Identification string HS_PROJECT_ID_STRING
	//	2.	ArtPaint version number ARTPAINT_VERSION
	//	3.	The length of stored window_settings struct
	//	4.	The settings for the project (a window_settings struct stored that is)
	//	5.	The width and height of image (in pixels) stored as uint32s
	//	6.	How many layers are stored.
	//	7.	Data for layers. Will be read by layer's readLayer.
	// We can assume that the project_file is a valid file.

	char text[B_FILE_NAME_LENGTH];
	sprintf(text, "Project file %s structure corrupted.", ref.name);

	char fileId[B_FILE_NAME_LENGTH];
	ssize_t size = file.Read(fileId, strlen(HS_PROJECT_ID_STRING));
	if (size < 0 || uint32(size) != strlen(HS_PROJECT_ID_STRING)) {
		_ShowAlert(text);
		return B_ERROR;
	}

	fileId[size] = '\0';
	if (strcmp(fileId, HS_PROJECT_ID_STRING) != 0) {
		_ShowAlert(text);
		return B_ERROR;
	}

	// The version number of program with which this was written. We actually do
	// not care about the version number, because it is known to be 1.0.0
	uint32 versionNumber;
	if (file.Read(&versionNumber, sizeof(uint32)) != sizeof(uint32)) {
		_ShowAlert(text);
		return B_ERROR;
	}
	versionNumber = B_BENDIAN_TO_HOST_INT32(versionNumber);

	// If the settings are of different length than the settings_struct, we
	// cannot use them.
	int32 settingsLength;
	if (file.Read(&settingsLength, sizeof(int32)) != sizeof(int32)) {
		_ShowAlert(text);
		return B_ERROR;
	}
	settingsLength = B_BENDIAN_TO_HOST_INT32(settingsLength);
	// We skip the settings, no need to bother converting them
	file.Seek(settingsLength, SEEK_CUR);

	uint32 width, height;
	if (file.Read(&width, sizeof(uint32)) != sizeof(uint32)
		|| file.Read(&height,sizeof(uint32)) != sizeof(uint32)) {
		_ShowAlert(text);
		return B_ERROR;
	}

	width = B_BENDIAN_TO_HOST_INT32(width);
	height = B_BENDIAN_TO_HOST_INT32(height);

	// We should create a PaintWindow and also an ImageView for it.
	PaintWindow* window = PaintWindow::CreatePaintWindow(NULL, ref.name);
	window->OpenImageView(width, height);

	// Read the layers from the file. First read how many layers there are.
	int32 layerCount;
	if (file.Read(&layerCount,sizeof(int32)) == sizeof(int32)) {
		layerCount = B_BENDIAN_TO_HOST_INT32(layerCount);
		Image* image = window->ReturnImageView()->ReturnImage();
		if (image->ReadLayersOldStyle(file, layerCount) == B_OK) {
			// This must be before the image-view is added
			window->SetProjectEntry(BEntry(&ref, true));
			window->AddImageView();
			window->ReadAttributes(file);
			return B_OK;
		}
	}

	_ShowAlert(text);

	delete window->ReturnImageView();
	window->Lock();
	window->Quit();

	return B_ERROR;
}


void
PaintApplication::HomeDirectory(BPath &path)
{
	// this from the newsletter 81
	app_info info;
	be_app->GetAppInfo(&info);
	path = _GetParentPath(info.ref);
}


void
PaintApplication::_ShowAlert(const BString& text)
{
	BAlert* alert = new BAlert("title", text.String(),
		StringServer::ReturnString(OK_STRING), NULL, NULL, B_WIDTH_AS_USUAL,
		B_WARNING_ALERT);
	alert->Go();
}


void
PaintApplication::_StorePath(const BMessage* message, const entry_ref& ref,
	char* target)
{
	bool storePath = false;
	if (message->FindBool("from_filepanel", &storePath) == B_OK) {
		if (storePath) {
			BPath path = _GetParentPath(ref);
			if (path.InitCheck() == B_OK && path.Path() != NULL)
				strcpy(target, path.Path());
		}
	}
}


/*static*/
BPath
PaintApplication::_GetParentPath(const entry_ref& entryRef)
{
	BEntry entry(&entryRef);

	BPath path;
	entry.GetPath(&path);
	path.GetParent(&path);

	return path;
}

int
main(int argc, char* argv[])
{
	PaintApplication* paintApp = new PaintApplication();
	if (paintApp) {
		paintApp->Run();
		delete paintApp;
	}

	return B_OK;
}


filter_result
AppKeyFilterFunction(BMessage* message,BHandler** handler, BMessageFilter*)
{
	const char* bytes;
	if ((!(modifiers() & B_COMMAND_KEY)) && (!(modifiers() & B_CONTROL_KEY))) {
		if (message->FindString("bytes", &bytes) == B_OK) {
			if (bytes[0] == B_TAB) {
				BView* view = dynamic_cast<BView*>(*handler);
				if (view && !(view->Flags() & B_NAVIGABLE)) {
					if (dynamic_cast<BTextView*>(*handler) == NULL)
						FloaterManager::ToggleFloaterVisibility();
				} else {
					FloaterManager::ToggleFloaterVisibility();
				}
			}
		}
	}
	return B_DISPATCH_MESSAGE;
}
