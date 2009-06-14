/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#define DEBUG 1

#include <Alert.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Directory.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <PictureButton.h>
#include <Path.h>
#include <Roster.h>
#include <stdio.h>
#include <string.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <Window.h>
#include <Clipboard.h>


#include "FloaterManager.h"
#include "BitmapUtilities.h"
#include "PaintApplication.h"
#include "PaintWindow.h"
#include "MessageConstants.h"
//#include "Tools.h"
#include "ColorPalette.h"
#include "FileIdentificationStrings.h"
#include "ToolImages.h"
#include "ToolSelectionWindow.h"
#include "ToolSetupWindow.h"
//#include "DrawingTools.h"
#include "LayerWindow.h"
#include "RefFilters.h"
#include "FilePanels.h"
#include "ImageView.h"
#include "Settings.h"
#include "HSStack.h"
#include "BrushStoreWindow.h"
#include "ManipulatorServer.h"
#include "ProjectFileFunctions.h"
#include "StringServer.h"
#include "ToolManager.h"
#include "UndoQueue.h"
#include "UtilityClasses.h"
#include "Image.h"


// application constructor function
PaintApplication::PaintApplication()
	: BApplication("application/x-vnd.hsuhonen-artpaint")
	, image_open_panel(NULL)
	, project_open_panel(NULL)
	, settings(NULL)
{
	// Some of the things in this function depend on the previously initialized
	// things, so the order may be important. This should be fixed in future.

	// create the settings
	settings = new global_settings();
	readPreferences();

	// Set the language
	StringServer::SetLanguage((languages)settings->language);

	// Set the tool
	tool_manager->ChangeTool(settings->primary_tool);

	// Set the undo-queue to right depth
	UndoQueue::SetQueueDepth(Settings()->undo_queue_depth);

//	// the first untitled window will have number 1
//	untitled_window_number = 1;

	// create the tool-images here
	ToolImages::createToolImages();

	// Read the add-ons. They will be read in another thread by the manipulator
	// server. This should be the last thing to read so that it does not
	// interfere with other reading.
	ManipulatorServer::ReadAddOns();
}


PaintApplication::~PaintApplication()
{
	if (image_open_panel) {
		delete image_open_panel->RefFilter();
		delete image_open_panel;
	}

	delete project_open_panel;

	writePreferences();
	delete settings;

	ToolManager::DestroyToolManager();
}


void
PaintApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case HS_NEW_PAINT_WINDOW: {
			// issued from paint-window's menubar->"Window"->"New Paint Window"
			PaintWindow::createPaintWindow();
		}	break;

		case HS_SHOW_IMAGE_OPEN_PANEL: {
			// issued from paint-window's menubar->"File"->"Open"->"Open Image…"
			BMessage filePanelMessage(B_REFS_RECEIVED);
			if (image_open_panel == NULL) {
				entry_ref ref;
				get_ref_for_path(settings->image_open_path, &ref);
				filePanelMessage.AddBool("from_filepanel", true);

				BMessenger app(this);
				image_open_panel = new BFilePanel(B_OPEN_PANEL, &app, &ref,
					B_FILE_NODE, true, NULL, new ImageFilter());
			}

			image_open_panel->SetMessage(&filePanelMessage);
			image_open_panel->Window()->SetTitle(BString("ArtPaint: ")
				.Append(StringServer::ReturnString(OPEN_IMAGE_STRING)).String());
			image_open_panel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);

			set_filepanel_strings(image_open_panel);
			image_open_panel->Show();
		}	break;

		case HS_SHOW_PROJECT_OPEN_PANEL: {
			BMessage filePanelMessage(B_REFS_RECEIVED);
			if (project_open_panel == NULL) {
				entry_ref ref;
				get_ref_for_path(settings->project_open_path, &ref);
				filePanelMessage.AddBool("from_filepanel", true);

				BMessenger app(this);
				project_open_panel = new BFilePanel(B_OPEN_PANEL, &app, &ref,
					B_FILE_NODE);
			}

			project_open_panel->SetMessage(&filePanelMessage);
			project_open_panel->Window()->SetTitle(BString("ArtPaint: ")
				.Append(StringServer::ReturnString(OPEN_PROJECT_STRING)).String());
			project_open_panel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);

			set_filepanel_strings(project_open_panel);
			project_open_panel->Show();
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
							PaintWindow::createPaintWindow(pastedBitmap, name);
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
	bool layer_window_visible = settings->layer_window_visible;
	bool tool_setup_window_visible = settings->tool_setup_window_visible;
	bool tool_select_window_visible = settings->tool_select_window_visible;
	bool palette_window_visible = settings->palette_window_visible;
	bool brush_window_visible = settings->brush_window_visible;

	if (BApplication::QuitRequested()) {
		// We will quit.
		settings->layer_window_visible = layer_window_visible;
		settings->tool_setup_window_visible = tool_setup_window_visible;
		settings->tool_select_window_visible = tool_select_window_visible;
		settings->palette_window_visible = palette_window_visible;
		settings->brush_window_visible = brush_window_visible;
		return true;
	}
	return false;
}


void
PaintApplication::ReadyToRun()
{
	// Open here the ToolSelectionWindow
	if (settings->tool_select_window_visible)
		ToolSelectionWindow::showWindow();

	// Open here the ToolSetupWindow
	if (settings->tool_setup_window_visible)
		ToolSetupWindow::showWindow(settings->setup_window_tool);

	// Test here the brush store window
	if (settings->brush_window_visible) {
		BrushStoreWindow* brush_window = new BrushStoreWindow();
		brush_window->Show();
	}

	if (settings->palette_window_visible)
		ColorPaletteWindow::showPaletteWindow(false);

	if (settings->layer_window_visible)
		LayerWindow::showLayerWindow();

	// Here we will open a PaintWindow if no image was loaded on startup. This
	// should be the last window opened so that it will be the active window.
	if (PaintWindow::CountPaintWindows() == 0)
		PaintWindow::createPaintWindow();
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
	entry_ref ref;
	BMessage* to_be_sent;
	BAlert* alert;

	message->GetInfo("refs", &type, &count);
	if ( type == B_REF_TYPE ) {
		for ( long i = --count; i >= 0; i-- ) {
			if ( message->FindRef("refs", i, &ref) == B_OK ) {
				BFile file;
				if ( file.SetTo(&ref, B_READ_ONLY) == B_OK ) {
					BNodeInfo node(&file);
					char mime_type[B_MIME_TYPE_LENGTH];
					// initialize the mime-type string
					strcpy(mime_type,"");
					node.GetType(mime_type);

					// here compare the mime-type to all possible
					// mime-types for the types we have created
					// type-strings are defined in FileIdentificationStrings.h
					if ((strcmp(mime_type,HS_PALETTE_MIME_STRING) == 0) ||
						(strcmp(mime_type,_OLD_HS_PALETTE_MIME_STRING) == 0)) {
						// Call the static showPaletteWindow-function.
						// Giving it an argument containing refs makes
						// it also load a palette.
						to_be_sent = new BMessage(HS_PALETTE_OPEN_REFS);
						to_be_sent->AddRef("refs",&ref);
						ColorPaletteWindow::showPaletteWindow(to_be_sent);
						delete to_be_sent;
					}
					else if ((strcmp(mime_type,HS_PROJECT_MIME_STRING) == 0) ||
							 (strcmp(mime_type,_OLD_HS_PROJECT_MIME_STRING) == 0)) {
						// We should read the project file and open a new window.
						// Read the first four bytes from the file to see if it is really
						// a project file.
						BEntry input_entry = BEntry(&ref);

						BPath input_path;
						input_entry.GetPath(&input_path);

						int32 file_id;
						int32 lendian;
						if (file.Read(&lendian,sizeof(int32)) == sizeof(int32)) {
							if (file.Read(&file_id,sizeof(int32)) == sizeof(int32)) {
								settings->insert_recent_project_path(input_path.Path());
								if (uint32(lendian) == 0xFFFFFFFF)
									file_id = B_LENDIAN_TO_HOST_INT32(file_id);
								else
									file_id = B_BENDIAN_TO_HOST_INT32(file_id);

								if (file_id == PROJECT_FILE_ID) {
									file.Seek(0,SEEK_SET);	// Rewind the file.
									bool store_path;
									if (message->FindBool("from_filepanel",&store_path) == B_OK) {
										if (store_path == true) {
											BEntry entry(&ref);
											entry.GetParent(&entry);
											BPath path;
											if ((entry.GetPath(&path) == B_OK) && (path.Path() != NULL)) {
												strcpy(settings->project_open_path,path.Path());
											}
										}
									}
									readProject(file,ref);
								}
								else {
									bool store_path;
									if (message->FindBool("from_filepanel",&store_path) == B_OK) {
										if (store_path == true) {
											BEntry entry(&ref);
											entry.GetParent(&entry);
											BPath path;
											if ((entry.GetPath(&path) == B_OK) && (path.Path() != NULL)) {
												strcpy(settings->project_open_path,path.Path());
											}
										}
									}
									file.Seek(0,SEEK_SET);
									readProjectOldStyle(file,ref);
								}
							}
						}
					}
					// The file was not one of ArtPaint's file types. Perhaps it is an image-file.
					// Try to read it using the Translation-kit.
					else if ((strncmp(mime_type,"image/",6) == 0) || (strcmp(mime_type,"") == 0)) {
						BEntry input_entry = BEntry(&ref);

						BPath input_path;
						input_entry.GetPath(&input_path);
						// The returned bitmap might be in 8-bit format. If that is the case, we should
						// convert to 32-bit.
						BBitmap* input_bitmap = BTranslationUtils::GetBitmapFile(input_path.Path());
						if (input_bitmap == NULL) {
							// even the translators did not work
							char alert_string[255];
							sprintf(alert_string,StringServer::ReturnString(UNSUPPORTED_FILE_TYPE_STRING),ref.name);
							alert = new BAlert("title",alert_string,StringServer::ReturnString(OK_STRING),NULL,NULL,B_WIDTH_AS_USUAL,B_WARNING_ALERT);
							alert->Go();
						}
						else {
							settings->insert_recent_image_path(input_path.Path());

							input_bitmap = BitmapUtilities::ConvertColorSpace(input_bitmap,B_RGBA32);
							BitmapUtilities::FixMissingAlpha(input_bitmap);

							BTranslatorRoster* roster = BTranslatorRoster::Default();

							translator_info in_info;
							translator_info test_info;

							roster->Identify(&file,NULL,&in_info);
							printf("%s\n",in_info.MIME);
							// Check here if the reverse translation can also be done.
							BBitmapStream image_buffer(input_bitmap);
							status_t output_error;

							PaintWindow* the_window;

							if ((output_error = roster->Identify(&image_buffer,NULL,&test_info,0,NULL,in_info.type)) == B_OK) {
								// Reverse translation is possible
								image_buffer.DetachBitmap(&input_bitmap);
								the_window = PaintWindow::createPaintWindow(input_bitmap,ref.name,in_info.type,ref,test_info.translator);
							}
							else {
								// Reverse translation is not possible
								image_buffer.DetachBitmap(&input_bitmap);
								the_window = PaintWindow::createPaintWindow(input_bitmap,ref.name,0,ref);
							}

							the_window->readAttributes(file);

							// Record the new image_open_path if necessary.
							bool store_path;
							if (message->FindBool("from_filepanel",&store_path) == B_OK) {
								BEntry entry(&ref);
								entry.GetParent(&entry);
								BPath path;
								if ((entry.GetPath(&path) == B_OK) && (path.Path() != NULL)) {
									strcpy(settings->image_open_path,path.Path());
								}
							}
							else {
							}
						}
					}
					else {
						char alert_string[255];
						sprintf(alert_string,StringServer::ReturnString(UNSUPPORTED_FILE_TYPE_STRING),ref.name);
						alert = new BAlert("title",alert_string,StringServer::ReturnString(OK_STRING),NULL,NULL,B_WIDTH_AS_USUAL,B_WARNING_ALERT);
						alert->Go();
					}
				}
			}
		}
	}
}


rgb_color
PaintApplication::GetColor(bool foreground)
{
	// here we return the tool that corresponds to button
	if (foreground)
		return settings->primary_color;
	return settings->secondary_color;
}


bool
PaintApplication::SetColor(rgb_color color, bool foreground)
{
	if (foreground)
		settings->primary_color = color;
	else
		settings->secondary_color = color;

	return true;
}


void
PaintApplication::readPreferences()
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
					status = settings->read_from_file(mainPreferences);
			}

			if (status != B_OK)
				;// Settings have the default values.

			// Here set the language for the StringServer
			StringServer::SetLanguage(languages(settings->language));

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
PaintApplication::writePreferences()
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
					settings->write_to_file(mainPreferences);
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
PaintApplication::readProject(BFile &file,entry_ref &ref)
{
	// This is the new way of reading a structured project file. The possibility to read old
	// project-files is maintained through readProjectOldStyle-function.

	bool is_little_endian;
	int32 lendian;
	file.Read(&lendian,sizeof(int32));
	if (lendian == 0x00000000) {
		is_little_endian = false;
	}
	else if (uint32(lendian) == 0xFFFFFFFF) {
		is_little_endian = true;
	}
	else
		return B_ERROR;

	int64 length = FindProjectFileSection(file,PROJECT_FILE_DIMENSION_SECTION_ID);
	if (length != 2*sizeof(int32))
		return B_ERROR;

	int32 width;
	int32 height;

	if (file.Read(&width,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (file.Read(&height,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	if (is_little_endian) {
		width = B_LENDIAN_TO_HOST_INT32(width);
		height = B_LENDIAN_TO_HOST_INT32(height);
	}
	else {
		width = B_BENDIAN_TO_HOST_INT32(width);
		height = B_BENDIAN_TO_HOST_INT32(height);
	}

	// Create a paint-window using the width and height
	PaintWindow* the_window = PaintWindow::createPaintWindow(NULL, ref.name);

	the_window->OpenImageView(width,height);
	// Then read the layer-data. Rewind the file and put the image-view to read
	// the data.
	ImageView* image_view = the_window->ReturnImageView();
	image_view->ReturnImage()->ReadLayers(file);

	// This must be before the image-view is added
	BEntry* entry = the_window->ProjectEntry();
	*entry = BEntry(&ref,true);

	the_window->AddImageView();

	// As last thing read the attributes from the file
	the_window->readAttributes(file);



	return B_OK;
}


status_t
PaintApplication::readProjectOldStyle(BFile& file, entry_ref& ref)
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
	char alert_text[256];
	char file_name[256];
	strncpy(file_name,ref.name,256);

	BAlert* alert;
	char file_id[256]; 		// The identification string.
	ssize_t bytes_read = file.Read(file_id, strlen(HS_PROJECT_ID_STRING));
	if (bytes_read < 0 || uint32(bytes_read) != strlen(HS_PROJECT_ID_STRING)) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.
		return B_ERROR;
	}
	file_id[bytes_read] = '\0';
	if (strcmp(file_id, HS_PROJECT_ID_STRING) != 0) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.
		return B_ERROR;

	}
	uint32 version_number; 	// The version number of program with which this was written.
							// We actually do not care about the version number, because it is
							// known to be 1.0.0
	if (file.Read(&version_number,sizeof(uint32)) != sizeof(uint32)) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.

		return B_ERROR;
	}
	version_number = B_BENDIAN_TO_HOST_INT32(version_number);


	int32 settings_length;	// If the settings are of different length than the settings_struct, we
							// cannot use them.
	if (file.Read(&settings_length,sizeof(int32)) != sizeof(int32)) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.

		return B_ERROR;
	}
	settings_length = B_BENDIAN_TO_HOST_INT32(settings_length);
	// We skip the settings, no need to bother converting them
	if (true) {
		file.Seek(settings_length,SEEK_CUR);
	}

	uint32 width,height;
	if (file.Read(&width,sizeof(uint32)) != sizeof(uint32)) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.
		return B_ERROR;
	}
	if (file.Read(&height,sizeof(uint32)) != sizeof(uint32)) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.
		return B_ERROR;
	}

	width = B_BENDIAN_TO_HOST_INT32(width);
	height = B_BENDIAN_TO_HOST_INT32(height);
	// We should create a PaintWindow and also an ImageView for it.
	PaintWindow* the_window = PaintWindow::createPaintWindow(NULL,file_name);
	the_window->OpenImageView(width,height);

//	BEntry* project_entry = the_window->ProjectEntry();
//	*project_entry = BEntry(&ref);

	// Here we are ready to read layers from the file.
	// First read how many layers there are.
	int32 layer_count;

	if (file.Read(&layer_count,sizeof(int32)) != sizeof(int32)) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.
		// Free the allocated memory.
		delete the_window->ReturnImageView();
		the_window->Lock();
		the_window->Quit();
		return B_ERROR;
	}
	layer_count = B_BENDIAN_TO_HOST_INT32(layer_count);
	if (the_window->ReturnImageView()->ReturnImage()->ReadLayersOldStyle(file,layer_count) != B_OK) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.
		delete the_window->ReturnImageView();
		the_window->Lock();
		the_window->Quit();
		//printf("layers corrupt\n");

		return B_ERROR;
	}

	// This must be before the image-view is added
	BEntry* entry = the_window->ProjectEntry();
	*entry = BEntry(&ref,true);

	the_window->AddImageView();
	// Change the zoom-level to be correct
//	the_window->Lock();
//	the_window->displayMag(the_window->Settings()->zoom_level);
//	the_window->Unlock();

	// As last thing read the attributes from the file
	the_window->readAttributes(file);

	return B_OK;
}


void
PaintApplication::HomeDirectory(BPath &path)
{
	// this from the newsletter 81
	app_info info;
	be_app->GetAppInfo(&info);
	BEntry appentry;
	appentry.SetTo(&info.ref);
	appentry.GetPath(&path);
	path.GetParent(&path);
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
		if (message->FindString("bytes",&bytes) == B_OK) {
			switch (bytes[0]) {
				case B_TAB: {
					BView* view = dynamic_cast<BView*>(*handler);
					if ((view == NULL)
						|| ((!(view->Flags() & B_NAVIGABLE))
							&& (dynamic_cast<BTextView*>(*handler) == NULL))) {
						FloaterManager::ToggleFloaterVisibility();
					}
				}
				break;
			}
		}
	}
	return B_DISPATCH_MESSAGE;
}
