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
#ifndef SELECTOR_TOOL_H
#define SELECTOR_TOOL_H

#include "DrawingTool.h"
#include "ToolEventAdapter.h"


class BitmapDrawer;
class BRadioButton;
class ControlSliderBox;
class PointStack;


class SelectorTool : public DrawingTool, public ToolEventAdapter {
public:
								SelectorTool();
	virtual						~SelectorTool();

			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*				makeConfigView();
			const void*			ToolCursor() const;
			const char*			HelpString(bool isInUse) const;

private:
								// These functions handle the magic wand thing.
								// They have been copied from FillTool and
								// altered a little.
			void				CheckLowerSpans(BPoint, BitmapDrawer*,
									PointStack&, int32, int32, uint32, int32,
									BBitmap*);
			void				CheckUpperSpans(BPoint, BitmapDrawer*,
									PointStack&, int32, int32, uint32, int32,
									BBitmap*);
			void				CheckBothSpans(BPoint, BitmapDrawer*,
									PointStack&, int32, int32, uint32, int32,
									BBitmap*);
			void				FillSpan(BPoint, BitmapDrawer*, int32, int32,
									uint32, int32, BBitmap*);
			BBitmap*			MakeFloodBinaryMap(BitmapDrawer*, int32, int32,
									int32, int32, uint32, BPoint);
};


class SelectorToolConfigView : public DrawingToolConfigView {
public:
								SelectorToolConfigView(DrawingTool* newTool);

	virtual	void				AttachedToWindow();

private:
			BRadioButton*		fAddArea;
			BRadioButton*		fSubstractArea;
			BRadioButton*		fFreeLine;
			BRadioButton*		fRectangle;
			BRadioButton*		fMagicWand;
			BRadioButton*		fScissors;
			ControlSliderBox*	fToleranceSlider;
};

#endif
