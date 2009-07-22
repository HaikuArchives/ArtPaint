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
class ImageView;
class ToolScript;


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
								TransparencyToolConfigView(DrawingTool* newTool);

	virtual	void				AttachedToWindow();

private:
			ControlSliderBox*	fSizeSlider;
			ControlSliderBox*	fSpeedSlider;
};

#endif
