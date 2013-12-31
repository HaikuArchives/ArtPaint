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
	virtual						~StraightLineTool();

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*				ConfigView();
			const	void*		ToolCursor() const;
			const	char*		HelpString(bool isInUse) const;
};


class StraightLineToolConfigView : public DrawingToolConfigView {
public:
									StraightLineToolConfigView(DrawingTool* tool);
	virtual							~StraightLineToolConfigView() {}

	virtual	void					AttachedToWindow();

private:
			NumberSliderControl*	fLineSize;
			BCheckBox*				fAntiAliasing;
			BCheckBox*				fAdjustableWidth;
};

#endif	// STRAIGHT_LINE_TOOL_H
