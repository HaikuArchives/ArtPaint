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
#include "SettingsServer.h"
#include "ToolManager.h"
#include "ToolSelectionWindow.h"
#include "ToolSetupWindow.h"
#include "UndoQueue.h"


#include <AboutWindow.h>
#include <Alert.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <Directory.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <Window.h>


#include <stdio.h>
#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PaintApplication"


PaintApplication::PaintApplication()
	:
	BApplication("application/x-vnd.artpaint"),
	fImageOpenPanel(NULL),
	fProjectOpenPanel(NULL),
	fShuttingDown(false)
{
	SettingsServer::Instantiate();
	ResourceServer::Instantiate();
	ManipulatorServer::Instantiate();

	// Some of the things in this function depend on the previously initialized
	// things, so the order may be important. This should be fixed in future.
	BMessage settings;
	if (SettingsServer* server = SettingsServer::Instance())
		server->GetApplicationSettings(&settings);

	// create the settings
	_ReadPreferences();

	int32 tool = FREE_LINE_TOOL;
	settings.FindInt32(skTool, &tool);
	ToolManager::Instance().ChangeTool(tool);

	int32 depth = 20;
	settings.FindInt32(skUndoQueueDepth, &depth);
	UndoQueue::SetQueueDepth(depth);
}


PaintApplication::~PaintApplication()
{
	if (fImageOpenPanel) {
		delete fImageOpenPanel->RefFilter();
		delete fImageOpenPanel;
	}

	delete fProjectOpenPanel;

	_WritePreferences();

	ToolManager::DestroyToolManager();

	ResourceServer::DestroyServer();
	SettingsServer::DestroyServer();
	ManipulatorServer::DestroyServer();
}


void
PaintApplication::AboutRequested()
{
	const char* authors[] = {
		"Heikki Suhonen",
		"Augustin Cavalier",
		"CodeforEvolution",
		"Dale Cieslak",
		"Humdinger",
		"Jérôme Duval",
		"Pete Goodeve",
		"Puck Meerburg",
		"julun",
		"stargater",
		NULL
	};

	BAboutWindow* aboutW = new BAboutWindow(B_TRANSLATE_SYSTEM_NAME("ArtPaint"),
		"application/x-vnd.artpaint");
	aboutW->AddDescription(
		B_TRANSLATE("ArtPaint is a painting and image-processing program for Haiku.")),
		aboutW->AddCopyright(2003, "Heikki Suhonen");
	aboutW->AddAuthors(authors);
	aboutW->Show();
}


void
PaintApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case HS_NEW_PAINT_WINDOW:
		{
			// issued from paint-window's menubar->"Window"->"New Paint Window"
			PaintWindow* window = PaintWindow::CreatePaintWindow();
			if (window)
				window->Show();
			break;
		}
		case HS_SHOW_IMAGE_OPEN_PANEL:
		{
			// issued from paint-window's menubar->"File"->"Open"->"Open Image…"
			BMessage filePanelMessage(B_REFS_RECEIVED);
			if (fImageOpenPanel == NULL) {
				entry_ref ref;
				BMessage settings;
				if (SettingsServer* server = SettingsServer::Instance())
					server->GetApplicationSettings(&settings);

				get_ref_for_path(settings.FindString(skImageOpenPath), &ref);
				filePanelMessage.AddBool("from_filepanel", true);

				BMessenger app(this);
				fImageOpenPanel = new BFilePanel(
					B_OPEN_PANEL, &app, &ref, B_FILE_NODE, true, NULL, new ImageFilter());
			}

			fImageOpenPanel->SetMessage(&filePanelMessage);
			fImageOpenPanel->Window()->SetTitle(
				B_TRANSLATE("ArtPaint: Open image" B_UTF8_ELLIPSIS));
			fImageOpenPanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);

			set_filepanel_strings(fImageOpenPanel);
			fImageOpenPanel->Show();
			break;
		}
		case HS_SHOW_PROJECT_OPEN_PANEL:
		{
			BMessage filePanelMessage(B_REFS_RECEIVED);
			if (fProjectOpenPanel == NULL) {
				entry_ref ref;
				BMessage settings;
				if (SettingsServer* server = SettingsServer::Instance())
					server->GetApplicationSettings(&settings);

				get_ref_for_path(settings.FindString(skProjectOpenPath), &ref);
				filePanelMessage.AddBool("from_filepanel", true);

				BMessenger app(this);
				fProjectOpenPanel = new BFilePanel(B_OPEN_PANEL, &app, &ref, B_FILE_NODE);
			}

			fProjectOpenPanel->SetMessage(&filePanelMessage);
			fProjectOpenPanel->Window()->SetTitle(
				B_TRANSLATE("ArtPaint: Open project" B_UTF8_ELLIPSIS));
			fProjectOpenPanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);

			set_filepanel_strings(fProjectOpenPanel);
			fProjectOpenPanel->Show();
			break;
		}
		case HS_SHOW_USER_DOCUMENTATION:
		{
			// issued from paint-window's menubar->"ArtPaint"->"User documentation"
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
			break;
		}
		case B_PASTE:
		{
			if (be_clipboard->Lock()) {
				BMessage* data = be_clipboard->Data();
				if (data) {
					BMessage message;
					if (data->FindMessage("image/bitmap", &message) == B_OK) {
						BBitmap* pastedBitmap = new BBitmap(&message);
						if (pastedBitmap && pastedBitmap->IsValid()) {
							char name[] = "Clip 1";
							PaintWindow* window
								= PaintWindow::CreatePaintWindow(pastedBitmap, name);
							if (window)
								window->Show();
						}
					}
				}
				be_clipboard->Unlock();
			}
			break;
		}
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
		{
			RefsReceived(message);
			break;
		}
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


bool
PaintApplication::ShuttingDown() const
{
	return fShuttingDown;
}


bool
PaintApplication::QuitRequested()
{
	fShuttingDown = true;

	BMessage settings;
	if (SettingsServer* server = SettingsServer::Instance())
		server->GetApplicationSettings(&settings);

	// Here we must collect information about the window's that are still open
	// because they will be closed in BApplication::QuitRequested().
	bool layer_window_visible = settings.FindBool(skLayerWindowVisible);
	bool tool_setup_window_visible = settings.FindBool(skToolSetupWindowVisible);
	bool tool_select_window_visible = settings.FindBool(skSelectToolWindowVisible);
	bool palette_window_visible = settings.FindBool(skPaletteWindowVisible);
	bool brush_window_visible = settings.FindBool(skBrushWindowVisible);

	if (BApplication::QuitRequested()) {
		if (SettingsServer* server = SettingsServer::Instance()) {
			server->SetValue(
				SettingsServer::Application, skToolSetupWindowVisible, tool_setup_window_visible);
			server->SetValue(
				SettingsServer::Application, skSelectToolWindowVisible, tool_select_window_visible);
			server->SetValue(
				SettingsServer::Application, skPaletteWindowVisible, palette_window_visible);
			server->SetValue(
				SettingsServer::Application, skBrushWindowVisible, brush_window_visible);
			server->SetValue(
				SettingsServer::Application, skLayerWindowVisible, layer_window_visible);
		}
		return true;
	}
	return false;
}


void
PaintApplication::ReadyToRun()
{
	_InstallMimeType();

	BMessage settings;
	if (SettingsServer* server = SettingsServer::Instance())
		server->GetApplicationSettings(&settings);

	bool visible = true;
	settings.FindBool(skSelectToolWindowVisible, &visible);
	if (visible)
		ToolSelectionWindow::showWindow();

	visible = true;
	settings.FindBool(skToolSetupWindowVisible, &visible);
	if (visible)
		ToolSetupWindow::ShowToolSetupWindow(settings.FindInt32(skTool));

	visible = true;
	settings.FindBool(skBrushWindowVisible, &visible);
	if (visible) {
		BrushStoreWindow* brush_window = new BrushStoreWindow();
		brush_window->Show();
	}

	visible = true;
	settings.FindBool(skPaletteWindowVisible, &visible);
	if (visible)
		ColorPaletteWindow::showPaletteWindow(); // TODO: was (false)

	visible = true;
	settings.FindBool(skLayerWindowVisible, &visible);
	if (visible)
		LayerWindow::showLayerWindow();

	// Here we will open a PaintWindow if no image was loaded on startup. This
	// should be the last window opened so that it will be the active window.
	if (PaintWindow::CountPaintWindows() == 0) {
		PaintWindow* window = PaintWindow::CreatePaintWindow();
		if (window)
			window->Show();
	}
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

	for (int32 i = --count; i >= 0; --i) {
		entry_ref ref;
		if (message->FindRef("refs", i, &ref) != B_OK)
			continue;

		BFile file;
		if (file.SetTo(&ref, B_READ_ONLY) != B_OK)
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
			message.AddRef("refs", &ref);
			ColorPaletteWindow::showPaletteWindow(&message);
		} else if ((strcmp(mimeType, HS_PROJECT_MIME_STRING) == 0)
			|| (strcmp(mimeType, _OLD_HS_PROJECT_MIME_STRING) == 0)) {

			SettingsServer* server = SettingsServer::Instance();
			if (server) {
				server->AddRecentProjectPath(path.Path());
				server->SetValue(
					SettingsServer::Application, skProjectOpenPath, _OpenPath(message, ref));
			}

			if (_ReadProject(file, ref) != B_OK && server)
				server->RemoveRecentProjectPath(path.Path());
		} else if (strncmp(mimeType, "image/", 6) == 0 || strcmp(mimeType, "") == 0) {
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
				status_t status
					= roster->Identify(&imageBuffer, NULL, &testInfo, 0, NULL, orgInfo.type);
				imageBuffer.DetachBitmap(&bitmap);

				if (status != B_OK) { // Reverse translation is not possible
					orgInfo.type = 0;
					testInfo.translator = 0;
				}

				if (SettingsServer* server = SettingsServer::Instance()) {
					server->AddRecentImagePath(path.Path());
					server->SetValue(
						SettingsServer::Application, skImageOpenPath, _OpenPath(message, ref));
				}

				PaintWindow* window = PaintWindow::CreatePaintWindow(
					bitmap, ref.name, orgInfo.type, ref, testInfo.translator);
				if (window) {
					window->ReadAttributes(file);
					window->Show();
				}
			} else {
				BString text = B_TRANSLATE("The file '%file%' is of unsupported type. "
					"You could try installing a translator for it if possible.");
				text.ReplaceAll("%file%", ref.name);
				_ShowAlert(text);
			}
		} else {
			BString text = B_TRANSLATE("The file '%file%' is of unsupported type. "
				"You could try installing a translator for it if possible.");
			text.ReplaceAll("%file%", ref.name);
			_ShowAlert(text);
		}
	}
}


rgb_color
PaintApplication::Color(bool foreground) const
{
	rgb_color primary = {0, 0, 0, 255};
	rgb_color secondary = {255, 255, 255, 255};

	rgb_color* color = foreground ? &primary : &secondary;
	if (SettingsServer* server = SettingsServer::Instance()) {
		BMessage settings;
		server->GetApplicationSettings(&settings);

		ssize_t dataSize;
		const rgb_color* data;
		if (settings.FindData((foreground ? skPrimaryColor : skSecondaryColor), B_RGB_COLOR_TYPE,
				(const void**)&data, &dataSize) == B_OK) {
			if (dataSize == sizeof(rgb_color))
				memcpy(color, data, sizeof(rgb_color));
		}
	}

	return *color;
}


void
PaintApplication::SetColor(rgb_color color, bool foreground)
{
	if (SettingsServer* server = SettingsServer::Instance()) {
		BString field = foreground ? skPrimaryColor : skSecondaryColor;
		server->SetValue(
			SettingsServer::Application, field, B_RGB_COLOR_TYPE, &color, sizeof(rgb_color));
	}
}


void
PaintApplication::_InstallMimeType()
{
	// install mime type of documents
	BMimeType mime(HS_PROJECT_MIME_STRING);
	if (mime.InitCheck() < B_OK)
		return;

	BString snifferRule;
	if (mime.IsInstalled() && mime.GetSnifferRule(&snifferRule) == B_OK && snifferRule.Length() > 0)
		return;

	mime.Delete();

	status_t ret = mime.Install();
	if (ret < B_OK) {
		fprintf(stderr, "Could not install mime type '" HS_PROJECT_MIME_STRING "': %s.\n",
			strerror(ret));
		return;
	}

	// set preferred app
	if (mime.SetPreferredApp("application/x-vnd.artpaint") < B_OK)
		fprintf(stderr, "Could not set preferred app!\n");

	// set descriptions
	if (mime.SetShortDescription(B_TRANSLATE_COMMENT(
			"ArtPaint project", "MIME type short description"))
		< B_OK)
		fprintf(stderr, "Could not set short description of mime type!\n");
	if (mime.SetLongDescription(B_TRANSLATE_COMMENT(
			"ArtPaint project format containing layers etc.", "MIME type long description"))
		!= B_OK)
		fprintf(stderr, "Could not set long description of mime type!\n");

	// set sniffer rule
	/*	According to Pete Goodeve's investigation of ArtPaint's project file format,
		this seems to be what's at the start of every file:
		0x ffffffff   01010101   02000000    02020202    00110011     08000000   00000000
		   little     file ID    number of   section     dimension    section    data?
		   endian                sections    start       section ID   length
	*/
	snifferRule = "0.50 ([4] 0x01010101) ([12] 0x0202020200110011)";
	if (mime.SetSnifferRule(snifferRule.String()) < B_OK) {
		BString parseError;
		BMimeType::CheckSnifferRule(snifferRule.String(), &parseError);
		fprintf(stderr, "Could not set sniffer rule of mime type: %s\n", parseError.String());
	}

	// set document icons
	BResources* resources = AppResources();
		// does not need to be freed (belongs to BApplication base)
	if (resources == NULL) {
		fprintf(stderr, "Could not find app resources.\n");
		return;
	}

	size_t size;
	const void* iconData;

	iconData = resources->LoadResource('VICN', HS_PROJECT_MIME_STRING, &size);
	if (iconData && mime.SetIcon((uint8*)iconData, size) < B_OK)
		fprintf(stderr, "Could not set vector icon of mime type.\n");
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
				; // We might create some default brushes.

			// Create a tool-manager object. Depends on the language being set.
			ToolManager::CreateToolManager();

			status = settingsDir.FindEntry("tool_preferences", &entry, true);
			if ((status != B_OK) && spareDirExists)
				status = spareDir.FindEntry("tool_preferences", &entry, true);

			if (status == B_OK) {
				BFile tools(&entry, B_READ_ONLY);
				if (tools.InitCheck() == B_OK) {
					if (ToolManager::Instance().ReadToolSettings(tools) == B_OK)
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

			BFile tools;
			if (settingsDir.CreateFile("tool_preferences", &tools, false) == B_OK)
				ToolManager::Instance().WriteToolSettings(tools);

			BFile colors;
			if (settingsDir.CreateFile("color_preferences", &colors, false) == B_OK)
				ColorSet::writeSets(colors);
		} else
			fprintf(stderr, "Could not write preferences.\n");
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
	if (lendian == 0x00000000)
		isLittleEndian = false;
	else if (uint32(lendian) == 0xFFFFFFFF)
		isLittleEndian = true;
	else
		return B_ERROR;

	int32 fileId;
	if (file.Read(&fileId, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	if (isLittleEndian)
		fileId = B_LENDIAN_TO_HOST_INT32(fileId);
	else
		fileId = B_BENDIAN_TO_HOST_INT32(fileId);

	file.Seek(0, SEEK_SET);
	if (fileId != PROJECT_FILE_ID)
		return _ReadProjectOldStyle(file, ref);

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
	if (paintWindow) {
		paintWindow->OpenImageView(width, height);
		// Then read the layer-data. Rewind the file and put the image-view to
		// read the data.
		ImageView* image_view = paintWindow->ReturnImageView();
		image_view->ReturnImage()->ReadLayers(file);

		// This must be before the image-view is added
		paintWindow->SetProjectEntry(BEntry(&ref, true));
		paintWindow->AddImageView();

		// As last thing read the attributes from the file
		paintWindow->ReadAttributes(file);
		paintWindow->Show();
		return B_OK;
	}
	return B_ERROR;
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
	sprintf(text, "Project file '%s' structure corrupted.", ref.name);

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
		|| file.Read(&height, sizeof(uint32)) != sizeof(uint32)) {
		_ShowAlert(text);
		return B_ERROR;
	}

	width = B_BENDIAN_TO_HOST_INT32(width);
	height = B_BENDIAN_TO_HOST_INT32(height);

	// We should create a PaintWindow and also an ImageView for it.
	PaintWindow* window = PaintWindow::CreatePaintWindow(NULL, ref.name);
	if (window) {
		window->OpenImageView(width, height);

		// Read the layers from the file. First read how many layers there are.
		int32 layerCount;
		if (file.Read(&layerCount, sizeof(int32)) == sizeof(int32)) {
			layerCount = B_BENDIAN_TO_HOST_INT32(layerCount);
			Image* image = window->ReturnImageView()->ReturnImage();
			if (image->ReadLayersOldStyle(file, layerCount) == B_OK) {
				// This must be before the image-view is added
				window->SetProjectEntry(BEntry(&ref, true));
				window->AddImageView();
				window->ReadAttributes(file);
				window->Show();
				return B_OK;
			}
		}

		_ShowAlert(text);

		delete window->ReturnImageView();
		window->Lock();
		window->Quit();
	}
	return B_ERROR;
}


void
PaintApplication::HomeDirectory(BPath& path)
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
		B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->Go();
}


const char*
PaintApplication::_OpenPath(const BMessage* message, const entry_ref& ref)
{
	bool storePath = false;
	if (message->FindBool("from_filepanel", &storePath) == B_OK) {
		if (storePath) {
			BPath path = _GetParentPath(ref);
			if (path.InitCheck() == B_OK && path.Path() != NULL)
				return path.Path();
		}
	}
	return "";
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
