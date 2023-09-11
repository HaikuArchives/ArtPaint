/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */

#include "ToolScript.h"


ToolScript::ToolScript(int32 type, tool_settings set, rgb_color c)
{
	tool_type = type;
	settings = set;
	color = c;

	max_points = 1; // At least one point is needed
	points = new BPoint[max_points];
	point_count = 0;
}


ToolScript::~ToolScript()
{
	delete[] points;
}


void
ToolScript::AddPoint(BPoint p)
{
	if (point_count == max_points) {
		max_points = 2 * max_points;
		BPoint* new_points = new BPoint[max_points];
		for (int32 i = 0; i < point_count; i++)
			new_points[i] = points[i];

		delete[] points;
		points = new_points;
	}
	points[point_count++] = p;
}


tool_settings*
ToolScript::ReturnSettings()
{
	return &settings;
}


rgb_color
ToolScript::ReturnColor()
{
	return color;
}


BPoint*
ToolScript::ReturnPoints()
{
	return points;
}


int32
ToolScript::PointCount()
{
	return point_count;
}
