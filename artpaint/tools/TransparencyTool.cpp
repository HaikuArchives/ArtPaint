/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "TransparencyTool.h"

#include "Brush.h"
#include "Cursors.h"
#include "Image.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "PixelOperations.h"
#include "Selection.h"
#include "ToolManager.h"
#include "ToolScript.h"


#include <Bitmap.h>
#include <CheckBox.h>
#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


using ArtPaint::Interface::NumberSliderControl;


TransparencyTool::TransparencyTool()
	: DrawingTool(B_TRANSLATE("Transparency tool"), "n",
		TRANSPARENCY_TOOL)
{
	// The pressure option controls the speed of transparency change.
	fOptions = SIZE_OPTION | PRESSURE_OPTION | TRANSPARENCY_OPTION | USE_BRUSH_OPTION;
	fOptionsCount = 4;

	SetOption(SIZE_OPTION, 1);
	SetOption(PRESSURE_OPTION, 1);
	SetOption(TRANSPARENCY_OPTION, 1);
	SetOption(USE_BRUSH_OPTION, B_CONTROL_OFF);
}


TransparencyTool::~TransparencyTool()
{
}


ToolScript*
TransparencyTool::UseTool(ImageView* view, uint32 buttons, BPoint point, BPoint)
{
	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	BWindow* window = view->Window();
	BBitmap* bitmap = view->ReturnImage()->ReturnActiveBitmap();

	ToolScript* the_script = new ToolScript(Type(), fToolSettings,
		((PaintApplication*)be_app)->Color(true));

	BRect bounds = bitmap->Bounds();
	uint32* bits_origin = (uint32*)bitmap->Bits();
	int32 bpr = bitmap->BytesPerRow() / 4;

	Selection* selection = view->GetSelection();

	// for the quick calculation of square-roots
	float sqrt_table[5500];
	for (int32 i=0;i<5500;i++)
		sqrt_table[i] = sqrt(i);

	float half_width = fToolSettings.size / 2;
	float half_height = fToolSettings.size / 2;
	Brush* brush;
	span* spans;
	uint32** brush_data;

	if (fToolSettings.use_current_brush == true) {
		brush = ToolManager::Instance().GetCurrentBrush();
		brush_data = brush->GetData(&spans);
		half_width = (brush->Width() - 1) / 2;
		half_height = (brush->Height() - 1) / 2;
	}

	BRect rc = BRect(floor(point.x - half_width), floor(point.y - half_height),
		ceil(point.x + half_width), ceil(point.y + half_height));

	if (selection != NULL && selection->IsEmpty() == false)
		bounds = selection->GetBoundingRect();

	rc = rc & bounds;

	SetLastUpdatedRect(rc);

	union color_conversion color;

	float pressure = (float)fToolSettings.pressure / 100.;

	uint8 transparency_value =
		((100. - (float)fToolSettings.transparency) / 100.) * 255;

	while (buttons) {
		if (selection == NULL || selection->IsEmpty() == true ||
			selection->ContainsPoint(point)) {

			the_script->AddPoint(point);

			int32 x_dist, y_sqr;

			int32 width = rc.IntegerWidth();
			int32 height = rc.IntegerHeight();
			if (fToolSettings.use_current_brush == true) {
				width -= 1;
				height -= 1;
			}

			for (int32 y = 0; y < height + 1; y++) {
				y_sqr = (int32)(point.y - rc.top - y);
				y_sqr *= y_sqr;
				int32 real_y = (int32)(rc.top + y);
				int32 real_x;
				for (int32 x = 0; x < width + 1; x++) {
					x_dist = (int32)(point.x - rc.left - x);
					real_x = (int32)(rc.left + x);
					float brush_val = 1.0;
					if (fToolSettings.use_current_brush == true)
						brush_val = (float)brush_data[y][x] / 32768.;

					if ((fToolSettings.use_current_brush == true && brush_val > 0.0) ||
						(fToolSettings.use_current_brush == false &&
						sqrt_table[x_dist * x_dist + y_sqr] <= half_width)) {
						color.word = *(bits_origin + real_y * bpr + real_x);
						if (selection == NULL ||
							selection->IsEmpty() == true ||
							selection->ContainsPoint(real_x, real_y)) {

							uint8 diff = fabs(color.bytes[3] - transparency_value);
							uint8 step = (uint8)(ceil(diff * pressure * brush_val / 2));

							if (color.bytes[3] < transparency_value) {
								color.bytes[3] = (uint8)min_c(color.bytes[3] +
									step,
									transparency_value);
								*(bits_origin + real_y*bpr + real_x) =
									color.word;
							} else if (color.bytes[3] > transparency_value) {
								color.bytes[3] = (uint8)max_c(color.bytes[3] -
									step,
									transparency_value);
								*(bits_origin + real_y*bpr + real_x) =
									color.word;
							}
						}
					}
				}
			}

			window->Lock();

			bool do_snooze = false;
			if (rc.IsValid()) {
				view->UpdateImage(rc);
				view->Sync();
				do_snooze = true;
			}

			view->getCoords(&point,&buttons);
			window->Unlock();
			half_width = fToolSettings.size / 2;
			half_height = half_width;
			if (fToolSettings.use_current_brush == true) {
				brush = ToolManager::Instance().GetCurrentBrush();
				brush_data = brush->GetData(&spans);
				half_width = (brush->Width() - 1) / 2;
				half_height = (brush->Height() - 1) / 2;
			}
			rc = BRect(floor(point.x - half_width), floor(point.y - half_height),
				ceil(point.x + half_width), ceil(point.y + half_height));
			rc = rc & bounds;
			SetLastUpdatedRect(LastUpdatedRect() | rc);
			if (do_snooze == true)
				snooze(20 * 1000);
		} else {
			window->Lock();
			view->getCoords(&point,&buttons);
			window->Unlock();
			snooze(20 * 1000);
		}
	}

	view->ReturnImage()->Render(rc);
	window->Lock();
	view->Draw(view->convertBitmapRectToView(rc));
	SetLastUpdatedRect(LastUpdatedRect() | rc);	// ???
	window->Unlock();

	return the_script;
}


int32
TransparencyTool::UseToolWithScript(ToolScript*, BBitmap*)
{
	return B_OK;
}


BView*
TransparencyTool::ConfigView()
{
	return new TransparencyToolConfigView(this);
}


const void*
TransparencyTool::ToolCursor() const
{
	return HS_TRANSPARENCY_CURSOR;
}


const char*
TransparencyTool::HelpString(bool isInUse) const
{
	return (isInUse
		? B_TRANSLATE("Adjusting the layer's transparency.")
		: B_TRANSLATE("Transparency tool"));
}


// #pragma mark -- TransparencyToolConfigView


TransparencyToolConfigView::TransparencyToolConfigView(DrawingTool* tool)
	: DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SIZE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(SIZE_OPTION));

		fSizeSlider =
			new NumberSliderControl(B_TRANSLATE("Size:"),
			"1", message, 1, 100, false);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", TRANSPARENCY_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(TRANSPARENCY_OPTION));

		fTransparencySlider =
			new NumberSliderControl(B_TRANSLATE("Transparency:"),
			"1", message, 0, 100, false);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", PRESSURE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(PRESSURE_OPTION));

		fSpeedSlider =
			new NumberSliderControl(B_TRANSLATE("Pressure:"),
			"1", message, 1, 100, false);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", USE_BRUSH_OPTION);
		message->AddInt32("value", 0x00000000);
		fUseBrush = new BCheckBox(B_TRANSLATE("Use current brush"),
			message);

		BGridLayout* sizeLayout = BGridLayoutBuilder(5.0, 5.0)
			.Add(fSizeSlider, 0, 0, 0, 0)
			.Add(fSizeSlider->LabelLayoutItem(), 0, 0)
			.Add(fSizeSlider->TextViewLayoutItem(), 1, 0)
			.Add(fSizeSlider->Slider(), 2, 0)
			.SetInsets(kWidgetInset, 0.0, 0.0, 0.0);

		sizeLayout->SetMinColumnWidth(0, StringWidth("LABELSIZE"));
		sizeLayout->SetMaxColumnWidth(1, StringWidth("100"));
		sizeLayout->SetMinColumnWidth(2, StringWidth("SLIDERSLIDERSLIDER"));

		BGridLayout* transLayout = BGridLayoutBuilder(5.0, 5.0)
			.Add(fTransparencySlider, 0, 1, 0, 0)
			.Add(fTransparencySlider->LabelLayoutItem(), 0, 1)
			.Add(fTransparencySlider->TextViewLayoutItem(), 1, 1)
			.Add(fTransparencySlider->Slider(), 2, 1)

			.Add(fSpeedSlider, 0, 2, 0, 0)
			.Add(fSpeedSlider->LabelLayoutItem(), 0, 2)
			.Add(fSpeedSlider->TextViewLayoutItem(), 1, 2)
			.Add(fSpeedSlider->Slider(), 2, 2)
			.SetInsets(kWidgetInset, 0.0, 0.0, 0.0);

		transLayout->SetMinColumnWidth(0, StringWidth("LABELSIZE"));
		transLayout->SetMaxColumnWidth(1, StringWidth("100"));
		transLayout->SetMinColumnWidth(2, StringWidth("SLIDERSLIDERSLIDER"));

		layout->AddView(BGroupLayoutBuilder(B_VERTICAL, kWidgetSpacing)
			.Add(sizeLayout)
			.Add(fUseBrush)
			.Add(transLayout)
			.TopView()
		);

		fUseBrush->SetValue(tool->GetCurrentValue(USE_BRUSH_OPTION));
		if (tool->GetCurrentValue(USE_BRUSH_OPTION) != B_CONTROL_OFF) {
			fSizeSlider->SetEnabled(FALSE);
		}
	}
}


void
TransparencyToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fSizeSlider->SetTarget(this);
	fSpeedSlider->SetTarget(this);
	fTransparencySlider->SetTarget(this);
	fUseBrush->SetTarget(this);
}


void
TransparencyToolConfigView::MessageReceived(BMessage* message)
{
	DrawingToolConfigView::MessageReceived(message);

	switch(message->what) {
		case OPTION_CHANGED: {
			if (message->FindInt32("option") == USE_BRUSH_OPTION) {
				if (fUseBrush->Value() == B_CONTROL_OFF)
					fSizeSlider->SetEnabled(TRUE);
				else
					fSizeSlider->SetEnabled(FALSE);
			}
		} break;
	}
}
