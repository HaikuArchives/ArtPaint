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

#include "DrawingTool.h"


class BRadioButton;
class CoordinateQueue;
class ImageView;
class ToolScript;


namespace ArtPaint {
	namespace Interface {
		class NumberSliderControl;
	}
}
using ArtPaint::Interface::NumberSliderControl;


class EraserTool : public DrawingTool {
public:
								EraserTool();
	virtual						~EraserTool();

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*				makeConfigView();
			const void*			ToolCursor() const;
			const char*			HelpString(bool isInUse) const;

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
									EraserToolConfigView(DrawingTool* tool);
	virtual							~EraserToolConfigView() {}

	virtual	void					AttachedToWindow();

private:
			NumberSliderControl*	fSizeSlider;
			BRadioButton*			fBackground;
			BRadioButton*			fTransparent;
};

#endif	// ERASER_TOOL_H
