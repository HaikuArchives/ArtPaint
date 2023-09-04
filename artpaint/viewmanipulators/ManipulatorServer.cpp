/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "ManipulatorServer.h"

#include "CropManipulator.h"
#include "FlipManipulator.h"
#include "FreeTransformManipulator.h"
#include "GUIManipulator.h"
#include "ManipulatorInformer.h"
#include "PaintApplication.h"
#include "Rotate90Manipulator.h"
#include "RotationManipulator.h"
#include "ScaleCanvasManipulator.h"
#include "ScaleManipulator.h"
#include "SettingsServer.h"
#include "TextManipulator.h"
#include "TranslationManipulator.h"
#include "TransparencyManipulator.h"


#include <Autolock.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <OS.h>
#include <Path.h>


#include <new>


BLocker ManipulatorServer::fLocker;
ManipulatorServer* ManipulatorServer::fManipulatorServer = NULL;


ManipulatorServer*
ManipulatorServer::Instance()
{
	return Instantiate();
}


ManipulatorServer::ManipulatorServer()
	:
	fAddonsLoaded(false)
{
	thread_id threadId
		= spawn_thread(_AddOnLoaderThread, "AddOnLoaderThread", B_NORMAL_PRIORITY, NULL);
	if (threadId >= 0)
		resume_thread(threadId);
}


ManipulatorServer::~ManipulatorServer()
{
	ImageList::const_iterator it;
	for (it = fAddOnImages.begin(); it != fAddOnImages.end(); ++it)
		unload_add_on(*it);
	fManipulatorServer = NULL;
}


ManipulatorServer*
ManipulatorServer::Instantiate()
{
	if (fManipulatorServer == NULL) {
		BAutolock _(&fLocker);
		if (fManipulatorServer == NULL)
			fManipulatorServer = new (std::nothrow) ManipulatorServer();
	}
	return fManipulatorServer;
}


void
ManipulatorServer::DestroyServer()
{
	if (fManipulatorServer) {
		delete fManipulatorServer;
		fManipulatorServer = NULL;
	}
}


Manipulator*
ManipulatorServer::ManipulatorFor(manipulator_type type, image_id imageId) const
{
	Manipulator* manipulator = NULL;

	switch (type) {
		case ADD_ON_MANIPULATOR:
		{
			Manipulator* (*Instantiate)(BBitmap*, ManipulatorInformer*);
			status_t status = get_image_symbol(
				imageId, "instantiate_add_on", B_SYMBOL_TYPE_TEXT, (void**)&Instantiate);
			if (status == B_OK) {
				manipulator = Instantiate(NULL, new ManipulatorInformer());
				manipulator->add_on_id = imageId;
				// Here we also give the add-on a chance to load its settings if
				// it is a GUIManipulator
				BNode node;
				GUIManipulator* gui_manipulator = dynamic_cast<GUIManipulator*>(manipulator);
				if (gui_manipulator && _GetNodeFor(imageId, &node) == B_OK)
					gui_manipulator->ReadSettings(&node);
			}
		} break;
		case FREE_TRANSFORM_MANIPULATOR:
		{
			manipulator = new FreeTransformManipulator(NULL);
		} break;
		case TRANSLATION_MANIPULATOR:
		{
			manipulator = new TranslationManipulator(NULL);
		} break;
		case TRANSLATE_SELECTION_MANIPULATOR:
		{
			manipulator = new TranslationManipulator(NULL);
			((TranslationManipulator*)manipulator)->SetTransformSelectionOnly(true);
		} break;
		case ROTATION_MANIPULATOR:
		{
			manipulator = new RotationManipulator(NULL);
		} break;
		case ROTATE_SELECTION_MANIPULATOR:
		{
			manipulator = new RotationManipulator(NULL);
			((RotationManipulator*)manipulator)->SetTransformSelectionOnly(true);
		} break;
		case HORIZ_FLIP_MANIPULATOR:
		{
			manipulator = new HorizFlipManipulator();
		} break;
		case VERT_FLIP_MANIPULATOR:
		{
			manipulator = new VertFlipManipulator();
		} break;
		case ROTATE_CW_MANIPULATOR:
		{
			manipulator = new RotationManipulator(NULL);
			((RotationManipulator*)manipulator)->SetAngle(90.);
			GUIManipulator* gui_manipulator = dynamic_cast<GUIManipulator*>(manipulator);
			if (gui_manipulator != NULL)
				gui_manipulator->EnableWindow(false);
		} break;
		case ROTATE_CCW_MANIPULATOR:
		{
			manipulator = new RotationManipulator(NULL);
			((RotationManipulator*)manipulator)->SetAngle(-90.);
			GUIManipulator* gui_manipulator = dynamic_cast<GUIManipulator*>(manipulator);
			if (gui_manipulator != NULL)
				gui_manipulator->EnableWindow(false);
		} break;
		case ROTATE_CW_CANVAS_MANIPULATOR:
		{
			manipulator = new Rotate90ClockwiseManipulator();
		} break;
		case ROTATE_CCW_CANVAS_MANIPULATOR:
		{
			manipulator = new Rotate90CounterclockwiseManipulator();
		} break;
		case SCALE_MANIPULATOR:
		{
			manipulator = new ScaleManipulator(NULL);
		} break;
		case SCALE_CANVAS_MANIPULATOR:
		{
			manipulator = new ScaleCanvasManipulator(NULL);
		} break;
		case SCALE_SELECTION_MANIPULATOR:
		{
			manipulator = new ScaleManipulator(NULL);
			((ScaleManipulator*)manipulator)->SetTransformSelectionOnly(true);
		} break;
		case CROP_MANIPULATOR:
		{
			manipulator = new CropManipulator(NULL);
		} break;
		case TRANSPARENCY_MANIPULATOR:
		{
			manipulator = new TransparencyManipulator(NULL);
		} break;
		case TEXT_MANIPULATOR:
		{
			TextManipulator* textManipulator = new TextManipulator(NULL);
			if (SettingsServer* server = SettingsServer::Instance()) {
				BMessage settings;
				if (server->ReadSettings("text", &settings) == B_OK)
					textManipulator->Restore(settings);
			}
			manipulator = textManipulator;
		} break;
		default:
			manipulator = NULL;
	}

	return manipulator;
}


void
ManipulatorServer::StoreManipulatorSettings(Manipulator* manipulator)
{
	// Currently the text-manipulator is the only internal manipulator that
	// stores its settings.
	TextManipulator* textManipulator = dynamic_cast<TextManipulator*>(manipulator);
	if (textManipulator) {
		BMessage settings;
		if (textManipulator->Save(settings) == B_OK) {
			if (SettingsServer* server = SettingsServer::Instance())
				server->WriteSettings("text", settings);
		}
	} else {
		GUIManipulator* guiManipulator = dynamic_cast<GUIManipulator*>(manipulator);
		if (guiManipulator) {
			BNode node;
			if (_GetNodeFor(guiManipulator->add_on_id, &node) == B_OK)
				guiManipulator->WriteSettings(&node);
		}
	}
}


status_t
ManipulatorServer::_AddOnLoaderThread(void* data)
{
	BPath path;
	PaintApplication::HomeDirectory(path);
	if (path.Append("add-ons", true) == B_OK)
		fManipulatorServer->_LoadAddOns(path);

	if (find_directory(B_USER_ADDONS_DIRECTORY, &path) == B_OK) {
		path.Append("ArtPaint");
		fManipulatorServer->_LoadAddOns(path);
	}

	if (find_directory(B_SYSTEM_ADDONS_DIRECTORY, &path) == B_OK) {
		path.Append("ArtPaint");
		fManipulatorServer->_LoadAddOns(path);
	}
	fManipulatorServer->fAddonsLoaded = true;

	return B_OK;
}


void
ManipulatorServer::_LoadAddOns(const BPath& path)
{
	BEntry entry;
	if (entry.SetTo(path.Path()) != B_OK)
		return;

	BDirectory addOnDir;
	if (addOnDir.SetTo(&entry) == B_OK) {
		while (addOnDir.GetNextEntry(&entry) == B_OK) {
			BPath addOnPath;
			entry.GetPath(&addOnPath);
			image_id imageId = load_add_on(addOnPath.Path());
			if (imageId > 0) {
				char addOnName[B_FILE_NAME_LENGTH];
				entry.GetName(addOnName);
				if (fAddonNames.find(addOnName) == fAddonNames.end()) {
					fAddonNames.insert(addOnName);
					fAddOnImages.push_back(imageId);
				}
			}
		}
	}
}


status_t
ManipulatorServer::_GetNodeFor(image_id imageId, BNode* node) const
{
	if (imageId > 0 && node != NULL) {
		image_info imageInfo;
		if (get_image_info(imageId, &imageInfo) == B_OK)
			node->SetTo(imageInfo.name);
		return node->InitCheck();
	}
	return B_ERROR;
}
