/*
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "SettingsServer.h"

#include "ColorPalette.h"
#include "Cursors.h"
#include "PaintWindow.h"
#include "Tools.h"


#include <Autolock.h>
#include <Control.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>


#include <new>


BLocker SettingsServer::fLocker;
SettingsServer* SettingsServer::fSettingsServer = NULL;


SettingsServer::SettingsServer()
{
	BMessage dummy;
	if (GetApplicationSettings(&dummy) != B_OK)
		_GetDefaultAppSettings(&fApplicationSettings);
}


SettingsServer::~SettingsServer()
{
	Sync();
	fSettingsServer = NULL;
}


SettingsServer*
SettingsServer::Instantiate()
{
	if (fSettingsServer == NULL) {
		BAutolock _(&fLocker);
		if (fSettingsServer == NULL)
			fSettingsServer = new (std::nothrow) SettingsServer();
	}
	return fSettingsServer;
}


void
SettingsServer::DestroyServer()
{
	if (fSettingsServer) {
		delete fSettingsServer;
		fSettingsServer = NULL;
	}
}


SettingsServer*
SettingsServer::Instance()
{
	return Instantiate();
}


void
SettingsServer::Sync()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		BDirectory dir(path.Path());
		status_t status = dir.CreateDirectory("./ArtPaint", &dir);
		if (status == B_FILE_EXISTS)
			status = dir.SetTo(&dir, "ArtPaint/");

		if (status == B_OK) {
			BFile file;
			if (dir.CreateFile("app", &file, false) == B_OK) {
				// Add the recent paths lists to the massage before written
				// out, so we can restore them on the next application start
				StringList::const_iterator it = fRecentImagePaths.begin();
				for (it = it; it != fRecentImagePaths.end(); ++it)
					fApplicationSettings.AddString("recent_image_path", *it);

				it = fRecentProjectPaths.begin();
				for (it = it; it != fRecentProjectPaths.end(); ++it)
					fApplicationSettings.AddString("recent_project_path", *it);

				// Add the recent image sizes to the massage before written
				// out, so we can restore them on the next application start
				ImageSizeList::const_iterator si = fRecentImageSizeList.begin();
				for (si = si; si != fRecentImageSizeList.end(); ++si) {
					fApplicationSettings.AddData("recent_image_size", B_RAW_TYPE,
						(const void*)&(*si), sizeof(BSize));
				}

				fApplicationSettings.Flatten(&file);

				fApplicationSettings.RemoveName("recent_image_path");
				fApplicationSettings.RemoveName("recent_project_path");
				fApplicationSettings.RemoveName("recent_image_size");
			}

			if (dir.CreateFile("window", &file, false) == B_OK) {
				if (!fWindowSettings.IsEmpty())
					fWindowSettings.Flatten(&file);
			}
		}
	}
}


status_t
SettingsServer::GetWindowSettings(BMessage* message)
{
	status_t status = B_ERROR;

	if (!message)
		return status;

	message->MakeEmpty();
	if (fWindowSettings.IsEmpty()) {
		status = _ReadSettings("window", fWindowSettings);
		if (status == B_OK && !fWindowSettings.IsEmpty())
			*message = fWindowSettings;
		else
			status = B_ERROR;
	} else {
		status = B_OK;
		*message = fWindowSettings;
	}

	return status;
}


void
SettingsServer::SetWindowSettings(const BMessage& message)
{
	fWindowSettings = message;
}


status_t
SettingsServer::GetDefaultWindowSettings(BMessage* message)
{
	if (!message)
		return B_ERROR;

	if (fDefaultWindowSettings.IsEmpty()) {
		fDefaultWindowSettings.AddFloat(skZoom, 1.0);
		fDefaultWindowSettings.AddUInt32(skTranslatorType, 0);
		fDefaultWindowSettings.AddPoint(skPosition, BPoint(0.0, 0.0));
		fDefaultWindowSettings.AddString(skMimeType, "image/x-be-bitmap");
		fDefaultWindowSettings.AddUInt32(skViews, HS_STATUS_VIEW | HS_HELP_VIEW);
		fDefaultWindowSettings.AddRect(skFrame, BRect(74.0, 92.0, 507.0, 466.0));
	}
	*message = fDefaultWindowSettings;
	return B_OK;
}


status_t
SettingsServer::GetApplicationSettings(BMessage* message)
{
	status_t status = B_ERROR;

	if (!message)
		return status;

	int32 i = 0;
	message->MakeEmpty();
	if (fApplicationSettings.IsEmpty()) {
		status = _ReadSettings("app", fApplicationSettings);
		if (status == B_OK && !fApplicationSettings.IsEmpty()) {
			BString path;	// Read the recent image paths
			while (fApplicationSettings.FindString("recent_image_path", i++,
				&path) == B_OK) {
				fRecentImagePaths.push_back(path);
			}
			fApplicationSettings.RemoveName("recent_image_path");

			i = 0;	// Read the recent project paths
			while (fApplicationSettings.FindString("recent_project_path", i++,
				&path) == B_OK) {
				fRecentProjectPaths.push_back(path);
			}
			fApplicationSettings.RemoveName("recent_project_path");

			i = 0;
			BSize size;
			ssize_t dataSize;
			const BSize* data;	// Read the recent image sizes
			while (fApplicationSettings.FindData("recent_image_size", B_RAW_TYPE,
				i++, (const void**)&data, &dataSize) == B_OK) {
				if (dataSize == sizeof(BSize)) {
					memcpy(&size, data, sizeof(BSize));
					fRecentImageSizeList.push_back(size);
				}
			}
			fApplicationSettings.RemoveName("recent_image_size");

			*message = fApplicationSettings;
		} else {
			status = B_ERROR;
		}
	} else {
		status = B_OK;
		*message = fApplicationSettings;
	}

	return status;
}


status_t
SettingsServer::SetValue(Setting type, const BString& field, bool value)
{
	if (BMessage* settings = _SettingsForType(type)) {
		if (settings->RemoveName(field.String()) == B_OK)
			return settings->AddBool(field.String(), value);
	}
	return B_ERROR;
}


status_t
SettingsServer::SetValue(Setting type, const BString& field, int32 value)
{
	if (BMessage* settings = _SettingsForType(type)) {
		if (settings->RemoveName(field.String()) == B_OK)
			return settings->AddInt32(field.String(), value);
	}
	return B_ERROR;
}


status_t
SettingsServer::SetValue(Setting type, const BString& field, const BRect& value)
{
	if (BMessage* settings = _SettingsForType(type)) {
		if (settings->RemoveName(field.String()) == B_OK)
			return settings->AddRect(field.String(), value);
	}
	return B_ERROR;
}


status_t
SettingsServer::SetValue(Setting type, const BString& field, const BPoint& value)
{
	if (BMessage* settings = _SettingsForType(type)) {
		if (settings->RemoveName(field.String()) == B_OK)
			return settings->AddPoint(field.String(), value);
	}
	return B_ERROR;
}


status_t
SettingsServer::SetValue(Setting type, const BString& field, const char* value)
{
	if (BMessage* settings = _SettingsForType(type)) {
		if (settings->RemoveName(field.String()) == B_OK)
			return settings->AddString(field.String(), value);
	}
	return B_ERROR;
}


status_t
SettingsServer::SetValue(Setting type, const BString& field, const BString& value)
{
	if (BMessage* settings = _SettingsForType(type)) {
		if (settings->RemoveName(field.String()) == B_OK)
			return settings->AddString(field.String(), value);
	}
	return B_ERROR;
}


status_t
SettingsServer::SetValue(Setting type, const BString& field, type_code typeCode,
	const void* value)
{
	if (BMessage* settings = _SettingsForType(type)) {
		if (settings->RemoveName(field.String()) == B_OK) {
			return settings->AddData(field.String(), typeCode, value,
				sizeof(value));
		}
	}
	return B_ERROR;
}


const StringList&
SettingsServer::RecentImagePaths() const
{
	return fRecentImagePaths;
}


void
SettingsServer::AddRecentImagePath(const BString& path)
{
	_InsertRecentPath(path, fRecentImagePaths);
}


const StringList&
SettingsServer::RecentProjectPaths() const
{
	return fRecentProjectPaths;
}


void
SettingsServer::AddRecentProjectPath(const BString& path)
{
	_InsertRecentPath(path, fRecentProjectPaths);
}


const ImageSizeList&
SettingsServer::RecentImageSizes() const
{
	return fRecentImageSizeList;
}


void
SettingsServer::AddRecentImageSize(const BSize& size)
{
	fRecentImageSizeList.remove(size);
	fRecentImageSizeList.push_front(size);
	if (fRecentImageSizeList.size() > 10)
		fRecentImageSizeList.resize(10);
}


status_t
SettingsServer::_ReadSettings(const BString& name, BMessage& message)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("ArtPaint");

		BDirectory dir(path.Path());
		if (dir.InitCheck() == B_OK && dir.IsDirectory()) {
			BEntry entry;
			if (dir.FindEntry(name.String(), &entry) == B_OK) {
				BFile file(&entry, B_READ_ONLY);
				return message.Unflatten(&file);
			}
		}
	}
	return B_ERROR;
}


void
SettingsServer::_InsertRecentPath(const BString& path, StringList& list)
{
	if (path.Length() > 0) {
		list.remove(path);
		list.push_front(path);
		if (list.size() > 10)
			list.resize(10);
	}
}


BMessage*
SettingsServer::_SettingsForType(Setting type)
{
	BMessage* settings;
	switch (type) {
		case Application: {
			settings = &fApplicationSettings;
		}	break;

		case PaintWindow: {
			settings = &fWindowSettings;
		}	break;

		default: {
			settings = NULL;
		}	break;
	}
	return settings;
}


void
SettingsServer::_GetDefaultAppSettings(BMessage* message)
{
	message->AddInt32("tool", FREE_LINE_TOOL);
	message->AddInt32("language", ENGLISH_LANGUAGE);
	message->AddInt32("cursor_mode", TOOL_CURSOR_MODE);
	message->AddInt32("settings_window_tab", 0);
	message->AddInt32("quit_confirm_mode", B_CONTROL_ON);
	message->AddInt32("undo_queue_depth", 20);
	message->AddInt32("palette_color_mode", HS_RGB_COLOR_MODE);

	rgb_color black = { 0, 0, 0, 255 };
	message->AddData("primary_color", B_RGB_COLOR_TYPE, (const void*)&black,
		sizeof(rgb_color));

	rgb_color white = { 255, 255, 255, 255 };
	message->AddData("secondary_color", B_RGB_COLOR_TYPE, (const void*)&white,
		sizeof(rgb_color));

	for (int32 i = 0; i < 10; ++i) {
		const int32 size = (i + 1) * 64;
		fRecentImageSizeList.push_back(BSize(size, size));
	}

	window_feel feel = B_NORMAL_WINDOW_FEEL;

	message->AddInt32("layer_window_feel", feel);
	message->AddBool("layer_window_visible", false);
	message->AddRect("layer_window_frame", BRect(300, 300, 400, 400));

	message->AddInt32("setup_tool_window_feel", feel);
	message->AddBool("setup_tool_window_visible", true);
	message->AddRect("setup_tool_window_frame", BRect(70, 31, 300, 72));

	message->AddInt32("select_tool_window_feel", feel);
	message->AddBool("select_tool_window_visible", true);
	message->AddRect("select_tool_window_frame", BRect(10, 31, 54, 596));

	message->AddInt32("palette_window_feel", feel);
	message->AddBool("palette_window_visible", false);
	message->AddRect("palette_window_frame", BRect(300, 100, 400, 200));

	message->AddInt32("brush_window_feel", feel);
	message->AddBool("brush_window_visible", false);
	message->AddRect("brush_window_frame", BRect(20, 20, 220, 220));

	message->AddBool("preferences_window_visible", false);
	message->AddRect("preferences_window_frame", BRect(100, 100, 350, 300));

	message->AddInt32("add_on_window_feel", feel);
	message->AddRect("add_on_window_frame", BRect(100, 100, 200, 200));

	BPath path;
	find_directory(B_USER_DIRECTORY, &path);

	message->AddString("image_open_path", path.Path());
	message->AddString("image_save_path", path.Path());

	message->AddString("project_open_path", path.Path());
	message->AddString("project_save_path", path.Path());
}
