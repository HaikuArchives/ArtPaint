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
class BSeparatorView;
class ImageView;
class ToolScript;


class EllipseTool : public DrawingTool {
public:
								EllipseTool();
	virtual						~EllipseTool();

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);


			BView*				ConfigView();
			const void*			ToolCursor() const;
			const char*			HelpString(bool isInUse) const;
};


class EllipseToolConfigView : public DrawingToolConfigView {
public:
								EllipseToolConfigView(DrawingTool* tool);
	virtual						~EllipseToolConfigView() {}

	virtual	void				AttachedToWindow();

private:
			BSeparatorView*		_SeparatorView(const char* label) const;

private:
			BCheckBox*			fFillEllipse;
			BRadioButton*		fCorner2Corner;
			BRadioButton*		fCenter2Corner;
			BCheckBox*			fAntiAlias;
};

#endif	// ELLIPSE_TOOL_H
