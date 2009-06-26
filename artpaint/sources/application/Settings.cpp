/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>

#include <string.h>

#include "Settings.h"
#include "PaintWindow.h"
#include "Tools.h"
#include "ColorPalette.h"
#include "PaintApplication.h"
#include "StringServer.h"
#include "Cursors.h"

window_settings::window_settings(const window_settings *s)
{
	version = WINDOW_SETTINGS_VERSION;
	frame_rect = s->frame_rect;
	zoom_level = s->zoom_level;
	view_position = s->view_position;
	views = s->views;
	strcpy(file_mime,s->file_mime);
}

window_settings::window_settings()
{
	version = WINDOW_SETTINGS_VERSION;
	frame_rect = BRect(300,300,600,500);
	zoom_level = 1.0;
	view_position = BPoint(0,0);
	file_type = 0;
	views = HS_STATUS_VIEW | HS_HELP_VIEW;
	strcpy(file_mime,"image/x-be-bitmap");
}


status_t window_settings::read_from_file(BFile &file)
{
	// Read from file one entry at a time.
	if (file.Read(&version,sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;

	// Check the version also.
	if (version != WINDOW_SETTINGS_VERSION)
		return B_ERROR;

	if (file.Read(&frame_rect,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;

	if (file.Read(&zoom_level,sizeof(float)) != sizeof(float))
		return B_ERROR;

	if (file.Read(&view_position,sizeof(BPoint)) != sizeof(BPoint))
		return B_ERROR;

	if (file.Read(&views,sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;

	if (file.Read(&file_type,sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;

	if (file.Read(file_mime,sizeof(file_mime)) != sizeof(file_mime))
		return B_ERROR;

	return B_OK;
}

status_t window_settings::write_to_file(BFile &file)
{
	// Write to file one entry at a time.
	if (file.Write(&version,sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;

	if (file.Write(&frame_rect,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;

	if (file.Write(&zoom_level,sizeof(float)) != sizeof(float))
		return B_ERROR;

	if (file.Write(&view_position,sizeof(BPoint)) != sizeof(BPoint))
		return B_ERROR;

	if (file.Write(&views,sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;

	if (file.Write(&file_type,sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;

	if (file.Write(file_mime,sizeof(file_mime)) != sizeof(file_mime))
		return B_ERROR;

	return B_OK;
}

global_settings::global_settings()
{
	// As last resort just create default-values
	version = GLOBAL_SETTINGS_VERSION;
	layer_window_frame = BRect(300,300,400,400);
	tool_select_window_frame = BRect(50,200,100,250);
	tool_setup_window_frame = BRect(50,350,100,400);
	palette_window_frame = BRect(300,100,400,200);
	brush_window_frame = BRect(20,20,220,220);
	global_setup_window_frame = BRect(100,100,350,300);
	add_on_window_frame = BRect(100,100,200,200);

	layer_window_visible = FALSE;
	tool_select_window_visible = TRUE;
	tool_setup_window_visible = TRUE;
	palette_window_visible = FALSE;
	brush_window_visible = FALSE;
	global_setup_window_visible = FALSE;

	tool_select_window_feel = B_NORMAL_WINDOW_FEEL;
	tool_setup_window_feel = B_NORMAL_WINDOW_FEEL;
	layer_window_feel = B_NORMAL_WINDOW_FEEL;
	palette_window_feel = B_NORMAL_WINDOW_FEEL;
	brush_window_feel = B_NORMAL_WINDOW_FEEL;
	add_on_window_feel = B_NORMAL_WINDOW_FEEL;

	palette_window_mode = HS_RGB_COLOR_MODE;

	primary_tool = FREE_LINE_TOOL;
	secondary_tool = FREE_LINE_TOOL;
	tertiary_tool= FREE_LINE_TOOL;
	setup_window_tool = FREE_LINE_TOOL;

	for (int32 i=0;i<RECENT_LIST_LENGTH;i++) {
		recent_image_width_list[i] = (i+1)*64;
		recent_image_height_list[i] = (i+1)*64;
	}

	rgb_color c = {0,0,0,0};
	primary_color = c;
	secondary_color = c;
	tertiary_color = c;

	language = ENGLISH_LANGUAGE;
	cursor_mode = TOOL_CURSOR_MODE;
	settings_window_tab_number = 0;
	quit_confirm_mode = B_CONTROL_ON;

	undo_queue_depth = 20;

	// Adjust the paths to the defaults.
	BPath path;

	find_directory(B_USER_DIRECTORY, &path);
	if (path.Path() != NULL) {
		strcpy(image_save_path, path.Path());
		strcpy(image_open_path, path.Path());
	}

	if (path.Path() != NULL) {
		strcpy(project_save_path,path.Path());
		strcpy(project_open_path,path.Path());
	}
}

global_settings::~global_settings()
{
}

status_t global_settings::read_from_file(BFile &file)
{
	// Read from file one entry at a time.
	if (file.Read(&version,sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;
	// Check the version also.
	if (version != GLOBAL_SETTINGS_VERSION) {
		version = GLOBAL_SETTINGS_VERSION;
		return B_ERROR;
	}
	if (file.Read(&primary_tool,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (file.Read(&secondary_tool,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (file.Read(&tertiary_tool,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (file.Read(&setup_window_tool,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	if (file.Read(&primary_color,sizeof(rgb_color)) != sizeof(rgb_color))
		return B_ERROR;
	if (file.Read(&secondary_color,sizeof(rgb_color)) != sizeof(rgb_color))
		return B_ERROR;
	if (file.Read(&tertiary_color,sizeof(rgb_color)) != sizeof(rgb_color))
		return B_ERROR;

	if (file.Read(&language,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (file.Read(&cursor_mode,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (file.Read(&settings_window_tab_number,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (file.Read(&quit_confirm_mode,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	if (file.Read(recent_image_width_list,sizeof(recent_image_width_list)) != sizeof(recent_image_width_list))
		return B_ERROR;
	if (file.Read(recent_image_height_list,sizeof(recent_image_height_list)) != sizeof(recent_image_height_list))
		return B_ERROR;

	if (file.Read(&layer_window_frame,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;
	if (file.Read(&tool_select_window_frame,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;
	if (file.Read(&tool_setup_window_frame,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;
	if (file.Read(&palette_window_frame,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;
	if (file.Read(&brush_window_frame,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;
	if (file.Read(&global_setup_window_frame,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;
	if (file.Read(&add_on_window_frame,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;

	if (file.Read(&layer_window_visible,sizeof(bool)) != sizeof(bool))
		return B_ERROR;
	if (file.Read(&tool_setup_window_visible,sizeof(bool)) != sizeof(bool))
		return B_ERROR;
	if (file.Read(&tool_select_window_visible,sizeof(bool)) != sizeof(bool))
		return B_ERROR;
	if (file.Read(&palette_window_visible,sizeof(bool)) != sizeof(bool))
		return B_ERROR;
	if (file.Read(&brush_window_visible,sizeof(bool)) != sizeof(bool))
		return B_ERROR;
	if (file.Read(&global_setup_window_visible,sizeof(bool)) != sizeof(bool))
		return B_ERROR;

	if (file.Read(&layer_window_feel,sizeof(window_feel)) != sizeof(window_feel))
		return B_ERROR;
	if (file.Read(&tool_setup_window_feel,sizeof(window_feel)) != sizeof(window_feel))
		return B_ERROR;
	if (file.Read(&tool_select_window_feel,sizeof(window_feel)) != sizeof(window_feel))
		return B_ERROR;
	if (file.Read(&palette_window_feel,sizeof(window_feel)) != sizeof(window_feel))
		return B_ERROR;
	if (file.Read(&brush_window_feel,sizeof(window_feel)) != sizeof(window_feel))
		return B_ERROR;
	if (file.Read(&add_on_window_feel,sizeof(window_feel)) != sizeof(window_feel))
		return B_ERROR;

	if (file.Read(&palette_window_mode,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	if (file.Read(&undo_queue_depth,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	if (file.Read(image_save_path,sizeof(image_save_path)) != sizeof(image_save_path))
		return B_ERROR;

	if (file.Read(project_save_path,sizeof(project_save_path)) != sizeof(project_save_path))
		return B_ERROR;

	if (file.Read(image_open_path,sizeof(image_open_path)) != sizeof(image_open_path))
		return B_ERROR;

	if (file.Read(project_open_path,sizeof(project_open_path)) != sizeof(project_open_path))
		return B_ERROR;

	int32 recentPathCount;
	if (file.Read(&recentPathCount, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	for (int32 i = 0; i < recentPathCount; ++i) {
		char path[B_PATH_NAME_LENGTH];
		if (file.Read(path, B_PATH_NAME_LENGTH) != B_PATH_NAME_LENGTH)
			return B_ERROR;
		fRecentImagePaths.push_back(BString(path));
	}

	if (file.Read(&recentPathCount, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	for (int32 i = 0; i < recentPathCount; ++i) {
		char path[B_PATH_NAME_LENGTH];
		if (file.Read(path, B_PATH_NAME_LENGTH) != B_PATH_NAME_LENGTH)
			return B_ERROR;
		fRecentProjectPaths.push_back(BString(path));
	}

	if (default_window_settings.read_from_file(file) != B_OK)
		return B_ERROR;

	return B_OK;
}


status_t global_settings::write_to_file(BFile &file)
{
	// Read from file one entry at a time.
	if (file.Write(&version,sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;

	if (file.Write(&primary_tool,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (file.Write(&secondary_tool,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (file.Write(&tertiary_tool,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (file.Write(&setup_window_tool,sizeof(int32)) != sizeof(int32))
		return B_ERROR;


	if (file.Write(&primary_color,sizeof(rgb_color)) != sizeof(rgb_color))
		return B_ERROR;
	if (file.Write(&secondary_color,sizeof(rgb_color)) != sizeof(rgb_color))
		return B_ERROR;
	if (file.Write(&tertiary_color,sizeof(rgb_color)) != sizeof(rgb_color))
		return B_ERROR;

	if (file.Write(&language,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (file.Write(&cursor_mode,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (file.Write(&settings_window_tab_number,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (file.Write(&quit_confirm_mode,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	if (file.Write(recent_image_width_list,sizeof(recent_image_width_list)) != sizeof(recent_image_width_list))
		return B_ERROR;
	if (file.Write(recent_image_height_list,sizeof(recent_image_height_list)) != sizeof(recent_image_height_list))
		return B_ERROR;

	if (file.Write(&layer_window_frame,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;
	if (file.Write(&tool_select_window_frame,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;
	if (file.Write(&tool_setup_window_frame,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;
	if (file.Write(&palette_window_frame,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;
	if (file.Write(&brush_window_frame,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;
	if (file.Write(&global_setup_window_frame,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;
	if (file.Write(&add_on_window_frame,sizeof(BRect)) != sizeof(BRect))
		return B_ERROR;

	if (file.Write(&layer_window_visible,sizeof(bool)) != sizeof(bool))
		return B_ERROR;
	if (file.Write(&tool_setup_window_visible,sizeof(bool)) != sizeof(bool))
		return B_ERROR;
	if (file.Write(&tool_select_window_visible,sizeof(bool)) != sizeof(bool))
		return B_ERROR;
	if (file.Write(&palette_window_visible,sizeof(bool)) != sizeof(bool))
		return B_ERROR;
	if (file.Write(&brush_window_visible,sizeof(bool)) != sizeof(bool))
		return B_ERROR;
	if (file.Write(&global_setup_window_visible,sizeof(bool)) != sizeof(bool))
		return B_ERROR;

	if (file.Write(&layer_window_feel,sizeof(window_feel)) != sizeof(window_feel))
		return B_ERROR;
	if (file.Write(&tool_setup_window_feel,sizeof(window_feel)) != sizeof(window_feel))
		return B_ERROR;
	if (file.Write(&tool_select_window_feel,sizeof(window_feel)) != sizeof(window_feel))
		return B_ERROR;
	if (file.Write(&palette_window_feel,sizeof(window_feel)) != sizeof(window_feel))
		return B_ERROR;
	if (file.Write(&brush_window_feel,sizeof(window_feel)) != sizeof(window_feel))
		return B_ERROR;
	if (file.Write(&add_on_window_feel,sizeof(window_feel)) != sizeof(window_feel))
		return B_ERROR;

	if (file.Write(&palette_window_mode,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	if (file.Write(&undo_queue_depth,sizeof(int32)) != sizeof(int32))
		return B_ERROR;


	if (file.Write(image_save_path,sizeof(image_save_path)) != sizeof(image_save_path))
		return B_ERROR;

	if (file.Write(project_save_path,sizeof(project_save_path)) != sizeof(project_save_path))
		return B_ERROR;

	if (file.Write(image_open_path,sizeof(image_open_path)) != sizeof(image_open_path))
		return B_ERROR;

	if (file.Write(project_open_path,sizeof(project_open_path)) != sizeof(project_open_path))
		return B_ERROR;

	int32 count = fRecentImagePaths.size();
	if (file.Write(&count, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	StringList::const_iterator it;
	const ssize_t lenght = B_PATH_NAME_LENGTH;
	for (it = fRecentImagePaths.begin(); it != fRecentImagePaths.end(); ++it) {
		if (file.Write((*it).String(), lenght) != lenght)
			return B_ERROR;
	}

	count = fRecentProjectPaths.size();
	if (file.Write(&count, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	for (it = fRecentProjectPaths.begin(); it != fRecentProjectPaths.end(); ++it) {
		if (file.Write((*it).String(), lenght) != lenght)
			return B_ERROR;
	}

	if (default_window_settings.write_to_file(file) != B_OK)
		return B_ERROR;

	return B_OK;
}

void global_settings::insert_recent_image_path(const char *path)
{
	_InsertRecentPath(fRecentImagePaths, path);
}

void global_settings::insert_recent_project_path(const char *path)
{
	_InsertRecentPath(fRecentProjectPaths, path);
}

void global_settings::_InsertRecentPath(StringList& list, const BString& path)
{
	if (path.Length() > 0) {
		list.remove(path);
		list.push_front(path);
		if (list.size() > RECENT_LIST_LENGTH)
			list.resize(RECENT_LIST_LENGTH);
	}
}
