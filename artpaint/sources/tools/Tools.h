/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef TOOLS_H
#define TOOLS_H

#include <SupportDefs.h>


// Here is enumeration that defines the drawing tool constants. After adding a
// constant here we should make also adjustments to places where tools are
// created. We should also change the version number of the prefs-file, because
// tool settings are stored in that file.

enum drawing_tools {
	NO_TOOL					= 999,
	FREE_LINE_TOOL			= 1000,
	STRAIGHT_LINE_TOOL		= 1010,
	RECTANGLE_TOOL			= 1020,
	ELLIPSE_TOOL			= 1030,
	BRUSH_TOOL				= 1040,
	HAIRY_BRUSH_TOOL		= 1050,
	AIR_BRUSH_TOOL			= 1060,
	BLUR_TOOL				= 1070,
	FILL_TOOL				= 1080,
	TEXT_TOOL				= 1090,
	TRANSPARENCY_TOOL		= 1100,
	ERASER_TOOL				= 1110,
	SELECTOR_TOOL			= 1120,
	COLOR_SELECTOR_TOOL		= 1130
};


// These constants define which options a tool can have. They are combined with
// bitwise and.

enum option_constants {
	SIZE_OPTION 				=	0x00000001,
	PRESSURE_OPTION				=	0x00000002,
	MODE_OPTION					=	0x00000004,
	SHAPE_OPTION				=	0x00000008,
	GRADIENT_ENABLED_OPTION		=	0x00000010,		// B_CONTROL_OFF is off, B_CONTROL_ON is on.
	GRADIENT_COLOR_OPTION		=	0x00000020,		// color in BGRA-format
	PREVIEW_ENABLED_OPTION		=	0x00000040,		// B_CONTROL_OFF is preview off, B_CONTROL_ON is on.
	FILL_ENABLED_OPTION			=	0x00000080,
	ROTATION_ENABLED_OPTION		=	0x00000100,
	ANTI_ALIASING_LEVEL_OPTION	=	0x00000200,
	CONTINUITY_OPTION			=	0x00000800,		// B_CONTROL_ON means continuous execution of tool.
	TOLERANCE_OPTION			=	0x00001000,
	TRANSPARENCY_OPTION			=	0x00002000

};


// These constants are used when an option controller doesn't have a numerical
// value (e.g. RadioButtons and menus). The items will have some of these
// constants as their "values".

enum controller_values {
	HS_FREE_LINE					=	'Frln',
	HS_RECTANGLE					=	'Reng',
	HS_MAGIC_WAND					=	'Mwnd',
	HS_INTELLIGENT_SCISSORS			=	'Insc',
	HS_CIRCLE						=	'CirL',
	HS_CORNER_TO_CORNER				=	'CoTc',
	HS_CENTER_TO_CORNER				=	'CeTc',
	HS_ALL_BUTTONS					=	'Albt',
	HS_PRESSED_BUTTONS				=	'Prbt',
	HS_AIRBRUSH_MODE				=	'Aibm',
	HS_SPRAY_MODE					=	'Sprm',
	HS_ERASE_TO_BACKGROUND_MODE		=	'EtBg',
	HS_ERASE_TO_TRANSPARENT_MODE	=	'EtTr'
};


// This struct stores tool's settings, it contains a lot of information that is
// not necessary for every tool, but it is easier to save and load. When we add
// stuff here, we must also add corresponding cases to functions in the
// DrawingTool-class.

struct tool_settings {
	int32	size;
	int32	pressure;
	int32	mode;
	int32	shape;
	int32	gradient_enabled;	// B_CONTROL_OFF or B_CONTROL_ON
	uint32	gradient_color;
	int32	preview_enabled;	// B_CONTROL_OFF or B_CONTROL_ON
	int32	fill_enabled;		// B_CONTROL_OFF or B_CONTROL_ON
	int32	rotation_enabled;	// B_CONTROL_OFF or B_CONTROL_ON
	int32	anti_aliasing_level;
	int32	continuity;
	int32	tolerance;
	int32	transparency;
};

#define	TOOL_SETTINGS_STRUCT_VERSION	0x00000001

#endif
