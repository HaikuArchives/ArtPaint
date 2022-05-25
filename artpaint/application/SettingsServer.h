/*
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef SETTINGSSERVER_H
#define SETTINGSSERVER_H

#include <Locker.h>
#include <Message.h>
#include <String.h>


#include <list>


typedef std::list<BString> StringList;
typedef std::list<BSize> ImageSizeList;


// application
static const char skTool[]						= "tool";
static const char skLanguage[]					= "language";
static const char skCursorMode[]				= "cursor_mode";
static const char skSettingsWindowTab[]			= "settings_window_tab";
static const char skQuitConfirmMode[]			= "quit_confirm_mode";
static const char skUndoQueueDepth[]			= "undo_queue_depth";
static const char skPaletteColorMode[]			= "palette_color_mode";

static const char skPrimaryColor[]				= "primary_color";
static const char skSecondaryColor[]			= "secondary_color";

static const char skLayerWindowFeel[]			= "layer_window_feel";
static const char skLayerWindowVisible[]		= "layer_window_visible";
static const char skLayerWindowFrame[]			= "layer_window_frame";

static const char skToolSetupWindowFeel[]		= "tool_setup_window_feel";
static const char skToolSetupWindowVisible[]	= "tool_setup_window_visible";
static const char skToolSetupWindowFrame[]		= "tool_setup_window_frame";

static const char skSelectToolWindowFeel[]		= "select_tool_window_feel";
static const char skSelectToolWindowVisible[]	= "select_tool_window_visible";
static const char skSelectToolWindowFrame[]		= "select_tool_window_frame";

static const char skPaletteWindowFeel[]			= "palette_window_feel";
static const char skPaletteWindowVisible[]		= "palette_window_visible";
static const char skPaletteWindowFrame[]		= "palette_window_frame";

static const char skBrushWindowFeel[]			= "brush_window_feel";
static const char skBrushWindowVisible[]		= "brush_window_visible";
static const char skBrushWindowFrame[]			= "brush_window_frame";

static const char skAddOnWindowFeel[]			= "add_on_window_feel";
static const char skAddOnWindowFrame[]			= "add_on_window_frame";

static const char skSettingsWindowFrame[]		= "settings_window_frame";

static const char skImageOpenPath[]				= "image_open_path";
static const char skImageSavePath[]				= "image_save_path";
static const char skProjectOpenPath[]			= "project_open_path";
static const char skProjectSavePath[]			= "project_save_path";


// paintwindow
static const char skZoom[]						= "zoom";
static const char skFrame[]						= "frame";
static const char skTranslatorType[]			= "translator_type";
static const char skPosition[]					= "position";
static const char skMimeType[]					= "mime_type";
static const char skViews[]						= "views";

static const char skBgGridSize[]				= "bg_grid_size";
static const char skBgColor1[]					= "bg_color1";
static const char skBgColor2[]					= "bg_color2";


class SettingsServer
{
	friend class PaintApplication;

public:
			enum Setting {
				Application = 1000,
				PaintWindow = 2000
			};

	static	SettingsServer*			Instance();

	static	status_t				ReadSettings(const BString& fileName,
										BMessage* settings);
	static	status_t				WriteSettings(const BString& fileName,
										const BMessage& settings);

			status_t				GetWindowSettings(BMessage* message);
			void					SetWindowSettings(const BMessage& message);
			status_t				GetDefaultWindowSettings(BMessage* message);

			status_t				GetApplicationSettings(BMessage* message);

			status_t				SetValue(Setting type, const BString& field,
										bool value);
			status_t				SetValue(Setting type, const BString& field,
										int32 value);
			status_t				SetValue(Setting type, const BString& field,
										uint32 value);
			status_t				SetValue(Setting type, const BString& field,
										const BRect& value);
			status_t				SetValue(Setting type, const BString& field,
										const BPoint& value);
			status_t				SetValue(Setting type, const BString& field,
										const char* value);
			status_t				SetValue(Setting type, const BString& field,
										const BString& value);
			status_t				SetValue(Setting type, const BString& field,
										type_code typeCode, const void* value,
										ssize_t size);

			const StringList&		RecentImagePaths() const;
			void					AddRecentImagePath(const BString& path);
			void					RemoveRecentImagePath(const BString& path);

			const StringList&		RecentProjectPaths() const;
			void					AddRecentProjectPath(const BString& path);
			void					RemoveRecentProjectPath(const BString& path);

			const ImageSizeList&	RecentImageSizes() const;
			void					AddRecentImageSize(const BSize& size);

			void					Sync();

private:
									SettingsServer();
									SettingsServer(const SettingsServer& server);
									~SettingsServer();

	static	SettingsServer*			Instantiate();
	static	void					DestroyServer();

			status_t				_ReadSettingsFor(const BString& name,
										BMessage* settings);
			status_t				_WriteSettingsFor(const BString& name,
										const BMessage& settings);

			void					_InsertRecentPath(const BString& path,
										StringList& list);
			BMessage*				_SettingsForType(Setting type);
			void					_GetDefaultAppSettings(BMessage* message);

private:
			BMessage				fWindowSettings;
			BMessage				fDefaultWindowSettings;

			BMessage				fApplicationSettings;

			StringList				fRecentImagePaths;
			StringList				fRecentProjectPaths;

			ImageSizeList			fRecentImageSizeList;

	static	BLocker					fLocker;
	static	SettingsServer*			fSettingsServer;
};

#endif // SETTINGSSERVER_H
