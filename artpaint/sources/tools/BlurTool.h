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
class Selection;


class BlurTool : public DrawingTool {
public:
								BlurTool();
	virtual						~BlurTool();

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*				makeConfigView();
			const void*			ReturnToolCursor();
			const char*			ReturnHelpString(bool isInUse);

private:
			Selection*			selection;
};


class BlurToolConfigView : public DrawingToolConfigView {
public:
								BlurToolConfigView(BRect rect, DrawingTool *t);

	virtual	void				AttachedToWindow();

private:
			ControlSliderBox*	fControlSliderBox;
			BCheckBox*			fContinuityCheckBox;
};

#endif
