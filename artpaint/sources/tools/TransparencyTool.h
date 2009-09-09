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
	virtual						~TransparencyTool();

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);


			BView*				makeConfigView();
			const void*			ToolCursor() const;
			const char*			HelpString(bool isInUse) const;
};


class TransparencyToolConfigView : public DrawingToolConfigView {
public:
								TransparencyToolConfigView(DrawingTool* tool);
	virtual						~TransparencyToolConfigView() {}

	virtual	void				AttachedToWindow();

private:
		NumberSliderControl*	fSizeSlider;
		NumberSliderControl*	fSpeedSlider;
};

#endif	// TRANSPARENCY_TOOL_H
