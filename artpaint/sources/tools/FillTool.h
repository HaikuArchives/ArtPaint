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
class ControlSliderBox;
class PointStack;
class Selection;


class FillTool : public DrawingTool {
public:
								FillTool();
	virtual						~FillTool();

			BView*				makeConfigView();
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

			void				CheckLowerSpans(BPoint, BitmapDrawer*,
									PointStack&, int32, int32, uint32, uint32,
									int32, Selection* = NULL);
			void				CheckUpperSpans(BPoint, BitmapDrawer*,
									PointStack&, int32, int32, uint32, uint32,
									int32, Selection* = NULL);
			void				CheckBothSpans(BPoint, BitmapDrawer*,
									PointStack&, int32, int32, uint32, uint32,
									int32, Selection* = NULL);
			void				FillSpan(BPoint, BitmapDrawer*, int32, int32,
									uint32, uint32, int32, Selection* = NULL);


			BPoint				GradientFill(ImageView*, uint32, BPoint, BPoint,
									Selection* = NULL);
			BBitmap*			MakeBinaryMap(BitmapDrawer*, int32, int32,
									int32, int32, uint32, Selection* = NULL);
			BBitmap*			MakeFloodBinaryMap(BitmapDrawer*, int32, int32,
									int32, int32, uint32, BPoint,
									Selection* = NULL);
			void				FillGradient(BitmapDrawer*, BBitmap*, int32,
									int32, int32, int32, int32, int32, uint32,
									uint32);
			void				FillGradientPreview(BitmapDrawer*, BBitmap*,
									int32, int32, int32, int32, int32, int32,
									uint32, uint32);
			BRect				calcBinaryMapBounds(BBitmap *boolean_map);
};


class FillToolConfigView : public DrawingToolConfigView {
public:
								FillToolConfigView(DrawingTool* newTool,
									uint32 c1, uint32 c2);

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			BCheckBox*			fFlod;
			BCheckBox*			fGradient;
			BCheckBox*			fPreview;

	class	GradientView;
			GradientView*		fGradientView;
			ControlSliderBox*	fToleranceSlider;
};

#endif
