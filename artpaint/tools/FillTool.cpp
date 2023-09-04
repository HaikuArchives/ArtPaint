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

#include "FillTool.h"

#include "BitmapDrawer.h"
#include "BitmapUtilities.h"
#include "ColorPalette.h"
#include "ColorView.h"
#include "Cursors.h"
#include "Image.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "PixelOperations.h"
#include "Selection.h"
#include "ToolScript.h"
#include "UtilityClasses.h"


#include <Catalog.h>
#include <CheckBox.h>
#include <Control.h>
#include <File.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>
#include <SeparatorView.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


using ArtPaint::Interface::NumberSliderControl;


FillTool::FillTool()
	:
	DrawingTool(B_TRANSLATE("Fill tool"), "i", FILL_TOOL)
{
	// Set options here. The MODE_OPTION is used for determining if we do flood
	// fill or some other type of fill.
	fOptions = GRADIENT_ENABLED_OPTION | PREVIEW_ENABLED_OPTION | TOLERANCE_OPTION | MODE_OPTION
		| SHAPE_OPTION;
	fOptionsCount = 5;
	binary_fill_map = NULL;

	// Initially disable the gradient.
	SetOption(GRADIENT_ENABLED_OPTION, B_CONTROL_OFF);
	// Initially enable the preview.
	SetOption(PREVIEW_ENABLED_OPTION, B_CONTROL_ON);
	SetOption(TOLERANCE_OPTION, 0);
	SetOption(MODE_OPTION, B_CONTROL_ON);
	SetOption(SHAPE_OPTION, GRADIENT_LINEAR);

	gradient_color1 = 0x00000000;
	gradient_color2 = 0xFFFFFFFF;
}


ToolScript*
FillTool::UseTool(ImageView* view, uint32 buttons, BPoint point, BPoint viewPoint)
{
	ToolScript* toolScript
		= new ToolScript(Type(), fToolSettings, ((PaintApplication*)be_app)->Color(true));
	toolScript->AddPoint(point);

	Selection* selection = view->GetSelection();
	selection = (selection->IsEmpty() ? NULL : selection);

	if (fToolSettings.gradient_enabled == B_CONTROL_ON) {
		// Do just a fill with gradient
		toolScript->AddPoint(GradientFill(view, buttons, point, viewPoint, selection));
	} else {
		// Do just a normal fill without gradient
		NormalFill(view, buttons, point, selection);
	}

	return toolScript;
}


int32
FillTool::UseToolWithScript(ToolScript*, BBitmap*)
{
	return B_OK;
}


BView*
FillTool::ConfigView()
{
	return new FillToolConfigView(this, gradient_color1, gradient_color2);
}


status_t
FillTool::NormalFill(ImageView* view, uint32 buttons, BPoint start, Selection* sel)
{
	// Get the necessary parameters
	BWindow* window = view->Window();
	if (window == NULL)
		return B_ERROR;

	uint32 tolerance = (uint32)((float)fToolSettings.tolerance / 100.0 * 255);

	filled_bitmap = view->ReturnImage()->ReturnActiveBitmap();
	BitmapDrawer* drawer = new BitmapDrawer(filled_bitmap);
	BRect bitmap_bounds = filled_bitmap->Bounds();
	bitmap_bounds.OffsetTo(BPoint(0, 0));

	// Get the color for the fill.
	bool use_fg_color = true;
	if (buttons == B_SECONDARY_MOUSE_BUTTON)
		use_fg_color = false;

	rgb_color c = ((PaintApplication*)be_app)->Color(use_fg_color);
	uint32 color = RGBColorToBGRA(c);

	// Get the old color.
	uint32 old_color = drawer->GetPixel(start);

	// If the old color is the same as new and the tolerance is 0, we should do nothing.
	if (old_color == color && tolerance == 0) {
		delete drawer;
		return B_OK;
	}

	// These are the edge coordinates of bitmap. It is still safe to
	// address a pixel at max_x,max_y or min_x,min_y.
	int32 min_x, min_y, max_x, max_y;
	min_x = (int32)bitmap_bounds.left;
	min_y = (int32)bitmap_bounds.top;
	max_x = (int32)bitmap_bounds.right;
	max_y = (int32)bitmap_bounds.bottom;

	if (bitmap_bounds.Contains(start) == TRUE) {
		if (fToolSettings.mode == B_CONTROL_ON) { // Do the flood fill
			// Here fill the area using drawer's SetPixel and GetPixel.
			// The algorithm uses 4-connected version of flood-fill.
			// The SetPixel and GetPixel functions are versions that
			// do not check bounds so we have to be careful not to exceed
			// bitmap's bounds.
			if (tolerance != 0) {
				binary_fill_map = new BBitmap(filled_bitmap->Bounds(), B_GRAY1);
				// Clear the binary map.
				uchar* binary_bits = (uchar*)binary_fill_map->Bits();
				uint32 binary_bitslength = binary_fill_map->BitsLength();
				for (uint32 i = 0; i < binary_bitslength; i++)
					*binary_bits++ = 0x00;
			}

			PointStack stack;
			stack.Push(start);

			while (!stack.IsEmpty()) {
				BPoint span_start = stack.Pop();
				if ((span_start.y == min_y) && (min_y != max_y)) {
					// Only check the spans below this line
					CheckSpans(span_start, drawer, stack, min_x, max_x, color, old_color, tolerance,
						sel, LOWER);
				} else if ((span_start.y == max_y) && (min_y != max_y)) {
					// Only check the spans above this line.
					CheckSpans(span_start, drawer, stack, min_x, max_x, color, old_color, tolerance,
						sel, UPPER);
				} else if (min_y != max_y) {
					// Check the spans above and below this line.
					CheckSpans(span_start, drawer, stack, min_x, max_x, color, old_color, tolerance,
						sel, BOTH);
				} else {
					// The image is only one pixel high. Fill the only span.
					FillSpan(span_start, drawer, min_x, max_x, color, old_color, tolerance, sel);
				}
			}
			if (tolerance != 0) {
				delete binary_fill_map;
				binary_fill_map = NULL;
			}
		} else { // Fill all the pixels that are within the tolerance.
			for (int32 y = min_y; y <= max_y; y++) {
				for (int32 x = min_x; x <= max_x; x++) {
					if ((sel == NULL || sel->IsEmpty() == true || sel->ContainsPoint(x, y))
						&& compare_2_pixels_with_variance(
							old_color, drawer->GetPixel(x, y), tolerance)) {
						drawer->SetPixel(x, y, color, sel);
					}
				}
			}
		}

		SetLastUpdatedRect(filled_bitmap->Bounds());
		window->Lock();
		view->UpdateImage(LastUpdatedRect());
		view->Sync();
		window->Unlock();
	}
	delete drawer;
	return B_OK;
}


void
FillTool::CheckSpans(BPoint span_start, BitmapDrawer* drawer, PointStack& stack, int32 min_x,
	int32 max_x, uint32 new_color, uint32 old_color, int32 tolerance, Selection* sel,
	span_type spans)
{
	// First get the vital data.
	int32 x, start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;
	bool inside_lower_span = FALSE;
	bool inside_upper_span = FALSE;

	// This is the case that takes the variance into account. We must use a
	// binary bitmap to see what parts have already been filled.
	uint32 binary_bpr = 0;
	uchar* binary_bits = NULL;

	if (binary_fill_map != NULL) {
		binary_bpr = binary_fill_map->BytesPerRow();
		binary_bits = (uchar*)binary_fill_map->Bits();
	}

	// Then go from start towards the left side of the bitmap.
	while ((sel == NULL || sel->IsEmpty() == TRUE || sel->ContainsPoint(x, y)) && (x >= min_x)
		&& (compare_2_pixels_with_variance(drawer->GetPixel(x, y), old_color, tolerance))) {
		if ((binary_bits != NULL)
			&& ((*(binary_bits + y * binary_bpr + (x / 8)) & (0x01 << (7 - x % 8))) != 0x00)) {
			x--;
			break;
		} else if (binary_bits != NULL)
			*(binary_bits + y * binary_bpr + (x / 8)) |= (0x01 << (7 - x % 8));

		drawer->SetPixel(x, y, new_color, sel);

		if (spans == BOTH || spans == LOWER) {
			if ((inside_lower_span == FALSE)
				&& (compare_2_pixels_with_variance(
					drawer->GetPixel(x, y + 1), old_color, tolerance))
				&& (sel == NULL || sel->IsEmpty() == TRUE
					|| sel->ContainsPoint(x, y + 1) == true)) {
				stack.Push(BPoint(x, y + 1));
				inside_lower_span = TRUE;
			} else if ((inside_lower_span == TRUE)
				&& (!compare_2_pixels_with_variance(
						drawer->GetPixel(x, y + 1), old_color, tolerance)
					|| (sel == NULL || sel->IsEmpty() == TRUE
						|| sel->ContainsPoint(x, y + 1) == false))) {
				inside_lower_span = FALSE;
			}
		}

		if (spans == BOTH || spans == UPPER) {
			if ((inside_upper_span == FALSE)
				&& (compare_2_pixels_with_variance(
					drawer->GetPixel(x, y - 1), old_color, tolerance))
				&& (sel == NULL || sel->IsEmpty() == TRUE
					|| sel->ContainsPoint(x, y - 1) == true)) {
				stack.Push(BPoint(x, y - 1));
				inside_upper_span = TRUE;
			} else if ((inside_upper_span == TRUE)
				&& (!compare_2_pixels_with_variance(
						drawer->GetPixel(x, y - 1), old_color, tolerance)
					|| (sel == NULL || sel->IsEmpty() == TRUE
						|| sel->ContainsPoint(x, y - 1) == false))) {
				inside_upper_span = FALSE;
			}
		}

		x--;
	}

	// Then go from start_x+1 towards the right side of the bitmap.
	// We might already be inside a lower span
	inside_lower_span
		= compare_2_pixels_with_variance(drawer->GetPixel(start_x, y + 1), old_color, tolerance)
		&& (sel == NULL || sel->IsEmpty() == TRUE || sel->ContainsPoint(start_x, y + 1));
	inside_upper_span
		= compare_2_pixels_with_variance(drawer->GetPixel(start_x, y - 1), old_color, tolerance)
		&& (sel == NULL || sel->IsEmpty() == TRUE || sel->ContainsPoint(start_x, y - 1));
	x = start_x + 1;
	while ((sel == NULL || sel->IsEmpty() == TRUE || sel->ContainsPoint(x, y)) && (x <= max_x)
		&& (compare_2_pixels_with_variance(drawer->GetPixel(x, y), old_color, tolerance))) {
		if ((binary_bits != NULL)
			&& ((*(binary_bits + y * binary_bpr + (x / 8)) & (0x01 << (7 - x % 8))) != 0x00)) {
			x++;
			break;
		} else if (binary_bits != NULL)
			*(binary_bits + y * binary_bpr + (x / 8)) |= (0x01 << (7 - x % 8));

		drawer->SetPixel(x, y, new_color, sel);

		if (spans == BOTH || spans == LOWER) {
			if ((inside_lower_span == FALSE)
				&& (compare_2_pixels_with_variance(
					drawer->GetPixel(x, y + 1), old_color, tolerance))
				&& (sel == NULL || sel->IsEmpty() == TRUE
					|| sel->ContainsPoint(x, y + 1) == true)) {
				stack.Push(BPoint(x, y + 1));
				inside_lower_span = TRUE;
			} else if ((inside_lower_span == TRUE)
				&& (!compare_2_pixels_with_variance(
						drawer->GetPixel(x, y + 1), old_color, tolerance)
					|| (sel == NULL || sel->IsEmpty() == TRUE
						|| sel->ContainsPoint(x, y + 1) == false))) {
				inside_lower_span = FALSE;
			}
		}

		if (spans == BOTH || spans == UPPER) {
			if ((inside_upper_span == FALSE)
				&& (compare_2_pixels_with_variance(
					drawer->GetPixel(x, y - 1), old_color, tolerance))
				&& (sel == NULL || sel->IsEmpty() == TRUE
					|| sel->ContainsPoint(x, y - 1) == true)) {
				stack.Push(BPoint(x, y - 1));
				inside_upper_span = TRUE;
			} else if ((inside_upper_span == TRUE)
				&& (!compare_2_pixels_with_variance(
						drawer->GetPixel(x, y - 1), old_color, tolerance)
					|| (sel == NULL || sel->IsEmpty() == TRUE
						|| sel->ContainsPoint(x, y - 1) == false))) {
				inside_upper_span = FALSE;
			}
		}

		x++;
	}
}


void
FillTool::FillSpan(BPoint span_start, BitmapDrawer* drawer, int32 min_x, int32 max_x,
	uint32 new_color, uint32 old_color, int32 tolerance, Selection* sel)
{
	// First get the vital data.
	int32 x, start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;

	uint32 binary_bpr = 0;
	uchar* binary_bits = NULL;

	if (binary_fill_map != NULL) {
		binary_bpr = binary_fill_map->BytesPerRow();
		binary_bits = (uchar*)binary_fill_map->Bits();
	}

	// Then go from start towards the left side of the bitmap.
	while ((sel == NULL || sel->IsEmpty() || sel->ContainsPoint(x, y)) && (x >= min_x)
		&& (compare_2_pixels_with_variance(drawer->GetPixel(x, y), old_color, tolerance))) {
		drawer->SetPixel(x, y, new_color, sel);
		if (binary_bits != NULL)
			*(binary_bits + y * binary_bpr + (x / 8))
				= *(binary_bits + y * binary_bpr + (x / 8)) | (0x01 << (7 - x % 8));
		x--;
	}

	// Then go from start_x+1 towards the right side of the bitmap.
	x = start_x + 1;
	while ((sel == NULL || sel->IsEmpty() || sel->ContainsPoint(x, y)) && (x <= max_x)
		&& (compare_2_pixels_with_variance(drawer->GetPixel(x, y), old_color, tolerance))) {
		drawer->SetPixel(x, y, new_color, sel);
		if (binary_bits != NULL)
			*(binary_bits + y * binary_bpr + (x / 8))
				= *(binary_bits + y * binary_bpr + (x / 8)) | (0x01 << (7 - x % 8));
		x++;
	}
}


BPoint
FillTool::GradientFill(
	ImageView* view, uint32 buttons, BPoint start, BPoint orig_view_point, Selection* sel)
{
	// First calculate points that are to be included in the fill to
	// a separate binary mask. Then go through the filled areas bounds
	// rectangle and fill only those pixels that the mask tells to.
	// The color of pixel is to be calculated from the original mousedown point
	// and the last mousedown point and from color values for mouse-button
	// and gradient color. If preview is enabled we should also update in real time
	// whenever possible.

	// Get the necessary parameters
	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	BWindow* window = view->Window();
	if (window == NULL)
		return BPoint(-1, -1);

	BBitmap* bitmap = view->ReturnImage()->ReturnActiveBitmap();
	BRect bitmap_bounds = bitmap->Bounds();
	bitmap_bounds.OffsetTo(BPoint(0, 0));

	BBitmap* srcBuffer = new BBitmap(bitmap);

	BBitmap* tmpBuffer = new BBitmap(bitmap);
	BitmapDrawer* drawer = new BitmapDrawer(tmpBuffer);
	union color_conversion clear_color;
	clear_color.word = 0xFFFFFFFF;
	clear_color.bytes[3] = 0x00;

	window->Lock();
	drawing_mode old_mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_INVERT);
	window->Unlock();
	BPoint original_point = start;
	BPoint new_point = start;

	if (bitmap_bounds.Contains(start) == TRUE) {

		// These are the edge coordinates of bitmap. It is still safe to
		// address a pixel at max_x,max_y or min_x,min_y.
		int32 min_x, min_y, max_x, max_y;
		min_x = (int32)bitmap_bounds.left;
		min_y = (int32)bitmap_bounds.top;
		max_x = (int32)bitmap_bounds.right;
		max_y = (int32)bitmap_bounds.bottom;

		// At this point we should take some action if min_x == max_x or min_y == max_y

		// Get the old color.
		uint32 old_color = drawer->GetPixel(start);

		uint32 gradient_color = gradient_color1;
		uint32 color = gradient_color2;

		// Here calculate the binary bitmap for the purpose of doing the gradient.
		BBitmap* binary_map;
		if (fToolSettings.mode == B_CONTROL_OFF) {
			// Not flood-mode
			binary_map = MakeBinaryMap(drawer, min_x, max_x, min_y, max_y, old_color, sel);
		} else {
			// Flood-mode
			binary_map
				= MakeFloodBinaryMap(drawer, min_x, max_x, min_y, max_y, old_color, start, sel);
		}

		// Here calculate the bounding rectangle of the filled area and
		// change the min and max coordinates to those edges of the rect.
		BRect filled_area_bounds = calcBinaryMapBounds(binary_map);

		BRect ellipse_rect = BRect(orig_view_point - BPoint(3, 3), orig_view_point + BPoint(3, 3));
		BPoint new_view_point = orig_view_point;
		BPoint prev_view_point = new_view_point;
		window->Lock();
		view->StrokeEllipse(ellipse_rect);
		window->Unlock();

		// Do not do the preview. Just read the coordinates.
		while (buttons) {
			float scale = view->getMagScale();

			window->Lock();
			view->getCoords(&new_point, &buttons, &new_view_point);
			window->Unlock();

			if (modifiers() & B_SHIFT_KEY) {
				// Make the new point be so that the angle is a multiple of 22.5°.
				float x_diff, y_diff;
				x_diff = fabs(original_point.x - new_point.x);
				y_diff = fabs(original_point.y - new_point.y);
				float len = sqrt(x_diff * x_diff + y_diff * y_diff);
				float angle = atan(y_diff / x_diff) * 180 / M_PI;

				angle = SnapToAngle(22.5, angle);

				y_diff = len * sin(angle * M_PI / 180.);
				x_diff = len * cos(angle * M_PI / 180.);

				float signed_x_diff = (new_view_point.x - orig_view_point.x);
				float signed_y_diff = (new_view_point.y - orig_view_point.y);
				if (signed_x_diff != 0) {
					new_view_point.x
						= orig_view_point.x + x_diff * signed_x_diff * scale / fabs(signed_x_diff);
					new_point.x = original_point.x + x_diff * signed_x_diff / fabs(signed_x_diff);
				}

				if (signed_y_diff != 0) {
					new_view_point.y
						= orig_view_point.y + y_diff * signed_y_diff * scale / fabs(signed_y_diff);
					new_point.y = original_point.y + y_diff * signed_y_diff / fabs(signed_y_diff);
				}
			}

			if (new_view_point != prev_view_point) {
				if (fToolSettings.preview_enabled == B_CONTROL_OFF) {
					window->Lock();
					BRect clear_rect;
					clear_rect.left = min_c(orig_view_point.x, prev_view_point.x);
					clear_rect.top = min_c(orig_view_point.y, prev_view_point.y);
					clear_rect.right = max_c(orig_view_point.x, prev_view_point.x);
					clear_rect.bottom = max_c(orig_view_point.y, prev_view_point.y);
					BPoint delta = prev_view_point - new_view_point;
					clear_rect.InsetBy(-abs(delta.x), -abs(delta.y));
					view->Draw(clear_rect);
					view->StrokeEllipse(ellipse_rect);
					view->StrokeLine(orig_view_point, new_view_point);
					window->Unlock();
				} else {
					// There should actually be a separate function (and maybe even a thread) that
					// calculates the preview in real time for some bitmap that is quite small
					// (about 200x200 pixels maximum). Then the gradient would be copied to bitmap
					// using some specialized function.
					BitmapUtilities::ClearBitmap(tmpBuffer, clear_color.word);

					if (fToolSettings.shape == GRADIENT_CONIC) {
						FillGradientConic(drawer, binary_map, start, new_point, min_x, max_x, min_y,
							max_y, color, gradient_color, 2, sel);
					} else if (fToolSettings.shape == GRADIENT_RADIAL) {
						FillGradientRadial(drawer, binary_map, start, new_point, min_x, max_x,
							min_y, max_y, color, gradient_color, 2, sel);
					} else if (fToolSettings.shape == GRADIENT_SQUARE) {
						FillGradientSquare(drawer, binary_map, start, new_point, min_x, max_x,
							min_y, max_y, color, gradient_color, 2, sel);
					} else {
						FillGradientLinear(drawer, binary_map, start, new_point, min_x, max_x,
							min_y, max_y, color, gradient_color, 2, sel);
					}

					bitmap->Lock();
					BitmapUtilities::CompositeBitmapOnSource(
						bitmap, srcBuffer, tmpBuffer, bitmap_bounds, src_over_fixed);
					bitmap->Unlock();

					window->Lock();
					view->SetDrawingMode(old_mode);
					view->UpdateImage(bitmap_bounds);
					view->Sync();
					view->SetDrawingMode(B_OP_INVERT);
					view->StrokeEllipse(ellipse_rect);
					view->StrokeLine(orig_view_point, new_view_point);
					window->Unlock();
				}
				prev_view_point = new_view_point;
			}

			snooze(20 * 1000);
		}

		BitmapUtilities::ClearBitmap(tmpBuffer, clear_color.word);

		// Here calculate the final gradient.
		if (fToolSettings.shape == GRADIENT_CONIC) {
			FillGradientConic(drawer, binary_map, start, new_point, min_x, max_x, min_y, max_y,
				color, gradient_color, 1, sel);
		} else if (fToolSettings.shape == GRADIENT_RADIAL) {
			FillGradientRadial(drawer, binary_map, start, new_point, min_x, max_x, min_y, max_y,
				color, gradient_color, 1, sel);
		} else if (fToolSettings.shape == GRADIENT_SQUARE) {
			FillGradientSquare(drawer, binary_map, start, new_point, min_x, max_x, min_y, max_y,
				color, gradient_color, 1, sel);
		} else {
			FillGradientLinear(drawer, binary_map, start, new_point, min_x, max_x, min_y, max_y,
				color, gradient_color, 1, sel);
		}
		// Update the image-view.
		bitmap->Lock();
		BitmapUtilities::CompositeBitmapOnSource(
			bitmap, srcBuffer, tmpBuffer, bitmap_bounds, src_over_fixed);
		bitmap->Unlock();

		delete binary_map;
		SetLastUpdatedRect(filled_area_bounds);
		window->Lock();
		view->SetDrawingMode(old_mode);
		view->UpdateImage(LastUpdatedRect());
		view->Sync();
		window->Unlock();
	}

	delete drawer;
	delete srcBuffer;
	delete tmpBuffer;

	return new_point;
}


BBitmap*
FillTool::MakeBinaryMap(BitmapDrawer* drawer, int32 min_x, int32 max_x, int32 min_y, int32 max_y,
	uint32 old_color, Selection* sel)
{
	// This function makes a binary bitmap that has ones where the
	// color of original bitmap is same as old_color, and zeroes elsewhere.
	BBitmap* binary_map = new BBitmap(BRect(min_x, min_y, max_x, max_y), B_GRAY1);
	memset(binary_map->Bits(), 0x00, binary_map->BitsLength());

	uchar* binary_bits = (uchar*)binary_map->Bits();
	int32 binary_bpr = binary_map->BytesPerRow();

	if ((sel == NULL) || (sel->IsEmpty() == TRUE)) {
		if (fToolSettings.tolerance == 0) {
			// Always collect eight pixels from the bitmap and
			// then move that data to the binary bitmap.
			uchar next_value = 0x00;
			for (int32 y = min_y; y <= max_y; y++) {
				int32 bytes_advanced = 0;
				for (int32 x = min_x; x <= max_x; x++) {
					if (((x % 8) == 0) && (x != 0)) {
						*binary_bits++ = next_value;
						bytes_advanced++;
						next_value = 0x00;
					}
					next_value
						|= ((old_color == drawer->GetPixel(x, y)) ? (0x01 << (7 - (x % 8))) : 0x00);
				}
				if ((max_x % 8) != 0) {
					*binary_bits++ = next_value;
					bytes_advanced++;
					next_value = 0x00;
				}
				binary_bits += binary_bpr - bytes_advanced;
			}
		} else {
			// Always collect eight pixels from the bitmap and
			// then move that data to the binary bitmap.
			uchar next_value = 0x00;
			uint32 tolerance = (uint32)((float)fToolSettings.tolerance / 100.0 * 255);
			for (int32 y = min_y; y <= max_y; y++) {
				int32 bytes_advanced = 0;
				for (int32 x = min_x; x <= max_x; x++) {
					if (((x % 8) == 0) && (x != 0)) {
						*binary_bits++ = next_value;
						bytes_advanced++;
						next_value = 0x00;
					}
					next_value |= ((compare_2_pixels_with_variance(
									   old_color, drawer->GetPixel(x, y), tolerance))
							? (0x01 << (7 - (x % 8)))
							: 0x00);
				}
				if ((max_x % 8) != 0) {
					*binary_bits++ = next_value;
					bytes_advanced++;
					next_value = 0x00;
				}
				binary_bits += binary_bpr - bytes_advanced;
			}
		}
	} else {
		if (fToolSettings.tolerance == 0) {
			// Always collect eight pixels from the bitmap and
			// then move that data to the binary bitmap.
			uchar next_value = 0x00;
			for (int32 y = min_y; y <= max_y; y++) {
				int32 bytes_advanced = 0;
				for (int32 x = min_x; x <= max_x; x++) {
					if (((x % 8) == 0) && (x != 0)) {
						*binary_bits++ = next_value;
						bytes_advanced++;
						next_value = 0x00;
					}
					next_value
						|= (((old_color == drawer->GetPixel(x, y)) && sel->ContainsPoint(x, y))
								? (0x01 << (7 - (x % 8)))
								: 0x00);
				}
				if ((max_x % 8) != 0) {
					*binary_bits++ = next_value;
					bytes_advanced++;
					next_value = 0x00;
				}
				binary_bits += binary_bpr - bytes_advanced;
			}
		} else {
			// Always collect eight pixels from the bitmap and
			// then move that data to the binary bitmap.
			uchar next_value = 0x00;
			uint32 tolerance = (uint32)((float)fToolSettings.tolerance / 100.0 * 255);
			for (int32 y = min_y; y <= max_y; y++) {
				int32 bytes_advanced = 0;
				for (int32 x = min_x; x <= max_x; x++) {
					if (((x % 8) == 0) && (x != 0)) {
						*binary_bits++ = next_value;
						bytes_advanced++;
						next_value = 0x00;
					}
					next_value |= ((compare_2_pixels_with_variance(
										old_color, drawer->GetPixel(x, y), tolerance)
										&& sel->ContainsPoint(x, y))
							? (0x01 << (7 - (x % 8)))
							: 0x00);
				}
				if ((max_x % 8) != 0) {
					*binary_bits++ = next_value;
					bytes_advanced++;
					next_value = 0x00;
				}
				binary_bits += binary_bpr - bytes_advanced;
			}
		}
	}
	return binary_map;
}


BBitmap*
FillTool::MakeFloodBinaryMap(BitmapDrawer* drawer, int32 min_x, int32 max_x, int32 min_y,
	int32 max_y, uint32 old_color, BPoint start, Selection* sel)
{
	// This function makes a binary bitmap of the image. It contains ones where
	// the flood fill should fill and zeroes elsewhere.
	BBitmap* binary_map;
	binary_map = binary_fill_map = new BBitmap(BRect(min_x, min_y, max_x, max_y), B_GRAY1);
	memset(binary_map->Bits(), 0x00, binary_map->BitsLength());

	// We can use the functions CheckLowerSpans, CheckUpperSpans and CheckBothSpans
	// to calculate the binary_fill_map and then return it.

	// Here we proceed just like in the case of a normal fill, except that we do not
	// show the intermediate fill to the user.

	// Here fill the area using drawer's SetPixel and GetPixel.
	// The algorithm uses 4-connected version of flood-fill.
	// The SetPixel and GetPixel functions are versions that
	// do not check bounds so we have to be careful not to exceed
	// bitmap's bounds.
	union color_conversion bg;
	bg.word = 0xFFFFFFFF;
	bg.bytes[3] = 0;

	uint32 color = bg.word; // This is the temporary color that will be used
							// to fill the bitmap.
	uint32 tolerance = (uint32)((float)fToolSettings.tolerance / 100.0 * 255);

	PointStack stack;
	stack.Push(start);

	while (!stack.IsEmpty()) {
		BPoint span_start = stack.Pop();
		if ((span_start.y == min_y) && (min_y != max_y)) {
			// Only check the spans below this line
			CheckSpans(
				span_start, drawer, stack, min_x, max_x, color, old_color, tolerance, sel, LOWER);
		} else if ((span_start.y == max_y) && (min_y != max_y)) {
			// Only check the spans above this line.
			CheckSpans(
				span_start, drawer, stack, min_x, max_x, color, old_color, tolerance, sel, UPPER);
		} else if (min_y != max_y) {
			// Check the spans above and below this line.
			CheckSpans(
				span_start, drawer, stack, min_x, max_x, color, old_color, tolerance, sel, BOTH);
		} else {
			// The image is only one pixel high. Fill the only span.
			FillSpan(span_start, drawer, min_x, max_x, color, old_color, tolerance, sel);
		}
	}

	// Remember to NULL the attribute binary_fill_map
	binary_fill_map = NULL;
	return binary_map;
}


void
FillTool::FillGradientLinear(BitmapDrawer* drawer, BBitmap* binary_map, BPoint start, BPoint end,
	int32 min_x, int32 max_x, int32 min_y, int32 max_y, uint32 new_color, uint32 gradient_color,
	uint8 skip, Selection* sel)
{
	uchar* binary_bits = (uchar*)binary_map->Bits();
	int32 binary_bpr = binary_map->BytesPerRow();

	float dx = (end.x - start.x);
	float dy = (end.y - start.y);

	float total_dist = sqrt(pow(dx, 2) + pow(dy, 2));
	float perp_angle = M_PI / 2; // 0;
	if (dy != 0)
		perp_angle = atan(-dx / dy);
	else if (dx < 0 and dy == 0)
		perp_angle = -perp_angle;

	float cos_angle = cos(perp_angle);
	float sin_angle = sin(perp_angle);

	union color_conversion source, dest, begin, temp;

	source.word = new_color;
	dest.word = gradient_color;

	int16 red_diff = (dest.bytes[2] - source.bytes[2]);
	int16 green_diff = (dest.bytes[1] - source.bytes[1]);
	int16 blue_diff = (dest.bytes[0] - source.bytes[0]);
	int16 alpha_diff = (dest.bytes[3] - source.bytes[3]);

	for (int32 y = min_y; y < max_y; y += skip) {
		for (int32 x = min_x; x < max_x; x += skip) {
			uint8 bits = *(binary_bits + (x / 8) + y * binary_bpr);
			if (bits & (0x01 << (7 - (x % 8)))) {
				float dist = (cos_angle * (start.y - y) - sin_angle * (start.x - x));

				union color_conversion out_color;

				if (dy > 0)
					dist = -dist;

				if (dist >= total_dist)
					out_color.word = source.word;
				else if (dist <= 0)
					out_color.word = dest.word;
				else {
					float ratio = 1. - abs(dist / total_dist);

					out_color.word = source.word;
					float r = out_color.bytes[2];
					float g = out_color.bytes[1];
					float b = out_color.bytes[0];
					float a = out_color.bytes[3];

					r += (float)red_diff * ratio;
					g += (float)green_diff * ratio;
					b += (float)blue_diff * ratio;
					a += (float)alpha_diff * ratio;

					out_color.bytes[0] = (uint8)b;
					out_color.bytes[1] = (uint8)g;
					out_color.bytes[2] = (uint8)r;
					out_color.bytes[3] = (uint8)a;
				}

				for (int dy = 0; dy < skip; ++dy) {
					for (int dx = 0; dx < skip; ++dx)
						drawer->SetPixel(x + dx, y + dy, out_color.word, sel);
				}
			}
		}
	}
}


void
FillTool::FillGradientRadial(BitmapDrawer* drawer, BBitmap* binary_map, BPoint start, BPoint end,
	int32 min_x, int32 max_x, int32 min_y, int32 max_y, uint32 new_color, uint32 gradient_color,
	uint8 skip, Selection* sel)
{
	uchar* binary_bits = (uchar*)binary_map->Bits();
	int32 binary_bpr = binary_map->BytesPerRow();

	float dx = (end.x - start.x);
	float dy = (end.y - start.y);

	float total_dist = sqrt(pow(dx, 2) + pow(dy, 2));

	union color_conversion source, dest, begin, temp;

	source.word = new_color;
	dest.word = gradient_color;

	int16 red_diff = (dest.bytes[2] - source.bytes[2]);
	int16 green_diff = (dest.bytes[1] - source.bytes[1]);
	int16 blue_diff = (dest.bytes[0] - source.bytes[0]);
	int16 alpha_diff = (dest.bytes[3] - source.bytes[3]);

	for (int32 y = min_y; y < max_y; y += skip) {
		for (int32 x = min_x; x < max_x; x += skip) {
			uint8 bits = *(binary_bits + (x / 8) + y * binary_bpr);
			if (bits & (0x01 << (7 - (x % 8)))) {
				float dist = sqrt(pow(start.y - y, 2) + pow(start.x - x, 2));

				union color_conversion out_color;

				if (dist >= total_dist)
					out_color.word = source.word;
				else {
					float ratio = 1. - abs(dist / total_dist);

					out_color.word = source.word;
					float r = out_color.bytes[2];
					float g = out_color.bytes[1];
					float b = out_color.bytes[0];
					float a = out_color.bytes[3];

					r += (float)red_diff * ratio;
					g += (float)green_diff * ratio;
					b += (float)blue_diff * ratio;
					a += (float)alpha_diff * ratio;

					out_color.bytes[0] = (uint8)b;
					out_color.bytes[1] = (uint8)g;
					out_color.bytes[2] = (uint8)r;
					out_color.bytes[3] = (uint8)a;
				}

				for (int dy = 0; dy < skip; ++dy) {
					for (int dx = 0; dx < skip; ++dx)
						drawer->SetPixel(x + dx, y + dy, out_color.word, sel);
				}
			}
		}
	}
}


void
FillTool::FillGradientSquare(BitmapDrawer* drawer, BBitmap* binary_map, BPoint start, BPoint end,
	int32 min_x, int32 max_x, int32 min_y, int32 max_y, uint32 new_color, uint32 gradient_color,
	uint8 skip, Selection* sel)
{
	uchar* binary_bits = (uchar*)binary_map->Bits();
	int32 binary_bpr = binary_map->BytesPerRow();

	float dx = (end.x - start.x);
	float dy = (end.y - start.y);

	float total_dist = sqrt(pow(dx, 2) + pow(dy, 2));

	float perp_angle = M_PI / 2; // 0;
	if (dy != 0)
		perp_angle = atan(-dx / dy);
	float cos_angle = cos(perp_angle);
	float sin_angle = sin(perp_angle);
	union color_conversion source, dest, begin, temp;

	source.word = new_color;
	dest.word = gradient_color;

	int16 red_diff = (dest.bytes[2] - source.bytes[2]);
	int16 green_diff = (dest.bytes[1] - source.bytes[1]);
	int16 blue_diff = (dest.bytes[0] - source.bytes[0]);
	int16 alpha_diff = (dest.bytes[3] - source.bytes[3]);

	for (int32 y = min_y; y < max_y; y += skip) {
		for (int32 x = min_x; x < max_x; x += skip) {
			uint8 bits = *(binary_bits + (x / 8) + y * binary_bpr);
			if (bits & (0x01 << (7 - (x % 8)))) {
				float xp = (start.x - x) * cos_angle + (start.y - y) * sin_angle;
				float yp = -(start.x - x) * sin_angle + (start.y - y) * cos_angle;
				float dist = max_c(abs(yp), abs(xp));

				union color_conversion out_color;

				if (dist >= total_dist)
					out_color.word = source.word;
				else {
					float ratio = 1. - abs(dist / total_dist);

					out_color.word = source.word;
					float r = out_color.bytes[2];
					float g = out_color.bytes[1];
					float b = out_color.bytes[0];
					float a = out_color.bytes[3];

					r += (float)red_diff * ratio;
					g += (float)green_diff * ratio;
					b += (float)blue_diff * ratio;
					a += (float)alpha_diff * ratio;

					out_color.bytes[0] = (uint8)b;
					out_color.bytes[1] = (uint8)g;
					out_color.bytes[2] = (uint8)r;
					out_color.bytes[3] = (uint8)a;
				}

				for (int dy = 0; dy < skip; ++dy) {
					for (int dx = 0; dx < skip; ++dx)
						drawer->SetPixel(x + dx, y + dy, out_color.word, sel);
				}
			}
		}
	}
}


void
FillTool::FillGradientConic(BitmapDrawer* drawer, BBitmap* binary_map, BPoint start, BPoint end,
	int32 min_x, int32 max_x, int32 min_y, int32 max_y, uint32 new_color, uint32 gradient_color,
	uint8 skip, Selection* sel)
{
	uchar* binary_bits = (uchar*)binary_map->Bits();
	int32 binary_bpr = binary_map->BytesPerRow();

	float dx = (end.x - start.x);
	float dy = (end.y - start.y);

	float perp_angle = M_PI / 2;
	if (dx != 0)
		perp_angle = atan(-dy / dx);
	else if (dy < 0 && dx == 0)
		perp_angle = -perp_angle;

	float cos_angle = cos(perp_angle);
	float sin_angle = sin(perp_angle);

	union color_conversion source, dest, begin, temp;

	source.word = new_color;
	dest.word = gradient_color;

	int16 red_diff = (dest.bytes[2] - source.bytes[2]);
	int16 green_diff = (dest.bytes[1] - source.bytes[1]);
	int16 blue_diff = (dest.bytes[0] - source.bytes[0]);
	int16 alpha_diff = (dest.bytes[3] - source.bytes[3]);

	for (int32 y = min_y; y < max_y; y += skip) {
		for (int32 x = min_x; x < max_x; x += skip) {
			uint8 bits = *(binary_bits + (x / 8) + y * binary_bpr);
			if (bits & (0x01 << (7 - (x % 8)))) {
				float rx = cos_angle * (x - start.x) - sin_angle * (y - start.y);
				float ry = sin_angle * (x - start.x) + cos_angle * (y - start.y);

				if (dx > 0) {
					rx = -rx;
					ry = -ry;
				}

				float ratio = (atan2(ry, rx) + M_PI) / 2.0 / M_PI;

				union color_conversion out_color;

				out_color.word = source.word;
				float r = out_color.bytes[2];
				float g = out_color.bytes[1];
				float b = out_color.bytes[0];
				float a = out_color.bytes[3];

				r += (float)red_diff * ratio;
				g += (float)green_diff * ratio;
				b += (float)blue_diff * ratio;
				a += (float)alpha_diff * ratio;

				out_color.bytes[0] = (uint8)b;
				out_color.bytes[1] = (uint8)g;
				out_color.bytes[2] = (uint8)r;
				out_color.bytes[3] = (uint8)a;

				for (int dy = 0; dy < skip; ++dy) {
					for (int dx = 0; dx < skip; ++dx)
						drawer->SetPixel(x + dx, y + dy, out_color.word, sel);
				}
			}
		}
	}
}


BRect
FillTool::calcBinaryMapBounds(BBitmap* boolean_map)
{
	// it seems like the monochrome-bitmap is aligned at 16-bit boundary instead
	// of 32-bit as the BeBook claims
	char* bits = (char*)boolean_map->Bits();
	int32 bpr = boolean_map->BytesPerRow();
	bool selected_line;

	// this is an invalid rect
	BRect rc = BRect(100000, 100000, -100000, -100000);

	int32 height = boolean_map->Bounds().IntegerHeight();
	for (int32 y = 0; y <= height; y++) {
		selected_line = FALSE;
		for (int32 i = 0; i < bpr; i++) {
			selected_line |= *bits != 0x00;
			if (*bits != 0x00) {
				for (int32 b = 0; b < 8; b++) {
					rc.left
						= min_c(rc.left, i * 8 + (((*bits >> (7 - b)) & 0x00000001) ? b : 100000));
					rc.right = max_c(
						rc.right, i * 8 + (((*bits >> (7 - b)) & 0x00000001) ? b : -100000));
				}
			}
			++bits;
		}
		if (selected_line) {
			rc.top = min_c(rc.top, y);
			rc.bottom = max_c(rc.bottom, y);
		}
	}

	return rc & boolean_map->Bounds();
}


status_t
FillTool::readSettings(BFile& file, bool is_little_endian)
{
	if (DrawingTool::readSettings(file, is_little_endian) != B_OK)
		return B_ERROR;

	if (file.Read(&gradient_color1, sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;

	if (file.Read(&gradient_color2, sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;

	if (is_little_endian) {
		gradient_color1 = B_LENDIAN_TO_HOST_INT32(gradient_color1);
		gradient_color2 = B_LENDIAN_TO_HOST_INT32(gradient_color2);
	} else {
		gradient_color1 = B_BENDIAN_TO_HOST_INT32(gradient_color1);
		gradient_color2 = B_BENDIAN_TO_HOST_INT32(gradient_color2);
	}

	return B_OK;
}


status_t
FillTool::writeSettings(BFile& file)
{
	if (DrawingTool::writeSettings(file) != B_OK)
		return B_ERROR;

	if (file.Write(&gradient_color1, sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;

	if (file.Write(&gradient_color2, sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;

	return B_OK;
}


const void*
FillTool::ToolCursor() const
{
	return HS_FILL_CURSOR;
}


const char*
FillTool::HelpString(bool isInUse) const
{
	return (isInUse ? B_TRANSLATE("Making a fill.")
					: B_TRANSLATE("Fill tool: SHIFT locks 22.5° angles"));
}


// #pragma mark -- GradientView


#define GRADIENT_ADJUSTED 'gRad'


class FillToolConfigView::GradientView : public BControl
{
public:
					GradientView(uint32, uint32);
	virtual 		~GradientView();

	virtual void 	AttachedToWindow();
	virtual void 	Draw(BRect);
	virtual void 	MessageReceived(BMessage*);
	virtual void 	MouseDown(BPoint);
	virtual void 	FrameResized(float newWidth, float newHeight);
	virtual void 	LayoutChanged();

private:
			void 	_CalculateGradient();

private:
		rgb_color 	fColor1;
		rgb_color 	fColor2;

		BBitmap* 	fGradient;
};


FillToolConfigView::GradientView::GradientView(uint32 c1, uint32 c2)
	:
	BControl("gradient view", "Gradient", new BMessage(GRADIENT_ADJUSTED),
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS),
	fGradient(NULL)
{
	fColor1 = BGRAColorToRGB(c1);
	fColor2 = BGRAColorToRGB(c2);

	Message()->AddInt32("color1", RGBColorToBGRA(fColor1));
	Message()->AddInt32("color2", RGBColorToBGRA(fColor2));

	SetExplicitMinSize(BSize(100.0, 20.0));
	SetExplicitMaxSize(BSize(B_SIZE_UNSET, 20.0));
}


FillToolConfigView::GradientView::~GradientView()
{
	delete fGradient;
}


void
FillToolConfigView::GradientView::AttachedToWindow()
{
	BControl::AttachedToWindow();

	delete fGradient;

	BRect bounds = Bounds();
	bounds.bottom = bounds.top;
	fGradient = new BBitmap(bounds, B_RGB32);

	_CalculateGradient();
}


void
FillToolConfigView::GradientView::LayoutChanged()
{
	BControl::LayoutChanged();

	delete fGradient;

	BRect bounds = Bounds();
	bounds.bottom = bounds.top;
	fGradient = new BBitmap(bounds, B_RGB32);

	_CalculateGradient();
}


void
FillToolConfigView::GradientView::Draw(BRect rect)
{
	BControl::Draw(rect);

	SetHighColor(0, 0, 0, 255);
	StrokeRect(Bounds());

	BRect bounds = Bounds();
	bounds.InsetBy(1.0, 1.0);
	DrawBitmap(fGradient, fGradient->Bounds(), bounds);
}


void
FillToolConfigView::GradientView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_PASTE:
		{
			if (message->WasDropped()) {
				BPoint dropPoint = ConvertFromScreen(message->DropPoint());

				ssize_t size;
				const void* data;
				if (message->FindData("RGBColor", B_RGB_COLOR_TYPE, &data, &size) == B_OK
					&& size == sizeof(rgb_color)) {
					if (dropPoint.x < (Bounds().right / 2.0))
						memcpy((void*)(&fColor1), data, size);
					else
						memcpy((void*)(&fColor2), data, size);

					Message()->ReplaceInt32("color1", RGBColorToBGRA(fColor1));
					Message()->ReplaceInt32("color2", RGBColorToBGRA(fColor2));

					_CalculateGradient();
					Draw(Bounds());
					Invoke();
				}
			}
		} break;
		default:
			BControl::MessageReceived(message);
	}
}


void
FillToolConfigView::GradientView::MouseDown(BPoint point)
{
	BControl::MouseDown(point);

	rgb_color color = fColor1;
	if (point.x > (Bounds().right / 2.0))
		color = fColor2;

	// Open the palette-view.
	ColorPaletteWindow::showPaletteWindow();
	ColorPaletteWindow::ChangePaletteColor(color);
}


void
FillToolConfigView::GradientView::FrameResized(float newWidth, float newHeight)
{
	delete fGradient;
	fGradient = new BBitmap(BRect(0, 0, newWidth, 0), B_RGB32);

	_CalculateGradient();
}


void
FillToolConfigView::GradientView::_CalculateGradient()
{
	union {
		uint8 bytes[4];
		uint32 word;
	} c;

	uint32 c1 = RGBColorToBGRA(fColor1);
	uint32 c2 = RGBColorToBGRA(fColor2);

	uint32* bits = (uint32*)fGradient->Bits();
	int32 width = fGradient->BytesPerRow() / 4;

	for (int32 x = 0; x < width; ++x) {
		uint32 fixed_alpha = (1.0 - float(x) / float(width + 1.0)) * 32768;
		c.word = mix_2_pixels_fixed(c1, c2, fixed_alpha);
		float coeff = c.bytes[3] / 255.0;
		if ((x % 2) == 0) {
			c.bytes[0] = (uint8)(c.bytes[0] * coeff);
			c.bytes[1] = (uint8)(c.bytes[1] * coeff);
			c.bytes[2] = (uint8)(c.bytes[2] * coeff);
		} else {
			c.bytes[0] = (uint8)(c.bytes[0] * coeff + (1 - coeff) * 255);
			c.bytes[1] = (uint8)(c.bytes[1] * coeff + (1 - coeff) * 255);
			c.bytes[2] = (uint8)(c.bytes[2] * coeff + (1 - coeff) * 255);
		}
		*bits++ = c.word;
	}
}


// #pragma mark -- FillToolConfigView


FillToolConfigView::FillToolConfigView(DrawingTool* tool, uint32 c1, uint32 c2)
	:
	DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", MODE_OPTION);
		message->AddInt32("value", 0x00000000);
		fFloodFill = new BCheckBox(B_TRANSLATE("Flood fill"), message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", GRADIENT_ENABLED_OPTION);
		message->AddInt32("value", 0x00000000);
		fGradient = new BCheckBox(B_TRANSLATE("Enable gradient"), message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SHAPE_OPTION);
		message->AddInt32("value", GRADIENT_LINEAR);

		fLinearGradient = new BRadioButton(B_TRANSLATE("Linear"), message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SHAPE_OPTION);
		message->AddInt32("value", GRADIENT_RADIAL);

		fRadialGradient = new BRadioButton(B_TRANSLATE("Radial"), message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SHAPE_OPTION);
		message->AddInt32("value", GRADIENT_SQUARE);

		fSquareGradient = new BRadioButton(B_TRANSLATE("Square"), message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SHAPE_OPTION);
		message->AddInt32("value", GRADIENT_CONIC);

		fConicGradient = new BRadioButton(B_TRANSLATE("Conic"), message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", PREVIEW_ENABLED_OPTION);
		message->AddInt32("value", 0x00000000);
		fPreview = new BCheckBox(B_TRANSLATE("Enable preview"), message);

		fGradientView = new GradientView(c1, c2);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", TOLERANCE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(TOLERANCE_OPTION));
		fTolerance
			= new NumberSliderControl(B_TRANSLATE("Tolerance:"), "0", message, 0, 100, false);
		fTolerance->SetValue(tool->GetCurrentValue(TOLERANCE_OPTION));

		BGridLayout* toleranceLayout = LayoutSliderGrid(fTolerance);

		layout->AddView(BGroupLayoutBuilder(B_VERTICAL, kWidgetSpacing)
			.Add(toleranceLayout)
			.AddStrut(kWidgetSpacing)
			.Add(SeparatorView(B_TRANSLATE("Mode")))
			.AddGroup(B_VERTICAL, kWidgetSpacing)
				.Add(fFloodFill)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.AddStrut(kWidgetSpacing)
			.Add(SeparatorView(B_TRANSLATE("Options")))
			.AddGroup(B_VERTICAL, kWidgetSpacing)
				.Add(fGradient)
				.AddGroup(B_VERTICAL, kWidgetSpacing)
					.Add(fLinearGradient)
					.Add(fRadialGradient)
					.Add(fSquareGradient)
					.Add(fConicGradient)
					.SetInsets(kWidgetInset * 2, 0.0, 0.0, 0.0)
				.End()
				.Add(fPreview)
				.Add(fGradientView)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.TopView()
		);

		fFloodFill->SetValue(tool->GetCurrentValue(MODE_OPTION));

		if (tool->GetCurrentValue(GRADIENT_ENABLED_OPTION) != B_CONTROL_OFF)
			fGradient->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(PREVIEW_ENABLED_OPTION) != B_CONTROL_OFF)
			fPreview->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(SHAPE_OPTION) == GRADIENT_CONIC)
			fConicGradient->SetValue(B_CONTROL_ON);
		else if (tool->GetCurrentValue(SHAPE_OPTION) == GRADIENT_RADIAL)
			fRadialGradient->SetValue(B_CONTROL_ON);
		else if (tool->GetCurrentValue(SHAPE_OPTION) == GRADIENT_SQUARE)
			fSquareGradient->SetValue(B_CONTROL_ON);
		else
			fLinearGradient->SetValue(B_CONTROL_ON);
	}
}


void
FillToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fFloodFill->SetTarget(this);
	fGradient->SetTarget(this);
	fPreview->SetTarget(this);
	fGradientView->SetTarget(this);
	fTolerance->SetTarget(this);
	fLinearGradient->SetTarget(this);
	fRadialGradient->SetTarget(this);
	fSquareGradient->SetTarget(this);
	fConicGradient->SetTarget(this);
}


void
FillToolConfigView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case GRADIENT_ADJUSTED:
		{
			uint32 color1;
			uint32 color2;
			message->FindInt32("color1", (int32*)&color1);
			message->FindInt32("color2", (int32*)&color2);
			(dynamic_cast<FillTool*>(Tool()))->SetGradient(color1, color2);
		} break;
		default:
			DrawingToolConfigView::MessageReceived(message);
	}
}
