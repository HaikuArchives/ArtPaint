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


class BCheckBox;
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

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*				ConfigView();
			const void*			ToolCursor() const;
			const char*			HelpString(bool isInUse) const;

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
	virtual void					MessageReceived(BMessage* message);

private:
			NumberSliderControl*	fSizeSlider;
			NumberSliderControl*	fPressureSlider;
			BRadioButton*			fBackground;
			BRadioButton*			fTransparent;
			BCheckBox*				fUseBrush;
};


#endif	// ERASER_TOOL_H
