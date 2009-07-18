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
#ifndef COLOR_SELECTOR_TOOL_H
#define COLOR_SELECTOR_TOOL_H

#include "DrawingTool.h"


#include <Window.h>


class ControlSliderBox;
class BStringView;


class ColorSelectorTool : public DrawingTool {
public:
								ColorSelectorTool();
	virtual						~ColorSelectorTool();

			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*				makeConfigView();
			const void*			ToolCursor() const;
			const char*			HelpString(bool isInUse) const;
};


class ColorSelectorView : public BView {
public:
								ColorSelectorView(BRect frame);

	virtual	void				Draw(BRect updateRect);

			void				ChangeValue(uint32);

private:
			uint32				selected_color;
			BStringView*		red_view;
			BStringView*		green_view;
			BStringView*		blue_view;
			BStringView*		alpha_view;
};


class ColorSelectorWindow : public BWindow {
public:
								ColorSelectorWindow(BPoint cursorLocation);

			void				ChangeValue(uint32 color);
			void				Move(BPoint cursorLocation);

private:
			BRect 				screen_bounds;
			ColorSelectorView*	cs_view;
};


class ColorSelectorToolConfigView : public DrawingToolConfigView {
public:
								ColorSelectorToolConfigView(BRect rect,
									DrawingTool* drawingTool);

	virtual	void				AttachedToWindow();

private:
			ControlSliderBox*	fSizeSlider;
};

#endif
