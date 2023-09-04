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

#include "BlurTool.h"

#include "BitmapDrawer.h"
#include "Brush.h"
#include "Cursors.h"
#include "Image.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "Selection.h"
#include "ToolManager.h"
#include "ToolScript.h"


#include <Catalog.h>
#include <CheckBox.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <SeparatorView.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


using ArtPaint::Interface::NumberSliderControl;


BlurTool::BlurTool()
	:
	DrawingTool(B_TRANSLATE("Blur tool"), "u", BLUR_TOOL)
{
	fOptions = SIZE_OPTION | CONTINUITY_OPTION | USE_BRUSH_OPTION;
	fOptionsCount = 3;

	SetOption(SIZE_OPTION, 1);
	SetOption(CONTINUITY_OPTION, B_CONTROL_OFF);
	SetOption(USE_BRUSH_OPTION, B_CONTROL_OFF);
}


BlurTool::~BlurTool()
{
}


ToolScript*
BlurTool::UseTool(ImageView* view, uint32 buttons, BPoint point, BPoint)
{
	/*
		This function uses a convolution matrix to do the blurring.
		The matrix is following:

				1/9		1/9		1/9

				1/9		1/9		1/9

				1/9		1/9		1/9
	*/
	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	BPoint prev_point;
	bool prev_use_brush = false;
	BWindow* window = view->Window();
	BBitmap* bitmap = view->ReturnImage()->ReturnActiveBitmap();
	BitmapDrawer* drawer = new BitmapDrawer(bitmap);

	ToolScript* the_script
		= new ToolScript(Type(), fToolSettings, ((PaintApplication*)be_app)->Color(true));

	selection = view->GetSelection();

	BRect bounds = bitmap->Bounds();
	uint32* bits_origin = (uint32*)bitmap->Bits();
	int32 bpr = bitmap->BytesPerRow() / 4;

	float width = fToolSettings.size;
	float height = fToolSettings.size;
	float half_width = (int32)width / 2;
	float half_height = (int32)height / 2;
	int32 previous_width = fToolSettings.size;
	int32 previous_height = fToolSettings.size;

	Brush* brush;
	BBitmap* brush_bmap;
	uint32* brush_bits;
	uint32 brush_bpr;

	if (fToolSettings.use_current_brush == true) {
		brush = ToolManager::Instance().GetCurrentBrush();
		brush_bmap = brush->GetBitmap();
		brush_bits = (uint32*)brush_bmap->Bits();
		brush_bpr = brush_bmap->BytesPerRow() / 4;
		width = brush->Width();
		height = brush->Height();
		half_width = (width - 1) / 2;
		half_height = (height - 1) / 2;
		previous_width = width;
		previous_height = height;
		prev_use_brush = true;
	}

	// this is the bitmap where the blurred image will be first made
	BBitmap* blurred = new BBitmap(BRect(0, 0, width + 1, height + 1), B_RGB_32_BIT);
	int32* blurred_bits;
	int32 blurred_bpr = blurred->BytesPerRow() / 4;

	BRect rc;
	// for the quick calculation of square-roots
	int32 sqrt_table[5500];
	for (int32 i = 0; i < 5500; i++)
		sqrt_table[i] = (int32)sqrt(i);

	prev_point = point - BPoint(1, 1);
	SetLastUpdatedRect(BRect(point, point));
	while (buttons) {
		if ((fToolSettings.continuity == B_CONTROL_ON)
			|| (fToolSettings.size != previous_width)
			|| (fToolSettings.use_current_brush == true
				&& (width != previous_width || height != previous_height))
			|| (point != prev_point)) {
			if ((fToolSettings.use_current_brush == false && fToolSettings.size != previous_width)
				|| (fToolSettings.use_current_brush != prev_use_brush)) {

				delete blurred;
				width = fToolSettings.size;
				height = fToolSettings.size;
				half_width = (int32)width / 2;
				half_height = (int32)height / 2;
				if (fToolSettings.use_current_brush == true) {
					brush = ToolManager::Instance().GetCurrentBrush();
					brush_bmap = brush->GetBitmap();
					brush_bits = (uint32*)brush_bmap->Bits();
					brush_bpr = brush_bmap->BytesPerRow() / 4;
					width = brush->Width();
					height = brush->Height();
					half_width = (width - 1) / 2;
					half_height = (height - 1) / 2;
				}
				blurred = new BBitmap(BRect(0, 0, width + 1, height + 1), B_RGB_32_BIT);
				previous_width = width;
				previous_height = height;
				prev_use_brush = true;
			} else
				prev_use_brush = false;

			blurred_bits = (int32*)blurred->Bits();

			rc = BRect(point.x - half_width, point.y - half_height, point.x + half_width,
				point.y + half_height);
			rc = rc & bounds;

			BPoint left_top = rc.LeftTop();
			uint32 new_pixel;
			float red, green, blue, alpha;
			int32 x_dist, y_sqr;

			int32 rc_width = rc.IntegerWidth();
			int32 rc_height = rc.IntegerHeight();
			if (fToolSettings.use_current_brush == true) {
				rc_width -= 1;
				rc_height -= 1;
			}

			for (int32 y = 0; y < rc_height + 1; y++) {
				y_sqr = (int32)(point.y - rc.top - y);
				y_sqr *= y_sqr;
				for (int32 x = 0; x < rc_width + 1; x++) {
					x_dist = (int32)(point.x - rc.left - x);
					float brush_val = 1.0;
					if (fToolSettings.use_current_brush == true) {
						union color_conversion brush_color;
						brush_color.word = *(brush_bits + x + y * brush_bpr);
						brush_val = brush_color.bytes[3];
					}
					if (((fToolSettings.use_current_brush == true && brush_val > 0.0)
							|| (fToolSettings.use_current_brush == false
								&& sqrt_table[x_dist * x_dist + y_sqr] <= half_width))
						&& (selection == NULL || selection->IsEmpty()
							|| selection->ContainsPoint(left_top + BPoint(x, y)))) {
						red = 0;
						green = 0;
						blue = 0;
						alpha = 0;
						for (int32 dy = -1; dy < 2; dy++) {
							for (int32 dx = -1; dx < 2; dx++) {
								int32 x_coord
									= (int32)min_c(max_c(left_top.x + x + dx, 0), rc.right);
								int32 y_coord
									= (int32)min_c(max_c(left_top.y + y + dy, 0), rc.bottom);
								new_pixel = drawer->GetPixel(x_coord, y_coord);

								blue += (float)((new_pixel >> 24) & 0xFF) / 9.0;
								green += (float)((new_pixel >> 16) & 0xFF) / 9.0;
								red += (float)((new_pixel >> 8) & 0xFF) / 9.0;
								alpha += (float)((new_pixel) &0xFF) / 9.0;
							}
						}
						// At this point we should round the values.
						*blurred_bits = (uint32)blue << 24 | (uint32)green << 16
							| (uint32)red << 8 | (uint32)alpha;
					} else
						*blurred_bits = drawer->GetPixel(left_top + BPoint(x, y));

					blurred_bits++;
				}
				blurred_bits += blurred_bpr - (int32)rc.Width();
			}

			blurred_bits = (int32*)blurred->Bits();
			if (rc.IsValid()) {
				for (int32 y = 0; y < rc_height + 1; y++) {
					for (int32 x = 0; x < rc_width + 1; x++) {
						*(bits_origin + (int32)(left_top.x + x) + (int32)((left_top.y + y) * bpr))
							= *blurred_bits;
						blurred_bits++;
					}
					blurred_bits += blurred_bpr - (int32)rc.Width();
				}
			}

			prev_point = point;
			the_script->AddPoint(point);
		} else
			rc = BRect(0, 0, -1, -1);

		window->Lock();
		if (rc.IsValid()) {
			view->UpdateImage(rc);
			view->Sync();
			SetLastUpdatedRect(LastUpdatedRect() | rc);
		}
		view->getCoords(&point, &buttons);
		window->Unlock();

		snooze(20 * 1000);
	}
	delete blurred;
	delete drawer;
	return the_script;
}


int32
BlurTool::UseToolWithScript(ToolScript*, BBitmap*)
{
	return B_OK;
}


BView*
BlurTool::ConfigView()
{
	return new BlurToolConfigView(this);
}


const void*
BlurTool::ToolCursor() const
{
	return HS_BLUR_CURSOR;
}


const char*
BlurTool::HelpString(bool isInUse) const
{
	return (isInUse ? B_TRANSLATE("Blurring the image.") : B_TRANSLATE("Blur tool"));
}


// #pragma mark -- BlurToolConfigView


BlurToolConfigView::BlurToolConfigView(DrawingTool* tool)
	:
	DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("value", 0x00000000);
		message->AddInt32("option", CONTINUITY_OPTION);

		fContinuity = new BCheckBox(B_TRANSLATE("Continuous"), message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SIZE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(SIZE_OPTION));

		fBlurSize = new NumberSliderControl(B_TRANSLATE("Size:"), "1", message, 1, 100, false);

		BGridLayout* sizeLayout = LayoutSliderGrid(fBlurSize);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", USE_BRUSH_OPTION);
		message->AddInt32("value", 0x00000000);
		fUseBrush = new BCheckBox(B_TRANSLATE("Use current brush"), message);

		layout->AddView(BGroupLayoutBuilder(B_VERTICAL, kWidgetSpacing)
			.Add(sizeLayout)
			.Add(fUseBrush)
			.AddStrut(kWidgetSpacing)
			.Add(SeparatorView(B_TRANSLATE("Mode")))
			.AddGroup(B_VERTICAL, kWidgetSpacing)
				.Add(fContinuity)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.TopView()
		);

		if (tool->GetCurrentValue(CONTINUITY_OPTION) != B_CONTROL_OFF)
			fContinuity->SetValue(B_CONTROL_ON);

		fUseBrush->SetValue(tool->GetCurrentValue(USE_BRUSH_OPTION));
		if (tool->GetCurrentValue(USE_BRUSH_OPTION) != B_CONTROL_OFF)
			fBlurSize->SetEnabled(FALSE);
	}
}


void
BlurToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fBlurSize->SetTarget(this);
	fContinuity->SetTarget(this);
	fUseBrush->SetTarget(this);
}


void
BlurToolConfigView::MessageReceived(BMessage* message)
{
	DrawingToolConfigView::MessageReceived(message);

	switch (message->what) {
		case OPTION_CHANGED:
		{
			if (message->FindInt32("option") == USE_BRUSH_OPTION) {
				if (fUseBrush->Value() == B_CONTROL_OFF)
					fBlurSize->SetEnabled(TRUE);
				else
					fBlurSize->SetEnabled(FALSE);
			}
		} break;
	}
}
