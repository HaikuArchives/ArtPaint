/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef ERASER_TOOL_H
#define ERASER_TOOL_H

#include "LineTool.h"


class BRadioButton;
class ControlSliderBox;
class CoordinateQueue;


class EraserTool : public LineTool {
public:
								EraserTool();
	virtual						~EraserTool();

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*				makeConfigView();
			const void*			ReturnToolCursor();
			const char*			ReturnHelpString(bool isInUse);

private:

			int32				read_coordinates();
	static	int32				CoordinateReader(void* data);

private:
			bool				reading_coordinates;

			ImageView*			image_view;
			CoordinateQueue*	coordinate_queue;
};


class EraserToolConfigView : public DrawingToolConfigView {
public:
								EraserToolConfigView(BRect rect, DrawingTool* t);

	virtual	void				AttachedToWindow();

private:
			ControlSliderBox*	fSizeSlider;
			BRadioButton*		fBackground;
			BRadioButton*		fTransparent;
};

#endif
