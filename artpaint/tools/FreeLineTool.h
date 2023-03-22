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
#ifndef FREE_LINE_TOOL_H
#define FREE_LINE_TOOL_H

#include "DrawingTool.h"


class BBitmap;
class BCheckBox;
class BView;
class CoordinateQueue;
class ImageView;
class ToolScript;


namespace ArtPaint {
	namespace Interface {
		class NumberSliderControl;
	}
}
using ArtPaint::Interface::NumberSliderControl;


class FreeLineTool : public DrawingTool {
public:
									FreeLineTool();
	virtual							~FreeLineTool();

			int32					UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*				UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*					ConfigView();
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


class FreeLineToolConfigView : public DrawingToolConfigView {
public:
									FreeLineToolConfigView(DrawingTool* tool);
	virtual							~FreeLineToolConfigView() {}

	virtual	void					AttachedToWindow();
	virtual void					MessageReceived(BMessage* message);

private:
			NumberSliderControl*	fLineSize;
			BCheckBox*				fUseBrush;
			NumberSliderControl*	fPressureSlider;
};

#endif	// FREE_LINE_TOOL_H
