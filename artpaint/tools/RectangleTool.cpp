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

#include "RectangleTool.h"

#include "BitmapDrawer.h"
#include "BitmapUtilities.h"
#include "Cursors.h"
#include "HSPolygon.h"
#include "Image.h"
#include "ImageUpdater.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "ToolScript.h"
#include "UtilityClasses.h"


#include <Catalog.h>
#include <CheckBox.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>
#include <SeparatorView.h>
#include <Window.h>


#include <math.h>
#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


RectangleTool::RectangleTool()
	:
	DrawingTool(B_TRANSLATE("Rectangle tool"), "r", RECTANGLE_TOOL)
{
	fOptions = FILL_ENABLED_OPTION | SIZE_OPTION | SHAPE_OPTION | ROTATION_ENABLED_OPTION
		| ANTI_ALIASING_LEVEL_OPTION | WIDTH_OPTION;
	fOptionsCount = 6;

	SetOption(FILL_ENABLED_OPTION, B_CONTROL_OFF);
	SetOption(SIZE_OPTION, 1);
	SetOption(SHAPE_OPTION, HS_CORNER_TO_CORNER);
	SetOption(ROTATION_ENABLED_OPTION, B_CONTROL_OFF);
	SetOption(ANTI_ALIASING_LEVEL_OPTION, B_CONTROL_OFF);
	SetOption(WIDTH_OPTION, 1);
}


ToolScript*
RectangleTool::UseTool(ImageView* view, uint32 buttons, BPoint point, BPoint view_point)
{
	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	BWindow* window = view->Window();
	drawing_mode old_mode;

	ToolScript* the_script = new (std::nothrow)
		ToolScript(Type(), fToolSettings, ((PaintApplication*)be_app)->Color(true));
	if (the_script == NULL) {
		return NULL;
	}
	BBitmap* buffer = view->ReturnImage()->ReturnActiveBitmap();
	if (buffer == NULL) {
		delete the_script;

		return NULL;
	}
	BBitmap* srcBuffer = new (std::nothrow) BBitmap(buffer);
	if (srcBuffer == NULL) {
		delete the_script;

		return NULL;
	}
	BBitmap* tmpBuffer = new (std::nothrow) BBitmap(buffer);
	if (tmpBuffer == NULL) {
		delete the_script;
		delete srcBuffer;
		return NULL;
	}
	union color_conversion clear_color, tmp_draw_color;
	clear_color.word = 0xFFFFFFFF;
	clear_color.bytes[3] = 0x00;
	tmp_draw_color.word = 0xFFFFFFFF;

	BitmapUtilities::ClearBitmap(tmpBuffer, clear_color.word);
	Selection* selection = view->GetSelection();

	bool draw_rectangle = true;

	bool fill = (GetCurrentValue(FILL_ENABLED_OPTION) == B_CONTROL_ON);
	bool anti_a = (GetCurrentValue(ANTI_ALIASING_LEVEL_OPTION) == B_CONTROL_ON);

	int32 width = GetCurrentValue(WIDTH_OPTION);

	if (fill == false && width > 1)
		fill = true;
	else if (fill == true)
		width = 0;

	int32 half_width = ceil(width / 2.);

	if (window != NULL) {
		ImageUpdater* imageUpdater = new ImageUpdater(view, 10000);

		BitmapDrawer* drawer = new BitmapDrawer(tmpBuffer);

		BPoint original_point;
		BRect bitmap_rect, old_rect, new_rect;
		window->Lock();
		rgb_color old_color = view->HighColor();
		old_mode = view->DrawingMode();

		window->Unlock();
		bool use_fg_color = true;
		if (buttons == B_SECONDARY_MOUSE_BUTTON)
			use_fg_color = false;

		rgb_color c = ((PaintApplication*)be_app)->Color(use_fg_color);
		original_point = point;
		bitmap_rect = BRect(point, point);
		old_rect = bitmap_rect;
		new_rect = view->convertBitmapRectToView(bitmap_rect);

		imageUpdater->AddRect(old_rect);

		while (buttons) {
			window->Lock();
			view->getCoords(&point, &buttons, &view_point);
			window->Unlock();
			bitmap_rect = MakeRectFromPoints(original_point, point);
			if (modifiers() & B_SHIFT_KEY) {
				// Make the rectangle square.
				float max_distance = max_c(bitmap_rect.Height(), bitmap_rect.Width());
				if (original_point.x == bitmap_rect.left)
					bitmap_rect.right = bitmap_rect.left + max_distance;
				else
					bitmap_rect.left = bitmap_rect.right - max_distance;

				if (original_point.y == bitmap_rect.top)
					bitmap_rect.bottom = bitmap_rect.top + max_distance;
				else
					bitmap_rect.top = bitmap_rect.bottom - max_distance;
			}
			if (modifiers() & B_COMMAND_KEY) {
				// Make the the rectangle original corner be at the center of
				// new rectangle.
				float y_distance = bitmap_rect.Height();
				float x_distance = bitmap_rect.Width();

				if (bitmap_rect.left == original_point.x)
					bitmap_rect.left = bitmap_rect.left - x_distance;
				else
					bitmap_rect.right = bitmap_rect.right + x_distance;

				if (bitmap_rect.top == original_point.y)
					bitmap_rect.top = bitmap_rect.top - y_distance;
				else
					bitmap_rect.bottom = bitmap_rect.bottom + y_distance;
			}
			new_rect = view->convertBitmapRectToView(bitmap_rect);

			BRect updated_rect = old_rect.InsetByCopy(-2, -2);

			BitmapUtilities::ClearBitmap(tmpBuffer, clear_color.word, &updated_rect);

			int32 max_half_width = min_c(half_width, bitmap_rect.Width() / 2.);
			int32 max_half_height = min_c(half_width, bitmap_rect.Height() / 2.);
			BRect outerRect = bitmap_rect.InsetByCopy(-half_width, -half_width);
			BRect innerRect = bitmap_rect.InsetByCopy(max_half_width, max_half_height);

			if (innerRect.Width() < 1)
				innerRect.InsetBy(-1, 0);
			if (innerRect.Height() < 1)
				innerRect.InsetBy(0, -1);

			BPoint* corners = new BPoint[4];
			corners[0] = outerRect.LeftTop();
			corners[1] = outerRect.RightTop();
			corners[2] = outerRect.RightBottom();
			corners[3] = outerRect.LeftBottom();
			HSPolygon* outerPoly = new HSPolygon(corners, 4);

			corners[0] = innerRect.LeftTop();
			corners[1] = innerRect.RightTop();
			corners[2] = innerRect.RightBottom();
			corners[3] = innerRect.LeftBottom();
			HSPolygon* innerPoly = new HSPolygon(corners, 4);

			drawer->DrawRectanglePolygon(outerPoly->GetPointList(), tmp_draw_color.word, fill, anti_a, selection);

			if (width > 1) {
				union color_conversion rev_color;
				rev_color.word = tmp_draw_color.word;
				rev_color.bytes[3] = 0xFF;
				drawer->DrawRectanglePolygon(
					innerPoly->GetPointList(), tmp_draw_color.word, fill, anti_a, selection,
					dst_out_fixed);
			}

			buffer->Lock();
			BitmapUtilities::CompositeBitmapOnSource(
				buffer, srcBuffer, tmpBuffer, updated_rect, src_over_fixed, RGBColorToBGRA(c));
			buffer->Unlock();

			imageUpdater->AddRect(updated_rect);

			SetLastUpdatedRect(LastUpdatedRect() | updated_rect);

			old_rect = outerPoly->BoundingBox().InsetByCopy(-width, -width);

			delete outerPoly;
			delete innerPoly;
			snooze(10 * 1000);
		}

		HSPolygon* poly;
		BPoint* corners = new BPoint[4];
		corners[0] = bitmap_rect.LeftTop();
		corners[1] = bitmap_rect.RightTop();
		corners[2] = bitmap_rect.RightBottom();
		corners[3] = bitmap_rect.LeftBottom();
		poly = new HSPolygon(corners, 4);

		float new_angle, prev_angle = 0;

		if (GetCurrentValue(ROTATION_ENABLED_OPTION) == B_CONTROL_ON) {
			BPoint centroid = bitmap_rect.LeftTop() + bitmap_rect.RightTop()
				+ bitmap_rect.RightBottom() + bitmap_rect.LeftBottom();
			centroid.x /= 4;
			centroid.y /= 4;
			prev_angle = new_angle = 0;
			bool continue_rotating = true;
			float original_angle;
			if (centroid.x != point.x)
				original_angle = atan((centroid.y - point.y)
					/ (centroid.x - point.x)) * 180 / M_PI;
			else
				original_angle = 90;

			int32 max_half_width = min_c(half_width, bitmap_rect.Width() / 2.);
			int32 max_half_height = min_c(half_width, bitmap_rect.Height() / 2.);
			BRect outerRect = bitmap_rect.InsetByCopy(-half_width, -half_width);
			BRect innerRect = bitmap_rect.InsetByCopy(max_half_width, max_half_height);

			if (innerRect.Width() < 1)
				innerRect.InsetBy(-1, 0);
			if (innerRect.Height() < 1)
				innerRect.InsetBy(0, -1);

			corners[0] = outerRect.LeftTop();
			corners[1] = outerRect.RightTop();
			corners[2] = outerRect.RightBottom();
			corners[3] = outerRect.LeftBottom();
			HSPolygon* outerPoly = new HSPolygon(corners, 4);

			corners[0] = innerRect.LeftTop();
			corners[1] = innerRect.RightTop();
			corners[2] = innerRect.RightBottom();
			corners[3] = innerRect.LeftBottom();
			HSPolygon* innerPoly = new HSPolygon(corners, 4);

			int32 max_dim = max_c(outerRect.Width(), outerRect.Height());

			BRect updated_rect = BRect(centroid.x - max_dim, centroid.y - max_dim,
				centroid.x + max_dim, centroid.y + max_dim);
			updated_rect.InsetBy(-width * 1.5, -width * 1.5);

			while (continue_rotating) {
				if (is_clicks_data_valid) {
					continue_rotating = FALSE;
					is_clicks_data_valid = FALSE;
				} else if (is_keys_data_valid) {
					if (last_key_event_bytes[0] == B_ESCAPE) {
						continue_rotating = FALSE;
						draw_rectangle = FALSE;
					}
					is_keys_data_valid = FALSE;
				} else {
					// Here we should rotate the polygon
					window->Lock();
					if (new_angle != prev_angle) {
						BitmapUtilities::ClearBitmap(tmpBuffer, clear_color.word);

						outerPoly->RotateAboutCenter(new_angle - prev_angle);
						innerPoly->RotateAboutCenter(new_angle - prev_angle);

						drawer->DrawRectanglePolygon(outerPoly->GetPointList(), tmp_draw_color.word, fill, anti_a, selection);

						if (width > 1) {
							union color_conversion rev_color;
							rev_color.word = tmp_draw_color.word;
							rev_color.bytes[3] = 0xFF;
							drawer->DrawRectanglePolygon(
								innerPoly->GetPointList(), tmp_draw_color.word, fill, anti_a, selection,
								dst_out_fixed);
						}

						buffer->Lock();
						BitmapUtilities::CompositeBitmapOnSource(
							buffer, srcBuffer, tmpBuffer, updated_rect, src_over_fixed, RGBColorToBGRA(c));
						buffer->Unlock();

						imageUpdater->AddRect(updated_rect);

						SetLastUpdatedRect(LastUpdatedRect() | updated_rect);
						prev_angle = new_angle;

						old_rect = updated_rect;
					}
					view->getCoords(&point, &buttons, &view_point);
					window->Unlock();
					if (centroid.x != point.x) {
						new_angle
							= atan((centroid.y - point.y) / (centroid.x - point.x)) * 180 / M_PI;
					} else
						new_angle = 90;

					new_angle -= original_angle;

					if (modifiers() & B_SHIFT_KEY)
						new_angle = SnapToAngle(22.5, new_angle);

					snooze(20 * 1000);
				}
			}
			poly->RotateAboutCenter(prev_angle);

		}

		buffer->Lock();
		if (draw_rectangle == true) {
			if (poly->GetPointCount() == 4) {
				if (width > 1) {
					int32 max_half_width = min_c(half_width, bitmap_rect.Width() / 2.);
					int32 max_half_height = min_c(half_width, bitmap_rect.Height() / 2.);
					BRect outerRect = bitmap_rect.InsetByCopy(-half_width, -half_width);
					BRect innerRect = bitmap_rect.InsetByCopy(max_half_width, max_half_height);

					if (innerRect.Width() < 1)
						innerRect.InsetBy(-1, 0);
					if (innerRect.Height() < 1)
						innerRect.InsetBy(0, -1);

					corners[0] = outerRect.LeftTop();
					corners[1] = outerRect.RightTop();
					corners[2] = outerRect.RightBottom();
					corners[3] = outerRect.LeftBottom();
					HSPolygon* outerPoly = new HSPolygon(corners, 4);

					corners[0] = innerRect.LeftTop();
					corners[1] = innerRect.RightTop();
					corners[2] = innerRect.RightBottom();
					corners[3] = innerRect.LeftBottom();
					HSPolygon* innerPoly = new HSPolygon(corners, 4);

					innerPoly->RotateAboutCenter(prev_angle);
					outerPoly->RotateAboutCenter(prev_angle);

					drawer->DrawRectanglePolygon(
						outerPoly->GetPointList(), tmp_draw_color.word, fill, anti_a, selection);

					union color_conversion rev_color;
					rev_color.word = tmp_draw_color.word;
					rev_color.bytes[3] = 0xFF;
					drawer->DrawRectanglePolygon(
						innerPoly->GetPointList(), tmp_draw_color.word, fill, anti_a, selection,
						dst_out_fixed);
					BitmapUtilities::CompositeBitmapOnSource(
						buffer, srcBuffer, tmpBuffer,
						outerPoly->BoundingBox().InsetByCopy(-half_width, -half_width),
						src_over_fixed, RGBColorToBGRA(c));
				} else {
					drawer->DrawRectanglePolygon(
						poly->GetPointList(), tmp_draw_color.word, fill, anti_a, selection);
					BitmapUtilities::CompositeBitmapOnSource(
						buffer, srcBuffer, tmpBuffer, poly->BoundingBox().InsetByCopy(-width, -width),
						src_over_fixed, RGBColorToBGRA(c));
				}
			} else {
				drawer->DrawRectanglePolygon(corners, tmp_draw_color.word, fill, anti_a, selection);
				BitmapUtilities::CompositeBitmapOnSource(
					buffer, srcBuffer, tmpBuffer, poly->BoundingBox().InsetByCopy(-half_width, -half_width),
					src_over_fixed, RGBColorToBGRA(c));
			}
		}

		buffer->Unlock();

		SetLastUpdatedRect(poly->BoundingBox().InsetByCopy(-(1.0 + width), -(1.0 + width)));

		if (poly->GetPointCount() == 4) {
			BPoint* points = poly->GetPointList();
			for (int32 i = 0; i < 4; i++)
				the_script->AddPoint(points[i]);
		} else {
			for (int32 i = 0; i < 4; i++)
				the_script->AddPoint(corners[i]);
		}
		delete poly;
		delete[] corners;

		window->Lock();
		view->SetHighColor(old_color);
		view->SetDrawingMode(old_mode);
		view->UpdateImage(LastUpdatedRect());
		view->Sync();
		window->Unlock();

		delete drawer;
		delete imageUpdater;
	}

	delete tmpBuffer;
	delete srcBuffer;

	return the_script;
}


int32
RectangleTool::UseToolWithScript(ToolScript*, BBitmap*)
{
	return B_OK;
}


BView*
RectangleTool::ConfigView()
{
	return new RectangleToolConfigView(this);
}


const void*
RectangleTool::ToolCursor() const
{
	return HS_RECTANGLE_CURSOR;
}


const char*
RectangleTool::HelpString(bool isInUse) const
{
	return (isInUse ? B_TRANSLATE("Drawing a rectangle.")
					: B_TRANSLATE("Rectangle: SHIFT for square, ALT for centered"));
}


// #pragma mark -- RectangleToolConfigView


RectangleToolConfigView::RectangleToolConfigView(DrawingTool* tool)
	:
	DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", WIDTH_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(WIDTH_OPTION));

		fLineWidth = new NumberSliderControl(B_TRANSLATE("Outline width:"), "1", message, 1, 100, false);

		BGridLayout* lineWidthLayout = LayoutSliderGrid(fLineWidth);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", FILL_ENABLED_OPTION);
		message->AddInt32("value", 0x00000000);

		fFillRectangle = new BCheckBox(B_TRANSLATE("Fill rectangle"), message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", ROTATION_ENABLED_OPTION);
		message->AddInt32("value", 0x00000000);

		fRotation = new BCheckBox(B_TRANSLATE("Enable rotation"), message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", ANTI_ALIASING_LEVEL_OPTION);
		message->AddInt32("value", 0x00000000);

		fAntiAlias = new BCheckBox(B_TRANSLATE("Enable antialiasing"), message);

		BSeparatorView* view = new BSeparatorView(B_TRANSLATE("Mode"), B_HORIZONTAL, B_FANCY_BORDER,
			BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));
		view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		layout->AddView(BGroupLayoutBuilder(B_VERTICAL, kWidgetSpacing)
			.Add(lineWidthLayout)
			.AddGroup(B_VERTICAL, kWidgetSpacing)
				.Add(fFillRectangle)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.AddStrut(kWidgetSpacing)
			.Add(SeparatorView(B_TRANSLATE("Options")))
			.AddGroup(B_VERTICAL, kWidgetSpacing)
				.Add(fRotation)
				.Add(fAntiAlias)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.TopView()
		);

		if (tool->GetCurrentValue(FILL_ENABLED_OPTION) != B_CONTROL_OFF)
			fFillRectangle->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(ROTATION_ENABLED_OPTION) != B_CONTROL_OFF)
			fRotation->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(ANTI_ALIASING_LEVEL_OPTION) != B_CONTROL_OFF)
			fAntiAlias->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(FILL_ENABLED_OPTION) != B_CONTROL_OFF)
			fLineWidth->SetEnabled(FALSE);
	}
}


void
RectangleToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fFillRectangle->SetTarget(this);
	fRotation->SetTarget(this);
	fAntiAlias->SetTarget(this);
	fLineWidth->SetTarget(this);
}


void
RectangleToolConfigView::MessageReceived(BMessage* message)
{
	DrawingToolConfigView::MessageReceived(message);

	switch (message->what) {
		case OPTION_CHANGED:
		{
			if (message->FindInt32("option") == FILL_ENABLED_OPTION)
				if (fFillRectangle->Value() == B_CONTROL_ON)
					fLineWidth->SetEnabled(FALSE);
				else
					fLineWidth->SetEnabled(TRUE);
		} break;
	}
}
