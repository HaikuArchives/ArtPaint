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
#ifndef AIR_BRUSH_TOOL_H
#define AIR_BRUSH_TOOL_H

#include "DrawingTool.h"


class BRadioButton;
class CoordinateQueue;
class ImageView;


namespace ArtPaint {
	namespace Interface {
		class NumberSliderControl;
	}
}
using ArtPaint::Interface::NumberSliderControl;


class AirBrushTool : public DrawingTool {
public:
								AirBrushTool();
	virtual						~AirBrushTool();

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*				ConfigView();
			const void*			ToolCursor() const;
			const char*			HelpString(bool isInUse) const;

private:
			int32				read_coordinates();
	static	int32				CoordinateReader(void*);

			bool				reading_coordinates;

			ImageView*			image_view;
			CoordinateQueue*	coordinate_queue;
			int32*				sqrt_table;
};


class AirBrushToolConfigView : public DrawingToolConfigView {
public:
								AirBrushToolConfigView(DrawingTool* tool);
	virtual						~AirBrushToolConfigView() {}

	virtual	void				AttachedToWindow();

private:
		NumberSliderControl*	fBrushSize;
		NumberSliderControl*	fBrushFlow;
		BRadioButton*			fSpray;
		BRadioButton*			fAirBrush;
};


#endif	// AIR_BRUSH_TOOL_H
