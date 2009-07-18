/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef SIMPLE_TOOL_H
#define SIMPLE_TOOL_H

#include "LineTool.h"


class BBitmap;
class BView;
class ControlSliderBox;
class CoordinateQueue;
class ImageView;
class ToolScript;


class SimpleTool : public LineTool {
public:
									SimpleTool();
	virtual							~SimpleTool();

			int32					UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*				UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*					makeConfigView();
			const void*				ToolCursor() const;
			const char*				HelpString(bool isInUse) const;

private:
			int32					read_coordinates();
	static	int32					CoordinateReader(void*);

private:
			bool					reading_coordinates;

			ImageView*				image_view;
			CoordinateQueue*		coordinate_queue;
};



class SimpleToolConfigView : public DrawingToolConfigView {
public:
									SimpleToolConfigView(BRect rect,
										DrawingTool* tool);

	virtual	void					AttachedToWindow();

private:
			ControlSliderBox*		size_slider;
};

#endif
