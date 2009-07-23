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

				ImageSizeList::const_iterator si = fRecentImageSizeList.begin();
				for (it = it; it != fRecentProjectPaths.end(); ++it) {
					fApplicationSettings.AddData("recent_image_size", B_RAW_TYPE,
						(const void*)&(*si), sizeof(BSize));
				}

				fApplicationSettings.Flatten(&file);

				fApplicationSettings.RemoveName("recent_image_path");
				fApplicationSettings.RemoveName("recent_project_path");
				fApplicationSettings.RemoveName("recent_image_size");
			}

			if (dir.CreateFile("window", &file, false) == B_OK)
				fWindowSettings.Flatten(&file);
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
		if (status == B_OK)
			*message = fWindowSettings;
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
		if (status == B_OK) {
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
			BSize* data = &size;	// Read the recent image sizes
			while (fApplicationSettings.FindData("recent_image_size", B_RAW_TYPE,
				i++, (const void**)&data, &dataSize) == B_OK) {
				if (dataSize == sizeof(BSize))
					fRecentImageSizeList.push_back(size);
			}
			fApplicationSettings.RemoveName("recent_image_size");

			*message = fApplicationSettings;
		}
	} else {
		status = B_OK;
		*message = fApplicationSettings;
	}

	return status;
}


void
SettingsServer::SetApplicationSettings(const BMessage& message)
{
	fApplicationSettings = message;
}


status_t
SettingsServer::GetDefaultApplicationSettings(BMessage* message)
{
	if (!message)
		return B_ERROR;

	BMessage tmp;
	if (fDefaultApplicationSettings.IsEmpty()) {
		tmp.AddInt32("tool", FREE_LINE_TOOL);
		tmp.AddInt32("language", ENGLISH_LANGUAGE);
		tmp.AddInt32("cursor_mode", TOOL_CURSOR_MODE);
		tmp.AddInt32("settings_window_tab", 0);
		tmp.AddInt32("quit_confirm_mode", B_CONTROL_ON);
		tmp.AddInt32("undo_queue_depth", 20);
		tmp.AddInt32("palette_color_mode", HS_RGB_COLOR_MODE);

		rgb_color black = { 0, 0, 0, 255 };
		tmp.AddData("primary_color", B_RGB_COLOR_TYPE, (const void*)&black,
			sizeof(rgb_color));

		rgb_color white = { 255, 255, 255, 255 };
		tmp.AddData("secondary_color", B_RGB_COLOR_TYPE, (const void*)&white,
			sizeof(rgb_color));

		for (int32 i = 0; i < 10; ++i) {
			const int32 size = (i + 1) * 64;
			fRecentImageSizeList.push_back(BSize(size, size));
		}

		window_feel feel = B_NORMAL_WINDOW_FEEL;

		tmp.AddInt32("layer_window_feel", feel);
		tmp.AddBool("layer_window_visible", false);
		tmp.AddRect("layer_window_frame", BRect(300, 300, 400, 400));

		tmp.AddInt32("setup_tool_window_feel", feel);
		tmp.AddBool("setup_tool_window_visible", true);
		tmp.AddRect("setup_tool_window_frame", BRect(70, 31, 300, 72));

		tmp.AddInt32("select_tool_window_feel", feel);
		tmp.AddBool("select_tool_window_visible", true);
		tmp.AddRect("select_tool_window_frame", BRect(10, 31, 54, 596));

		tmp.AddInt32("palette_window_feel", feel);
		tmp.AddBool("palette_window_visible", false);
		tmp.AddRect("palette_window_frame", BRect(300, 100, 400, 200));

		tmp.AddInt32("brush_window_feel", feel);
		tmp.AddBool("brush_window_visible", false);
		tmp.AddRect("brush_window_frame", BRect(20, 20, 220, 220));

		tmp.AddBool("preferences_window_visible", false);
		tmp.AddRect("preferences_window_frame", BRect(100, 100, 350, 300));

		tmp.AddInt32("add_on_window_feel", feel);
		tmp.AddRect("add_on_window_frame", BRect(100, 100, 200, 200));

		BPath path;
		find_directory(B_USER_DIRECTORY, &path);

		tmp.AddString("image_open_path", path.Path());
		tmp.AddString("image_save_path", path.Path());

		tmp.AddString("project_open_path", path.Path());
		tmp.AddString("project_save_path", path.Path());

		fApplicationSettings = tmp;
	}

	*message = fDefaultApplicationSettings;
	return B_OK;
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
