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
#ifndef RECTANGLE_TOOL_H
#define RECTANGLE_TOOL_H

#include "DrawingTool.h"
#include "ToolEventAdapter.h"


class BCheckBox;
class BRadioButton;


class RectangleTool : public DrawingTool, public ToolEventAdapter {
public:
							RectangleTool();
	virtual					~RectangleTool();

			int32			UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*		UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*			makeConfigView();
			const void*		ToolCursor() const;
			const char*		HelpString(bool isInUse) const;
};


class RectangleToolConfigView : public DrawingToolConfigView {
public:
							RectangleToolConfigView(BRect rect, DrawingTool* t);

	virtual	void			AttachedToWindow();

private:
			BCheckBox* 		fFillCheckBox;
			BRadioButton*	fCorner2Corner;
			BRadioButton*	fCenter2Corner;
			BCheckBox*		fRotationCheckBox;
			BCheckBox*		fAntiAliasCheckBox;
};

#endif
