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


class ControlSlider;
class ControlSliderBox;
class ImageView;


class HairyBrushTool : public DrawingTool {
public:
								HairyBrushTool();
	virtual						~HairyBrushTool();

			int32				UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*			UseTool(ImageView*, uint32, BPoint, BPoint);


			BView*				makeConfigView();
			const void*			ToolCursor() const;
			const char*			HelpString(bool isInUse) const;

private:
			ImageView*			image_view;
//	CoordinateQueue	*coordinate_queue;
//	bool		reading_coordinates;
//	static	int32	CoordinateReader(void*);
//	int32	read_coordinates();
};


class HairyBrushToolConfigView : public DrawingToolConfigView {
public:
								HairyBrushToolConfigView(DrawingTool* newTool);

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			ControlSliderBox*	fHairAmountSlider;
			ControlSliderBox*	fWidthSlider;
			ControlSlider*		fColorVarianceSlider;
			ControlSlider*		fColorAmountSlider;

};



inline float
random_round(float number, float r)
{
	const float dec = number - floor(number);
	return (dec < r ? floor(number) : ceil(number));
}

#endif
