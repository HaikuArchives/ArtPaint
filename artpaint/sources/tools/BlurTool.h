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
#ifndef BLUR_TOOL_H
#define BLUR_TOOL_H

#include "DrawingTool.h"


class BCheckBox;
class ControlSliderBox;
class ImageView;
class Selection;


class BlurTool : public DrawingTool {
public:
								BlurTool();
	virtual						~BlurTool();

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*				makeConfigView();
			const void*			ToolCursor() const;
			const char*			HelpString(bool isInUse) const;

private:
			Selection*			selection;
};


class BlurToolConfigView : public DrawingToolConfigView {
public:
								BlurToolConfigView(DrawingTool* newTool);

	virtual	void				AttachedToWindow();

private:
			ControlSliderBox*	fControlSliderBox;
			BCheckBox*			fContinuityCheckBox;
};

#endif
