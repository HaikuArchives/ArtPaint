/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef SETTINGS_H
#define SETTINGS_H

#include <InterfaceDefs.h>
#include <Mime.h>
#include <Rect.h>
#include <Window.h>

// each window has it's own window-settings and they are stored into
// project-file or image-file's attributes
// global settings also has a default window setting that is used if no-other
// setting is provided
// whenever we change this structure we should do the following:
//
//		1.	change settings constructors
//		2.	increment the SETTINGS_VERSIONs
//		3.	change the functions that read and write settings

#define WINDOW_SETTINGS_VERSION	0x01000004
#define GLOBAL_SETTINGS_VERSION 0x01000016

//#define	SETTINGS_FILE_VERSION	0x00000013


#define	RECENT_LIST_LENGTH		10

struct window_settings {
	uint32		version;			// version number of this struct
	BRect		frame_rect;			// this is window's frame rectangle
	float		zoom_level;			// this is the current zoom level
	BPoint		view_position;		// the position to which the view has been scrolled
	uint32		views;				// what views are opened

	uint32		file_type;			// the magic number for file-type
	char		file_mime[B_MIME_TYPE_LENGTH+10];	// and the mime-string

	window_settings(const window_settings*);
	window_settings();

status_t	read_from_file(BFile&);
status_t	write_to_file(BFile&);
};

// these are the global settings
struct global_settings {
	uint32		version;			// version number of this struct



	int32		primary_tool;		// these are the tools
	int32		secondary_tool;		// for all three mousebuttons
	int32		tertiary_tool;

	int32		setup_window_tool;	// The tool that is selected in the setup-window.

	rgb_color	primary_color;		// These are the colors
	rgb_color	secondary_color;	// that are in use for
	rgb_color	tertiary_color;		// each mousebutton.

	int32		language;
	int32		cursor_mode;
	int32		settings_window_tab_number;
	int32		quit_confirm_mode;

	int32		recent_image_width_list[RECENT_LIST_LENGTH];
	int32		recent_image_height_list[RECENT_LIST_LENGTH];

	BRect		layer_window_frame;			// these are the frame
	BRect		tool_select_window_frame;	// rectangles for various
	BRect		tool_setup_window_frame;	// global windows, when opening
	BRect		palette_window_frame;		// windows we should check that
	BRect		brush_window_frame;			// they are within the screen's bounds
	BRect		global_setup_window_frame;
	BRect		add_on_window_frame;

	bool		layer_window_visible;
	bool		tool_setup_window_visible;
	bool		tool_select_window_visible;
	bool		palette_window_visible;
	bool		brush_window_visible;
	bool		global_setup_window_visible;


	// These are the window feels: B_NORMAL_WINDOW_FEEL or B_FLOATING_SUBSET_WINDOW_FEEL
	// or perhaps B_FLOATING_APP_FEEL
	window_feel	layer_window_feel;
	window_feel	tool_setup_window_feel;
	window_feel tool_select_window_feel;
	window_feel	palette_window_feel;
	window_feel brush_window_feel;
	window_feel	add_on_window_feel;

	int32		palette_window_mode;

	int32		undo_queue_depth;

	char		image_save_path[B_PATH_NAME_LENGTH];
	char		project_save_path[B_PATH_NAME_LENGTH];
	char		image_open_path[B_PATH_NAME_LENGTH];
	char		project_open_path[B_PATH_NAME_LENGTH];

	char		*recent_image_paths[RECENT_LIST_LENGTH];
	char		*recent_project_paths[RECENT_LIST_LENGTH];

	window_settings	default_window_settings;	// this stores the default-settings
												// for a paint-window



	global_settings();
	~global_settings();

status_t	read_from_file(BFile&);
status_t	write_to_file(BFile&);



void			insert_recent_image_path(const char*);
void			insert_recent_project_path(const char*);
};


#endif
