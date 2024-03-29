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
#include "ToolEventAdapter.h"


class BCheckBox;
class BRadioButton;
class BSeparatorView;
class ImageView;
class ToolScript;


namespace ArtPaint {
	namespace Interface {
		class NumberSliderControl;
	}
}
using ArtPaint::Interface::NumberSliderControl;


class EllipseTool : public DrawingTool, public ToolEventAdapter {
public:
								EllipseTool();

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
	virtual void				MessageReceived(BMessage* message);

private:
			BCheckBox*			fFillEllipse;
			BRadioButton*		fCorner2Corner;
			BRadioButton*		fCenter2Corner;
			BCheckBox*			fAntiAlias;
			BCheckBox*          fRotation;
			NumberSliderControl* fLineWidth;
};


#endif	// ELLIPSE_TOOL_H
