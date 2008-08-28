/* 

	Filename:	LineTool.h
	Contents:	LineTool-class declaration	
	Author:		Heikki Suhonen
	
*/


#ifndef LINE_TOOL_H
#define LINE_TOOL_H

#include "DrawingTool.h"

// This class defines common elements for tools that draw some kind of lines.

class LineTool : public DrawingTool {
protected:
// these are for the pointlist that is to be built by DDA
	BPoint *point_list;
	int32 number_of_points;

// this function calculates all the points on line between two points
// using DDA-algorithm
void	calculateLineDDA(BPoint start, BPoint end);

public:
		LineTool(const char *tool_name, int32 tool_type);
virtual	~LineTool();
	
ToolScript*	UseTool(ImageView*,uint32,BPoint,BPoint);
};

#endif