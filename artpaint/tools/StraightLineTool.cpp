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

#include "StraightLineTool.h"

#include "BitmapDrawer.h"
#include "BitmapUtilities.h"
#include "Brush.h"
#include "Cursors.h"
#include "HSPolygon.h"
#include "Image.h"
#include "ImageUpdater.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "Selection.h"
#include "ToolManager.h"
#include "ToolScript.h"
#include "UtilityClasses.h"


#include <Catalog.h>
#include <CheckBox.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <SeparatorView.h>
#include <Window.h>


#include <math.h>
#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


using ArtPaint::Interface::NumberSliderControl;


StraightLineTool::StraightLineTool()
	:
	DrawingTool(B_TRANSLATE("Straight line tool"), "l", STRAIGHT_LINE_TOOL)
{
	fOptions = SIZE_OPTION | ANTI_ALIASING_LEVEL_OPTION | MODE_OPTION | USE_BRUSH_OPTION
		| PRESSURE_OPTION;
	fOptionsCount = 5;

	SetOption(SIZE_OPTION, 1);
	SetOption(MODE_OPTION, B_CONTROL_ON);
	SetOption(ANTI_ALIASING_LEVEL_OPTION, B_CONTROL_OFF);
	SetOption(USE_BRUSH_OPTION, B_CONTROL_OFF);
	SetOption(PRESSURE_OPTION, 100);
}


ToolScript*
StraightLineTool::UseTool(ImageView* view, uint32 buttons, BPoint point, BPoint view_point)
{
	// In this function we calculate the line as a polygon. We make the polygon
	// by first making a horizontal rectangle of appropriate size and the
	// rotating around the starting-point by proper angle.

	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	BWindow* window = view->Window();
	drawing_mode old_mode;

	BBitmap* buffer = view->ReturnImage()->ReturnActiveBitmap();
	if (buffer == NULL)
		return NULL;

	BBitmap* srcBuffer = new (std::nothrow) BBitmap(buffer);
	if (srcBuffer == NULL)
		return NULL;

	BBitmap* tmpBuffer = new (std::nothrow) BBitmap(buffer);
	if (tmpBuffer == NULL) {
		delete srcBuffer;
		return NULL;
	}

	union color_conversion clear_color, tmp_draw_color, draw_color;
	clear_color.word = 0xFFFFFFFF;
	clear_color.bytes[3] = 0x00;
	tmp_draw_color.word = 0xFFFFFFFF;

	BitmapUtilities::ClearBitmap(tmpBuffer, clear_color.word);

	Selection* selection = view->GetSelection();

	bool anti_alias = true;
	if (GetCurrentValue(ANTI_ALIASING_LEVEL_OPTION) == B_CONTROL_OFF)
		anti_alias = false;

	float wid = tmpBuffer->Bounds().Width();
	float hgt = tmpBuffer->Bounds().Height();

	if (window != NULL) {
		ToolScript* the_script
			= new ToolScript(Type(), fToolSettings, ((PaintApplication*)be_app)->Color(true));

		BitmapDrawer* drawer = new BitmapDrawer(tmpBuffer);
		if (drawer == NULL) {
			delete the_script;

			return NULL;
		}

		BPoint original_point, original_view_point, prev_view_point;
		BPoint prev_point;
		BRect bitmap_rect, old_rect, new_rect;
		HSPolygon* view_polygon = NULL;
		BPoint point_list[4];

		float pressure = (float)fToolSettings.pressure / 100.;

		original_point = point;
		bool use_fg_color = true;
		if (buttons == B_SECONDARY_MOUSE_BUTTON)
			use_fg_color = false;

		rgb_color c = ((PaintApplication*)be_app)->Color(use_fg_color);
		draw_color.bytes[0] = c.blue;
		draw_color.bytes[1] = c.green;
		draw_color.bytes[2] = c.red;
		draw_color.bytes[3] = c.alpha * pressure;

		prev_view_point = original_view_point = view_point;
		prev_point = point;

		Brush* brush;
		float brush_width_per_2;
		float brush_height_per_2;
		int diameter = fToolSettings.size;
		if ((diameter % 2) == 0)
			diameter++;

		if (fToolSettings.use_current_brush == true) {
			brush = ToolManager::Instance().GetCurrentBrush();
			brush_width_per_2 = floor(brush->Width() / 2);
			brush_height_per_2 = floor(brush->Height() / 2);

			brush->draw(tmpBuffer,
				BPoint(prev_point.x - brush_width_per_2, prev_point.y - brush_height_per_2),
				selection);
		} else {
			brush_width_per_2 = floor(fToolSettings.size / 2);
			brush_height_per_2 = brush_width_per_2;

			if (diameter != 1) {
				drawer->DrawLine(original_point, point, tmp_draw_color.word, diameter, anti_alias,
					selection, src_over_fixed);
			} else {
				drawer->DrawHairLine(original_point, point, tmp_draw_color.word, anti_alias,
					selection, src_over_fixed);
			}
		}

		BRect updated_rect;
		updated_rect.left = min_c(point.x - brush_width_per_2, prev_point.x - brush_width_per_2);
		updated_rect.top = min_c(point.y - brush_height_per_2, prev_point.y - brush_height_per_2);
		updated_rect.right = max_c(point.x + brush_width_per_2, prev_point.x + brush_width_per_2);
		updated_rect.bottom
			= max_c(point.y + brush_height_per_2, prev_point.y + brush_height_per_2);

		buffer->Lock();
		BitmapUtilities::CompositeBitmapOnSource(
			buffer, srcBuffer, tmpBuffer, updated_rect, src_over_fixed, draw_color.word);
		buffer->Unlock();

		SetLastUpdatedRect(updated_rect);
		the_script->AddPoint(point);

		ImageUpdater* imageUpdater = new ImageUpdater(view, 2000);
		imageUpdater->AddRect(updated_rect);

		float angle = 0;

		while (buttons) {
			window->Lock();
			view->getCoords(&point, &buttons, &view_point);
			float scale = view->getMagScale();
			window->Unlock();

			if (modifiers() & B_SHIFT_KEY) {
				// Make the new point be so that the angle is a multiple of 22.5°.
				float x_diff, y_diff;
				x_diff = fabs(original_point.x - point.x);
				y_diff = fabs(original_point.y - point.y);
				float len = sqrt(x_diff * x_diff + y_diff * y_diff);
				float angle = atan(y_diff / x_diff) * 180 / M_PI;

				angle = SnapToAngle(22.5, angle);

				y_diff = len * sin(angle * M_PI / 180.);
				x_diff = len * cos(angle * M_PI / 180.);

				float signed_x_diff = (view_point.x - original_view_point.x);
				float signed_y_diff = (view_point.y - original_view_point.y);
				if (signed_x_diff != 0) {
					view_point.x = original_view_point.x
						+ x_diff * signed_x_diff * scale / fabs(signed_x_diff);
					point.x = original_point.x + x_diff * signed_x_diff / fabs(signed_x_diff);
				}

				if (signed_y_diff != 0) {
					view_point.y = original_view_point.y
						+ y_diff * signed_y_diff * scale / fabs(signed_y_diff);
					point.y = original_point.y + y_diff * signed_y_diff / fabs(signed_y_diff);
				}
			}

			updated_rect.left = max_c(0,
				min_c(prev_point.x - brush_width_per_2 - 1,
					original_point.x - brush_width_per_2 - 1));
			updated_rect.top = max_c(0,
				min_c(prev_point.y - brush_height_per_2 - 1,
					original_point.y - brush_height_per_2 - 1));
			updated_rect.right = min_c(wid,
				max_c(prev_point.x + brush_width_per_2 + 1,
					original_point.x + brush_width_per_2 + 1));
			updated_rect.bottom = min_c(hgt,
				max_c(prev_point.y + brush_height_per_2 + 1,
					original_point.y + brush_height_per_2 + 1));

			BitmapUtilities::ClearBitmap(tmpBuffer, clear_color.word, &updated_rect);

			if (fToolSettings.use_current_brush == true)
				brush->draw_line(tmpBuffer, original_point, point, selection);
			else {
				if (diameter != 1 && fToolSettings.mode == B_CONTROL_OFF) {
					drawer->DrawLine(original_point, point, tmp_draw_color.word, diameter,
						anti_alias, selection, src_over_fixed);
				} else {
					drawer->DrawHairLine(original_point, point, tmp_draw_color.word, anti_alias,
						selection, src_over_fixed);
				}
			}

			imageUpdater->AddRect(updated_rect);

			SetLastUpdatedRect(LastUpdatedRect() | updated_rect);

			buffer->Lock();
			BitmapUtilities::CompositeBitmapOnSource(
				buffer, srcBuffer, tmpBuffer, updated_rect, src_over_fixed, draw_color.word);
			buffer->Unlock();

			prev_point = point;
		}

		angle = atan2((point.y - original_point.y), (point.x - original_point.x)) * 180 / M_PI;

		int32 size = GetCurrentValue(SIZE_OPTION);
		bool draw_line = true;
		BRect old_bbox;

		if (fToolSettings.mode == B_CONTROL_ON
			&& fToolSettings.use_current_brush == B_CONTROL_OFF) { // Adjust the width of the line.
			bitmap_rect = BRect(original_point.x,
				original_point.y - floor(((float)GetCurrentValue(SIZE_OPTION) - 1.0) / 2.0),
				original_point.x
					+ sqrt(pow(original_point.x - point.x, 2) + pow(original_point.y - point.y, 2)),
				original_point.y + ceil(((float)GetCurrentValue(SIZE_OPTION) - 1.0) / 2.0));

			new_rect = old_rect = view->convertBitmapRectToView(bitmap_rect);
			point_list[0] = new_rect.LeftTop();
			point_list[1] = new_rect.RightTop();
			point_list[2] = new_rect.RightBottom();
			point_list[3] = new_rect.LeftBottom();
			view_polygon = new HSPolygon(point_list, 4);
			view_polygon->Rotate(original_view_point, angle);
			old_bbox = view_polygon->BoundingBox();

			bool continue_adjusting_width = true;
			BPoint p1 = original_point;
			BPoint width_point;
			view_polygon = new HSPolygon(NULL, 0);
			BRect orig_rect = bitmap_rect;
			orig_rect.bottom = orig_rect.top = original_point.y;

			size = 0;

			while (continue_adjusting_width) {
				if (is_clicks_data_valid) {
					continue_adjusting_width = false;
					is_clicks_data_valid = false;
				} else if (is_keys_data_valid) {
					if (last_key_event_bytes[0] == B_ESCAPE) {
						continue_adjusting_width = false;
						draw_line = false;
					}
					is_keys_data_valid = false;
				} else {
					if (view->LockLooper()) {
						new_rect = orig_rect;
						new_rect.bottom += size / 2;
						new_rect.top -= size / 2;
						new_rect = view->convertBitmapRectToView(new_rect);
						if (new_rect != old_rect) {
							float size_diff = fabs(old_rect.Height() - new_rect.Height());
							if (size > 0) {
								float scale = view->getMagScale();
								BRect bbox = view_polygon->BoundingBox();
								bbox = view->convertViewRectToBitmap(bbox);
								bbox.InsetBy(-size, -size);
								BitmapUtilities::ClearBitmap(tmpBuffer, clear_color.word, &bbox);

								point_list[0] = new_rect.LeftTop();
								point_list[1] = new_rect.RightTop();
								point_list[2] = new_rect.RightBottom();
								point_list[3] = new_rect.LeftBottom();
								view_polygon = new HSPolygon(point_list, 4);

								BPoint pivot = original_point;
								pivot.x *= scale;
								pivot.y *= scale;
								view_polygon->Rotate(pivot, angle);

								BPolygon* bpoly = view_polygon->GetBPolygon();
								if (size > 2) {
									drawer->DrawLine(original_point, point, tmp_draw_color.word,
										size, anti_alias, selection, src_over_fixed);
								} else {
									drawer->DrawHairLine(original_point, point, tmp_draw_color.word,
										anti_alias, selection, src_over_fixed);
								}
								delete bpoly;

								updated_rect
									= view->convertViewRectToBitmap(view_polygon->BoundingBox());

								updated_rect.InsetBy(-size_diff, -size_diff);

								buffer->Lock();
								BitmapUtilities::CompositeBitmapOnSource(buffer, srcBuffer,
									tmpBuffer, updated_rect, src_over_fixed, draw_color.word);
								buffer->Unlock();

								SetLastUpdatedRect(LastUpdatedRect() | updated_rect);
								imageUpdater->AddRect(updated_rect);
							}
							old_rect = new_rect;
						}
						view->getCoords(&width_point, &buttons);
						view->UnlockLooper();
						width_point = width_point - p1;
						BPoint spare = width_point;
						width_point.x = cos(-angle / 180 * M_PI) * spare.x
							- sin(-angle / 180 * M_PI) * spare.y;
						width_point.y = sin(-angle / 180 * M_PI) * spare.x
							+ cos(-angle / 180 * M_PI) * spare.y;
						size = (int32)(2 * fabs(width_point.y));
					}
					snooze(20 * 1000);
				}
			}
		}
		updated_rect = MakeRectFromPoints(original_point, point);

		// This extension might actually be too little.
		updated_rect.left -= size / 2;
		updated_rect.top -= size / 2;
		updated_rect.right += size / 2;
		updated_rect.bottom += size / 2;

		if (updated_rect.Contains(old_bbox) == FALSE)
			updated_rect = old_bbox;

		delete view_polygon;

		buffer->Lock();
		BitmapUtilities::CompositeBitmapOnSource(
			buffer, srcBuffer, tmpBuffer, updated_rect, src_over_fixed, draw_color.word);
		buffer->Unlock();

		SetLastUpdatedRect(LastUpdatedRect() | updated_rect);
		imageUpdater->AddRect(updated_rect);

		imageUpdater->ForceUpdate();
		delete imageUpdater;

		delete drawer;

		the_script->AddPoint(original_point);
		the_script->AddPoint(point);
		return the_script;
	}

	return NULL;
}


int32
StraightLineTool::UseToolWithScript(ToolScript*, BBitmap*)
{
	return B_OK;
}


BView*
StraightLineTool::ConfigView()
{
	return new StraightLineToolConfigView(this);
}


const void*
StraightLineTool::ToolCursor() const
{
	return HS_LINE_CURSOR;
}


const char*
StraightLineTool::HelpString(bool isInUse) const
{
	return (isInUse ? B_TRANSLATE("Drawing a straight line.")
					: B_TRANSLATE("Straight line: SHIFT locks 22.5° angles"));
}


// #pragma mark -- StraightLineToolConfigView


StraightLineToolConfigView::StraightLineToolConfigView(DrawingTool* tool)
	:
	DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SIZE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(SIZE_OPTION));

		fLineSize = new NumberSliderControl(B_TRANSLATE("Width:"), "1", message, 1, 100, false);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", ANTI_ALIASING_LEVEL_OPTION);
		message->AddInt32("value", 0x00000000);

		fAntiAliasing = new BCheckBox(B_TRANSLATE("Enable antialiasing"), message);
		if (tool->GetCurrentValue(ANTI_ALIASING_LEVEL_OPTION) != B_CONTROL_OFF)
			fAntiAliasing->SetValue(B_CONTROL_ON);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", MODE_OPTION);
		message->AddInt32("value", 0x00000000);

		fAdjustableWidth = new BCheckBox(B_TRANSLATE("Adjustable width"), message);
		if (tool->GetCurrentValue(MODE_OPTION) != B_CONTROL_OFF) {
			fAdjustableWidth->SetValue(B_CONTROL_ON);
			fLineSize->SetEnabled(FALSE);
		}

		BGridLayout* lineSizeLayout = LayoutSliderGrid(fLineSize);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", USE_BRUSH_OPTION);
		message->AddInt32("value", 0x00000000);
		fUseBrush = new BCheckBox(B_TRANSLATE("Use current brush"), message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", PRESSURE_OPTION);
		message->AddInt32("value", 100);

		fPressureSlider
			= new NumberSliderControl(B_TRANSLATE("Pressure:"), "100", message, 1, 100, false);

		BGridLayout* pressureLayout = LayoutSliderGrid(fPressureSlider);

		layout->AddView(BGroupLayoutBuilder(B_VERTICAL, kWidgetSpacing)
			.Add(lineSizeLayout)
			.Add(pressureLayout)
			.Add(fUseBrush)
			.AddStrut(kWidgetSpacing)
			.Add(SeparatorView(B_TRANSLATE("Options")))
			.AddGroup(B_VERTICAL, kWidgetSpacing)
				.Add(fAdjustableWidth)
				.Add(fAntiAliasing)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.TopView()
		);

		fUseBrush->SetValue(tool->GetCurrentValue(USE_BRUSH_OPTION));
		if (fUseBrush->Value() == B_CONTROL_ON) {
			fLineSize->SetEnabled(FALSE);
			fAdjustableWidth->SetEnabled(FALSE);
			fAntiAliasing->SetEnabled(FALSE);
		} else {
			fLineSize->SetEnabled(TRUE);
			fAdjustableWidth->SetEnabled(TRUE);
			fAntiAliasing->SetEnabled(TRUE);
		}

		fPressureSlider->SetValue(tool->GetCurrentValue(PRESSURE_OPTION));
	}
}


void
StraightLineToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fLineSize->SetTarget(this);
	fAntiAliasing->SetTarget(this);
	fAdjustableWidth->SetTarget(this);
	fUseBrush->SetTarget(this);
	fPressureSlider->SetTarget(this);
}


void
StraightLineToolConfigView::MessageReceived(BMessage* message)
{
	DrawingToolConfigView::MessageReceived(message);

	switch (message->what) {
		case OPTION_CHANGED:
		{
			if (message->FindInt32("option") == MODE_OPTION) {
				if (fAdjustableWidth->Value() == B_CONTROL_OFF)
					fLineSize->SetEnabled(TRUE);
				else
					fLineSize->SetEnabled(FALSE);
			} else if (message->FindInt32("option") == USE_BRUSH_OPTION) {
				if (fUseBrush->Value() == B_CONTROL_ON) {
					fLineSize->SetEnabled(FALSE);
					fAdjustableWidth->SetEnabled(FALSE);
					fAntiAliasing->SetEnabled(FALSE);
				} else {
					fLineSize->SetEnabled(TRUE);
					fAdjustableWidth->SetEnabled(TRUE);
					fAntiAliasing->SetEnabled(TRUE);
				}
			}
		} break;
	}
}
