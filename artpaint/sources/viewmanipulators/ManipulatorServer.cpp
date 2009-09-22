/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <ClassInfo.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <OS.h>
#include <Path.h>
#include <stdio.h>
#include <string.h>

#include "AddOns.h"
#include "ManipulatorInformer.h"
#include "ManipulatorServer.h"
#include "RotationManipulator.h"
#include "FlipManipulator.h"
#include "ScaleManipulator.h"
#include "TranslationManipulator.h"
#include "CropManipulator.h"
#include "TransparencyManipulator.h"
#include "TextManipulator.h"
#include "PaintApplication.h"
#include "FreeTransformManipulator.h"
#include "Rotate90Manipulator.h"

int32 ManipulatorServer::number_of_addons = 0;
image_id* ManipulatorServer::addon_array = NULL;
int32 ManipulatorServer::max_number_of_addons = 0;
bool ManipulatorServer::addons_available = FALSE;

Manipulator* ManipulatorServer::ReturnManipulator(manipulator_type type,int32 add_on_id)
{
	switch (type) {
		case ADD_ON_MANIPULATOR:
			{
				Manipulator* (*instantiator)(BBitmap*,ManipulatorInformer*);
				status_t err = get_image_symbol(add_on_id,"instantiate_add_on",B_SYMBOL_TYPE_TEXT,(void**)&instantiator);
				if (err == B_NO_ERROR) {
					Manipulator *manipulator = instantiator(NULL,new ManipulatorInformer());
					manipulator->add_on_id = add_on_id;
					// Here we also give the add-on a chance to load its
					// settings if it is a GUIManipulator
					GUIManipulator *gui_manipulator = cast_as(manipulator,GUIManipulator);
					if (gui_manipulator != NULL) {
						BNode *add_on_node = GetAddOnNode(add_on_id);
						gui_manipulator->ReadSettings(add_on_node);
						delete add_on_node;
					}
					return manipulator;
				}
				else {
				//	printf("Could not get symbol: instantiate_add_on\n");
					return NULL;
				}
			}

		case FREE_TRANSFORM_MANIPULATOR:
			return new FreeTransformManipulator(NULL);

		case TRANSLATION_MANIPULATOR:
			return new TranslationManipulator(NULL);

		case ROTATION_MANIPULATOR:
			return new RotationManipulator(NULL);

		case HORIZ_FLIP_MANIPULATOR:
			return new HorizFlipManipulator();

		case VERT_FLIP_MANIPULATOR:
			return new VertFlipManipulator();

		case ROTATE_CW_MANIPULATOR:
			return new Rotate90ClockwiseManipulator();

		case ROTATE_CCW_MANIPULATOR:
			return new Rotate90CounterclockwiseManipulator();

		case SCALE_MANIPULATOR:
			return new ScaleManipulator(NULL);

		case CROP_MANIPULATOR:
			return new CropManipulator(NULL);

		case TRANSPARENCY_MANIPULATOR:
			return new TransparencyManipulator(NULL);

		case TEXT_MANIPULATOR:
			TextManipulator *text_manipulator;
			{
				BPath a_path;
				text_manipulator = new TextManipulator(NULL);
				if (find_directory(B_USER_SETTINGS_DIRECTORY,&a_path) == B_NO_ERROR) {
					BDirectory user_settings_directory;
					BDirectory settings_directory;
					user_settings_directory.SetTo(a_path.Path());
					status_t err = settings_directory.SetTo(&user_settings_directory,"ArtPaint/");;
					if (err == B_OK) {
						BEntry text_tool_entry;
						err = settings_directory.FindEntry("text_tool",&text_tool_entry,TRUE);
						if (err == B_OK) {
							BFile text_tool_file(&text_tool_entry,B_READ_ONLY);
							err = text_tool_file.InitCheck();
							if (err == B_NO_ERROR) {
								text_manipulator->ReadSettings(&text_tool_file);
							}
						}
					}
				}
			}
			return text_manipulator;

		default:
			return NULL;
	}
}




void ManipulatorServer::ReadAddOns()
{
	thread_id reader = spawn_thread(add_on_reader,"add_on_reader",B_NORMAL_PRIORITY,NULL);
	resume_thread(reader);
}



int32 ManipulatorServer::add_on_reader(void*)
{
	// Try to read the add-ons from the following places.
	// 	1.	ArtPaint/add-ons
	// 	2.	B_USER_ADDONS_DIRECTORY/ArtPaint
	//	3.	B_COMMON_ADDONS_DIRECTORY/ArtPaint
	//
	// We should try to make sure, not to read the same add-on more than once. And
	// if multiple versions of the same add-on are available to use the latest version
	// of them. Only one add-on per name will be loaded.

	status_t current_status;
	BPath add_on_path;
	PaintApplication::HomeDirectory(add_on_path);
	StringSet *addon_names = new StringSet();

	if ((current_status = add_on_path.Append("add-ons",TRUE)) == B_NO_ERROR) {
		BEntry current_entry;
		BDirectory add_on_dir;

		if ((current_status = current_entry.SetTo(add_on_path.Path())) != B_NO_ERROR)
			return current_status;

		if (addon_array == NULL) {
			max_number_of_addons = 2;
			addon_array = new image_id[max_number_of_addons];
		}


		image_id current_add_on_id;
		char current_add_on_name[B_FILE_NAME_LENGTH];

		if ( (current_status = add_on_dir.SetTo(&current_entry)) == B_OK)	{
			while ((current_status = add_on_dir.GetNextEntry(&current_entry)) == B_OK) {
				current_entry.GetPath(&add_on_path);
				current_add_on_id = load_add_on(add_on_path.Path());
				if (current_add_on_id > 0) {
					current_entry.GetName(current_add_on_name);
					if (!addon_names->ContainsString(current_add_on_name)) {
							if (number_of_addons == max_number_of_addons) {
								max_number_of_addons *= 2;
								image_id *new_array = new image_id[max_number_of_addons];
								for (int32 i=0;i<number_of_addons;i++) {
									new_array[i]= addon_array[i];
								}
								delete[] addon_array;
								addon_array = new_array;
							}
							addon_array[number_of_addons] = current_add_on_id;
							number_of_addons++;
							addon_names->AddString(current_add_on_name);
					}
				}
				else {
//					printf("Could not load add-on:%s\n",add_on_path.Path());
				}
			}
		}
	}
	if ((find_directory(B_USER_ADDONS_DIRECTORY,&add_on_path) == B_OK) && (add_on_path.Append("ArtPaint") == B_NO_ERROR)) {
		BEntry current_entry;
		BDirectory add_on_dir;

		if ((current_status = current_entry.SetTo(add_on_path.Path())) != B_NO_ERROR)
			return current_status;

		if (addon_array == NULL) {
			max_number_of_addons = 2;
			addon_array = new image_id[max_number_of_addons];
		}


		image_id current_add_on_id;
		char current_add_on_name[B_FILE_NAME_LENGTH];

		if ( (current_status = add_on_dir.SetTo(&current_entry)) == B_OK)	{
			while ((current_status = add_on_dir.GetNextEntry(&current_entry)) == B_OK) {
				current_entry.GetPath(&add_on_path);
				current_add_on_id = load_add_on(add_on_path.Path());
				if (current_add_on_id > 0) {
					current_entry.GetName(current_add_on_name);
					if (!addon_names->ContainsString(current_add_on_name)) {
							if (number_of_addons == max_number_of_addons) {
								max_number_of_addons *= 2;
								image_id *new_array = new image_id[max_number_of_addons];
								for (int32 i=0;i<number_of_addons;i++) {
									new_array[i]= addon_array[i];
								}
								delete[] addon_array;
								addon_array = new_array;
							}
							addon_array[number_of_addons] = current_add_on_id;
							number_of_addons++;
							addon_names->AddString(current_add_on_name);
					}
				}
				else {
				}
			}
		}
	}
	if ((find_directory(B_COMMON_ADDONS_DIRECTORY,&add_on_path) == B_OK) && (add_on_path.Append("ArtPaint") == B_NO_ERROR)) {
		BEntry current_entry;
		BDirectory add_on_dir;

		if ((current_status = current_entry.SetTo(add_on_path.Path())) != B_NO_ERROR)
			return current_status;

		if (addon_array == NULL) {
			max_number_of_addons = 2;
			addon_array = new image_id[max_number_of_addons];
		}


		image_id current_add_on_id;
		char current_add_on_name[B_FILE_NAME_LENGTH];

		if ( (current_status = add_on_dir.SetTo(&current_entry)) == B_OK)	{
			while ((current_status = add_on_dir.GetNextEntry(&current_entry)) == B_OK) {
				current_entry.GetPath(&add_on_path);
				current_add_on_id = load_add_on(add_on_path.Path());
				if (current_add_on_id > 0) {
					current_entry.GetName(current_add_on_name);
					if (!addon_names->ContainsString(current_add_on_name)) {
							if (number_of_addons == max_number_of_addons) {
								max_number_of_addons *= 2;
								image_id *new_array = new image_id[max_number_of_addons];
								for (int32 i=0;i<number_of_addons;i++) {
									new_array[i]= addon_array[i];
								}
								delete[] addon_array;
								addon_array = new_array;
							}
							addon_array[number_of_addons] = current_add_on_id;
							number_of_addons++;
							addon_names->AddString(current_add_on_name);
					}
				}
				else {
				}
			}
		}
	}
	addons_available = TRUE;

	delete addon_names;
	return B_OK;
}


void ManipulatorServer::StoreManipulatorSettings(Manipulator *manipulator)
{
	TextManipulator *text_manipulator = cast_as(manipulator,TextManipulator);
	// Currently the text-manipulator is the only internal manipulator
	// that stores its settings. It should store its settings in the settings
	// directory.
	if (text_manipulator != NULL) {
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
				BFile text_tool_file;
				err = settings_directory.CreateFile("text_tool",&text_tool_file,FALSE);
				if (err == B_OK) {
					text_manipulator->WriteSettings(&text_tool_file);
				}
			}
		}
	}
	else {
		GUIManipulator *gui_manipulator = cast_as(manipulator,GUIManipulator);
		// An add-on that is a GUIManipulator should store its settings as attributes
		// to the file where it is loaded.
		if (gui_manipulator != NULL) {
			BNode *node = GetAddOnNode(gui_manipulator->add_on_id);
			if (node != NULL) {
				gui_manipulator->WriteSettings(node);
				delete node;
			}
		}
	}

}



BNode* ManipulatorServer::GetAddOnNode(image_id add_on_id)
{
	// Disqualify illegal image ids.
	if (add_on_id <= 0)
		return NULL;

	image_info info;
	if (get_image_info(add_on_id,&info) == B_NO_ERROR) {
		BNode *node = new BNode();

		if (node->SetTo(info.name) != B_NO_ERROR) {
			delete node;
			return NULL;
		}
		else
			return node;
	}
	else
		return NULL;
}


StringSet::StringSet()
{
	array_length = 1024;
	string_array = new char*[array_length];
	for (int32 i=0;i<array_length;i++) {
		string_array[i] = NULL;
	}
	entry_count = 0;
}

StringSet::~StringSet()
{
	for (int32 i=0;i<entry_count;i++) {
		delete[] string_array[i];
		string_array[i] = NULL;
	}

	delete[] string_array;
}

void StringSet::AddString(const char *s)
{
	if (entry_count == array_length) {
		array_length *= 2;
		char **new_array = new char*[array_length];

		for (int32 i=0;i<entry_count;i++) {
			new_array[i] = string_array[i];
			string_array[i] = NULL;
		}
		delete[] string_array;
		string_array = new_array;
	}
	if (ContainsString(s) == FALSE) {
		string_array[entry_count] = new char[strlen(s)+1];
		strcpy(string_array[entry_count++],s);
	}
}

bool StringSet::ContainsString(const char *s)
{
	bool found = FALSE;

	int32 i=0;
	while ((i<entry_count) && (found == FALSE)) {
		if (strcmp(string_array[i],s) == 0)
			found = TRUE;
		i++;
	}

	return found;
}
