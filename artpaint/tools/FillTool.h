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
#ifndef FILL_TOOL_H
#define FILL_TOOL_H

#include "DrawingTool.h"


class BCheckBox;
class BitmapDrawer;
class BRadioButton;
class BSeparatorView;
class ImageView;
class PointStack;
class Selection;
class ToolScript;


namespace ArtPaint {
	namespace Interface {
		class NumberSliderControl;
	}
}
using ArtPaint::Interface::NumberSliderControl;


#define GRADIENT_LINEAR		1
#define GRADIENT_RADIAL		2
#define GRADIENT_SQUARE		3
#define GRADIENT_CONIC		4


class FillTool : public DrawingTool {
public:
								FillTool();
	virtual						~FillTool();

			BView*				ConfigView();
			int32				UseToolWithScript(ToolScript*,BBitmap*);
			ToolScript*			UseTool(ImageView*,uint32,BPoint,BPoint);

			status_t			readSettings(BFile&,bool);
			status_t			writeSettings(BFile&);

			void				SetGradient(uint32 c1, uint32 c2) {
									gradient_color1 = c1;
									gradient_color2 = c2;
								}

			const void*			ToolCursor() const;
			const char*			HelpString(bool isInUse) const;

private:
			uint32				gradient_color1;
			uint32				gradient_color2;

			BBitmap*			filled_bitmap;
			BBitmap*			binary_fill_map;

			status_t			NormalFill(ImageView*, uint32, BPoint,
									Selection* = NULL);

			void				CheckSpans(BPoint, BitmapDrawer*,
									PointStack&, int32, int32, uint32, uint32,
									int32, Selection* = NULL,
									span_type spans = BOTH);
			void				FillSpan(BPoint, BitmapDrawer*, int32, int32,
									uint32, uint32, int32, Selection* = NULL);


			BPoint				GradientFill(ImageView*, uint32, BPoint, BPoint,
									Selection* = NULL);
			BBitmap*			MakeBinaryMap(BitmapDrawer*, int32, int32,
									int32, int32, uint32, Selection* = NULL);
			BBitmap*			MakeFloodBinaryMap(BitmapDrawer*, int32, int32,
									int32, int32, uint32, BPoint,
									Selection* = NULL);
			void				FillGradientLinear(BitmapDrawer*, BBitmap*, BPoint,
									BPoint, int32, int32, int32, int32, uint32,
									uint32, uint8 skip = 1);
			void				FillGradientRadial(BitmapDrawer*, BBitmap*, BPoint,
									BPoint, int32, int32, int32, int32, uint32,
									uint32, uint8 skip = 1);
			void				FillGradientSquare(BitmapDrawer*, BBitmap*, BPoint,
									BPoint, int32, int32, int32, int32, uint32,
									uint32, uint8 skip = 1);
			void				FillGradientConic(BitmapDrawer*, BBitmap*, BPoint,
									BPoint, int32, int32, int32, int32, uint32,
									uint32, uint8 skip = 1);

			BRect				calcBinaryMapBounds(BBitmap *boolean_map);
};


class FillToolConfigView : public DrawingToolConfigView {
public:
									FillToolConfigView(DrawingTool* tool,
										uint32 c1, uint32 c2);
	virtual							~FillToolConfigView() {}

	virtual	void					AttachedToWindow();
	virtual	void					MessageReceived(BMessage* message);

private:
			BCheckBox*				fFloodFill;
			BCheckBox*				fGradient;
			BCheckBox*				fPreview;

	class	GradientView;
			GradientView*			fGradientView;
			NumberSliderControl*	fTolerance;

			BRadioButton*			fLinearGradient;
			BRadioButton*			fRadialGradient;
			BRadioButton*			fSquareGradient;
			BRadioButton*			fConicGradient;
};

#endif	// FILL_TOOL_H
