/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef PROJECT_FILE_FUNCTIONS_H
#define PROJECT_FILE_FUNCTIONS_H

#include <SupportDefs.h>

int64	FindProjectFileSection(BFile&,int32 section_id);


// These are used as markers in the project file.
#define PROJECT_FILE_ID								0x01010101
#define PROJECT_FILE_SECTION_START					0x02020202
#define PROJECT_FILE_SECTION_END					0x02020203
#define PROJECT_FILE_DIMENSION_SECTION_ID			0x11001100
#define PROJECT_FILE_LAYER_SECTION_ID				0x22002200
#define PROJECT_FILE_LAYER_START_MARKER				0x33003300
#define PROJECT_FILE_LAYER_END_MARKER				0x44004400
#define	PROJECT_FILE_LAYER_EXTRA_DATA_START_MARKER	0x55005500
#define	PROJECT_FILE_LAYER_EXTRA_DATA_END_MARKER	0x66006600


// These are constants for the possible compression schemes in the
// project-files.
enum {
	NO_COMPRESSION = 0x00000000,
	QUADTREE_COMPRESSION = 0x00000001,
	RLE_COMPRESSION = 0x00000002,
	ZLIB_COMPRESSION = 0x00000004
};

#endif
