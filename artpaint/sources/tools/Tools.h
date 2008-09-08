/*

	Filename:	Tools.h
	Contents:	Constants that define the tools
	Author:		Heikki Suhonen

*/

#ifndef TOOLS_H
#define TOOLS_H

#include <SupportDefs.h>

// Here is enumeration that defines the drawing tool constants.
// After adding a constant here we should make also adjustments to
// places where tools are created (i.e. in ArtPaintApp.cpp).
// We should also change the version number of the prefs-file, because
// tool settings are stored in that file.
enum drawing_tools {
	NO_TOOL =	'NoTl',
	FREE_LINE_TOOL =	'SiTT',
	SELECTOR_TOOL =	'SelT',
	FILL_TOOL = 'FlTl',
	RECTANGLE_TOOL = 'RcTl',
	ELLIPSE_TOOL = 'EliT',
	STRAIGHT_LINE_TOOL = 'SlnT',
	BRUSH_TOOL = 'brTl',
	BLUR_TOOL = 'BluT',
	COLOR_SELECTOR_TOOL	= 'CslT',
	TRANSPARENCY_TOOL = 'TraT',
	AIR_BRUSH_TOOL = 'Abrt',
	HAIRY_BRUSH_TOOL = 'Hbrt',
	ERASER_TOOL = 'Erst',
	TEXT_TOOL = 'Txtl'
};


//#define	NO_TOOL_NAME				"No Tool"
//#define	SIMPLE_TEST_TOOL_NAME		"Free Line"
//#define	SELECTOR_TOOL_NAME			"Selector Tool"
//#define	FILL_TOOL_NAME				"Fill"
//#define RECTANGLE_TOOL_NAME			"Rectangle"
//#define	ELLIPSE_TOOL_NAME			"Ellipse"
//#define STRAIGHT_LINE_TOOL_NAME		"Line"
//#define	BRUSH_TOOL_NAME				"Brush Stroke"
//#define BLUR_TOOL_NAME				"Blur"
//#define COLOR_SELECTOR_TOOL_NAME	"Color Selector"
//#define	TRANSPARENCY_TOOL_NAME		"Transparency Tool"
//#define AIR_BRUSH_TOOL_NAME			"AirBrush"
//#define	HAIRY_BRUSH_TOOL_NAME		"Hairy Brush"
//#define ERASER_TOOL_NAME			"Eraser"
//#define TEXT_TOOL_NAME				"Text Tool"

// This defines the tool name that has most letters.
// But the name that has the longest display representation may
// vary due to different fonts and such. The name should be
// reasonably long because tool-setup-window's width is based on it.
//#define HS_LONGEST_TOOL_NAME	"Transparency Tool"


// These constants define which options a tool can have.
// They are combined with bitwise and.
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


// These constants are used when an option controller doesn't have
// a numerical value (e.g. RadioButtons and menus).
// The items will have some of these constants as their "values".
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



// this struct stores tool's settings, it contains a lot of
// information that is not necessary for every tool, but it
// is easier to save and load
// When we add stuff here, we must also add corresponding cases
// to functions in the DrawingTool-class.
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
