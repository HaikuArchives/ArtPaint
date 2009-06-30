/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef COLOR_SELECTOR_TOOL_H
#define COLOR_SELECTOR_TOOL_H

#include <StringView.h>
#include <Window.h>

#include "DrawingTool.h"

class ControlSliderBox;

class ColorSelectorTool : public DrawingTool {

public:
		ColorSelectorTool();
virtual	~ColorSelectorTool();

ToolScript*	UseTool(ImageView*,uint32,BPoint,BPoint);

BView*	makeConfigView();
const	char*	ReturnHelpString(bool);
const	void*	ReturnToolCursor();
};

class ColorSelectorView : public BView {
		uint32		selected_color;
		BStringView	*red_view;
		BStringView *green_view;
		BStringView	*blue_view;
		BStringView	*alpha_view;

public:
		ColorSelectorView(BRect);

void	Draw(BRect);

void	ChangeValue(uint32);
};

class ColorSelectorWindow : public BWindow {
		BRect 				screen_bounds;
		ColorSelectorView	*cs_view;
public:
		ColorSelectorWindow(BPoint);

void	ChangeValue(uint32);
void	Move(BPoint);
};


class ColorSelectorToolConfigView : public DrawingToolConfigView {
		ControlSliderBox	*size_slider;
public:
		ColorSelectorToolConfigView(BRect rect,DrawingTool *t);

void	AttachedToWindow();
};
#endif
