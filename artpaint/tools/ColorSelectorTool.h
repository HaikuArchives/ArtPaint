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


#include "CheckBox.h"


class ImageView;


namespace ArtPaint {
	namespace Interface {
		class NumberSliderControl;
	}
}
using ArtPaint::Interface::NumberSliderControl;


class ColorSelectorTool : public DrawingTool {
public:
							ColorSelectorTool();

			ToolScript*		UseTool(ImageView*, uint32, BPoint, BPoint);

			BView*			ConfigView();
			const void*		ToolCursor() const;
			const char*		HelpString(bool isInUse) const;
};


class ColorSelectorToolConfigView : public DrawingToolConfigView {
public:
							ColorSelectorToolConfigView(DrawingTool* tool);

	virtual	void			AttachedToWindow();

private:
			NumberSliderControl* fSizeSlider;
			BCheckBox* 			 fAllLayersCheckbox;
};


#endif	// COLOR_SELECTOR_TOOL_H
