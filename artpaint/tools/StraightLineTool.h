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
#ifndef STRAIGHT_LINE_TOOL_H
#define STRAIGHT_LINE_TOOL_H

#include "DrawingTool.h"
#include "ToolEventAdapter.h"


class BCheckBox;
class ImageView;
class ToolScript;


namespace ArtPaint {
	namespace Interface {
		class NumberSliderControl;
	}
}
using ArtPaint::Interface::NumberSliderControl;


class StraightLineTool : public DrawingTool, public ToolEventAdapter {
public:
								StraightLineTool();

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*				ConfigView();
			const	void*		ToolCursor() const;
			const	char*		HelpString(bool isInUse) const;
};


class StraightLineToolConfigView : public DrawingToolConfigView {
public:
								StraightLineToolConfigView(DrawingTool* tool);
	virtual						~StraightLineToolConfigView() {}

	virtual	void				AttachedToWindow();
	virtual void				MessageReceived(BMessage* message);

private:
			NumberSliderControl* fLineSize;
			NumberSliderControl* fPressureSlider;
			BCheckBox*			fAntiAliasing;
			BCheckBox*			fAdjustableWidth;
			BCheckBox*			fUseBrush;
};


#endif	// STRAIGHT_LINE_TOOL_H
