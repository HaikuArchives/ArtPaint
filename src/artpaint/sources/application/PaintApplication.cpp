/* 

	Filename:	ArtPaintApp.cpp
	Contents:	main function + BApplication class definition
	Author:		Heikki Suhonen
	
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
			:	BApplication("application/x-vnd.hsuhonen-artpaint")
{	
	
	// Some of the things in this function depend on the previously initialized things,
	// so the order may be important. This should be fixed in future.	


	// create the settings
	settings = new global_settings();
	readPreferences();

	// Set the language
	StringServer::SetLanguage((languages)settings->language);

	// Set the tool
	tool_manager->ChangeTool(settings->primary_tool);
		
	// NULL the window pointers
	image_open_panel = NULL;
	project_open_panel = NULL;

	// Set the undo-queue to right depth
	UndoQueue::SetQueueDepth(Settings()->undo_queue_depth);

	
//	// the first untitled window will have number 1
//	untitled_window_number = 1;
	
	// create the tool-images here
	ToolImages::createToolImages();

	// Read the add-ons. They will be read in another thread by the manipulator server.
	// This should be the last thing to read so that it does not interfere with other
	// reading.
	ManipulatorServer::ReadAddOns();
}


PaintApplication::~PaintApplication()
{
	if (image_open_panel != NULL)
		delete image_open_panel;
	
	if (project_open_panel != NULL)
		delete project_open_panel;

	writePreferences();
	delete settings;

	ToolManager::DestroyToolManager();
}

void PaintApplication::MessageReceived(BMessage *message)
{
	// this is used for replying to messages
	BMessage reply_message;
	 
	// this is used to change the file-panel's message
	BMessage *file_panel_message;
	
	switch (message->what) {
		// this comes from a paint-window's menubar->"Window"->"New Paint Window"
		case HS_NEW_PAINT_WINDOW:
			PaintWindow::createPaintWindow();
			break;		
	
		// these next constants deal with opening images and projects
		// first loading a image
		// this comes from a paint-window's menubar->"File"->"Open"->"Open Image…"
		case HS_SHOW_IMAGE_OPEN_PANEL:
			file_panel_message = new BMessage(B_REFS_RECEIVED);
			if (image_open_panel == NULL) {
				entry_ref *ref = new entry_ref();
				get_ref_for_path(settings->image_open_path,ref);
				file_panel_message->AddBool("from_filepanel",TRUE);
				image_open_panel = new BFilePanel(B_OPEN_PANEL,new BMessenger(this),ref,B_FILE_NODE,true,NULL, new ImageFilter());	
				delete ref;
			}
			
			char string[256];
			sprintf(string,"ArtPaint: %s",StringServer::ReturnString(OPEN_IMAGE_STRING));
			image_open_panel->Window()->SetTitle(string);
			
			image_open_panel->SetMessage(file_panel_message);
			image_open_panel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
			set_filepanel_strings(image_open_panel);
			image_open_panel->Show();
			delete file_panel_message;
			break;
	
		case HS_SHOW_PROJECT_OPEN_PANEL:
			{
				file_panel_message = new BMessage(B_REFS_RECEIVED);
				if (project_open_panel == NULL) {
					entry_ref *ref = new entry_ref();
					get_ref_for_path(settings->project_open_path,ref);
					file_panel_message->AddBool("from_filepanel",TRUE);				
					project_open_panel = new BFilePanel(B_OPEN_PANEL,new BMessenger(this),ref,B_FILE_NODE,true,NULL);	
					delete ref;
				}		
				char string[256];
				sprintf(string,"ArtPaint: %s",StringServer::ReturnString(OPEN_PROJECT_STRING));
				project_open_panel->Window()->SetTitle(string);		
				project_open_panel->SetMessage(file_panel_message);
				project_open_panel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
				set_filepanel_strings(project_open_panel);
				project_open_panel->Show();
				delete file_panel_message;
			}
			break;
		// this comes from a paint-window's menubar->"Help"->"User Documentation"
		case HS_SHOW_USER_DOCUMENTATION:
			{
				// Here start the NetPositive with the right page. 
				BPath home_path;
				PaintApplication::HomeDirectory(home_path);
			
				// Force normalization of the path to check validity.
				if (home_path.Append("Documentation/",TRUE) == B_NO_ERROR) {
					// Take the file-name from the message
					const char *document_name;
					message->FindString("document",&document_name);
					if (home_path.Append(document_name,TRUE) == B_NO_ERROR) {
						char url[512];
						sprintf(url,"file://%s",home_path.Path());
						
						// this comes from Be Newsletter 88
						BMessage url_message(B_ARGV_RECEIVED);
						url_message.AddString("argv","NetPositive");
						url_message.AddString("argv",url);
						url_message.AddInt32("argc",2);
						
						BMessenger messenger("application/x-vnd.Be-NPOS", -1, NULL); 
						
						if (messenger.IsValid()) { 
							messenger.SendMessage( &url_message ); 
						}
						else { 
							be_roster->Launch ("application/x-vnd.Be-NPOS", &url_message ); 
						} 			
					}
				}
				
				break;
			}

		case B_PASTE:
			{
				be_clipboard->Lock();
				BMessage *bitmap_message = new BMessage();
				BMessage *clipboard_message = be_clipboard->Data();
				if (clipboard_message != NULL) {
					if (clipboard_message->FindMessage("image/bitmap",bitmap_message) == B_OK) {
						if (bitmap_message != NULL) {
							BBitmap *pasted_bitmap = new BBitmap(bitmap_message);
							delete bitmap_message;
							if ((pasted_bitmap != NULL) && (pasted_bitmap->IsValid() == TRUE)) {
								PaintWindow::createPaintWindow(pasted_bitmap,"Clip 1");
							}
						}
					}
				}
				be_clipboard->Unlock();
			}
			break;
		
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


bool PaintApplication::QuitRequested()
{
	// Here we must collect information about the window's that are still open
	// because they will be closed in BApplication::QuitRequested().
	bool layer_window_visible = settings->layer_window_visible;
	bool tool_setup_window_visible = settings->tool_setup_window_visible;
	bool tool_select_window_visible = settings->tool_select_window_visible;
	bool palette_window_visible = settings->palette_window_visible;
	bool brush_window_visible = settings->brush_window_visible;
	
	if (BApplication::QuitRequested() == TRUE) {
		// We will quit.
		settings->layer_window_visible = layer_window_visible;
		settings->tool_setup_window_visible = tool_setup_window_visible;
		settings->tool_select_window_visible = tool_select_window_visible;
		settings->palette_window_visible = palette_window_visible;
		settings->brush_window_visible = brush_window_visible;
				
		return TRUE;
	}
	else {
		return FALSE;
	}
}

void PaintApplication::ReadyToRun()
{
	// Open here the ToolSelectionWindow
	if (settings->tool_select_window_visible == TRUE)
		ToolSelectionWindow::showWindow();
		
	// Open here the ToolSetupWindow
	if (settings->tool_setup_window_visible == TRUE)	
		ToolSetupWindow::showWindow(settings->setup_window_tool);
//	else
//		printf("tool-setup-window is not visible\n");
		
	// Test here the brush store window
	if (settings->brush_window_visible == TRUE) {
		BrushStoreWindow *brush_window = new BrushStoreWindow();
		brush_window->Show();
	}

	if (settings->palette_window_visible == TRUE) {
		ColorPaletteWindow::showPaletteWindow(FALSE);
	}

	if (settings->layer_window_visible == TRUE) {
		LayerWindow::showLayerWindow();
	}
	// Here we will open a PaintWindow if no image was loaded on startup.
	// This should be the last window opened so that it will be the active window.
	if (PaintWindow::CountPaintWindows() == 0)
		PaintWindow::createPaintWindow();	
}


void PaintApplication::RefsReceived(BMessage *message)
{
	// here we will determine which type of file was opened
	// and then initiate the right function for opening it
	// the files will be checked for their mime-types and some
	// also additionally for their file-identification strings
	uint32 type;        
	int32 count; 
	entry_ref ref; 
	BMessage *to_be_sent;
	BAlert *alert;
	
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
								if (lendian == 0xFFFFFFFF)
									file_id = B_LENDIAN_TO_HOST_INT32(file_id);
								else
									file_id = B_BENDIAN_TO_HOST_INT32(file_id);
								
								if (file_id == PROJECT_FILE_ID) {
									file.Seek(0,SEEK_SET);	// Rewind the file.
									bool store_path;
									if (message->FindBool("from_filepanel",&store_path) == B_OK) {
										if (store_path == TRUE) {
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
										if (store_path == TRUE) {
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
						BBitmap *input_bitmap = BTranslationUtils::GetBitmapFile(input_path.Path());
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

							BTranslatorRoster *roster = BTranslatorRoster::Default();

							translator_info in_info;
							translator_info test_info;
							
							roster->Identify(&file,NULL,&in_info);
							printf("%s\n",in_info.MIME);
							// Check here if the reverse translation can also be done.
							BBitmapStream image_buffer(input_bitmap);
							status_t output_error;

							PaintWindow *the_window;
							
							if ((output_error = roster->Identify(&image_buffer,NULL,&test_info,0,NULL,in_info.type)) == B_NO_ERROR) {
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


rgb_color PaintApplication::GetColor(bool foreground)
{
	// here we return the tool that corresponds to button
	if (foreground)
		return settings->primary_color;
	else 
		return settings->secondary_color;
}

bool PaintApplication::SetColor(rgb_color color,bool foreground)
{
	if (foreground)
		settings->primary_color = color;
	else 
		settings->secondary_color = color;

	return TRUE;
}


//void PaintApplication::createPaintWindow(BBitmap *a_bitmap,char *file_name,int32 type, entry_ref &ref)
//{
//	PaintWindow *a_window;
//	
//	if (a_bitmap == NULL) {
//		char title[100];
//		sprintf(title,"%s - %d",StringServer::ReturnString(UNTITLED_STRING),untitled_window_number);
//		a_window = new PaintWindow(title,settings->default_window_settings.frame_rect,HS_MENU_BAR | HS_STATUS_VIEW | HS_HELP_VIEW | HS_SIZING_VIEW,&(settings->default_window_settings));
//		untitled_window_number++;
//	}
//	
//	else {
////		// in this case we should first try to read window's rect from attributes
////		// should also check that the rect is inside the screen's bounds
////		BNode a_node(&ref);
////		BNodeInfo a_node_info(&a_node);
////		BRect a_rect;
////		if (a_node.ReadAttr("ArtP:frame_rect",B_RECT_TYPE,0,&a_rect,sizeof(BRect)) < 0)
//		a_window = new PaintWindow(file_name,settings->default_window_settings.frame_rect, HS_MENU_BAR | HS_STATUS_VIEW | HS_HELP_VIEW,&(settings->default_window_settings));
////		else {
////			a_rect = FitRectToScreen(a_rect);
////			a_window = new PaintWindow(file_name,a_rect, HS_MENU_BAR | HS_STATUS_VIEW | HS_HELP_VIEW,&(settings->default_window_settings));
////		}		
//			
//		// some of these might also come from attributes
//		a_window->Settings()->zoom_level = settings->default_window_settings.zoom_level;
//		a_window->Settings()->view_position = settings->default_window_settings.view_position;
//		a_window->Settings()->file_type = type;
//		a_node_info.GetType(a_window->Settings()->file_mime);
//		BEntry *entry = a_window->ImageEntry();
//		*entry = BEntry(&ref,TRUE);				
//
//		// make the window read it's attributes
//		a_window->readAttributes(a_node);
//
//		// Open an imageview for the window and add a layer to it.
//		a_window->OpenImageView(a_bitmap->Bounds().Width()+1,a_bitmap->Bounds().Height()+1);
//		a_window->ReturnImageView()->ReturnImage()->InsertLayer(a_bitmap);
//		a_window->AddImageView();
//	}
//	
//	// Change the zoom-level to be correct
//	a_window->Lock();
//	a_window->displayMag(a_window->Settings()->zoom_level);
//	a_window->Unlock();
//}


void PaintApplication::readPreferences()
{
	bool create_default_tools = true; 
	bool create_default_colorset = true;

	BPath a_path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&a_path) == B_NO_ERROR) {
		BDirectory user_settings_directory;
		BDirectory settings_directory;
		BDirectory spare_settings_directory;
		user_settings_directory.SetTo(a_path.Path());
		status_t main_err = settings_directory.SetTo(&user_settings_directory,"ArtPaint/");;
		HomeDirectory(a_path);
		spare_settings_directory.SetTo(a_path.Path());
		status_t spare_err = spare_settings_directory.SetTo(&spare_settings_directory,"settings/");
		
		if ((main_err == B_OK) || (spare_err == B_OK)) {
			BEntry brush_entry;
			status_t err;
			err = settings_directory.FindEntry("brushes",&brush_entry,TRUE);
			if ((err != B_OK) && (spare_settings_directory.InitCheck() == B_NO_ERROR))
				err = spare_settings_directory.FindEntry("brushes",&brush_entry,TRUE);

			if (err == B_OK) {
				BFile brush_file(&brush_entry,B_READ_ONLY);
				err = brush_file.InitCheck();
				if (err == B_NO_ERROR)
					BrushStoreWindow::readBrushes(brush_file);
			}
			if (err != B_OK) {
				// We might create some default brushes.	
				
			}					
			

			BEntry main_preferences_entry;
			err = settings_directory.FindEntry("main_preferences",&main_preferences_entry,TRUE);
			if ((err != B_OK) && (spare_settings_directory.InitCheck() == B_NO_ERROR))
				err = spare_settings_directory.FindEntry("main_preferences",&main_preferences_entry,TRUE);

			if (err == B_OK) {
				BFile main_preferences_file(&main_preferences_entry,B_READ_ONLY);
				err = main_preferences_file.InitCheck();
				if (err == B_NO_ERROR)
					err = settings->read_from_file(main_preferences_file);	
			}
			if (err != B_OK) {
				// Settings have the default values.
			}

			// Here set the language for the StringServer
			StringServer::SetLanguage((languages)settings->language);
			
			// Create a tool-manager object. This depends on the language being set.
			ToolManager::CreateToolManager();

			BEntry tool_entry;
			err = settings_directory.FindEntry("tool_preferences",&tool_entry,TRUE);			

			if ((err != B_OK) && (spare_settings_directory.InitCheck() == B_NO_ERROR))
				err = spare_settings_directory.FindEntry("tool_preferences",&tool_entry,TRUE);

			if (err == B_OK) {
				BFile tool_file(&tool_entry,B_READ_ONLY);
				if (err == B_NO_ERROR) {
					tool_manager->ReadToolSettings(tool_file);
				}
			}

			
			BEntry color_entry;
			err = settings_directory.FindEntry("color_preferences",&color_entry,TRUE);

			if ((err != B_OK) && (spare_settings_directory.InitCheck() == B_NO_ERROR))
				err = spare_settings_directory.FindEntry("color_preferences",&color_entry,TRUE);

			if (err == B_OK) {
				BFile color_file(&color_entry,B_READ_ONLY);
				err = color_file.InitCheck();
				if (err == B_OK) {
					if (ColorSet::readSets(color_file) == B_OK)
						create_default_colorset = FALSE;
				}
			}

		}
		else {
		//	printf("No settings were found\n");
		}
	}

	if (create_default_colorset) {
		// We might look into apps directory and palette directory for some
		// colorsets
		new ColorSet(16);
	}
	if (create_default_tools) {
		// Create a tool-manager object.
		ToolManager::CreateToolManager();		
	}
}

void PaintApplication::writePreferences()
{
	BPath a_path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&a_path) == B_NO_ERROR) {
		BDirectory user_settings_directory;
		BDirectory settings_directory;
		user_settings_directory.SetTo(a_path.Path());
		status_t err = user_settings_directory.CreateDirectory("./ArtPaint",&settings_directory);
		if (err == B_FILE_EXISTS) {
			err = settings_directory.SetTo(&user_settings_directory,"ArtPaint/");
		}
		if (err == B_OK) {
			// Here we create several preferences files. One for each internal
			// manipulator, main preferences file and brushes and palettes files.
			// Later we might add even more files for example a file for each tool.
			// Now all the tool-settings are stored in one file. Actually the manipulator
			// preference files will be created by ManipulatorServer.
			BFile brush_file;
			err = settings_directory.CreateFile("brushes",&brush_file,FALSE);
			if (err == B_OK) {
				BrushStoreWindow::writeBrushes(brush_file);
			}

			BFile main_preferences_file;
			err = settings_directory.CreateFile("main_preferences",&main_preferences_file,FALSE);
			if (err == B_OK) {
				err = settings->write_to_file(main_preferences_file);
			}

			BFile tool_file;
			err = settings_directory.CreateFile("tool_preferences",&tool_file,FALSE);
			if (err == B_OK) {
				tool_manager->WriteToolSettings(tool_file);
			}
			
			BFile color_palette_file;
			err = settings_directory.CreateFile("color_preferences",&color_palette_file,FALSE);
			if (err == B_OK) {
				err = ColorSet::writeSets(color_palette_file);
			}
		}
		else {
		//	printf("Could not write preferences. Hmmm?\n");
		}
	}		
}


status_t PaintApplication::readProject(BFile &file,entry_ref &ref)
{
	// This is the new way of reading a structured project file. The possibility to read old
	// project-files is maintained through readProjectOldStyle-function.
	
	bool is_little_endian;
	int32 lendian;
	file.Read(&lendian,sizeof(int32));
	if (lendian == 0x00000000) {
		is_little_endian = FALSE;
	}
	else if (lendian == 0xFFFFFFFF) {
		is_little_endian = TRUE;
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
//	PaintWindow *the_window = new PaintWindow(ref.name,settings->default_window_settings.frame_rect,HS_STATUS_VIEW|HS_MENU_BAR,&(settings->default_window_settings));
	PaintWindow *the_window = PaintWindow::createPaintWindow(NULL,ref.name);


	the_window->OpenImageView(width,height);
	// Then read the layer-data. Rewind the file and put the image-view to read the data.
	ImageView *image_view = the_window->ReturnImageView();			
	image_view->ReturnImage()->ReadLayers(file);

	// This must be before the image-view is added
	BEntry *entry = the_window->ProjectEntry();
	*entry = BEntry(&ref,TRUE);				

	the_window->AddImageView();
	
	// As last thing read the attributes from the file
	the_window->readAttributes(file);



	return B_OK;
}


status_t PaintApplication::readProjectOldStyle(BFile &file,entry_ref &ref)
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
	
	BAlert *alert;
	int32 bytes_read;
	char file_id[256]; 		// The identification string.
	if ((bytes_read = file.Read(file_id,strlen(HS_PROJECT_ID_STRING))) != strlen(HS_PROJECT_ID_STRING)) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.
		return B_ERROR;					
	}
	file_id[bytes_read] = '\0';	
	if (strcmp(file_id,HS_PROJECT_ID_STRING) != 0) {
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
	if (TRUE) {
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
//	PaintWindow *the_window = new PaintWindow(file_name,project_settings->frame_rect,HS_STATUS_VIEW|HS_HELP_VIEW|HS_MENU_BAR,project_settings);
	PaintWindow *the_window = PaintWindow::createPaintWindow(NULL,file_name);
	the_window->OpenImageView(width,height);

//	BEntry *project_entry = the_window->ProjectEntry();
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
	BEntry *entry = the_window->ProjectEntry();
	*entry = BEntry(&ref,TRUE);				

	the_window->AddImageView();
	// Change the zoom-level to be correct
//	the_window->Lock();
//	the_window->displayMag(the_window->Settings()->zoom_level);
//	the_window->Unlock();

	// As last thing read the attributes from the file
	the_window->readAttributes(file);



	return B_OK;
}

void PaintApplication::HomeDirectory(BPath &path)
{
	// this from the newsletter 81
	app_info info;   
	be_app->GetAppInfo(&info); 
	BEntry appentry; 
	appentry.SetTo(&info.ref); 
	appentry.GetPath(&path); 
	path.GetParent(&path); 
	//printf("HomeDirectory called\n");
}


// the main function
int main()
{
//	SetNewLeakChecking(true);
//	SetMallocLeakChecking(true);
	PaintApplication *my_app;

	// create app
	my_app = new PaintApplication();
	
	// only run app if it could be created
	if (my_app != NULL) {
		// run app
		my_app->Run();

		// delete app
		delete my_app;
	}

	return B_NO_ERROR;
}


filter_result AppKeyFilterFunction(BMessage *message,BHandler **handler,BMessageFilter*)
{
	const char *bytes;
	if ((!(modifiers() & B_COMMAND_KEY)) && (!(modifiers() & B_CONTROL_KEY))) {
		if (message->FindString("bytes",&bytes) == B_NO_ERROR) {
			switch (bytes[0]) {
				case B_TAB:
				{

					BView *view = dynamic_cast<BView*>(*handler);
					if ((view == NULL) || ((!(view->Flags() & B_NAVIGABLE)) &&
						(dynamic_cast<BTextView *>(*handler) == NULL))) {
						FloaterManager::ToggleFloaterVisibility();
					}
				}
				break;
			}
		}
	}

	return B_DISPATCH_MESSAGE;
}
