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
#ifndef ELLIPSE_TOOL_H
#define ELLIPSE_TOOL_H

#include "DrawingTool.h"


class BCheckBox;
class BRadioButton;


class EllipseTool : public DrawingTool {
public:
							EllipseTool();
	virtual					~EllipseTool();

			int32			UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*		UseTool(ImageView*, uint32, BPoint, BPoint);


			BView*			makeConfigView();
			const void*		ReturnToolCursor();
			const char*		ReturnHelpString(bool isInUse);
};


class EllipseToolConfigView : public DrawingToolConfigView {
public:
							EllipseToolConfigView(BRect rect, DrawingTool* t);

	virtual	void			AttachedToWindow();

private:
			BCheckBox*		fFillEllipse;
			BRadioButton*	fCorner2Corner;
			BRadioButton*	fCenter2Corner;
};

#endif
