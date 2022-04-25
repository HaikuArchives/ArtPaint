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
#include "Cursors.h"
#include "HSPolygon.h"
#include "Image.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "Selection.h"
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
	: DrawingTool(B_TRANSLATE("Straight line"),
		STRAIGHT_LINE_TOOL)
{
	fOptions = SIZE_OPTION | ANTI_ALIASING_LEVEL_OPTION | MODE_OPTION;
	fOptionsCount = 3;

	SetOption(SIZE_OPTION, 1);
	SetOption(MODE_OPTION, B_CONTROL_ON);
	SetOption(ANTI_ALIASING_LEVEL_OPTION, B_CONTROL_OFF);
}


StraightLineTool::~StraightLineTool()
{
}


ToolScript*
StraightLineTool::UseTool(ImageView* view, uint32 buttons, BPoint point,
	BPoint view_point)
{
	// In this function we calculate the line as a polygon. We make the polygon
	// by first making a horizontal rectangle of appropriate size and the
	// rotating around the starting-point by proper angle.

	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	BWindow *window = view->Window();
	drawing_mode old_mode;
	BBitmap *bitmap = view->ReturnImage()->ReturnActiveBitmap();
//	BView *bitmap_view = view->getBufferView();

	Selection *selection = view->GetSelection();

	if (window != NULL) {
		ToolScript* the_script = new ToolScript(Type(), fToolSettings,
			((PaintApplication*)be_app)->Color(true));

		BitmapDrawer *drawer = new BitmapDrawer(bitmap);

		BPoint original_point,original_view_point,prev_view_point;
		BRect bitmap_rect,old_rect,new_rect;
		HSPolygon *view_polygon = NULL;
		BPoint point_list[4];

		window->Lock();
		old_mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_INVERT);
		window->Unlock();
		original_point = point;
		rgb_color c = ((PaintApplication*)be_app)->Color(true);

		prev_view_point = original_view_point = view_point;
		bitmap_rect = BRect(point.x, point.y -
			floor(((float)GetCurrentValue(SIZE_OPTION) - 1.0) / 2.0), point.x,
			point.y + ceil(((float)GetCurrentValue(SIZE_OPTION) - 1.0) / 2.0));
		old_rect = new_rect = view->convertBitmapRectToView(bitmap_rect);
		point_list[0] = new_rect.LeftTop();
		point_list[1] = new_rect.RightTop();
		point_list[2] = new_rect.RightBottom();
		point_list[3] = new_rect.LeftBottom();

		window->Lock();
		if ((GetCurrentValue(SIZE_OPTION) > 2) && (fToolSettings.mode == B_CONTROL_OFF)) {
			view_polygon = new HSPolygon(point_list,4);
			BPolygon *bpoly = view_polygon->GetBPolygon();
			view->StrokePolygon(bpoly);
			delete bpoly;
		}
		else
			view->StrokeLine(original_view_point,view_point);
		window->Unlock();

		float angle = 0;
		while (buttons) {
			window->Lock();
			view->getCoords(&point,&buttons,&view_point);
			if (modifiers() & B_LEFT_CONTROL_KEY) {
				// Make the new point be so that the angle is a multiple of 45°.
				float x_diff,y_diff;
				x_diff = fabs(original_point.x-point.x);
				y_diff = fabs(original_point.y-point.y);

				if (x_diff < y_diff) {
					if (x_diff < y_diff/2)
						x_diff = 0;
					else
						x_diff = y_diff;
				}
				else {
					if (y_diff < x_diff/2)
						y_diff = 0;
					else
						y_diff = x_diff;
				}

				float signed_x_diff = (point.x-original_point.x);
				float signed_y_diff = (point.y-original_point.y);

				if (signed_x_diff != 0)
					point.x = original_point.x + x_diff * signed_x_diff/fabs(signed_x_diff);

				if (signed_y_diff != 0)
					point.y = original_point.y + y_diff * signed_y_diff/fabs(signed_y_diff);

				x_diff = fabs(original_view_point.x-view_point.x);
				y_diff = fabs(original_view_point.y-view_point.y);

				if (x_diff < y_diff) {
					if (x_diff < y_diff/2)
						x_diff = 0;
					else
						x_diff = y_diff;
				}
				else {
					if (y_diff < x_diff/2)
						y_diff = 0;
					else
						y_diff = x_diff;
				}


				signed_x_diff = (view_point.x-original_view_point.x);
				signed_y_diff = (view_point.y-original_view_point.y);
				if (signed_x_diff != 0) {
					view_point.x = original_view_point.x +
						x_diff * signed_x_diff / fabs(signed_x_diff);
				}

				if (signed_y_diff != 0) {
					view_point.y = original_view_point.y +
						y_diff * signed_y_diff / fabs(signed_y_diff);
				}
			}

			bitmap_rect = BRect(original_point.x,
				original_point.y - floor(((float)GetCurrentValue(SIZE_OPTION) - 1.0) / 2.0),
				original_point.x + sqrt(pow(original_point.x - point.x, 2) +
				pow(original_point.y - point.y, 2)), original_point.y +
				ceil(((float)GetCurrentValue(SIZE_OPTION) - 1.0) / 2.0));

			new_rect = view->convertBitmapRectToView(bitmap_rect);
			if (old_rect != new_rect) {
				if ((GetCurrentValue(SIZE_OPTION) > 2) && (fToolSettings.mode == B_CONTROL_OFF)) {
					BRect bbox = view_polygon->BoundingBox();
					view->Draw(bbox);
					point_list[0] = new_rect.LeftTop();
					point_list[1] = new_rect.RightTop();
					point_list[2] = new_rect.RightBottom();
					point_list[3] = new_rect.LeftBottom();
					view_polygon = new HSPolygon(point_list,4);
					angle = atan2((view_point.y-original_view_point.y),
						(view_point.x - original_view_point.x)) * 180 / M_PI;
					view_polygon->Rotate(original_view_point,angle);
					BPolygon *bpoly = view_polygon->GetBPolygon();
					view->StrokePolygon(bpoly);
					delete bpoly;
				}
				else {
					float left = min_c(original_view_point.x, prev_view_point.x);
					float top = min_c(original_view_point.y, prev_view_point.y);
					float right = max_c(original_view_point.x, prev_view_point.x);
					float bottom = max_c(original_view_point.y, prev_view_point.y);
					BRect bbox(left, top, right, bottom);
					view->Draw(bbox);
					view->StrokeLine(original_view_point,view_point);
					angle = atan2((view_point.y-original_view_point.y),
						(view_point.x - original_view_point.x)) * 180 / M_PI;
					prev_view_point = view_point;
				}
				old_rect = new_rect;
			}
			window->Unlock();
			snooze(20 * 1000);
		}
		int32 size = GetCurrentValue(SIZE_OPTION);
		bool draw_line = true;
		new_rect = old_rect;
		if (fToolSettings.mode == B_CONTROL_ON) { // Adjust the width of the line.
			bool continue_adjusting_width = true;
			BPoint p1 = original_point;
			BPoint width_point;
			view_polygon = new HSPolygon(NULL,0);
			BRect orig_rect = bitmap_rect;
			orig_rect.bottom = orig_rect.top = original_point.y;

			size = 0;

			while (continue_adjusting_width) {
				if (is_clicks_data_valid) {
					continue_adjusting_width = false;
					is_clicks_data_valid = false;
				}
				else if (is_keys_data_valid) {
					if (last_key_event_bytes[0] == B_ESCAPE) {
						continue_adjusting_width = false;
						draw_line = false;
					}
					is_keys_data_valid = false;
				}
				else {
					if (view->LockLooper()) {
						new_rect = orig_rect;
						new_rect.bottom += size/2;
						new_rect.top -= size/2;
						new_rect = view->convertBitmapRectToView(new_rect);
						if (new_rect != old_rect) {
							if (size > 0) {
								BRect bbox = view_polygon->BoundingBox();
								view->Draw(bbox);
								point_list[0] = new_rect.LeftTop();
								point_list[1] = new_rect.RightTop();
								point_list[2] = new_rect.RightBottom();
								point_list[3] = new_rect.LeftBottom();
								view_polygon = new HSPolygon(point_list,4);
								view_polygon->Rotate(original_view_point,angle);
								BPolygon* bpoly = view_polygon->GetBPolygon();
								view->StrokePolygon(bpoly);
								delete bpoly;
							}
							old_rect = new_rect;
						}
						view->getCoords(&width_point,&buttons);
						view->UnlockLooper();
						width_point = width_point-p1;
						BPoint spare = width_point;
						width_point.x = cos(-angle / 180 * M_PI) * spare.x -
							sin(-angle / 180 * M_PI) * spare.y;
						width_point.y = sin(-angle / 180 * M_PI) * spare.x +
							cos(-angle / 180 * M_PI) * spare.y;
						size = (int32)(2 * fabs(width_point.y));
					}
					snooze(20 * 1000);
				}
			}
		}
		delete view_polygon;

		bool anti_alias = true;
		if (GetCurrentValue(ANTI_ALIASING_LEVEL_OPTION) == B_CONTROL_OFF)
			anti_alias = false;

		if (size > 1)
			drawer->DrawLine(original_point,point,RGBColorToBGRA(c),size,anti_alias,selection);
		else
			drawer->DrawHairLine(original_point,point,RGBColorToBGRA(c),anti_alias,selection);

		BRect updated_rect = MakeRectFromPoints(original_point, point);

		// This extension might actually be too little.
		updated_rect.left -= size/2;
		updated_rect.top -= size/2;
		updated_rect.right += size/2;
		updated_rect.bottom += size/2;

		SetLastUpdatedRect(updated_rect);
		window->Lock();
		view->SetDrawingMode(old_mode);
		view->UpdateImage(updated_rect);
		view->Sync();
		window->Unlock();

		delete drawer;

		the_script->AddPoint(original_point);
		the_script->AddPoint(point);
		return the_script;
	}

	return NULL;
}


int32
StraightLineTool::UseToolWithScript(ToolScript*,BBitmap*)
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
	return B_TRANSLATE(isInUse ? "Drawing a straight line."
		: "Press the mouse-button to draw a straight line.");
}


// #pragma mark -- StraightLineToolConfigView


StraightLineToolConfigView::StraightLineToolConfigView(DrawingTool* tool)
	: DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SIZE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(SIZE_OPTION));

		fLineSize = new NumberSliderControl(
			B_TRANSLATE("Size"),
			"1", message, 1, 100, false);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", ANTI_ALIASING_LEVEL_OPTION);
		message->AddInt32("value", 0x00000000);

		fAntiAliasing =
			new BCheckBox(B_TRANSLATE("Enable antialiasing"),
			message);
		if (tool->GetCurrentValue(ANTI_ALIASING_LEVEL_OPTION) != B_CONTROL_OFF)
			fAntiAliasing->SetValue(B_CONTROL_ON);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", MODE_OPTION);
		message->AddInt32("value", 0x00000000);

		fAdjustableWidth =
			new BCheckBox(B_TRANSLATE("Adjustable width"),
			message);
		if (tool->GetCurrentValue(MODE_OPTION) != B_CONTROL_OFF)
			fAdjustableWidth->SetValue(B_CONTROL_ON);

		BGridLayout* lineSizeLayout = LayoutSliderGrid(fLineSize);

		layout->AddView(BGroupLayoutBuilder(B_VERTICAL, kWidgetSpacing)
			.Add(lineSizeLayout)
			.AddStrut(kWidgetSpacing)
			.Add(SeparatorView(B_TRANSLATE("Options")))
			.AddGroup(B_VERTICAL, kWidgetSpacing)
				.Add(fAdjustableWidth)
				.Add(fAntiAliasing)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.TopView()
		);
	}
}


void
StraightLineToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fLineSize->SetTarget(this);
	fAntiAliasing->SetTarget(this);
	fAdjustableWidth->SetTarget(this);
}
