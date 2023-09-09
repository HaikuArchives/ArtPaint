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
#ifndef HAIRY_BRUSH_TOOL_H
#define HAIRY_BRUSH_TOOL_H

#include "DrawingTool.h"


class BSlider;
class ImageView;
class ToolScript;


namespace ArtPaint {
	namespace Interface {
		class NumberSliderControl;
	}
}
using ArtPaint::Interface::NumberSliderControl;


class HairyBrushTool : public DrawingTool {
public:
								HairyBrushTool();
	virtual						~HairyBrushTool();

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);


			BView*				ConfigView();
			const void*			ToolCursor() const;
			const char*			HelpString(bool isInUse) const;

private:
			ImageView*			image_view;
};


class HairyBrushToolConfigView : public DrawingToolConfigView {
public:
								HairyBrushToolConfigView(DrawingTool* tool);
	virtual						~HairyBrushToolConfigView() {}

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
		NumberSliderControl*	fBrushSize;
		NumberSliderControl*	fBrushHairs;
		BSlider*				fColorAmount;
		BSlider*				fColorVariance;
};


inline float
random_round(float number, float r)
{
	const float dec = number - floor(number);
	return (dec < r ? floor(number) : ceil(number));
}

#endif	// HAIRY_BRUSH_TOOL_H
