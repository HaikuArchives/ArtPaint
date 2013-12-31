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


const uint32 gListSize = 10;


const char skRecentImageSize[] = "recent_image_size";
const char skRecentImagePath[] = "recent_image_path";
const char skRecentProjectPath[] = "recent_project_path";


BLocker SettingsServer::fLocker;
SettingsServer* SettingsServer::fSettingsServer = NULL;


SettingsServer*
SettingsServer::Instance()
{
	return Instantiate();
}


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


status_t
SettingsServer::ReadSettings(const BString& name, BMessage* settings)
{
	if (!settings)
		return B_ERROR;

	BPath path;
	settings->MakeEmpty();
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("ArtPaint");

		BDirectory dir(path.Path());
		if (dir.InitCheck() == B_OK && dir.IsDirectory()) {
			BEntry entry;
			if (dir.FindEntry(name.String(), &entry) == B_OK) {
				BFile file(&entry, B_READ_ONLY);
				return settings->Unflatten(&file);
			}
		}
	}
	return B_ERROR;
}


status_t
SettingsServer::WriteSettings(const BString& name, const BMessage& settings)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {

		BDirectory dir(path.Path());
		status_t status = dir.CreateDirectory("./ArtPaint", &dir);
		if (status == B_FILE_EXISTS)
			status = dir.SetTo(&dir, "ArtPaint/");

		if (status == B_OK) {
			BFile file;
			if (dir.CreateFile(name.String(), &file, false) == B_OK)
				return settings.Flatten(&file);
		}
	}
	return B_ERROR;
}


status_t
SettingsServer::GetWindowSettings(BMessage* message)
{
	status_t status = B_ERROR;

	if (!message)
		return status;

	if (fWindowSettings.IsEmpty()) {
		status = ReadSettings("window", &fWindowSettings);
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
		fDefaultWindowSettings.AddRect(skFrame, BRect(74.0, 112.0, 507.0, 486.0));
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
	if (fApplicationSettings.IsEmpty()) {
		status = ReadSettings("application", &fApplicationSettings);
		if (status == B_OK && !fApplicationSettings.IsEmpty()) {
			BString path;	// Read the recent image paths
			while (fApplicationSettings.FindString(skRecentImagePath, i++,
				&path) == B_OK) {
				fRecentImagePaths.push_back(path);
			}
			fApplicationSettings.RemoveName(skRecentImagePath);

			i = 0;	// Read the recent project paths
			while (fApplicationSettings.FindString(skRecentProjectPath, i++,
				&path) == B_OK) {
				fRecentProjectPaths.push_back(path);
			}
			fApplicationSettings.RemoveName(skRecentProjectPath);

			i = 0;
			BSize size;
			ssize_t dataSize;
			const BSize* data;	// Read the recent image sizes
			while (fApplicationSettings.FindData(skRecentImageSize, B_RAW_TYPE,
				i++, (const void**)&data, &dataSize) == B_OK) {
				if (dataSize == sizeof(BSize)) {
					memcpy(&size, data, sizeof(BSize));
					fRecentImageSizeList.push_back(size);
				}
			}
			fApplicationSettings.RemoveName(skRecentImageSize);

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


void
SettingsServer::RemoveRecentImagePath(const BString& path)
{
	fRecentImagePaths.remove(path);
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


void
SettingsServer::RemoveRecentProjectPath(const BString& path)
{
	fRecentProjectPaths.remove(path);
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
	if (fRecentImageSizeList.size() > gListSize)
		fRecentImageSizeList.resize(gListSize);
}


void
SettingsServer::Sync()
{
	// Add the recent paths lists to the massage before written out, so we can
	// restore them on the next application start
	StringList::const_iterator it = fRecentImagePaths.begin();
	for (it = it; it != fRecentImagePaths.end(); ++it)
		fApplicationSettings.AddString(skRecentImagePath, *it);

	it = fRecentProjectPaths.begin();
	for (it = it; it != fRecentProjectPaths.end(); ++it)
		fApplicationSettings.AddString(skRecentProjectPath, *it);

	// Add the recent image sizes to the message before written out, so we can
	// restore them on the next application start
	ImageSizeList::const_iterator si = fRecentImageSizeList.begin();
	for (si = si; si != fRecentImageSizeList.end(); ++si) {
		fApplicationSettings.AddData(skRecentImageSize, B_RAW_TYPE,
			(const void*)&(*si), sizeof(BSize));
	}

	WriteSettings("application", fApplicationSettings);

	fApplicationSettings.RemoveName(skRecentImagePath);
	fApplicationSettings.RemoveName(skRecentProjectPath);
	fApplicationSettings.RemoveName(skRecentImageSize);

	if (!fWindowSettings.IsEmpty())
		WriteSettings("window", fWindowSettings);
}


void
SettingsServer::_InsertRecentPath(const BString& path, StringList& list)
{
	if (path.Length() > 0) {
		list.remove(path);
		list.push_front(path);
		if (list.size() > gListSize)
			list.resize(gListSize);
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
	message->AddInt32(skTool, FREE_LINE_TOOL);
	message->AddInt32(skLanguage, ENGLISH_LANGUAGE);
	message->AddInt32(skCursorMode, TOOL_CURSOR_MODE);
	message->AddInt32(skSettingsWindowTab, 0);
	message->AddInt32(skQuitConfirmMode, B_CONTROL_ON);
	message->AddInt32(skUndoQueueDepth, 20);
	message->AddInt32(skPaletteColorMode, HS_RGB_COLOR_MODE);

	rgb_color black = { 0, 0, 0, 255 };
	message->AddData(skPrimaryColor, B_RGB_COLOR_TYPE, (const void*)&black,
		sizeof(rgb_color));

	rgb_color white = { 255, 255, 255, 255 };
	message->AddData(skSecondaryColor, B_RGB_COLOR_TYPE, (const void*)&white,
		sizeof(rgb_color));

	for (uint32 i = 0; i < gListSize; ++i) {
		const int32 size = (i + 1) * 64;
		fRecentImageSizeList.push_back(BSize(size, size));
	}

	window_feel feel = B_NORMAL_WINDOW_FEEL;

	message->AddInt32(skLayerWindowFeel, feel);
	message->AddBool(skLayerWindowVisible, false);
	message->AddRect(skLayerWindowFrame, BRect(300, 300, 400, 400));

	message->AddInt32(skToolSetupWindowFeel, feel);
	message->AddBool(skToolSetupWindowVisible, true);
	message->AddRect(skToolSetupWindowFrame, BRect(70, 31, 300, 72));

	message->AddInt32(skSelectToolWindowFeel, feel);
	message->AddBool(skSelectToolWindowVisible, true);
	message->AddRect(skSelectToolWindowFrame, BRect(10, 31, 54, 596));

	message->AddInt32(skPaletteWindowFeel, feel);
	message->AddBool(skPaletteWindowVisible, false);
	message->AddRect(skPaletteWindowFrame, BRect(300, 100, 400, 200));

	message->AddInt32(skBrushWindowFeel, feel);
	message->AddBool(skBrushWindowVisible, false);
	message->AddRect(skBrushWindowFrame, BRect(20, 20, 220, 220));

	message->AddInt32(skAddOnWindowFeel, feel);
	message->AddRect(skAddOnWindowFrame, BRect(100, 100, 200, 200));

	message->AddRect(skSettingsWindowFrame, BRect(100, 100, 350, 300));

	BPath path;
	find_directory(B_USER_DIRECTORY, &path);

	message->AddString(skImageOpenPath, path.Path());
	message->AddString(skImageSavePath, path.Path());

	message->AddString(skProjectOpenPath, path.Path());
	message->AddString(skProjectSavePath, path.Path());
}
