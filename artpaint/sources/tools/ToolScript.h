/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef TOOL_SCRIPT_H
#define TOOL_SCRIPT_H

#include <InterfaceDefs.h>
#include <Point.h>

#include "Tools.h"

class ToolScript {
	rgb_color 		color;
	tool_settings	settings;
	int32			tool_type;

	BPoint			*points;
	int32			point_count;
	int32			max_points;

public:
		ToolScript(int32,tool_settings,rgb_color);
		~ToolScript();

void			AddPoint(BPoint);

tool_settings*	ReturnSettings();
rgb_color		ReturnColor();
BPoint*			ReturnPoints();
int32			PointCount();
};


#endif
