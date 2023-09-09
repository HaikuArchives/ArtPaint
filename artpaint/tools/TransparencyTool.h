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
#ifndef TRANSPARENCY_TOOL_H
#define TRANSPARENCY_TOOL_H

#include "DrawingTool.h"


class BCheckBox;
class CoordinateQueue;
class ImageView;
class ToolScript;


namespace ArtPaint {
	namespace Interface {
		class NumberSliderControl;
	}
}
using ArtPaint::Interface::NumberSliderControl;


class TransparencyTool : public DrawingTool {
public:
								TransparencyTool();

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
};


class TransparencyToolConfigView : public DrawingToolConfigView {
public:
								TransparencyToolConfigView(DrawingTool* tool);
	virtual						~TransparencyToolConfigView() {}

	virtual	void				AttachedToWindow();
	virtual void				MessageReceived(BMessage* message);

private:
		NumberSliderControl*	fSizeSlider;
		NumberSliderControl*	fSpeedSlider;
		NumberSliderControl*	fTransparencySlider;
		BCheckBox*				fUseBrush;
};


#endif	// TRANSPARENCY_TOOL_H
