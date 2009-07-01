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


class ControlSliderBox;


class TransparencyTool : public DrawingTool {
public:
								TransparencyTool();
	virtual						~TransparencyTool();

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);


			BView*				makeConfigView();
			const void*			ReturnToolCursor();
			const char*			ReturnHelpString(bool isInUse);
};


class TransparencyToolConfigView : public DrawingToolConfigView {
public:
								TransparencyToolConfigView(BRect rect,
									DrawingTool* drawingTool);

	virtual	void				AttachedToWindow();

private:
			ControlSliderBox*	fSizeSlider;
			ControlSliderBox*	fSpeedSlider;
};

#endif
