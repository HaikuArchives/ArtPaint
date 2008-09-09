/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <stdlib.h>


#include "LineTool.h"

// this class is a base-class for line-drawing tools
LineTool::LineTool(const char *tool_name, int32 tool_type)
		:	DrawingTool(tool_name,tool_type)
{
	// here initialize few member variables
	point_list = NULL;
	number_of_points = 0;
}


LineTool::~LineTool()
{
	// free whatever storage we allocated e.g. the pointlist

	delete[] point_list;
}


ToolScript* LineTool::UseTool(ImageView*,uint32,BPoint,BPoint)
{
	// this class still does not implement drawing functionality
	return NULL;
}

void LineTool::calculateLineDDA(BPoint start, BPoint end)
{
	// use DDA-algorithm to calculate line between the two argument points
	// place the resulting points into point_list

	// first check whether the line is longer in x direction than y
	bool increase_x = fabs(start.x - end.x) >= fabs(start.y - end.y);
	// check which direction the line is going
	float sign_x;
	float sign_y;

	if ((end.x-start.x) != 0) {
		sign_x = (end.x-start.x)/fabs(start.x - end.x);
	}
	else {
		sign_x = 0;
	}
	if ((end.y-start.y) != 0) {
		sign_y = (end.y-start.y)/fabs(start.y - end.y);
	}
	else {
		sign_y = 0;
	}
	// delete the previous point-list
	delete[] point_list;
	number_of_points = 0;
	if (increase_x) {
		// allocate the point-list
		number_of_points = 	(int32)fabs(start.x - end.x) + 1;
		point_list = new BPoint[number_of_points];
		float y_add = ((float)fabs(start.y - end.y)) / ((float)fabs(start.x - end.x));

		for (int32 i=0;i<number_of_points;i++) {
			point_list[i] = start;
			start.x += sign_x;
			start.y += sign_y * y_add;
		}
	}

	else {
		number_of_points = 	(int32)fabs(start.y - end.y) + 1;
		point_list = new BPoint[number_of_points];
		float x_add = ((float)fabs(start.x - end.x)) / ((float)fabs(start.y - end.y));

		for (int32 i=0;i<number_of_points;i++) {
			point_list[i] = start;
			start.y += sign_y;
			start.x += sign_x * x_add;
		}
	}
}
