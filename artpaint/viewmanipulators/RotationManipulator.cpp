/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "RotationManipulator.h"
#include "BitmapUtilities.h"
#include "HSPolygon.h"
#include "ImageView.h"
#include "MessageConstants.h"
#include "PixelOperations.h"
#include "Selection.h"
#include "UtilityClasses.h"


#include <Catalog.h>
#include <ClassInfo.h>
#include <LayoutBuilder.h>
#include <StatusBar.h>
#include <TextControl.h>
#include <Window.h>


#include <ctype.h>
#include <math.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Manipulators"


#define PI M_PI


RotationManipulator::RotationManipulator(BBitmap* bitmap)
	:
	WindowGUIManipulator(),
	selection(NULL),
	orig_selection_map(NULL),
	transform_selection_only(false)
{
	settings = new RotationManipulatorSettings();
	config_view = NULL;
	preview_bitmap = NULL;
	copy_of_the_preview_bitmap = NULL;

	SetPreviewBitmap(bitmap);

	last_calculated_resolution = 0;

	BPoint poly_points[8];
	poly_points[0] = BPoint(-5, 0);
	poly_points[1] = BPoint(0, 0);
	poly_points[2] = BPoint(0, -5);
	poly_points[3] = BPoint(0, 0);
	poly_points[4] = BPoint(5, 0);
	poly_points[5] = BPoint(0, 0);
	poly_points[6] = BPoint(0, 5);
	poly_points[7] = BPoint(0, 0);

	view_polygon = new HSPolygon(poly_points, 8);
}


RotationManipulator::~RotationManipulator()
{
	delete copy_of_the_preview_bitmap;
	delete view_polygon;

	if (config_view != NULL) {
		config_view->RemoveSelf();
		delete config_view;
	}

	if (orig_selection_map != NULL) {
		delete orig_selection_map;
		orig_selection_map = NULL;
	}
}


void
RotationManipulator::SetPreviewBitmap(BBitmap* bitmap)
{
	if (config_view != NULL)
		config_view->SetAngle(0.0);

	if ((bitmap == NULL) || (preview_bitmap == NULL)
		|| (bitmap->Bounds() != preview_bitmap->Bounds())) {
		try {
			if (preview_bitmap != NULL)
				delete copy_of_the_preview_bitmap;

			if (bitmap != NULL) {
				preview_bitmap = bitmap;
				copy_of_the_preview_bitmap = DuplicateBitmap(preview_bitmap);
				BRect bounds = preview_bitmap->Bounds();
				settings->origo = BPoint((bounds.right - bounds.left) / 2 + bounds.left,
					(bounds.bottom - bounds.top) / 2 + bounds.top);
			} else {
				preview_bitmap = NULL;
				copy_of_the_preview_bitmap = NULL;
			}
		}
		catch (std::bad_alloc e) {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
			throw e;
		}
	} else {
		// Just update the copy_of_the_preview_bitmap
		preview_bitmap = bitmap;
		uint32* source = (uint32*)preview_bitmap->Bits();
		uint32* target = (uint32*)copy_of_the_preview_bitmap->Bits();
		int32 bitslength
			= min_c(preview_bitmap->BitsLength(), copy_of_the_preview_bitmap->BitsLength());
		memcpy(target, source, bitslength);
	}

	if (preview_bitmap != NULL) {
		double speed = GetSystemClockSpeed() / 15000;

		BRect bounds = preview_bitmap->Bounds();
		float num_pixels = (bounds.Width() + 1) * (bounds.Height() + 1);
		lowest_available_quality = 1;
		while ((2 * num_pixels / lowest_available_quality / lowest_available_quality) > speed)
			lowest_available_quality *= 2;

		highest_available_quality = max_c(lowest_available_quality / 2, 1);
	} else {
		lowest_available_quality = 1;
		highest_available_quality = 1;
	}
	last_calculated_resolution = lowest_available_quality;
}


BRegion
RotationManipulator::Draw(BView* view, float mag_scale)
{
	float x = settings->origo.x;
	float y = settings->origo.y;

	view->StrokeLine(BPoint(mag_scale * x - 10, mag_scale * y),
		BPoint(mag_scale * x + 10, mag_scale * y), B_MIXED_COLORS);
	view->StrokeLine(BPoint(mag_scale * x, mag_scale * y - 10),
		BPoint(mag_scale * x, mag_scale * y + 10), B_MIXED_COLORS);
	view->StrokeEllipse(BRect(BPoint(mag_scale * x - 5, mag_scale * y - 5),
		BPoint(mag_scale * x + 5, mag_scale * y + 5)), B_MIXED_COLORS);

	BRegion updated_region;
	updated_region.Set(
		BRect(mag_scale * x - 10, mag_scale * y - 10, mag_scale * x + 10, mag_scale * y + 10));
	return updated_region;
}


void
RotationManipulator::MouseDown(BPoint point, uint32 buttons, BView*, bool first_click)
{
	if (first_click == TRUE) {
		if (!(buttons & B_PRIMARY_MOUSE_BUTTON)
			|| ((fabs(point.x - settings->origo.x) < 10)
				&& (fabs(point.y - settings->origo.y) < 10)))
			move_origo = TRUE;
		else
			move_origo = FALSE;
	}

	if (!move_origo) {
		// Here we calculate the new angle
		previous_angle = settings->angle;
		float new_angle;

		float dy = point.y - settings->origo.y;
		float dx = point.x - settings->origo.x;
		new_angle = atan2(dy, dx);
		new_angle = new_angle / PI * 180;
		if (first_click == TRUE) {
			if (modifiers() & B_SHIFT_KEY)
				new_angle = SnapToAngle(22.5, new_angle, 360.);

			starting_angle = new_angle;
		} else {
			float delta = settings->angle + new_angle - starting_angle;
			if (modifiers() & B_SHIFT_KEY) {
				delta = SnapToAngle(22.5, delta, 360.);
				new_angle = delta - settings->angle + starting_angle;
			}

			settings->angle += new_angle - starting_angle;

			starting_angle = new_angle;
			if ((config_view != NULL) && (new_angle != previous_angle))
				config_view->SetAngle(settings->angle);
		}
	} else {
		// Set the new origo for rotation and reset the angle.
		previous_angle = settings->angle;
		settings->angle = 0;

		settings->origo = point;
	}
}


BBitmap*
RotationManipulator::ManipulateBitmap(
	ManipulatorSettings* set, BBitmap* original, BStatusBar* status_bar)
{
	// Here move the contents of the original-bitmap to the new_bitmap,
	// rotated by s->angle around s->origin.
	RotationManipulatorSettings* new_settings = cast_as(set, RotationManipulatorSettings);

	if (transform_selection_only == true)
		return NULL;

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	if (new_settings->angle == 0)
		return NULL;

	BBitmap* new_bitmap;
	if (original != preview_bitmap) {
		original->Lock();
		BRect bitmap_frame = original->Bounds();
		new_bitmap = new BBitmap(bitmap_frame, B_RGB32);
		original->Unlock();
		if (new_bitmap->IsValid() == FALSE)
			throw std::bad_alloc();
	} else {
		new_bitmap = original;
		original = copy_of_the_preview_bitmap;
	}

	// We should calculate the new pixel value as weighted average from the
	// four pixels that will be under the inverse-rotated pixel.
	//
	//			X X			When inverse rotated from new bitmap,
	//			X X			the pixel will cover four pixels, each
	//						with a different weight.
	//
	BPoint center = new_settings->origo;

	float the_angle = new_settings->angle;
	float sin_angle = sin(-the_angle / 360 * 2 * PI);
	float cos_angle = cos(-the_angle / 360 * 2 * PI);

	int32* target_bits = (int32*)new_bitmap->Bits();
	int32* source_bits = (int32*)original->Bits();
	int32 source_bpr = original->BytesPerRow() / 4;
	int32 target_bpr = new_bitmap->BytesPerRow() / 4;

	// copy the points according to angle
	float height = new_bitmap->Bounds().Height();
	float width = new_bitmap->Bounds().Width();
	float left = original->Bounds().left;
	float right = original->Bounds().right;
	float top = original->Bounds().top;
	float bottom = original->Bounds().bottom;
	float center_x = center.x;
	float center_y = center.y;
	float source_x, source_y;
	float y_times_sin = (-center_y) * sin_angle;
	float y_times_cos = (-center_y) * cos_angle;

	BWindow* status_bar_window = status_bar->Window();

	float floor_x, ceil_x, floor_y, ceil_y; // was int32 before optimization

	uint32 p1, p2, p3, p4;

	union color_conversion background;

	// Transparent background.
	background.bytes[0] = 0xFF;
	background.bytes[1] = 0xFF;
	background.bytes[2] = 0xFF;
	background.bytes[3] = 0x00;

	if (selection->IsEmpty()) {
		for (float y = 0; y <= height; y++) {
			float x_times_sin = (-center_x) * sin_angle;
			float x_times_cos = (-center_x) * cos_angle;
			for (float x = 0; x <= width; x++) {
				// rotate here around the origin
				source_x = x_times_cos - y_times_sin;
				source_y = x_times_sin + y_times_cos;
				// translate back to correct position
				source_x += center_x;
				source_y += center_y;

				floor_x = floor(source_x);
				ceil_x = floor_x + 1;
				floor_y = floor(source_y);
				ceil_y = floor_y + 1;

				float u = source_x - floor_x;
				float v = source_y - floor_y;
				// Then add the weighted sums of the four pixels.
				if ((floor_x <= right) && (floor_y <= bottom) && (floor_x >= left)
					&& (floor_y >= top))
					p1 = *(source_bits + (int32)floor_x + (int32)floor_y * source_bpr);
				else
					p1 = background.word;

				if ((ceil_x <= right) && (floor_y <= bottom) && (ceil_x >= left)
					&& (floor_y >= top))
					p2 = *(source_bits + (int32)ceil_x + (int32)floor_y * source_bpr);
				else
					p2 = background.word;

				if ((floor_x <= right) && (ceil_y <= bottom) && (floor_x >= left)
					&& (ceil_y >= top))
					p3 = *(source_bits + (int32)floor_x + (int32)ceil_y * source_bpr);
				else
					p3 = background.word;

				if ((ceil_x <= right) && (ceil_y <= bottom) && (ceil_x >= left)
					&& (ceil_y >= top))
					p4 = *(source_bits + (int32)ceil_x + (int32)ceil_y * source_bpr);
				else
					p4 = background.word;

				*target_bits++ = bilinear_interpolation(p1, p2, p3, p4, u, v);
				x_times_sin += sin_angle;
				x_times_cos += cos_angle;
			}
			y_times_sin += sin_angle;
			y_times_cos += cos_angle;
			if ((((int32)y % 20) == 0) && (status_bar != NULL) && (status_bar_window != NULL)) {
				status_bar_window->Lock();
				status_bar->Update(100.0 / (float)height * 20);
				status_bar_window->Unlock();
			}
		}
	} else {
		if (orig_selection_map != NULL)
			selection->ReplaceSelection(orig_selection_map);

		// This should be done by first rotating the selection and then looking up what pixels
		// of original image correspond to the pixels in the rotated selection. The only problem
		// with this approach is if we want to clear the selection first so we must clear it before
		// rotating the selection.
		for (int32 y = 0; y <= height; y++) {
			for (int32 x = 0; x <= width; x++) {
				if (selection->ContainsPoint(x, y))
					*(target_bits + x + y * target_bpr) = background.word;
				else
					*(target_bits + x + y * target_bpr) = *(source_bits + x + y * source_bpr);
			}
		}

		// We must make a copy of the selection in order to be able to rotate it
		selection->RotateTo(center, the_angle);
		selection->Recalculate();

		BRect selection_bounds = selection->GetBoundingRect();
		int32 sel_top = (int32)selection_bounds.top;
		int32 sel_bottom = (int32)selection_bounds.bottom;
		int32 sel_left = (int32)selection_bounds.left;
		int32 sel_right = (int32)selection_bounds.right;
		y_times_sin = (sel_top - center_y) * sin_angle;
		y_times_cos = (sel_top - center_y) * cos_angle;
		for (float y = sel_top; y <= sel_bottom; y++) {
			float x_times_sin = (sel_left - center_x) * sin_angle;
			float x_times_cos = (sel_left - center_x) * cos_angle;
			for (float x = sel_left; x <= sel_right; x++) {
				if (selection->ContainsPoint(int32(x), int32(y))) {
					// rotate here around the origin
					source_x = x_times_cos - y_times_sin;
					source_y = x_times_sin + y_times_cos;
					// translate back to correct position
					source_x += center_x;
					source_y += center_y;

					floor_x = floor(source_x);
					ceil_x = floor_x + 1;
					floor_y = floor(source_y);
					ceil_y = floor_y + 1;

					float u = source_x - floor_x;
					float v = source_y - floor_y;
					// Then add the weighted sums of the four pixels.
					if ((floor_x <= right) && (floor_y <= bottom) && (floor_x >= left)
						&& (floor_y >= top)) {
						p1 = src_over_fixed(*(target_bits + (int32)x + (int32)y * target_bpr),
							*(source_bits + (int32)floor_x + (int32)floor_y * source_bpr));
					} else
						p1 = background.word;

					// second
					if ((ceil_x <= right) && (floor_y <= bottom) && (ceil_x >= left)
						&& (floor_y >= top)) {
						p2 = src_over_fixed(*(target_bits + (int32)x + (int32)y * target_bpr),
							*(source_bits + (int32)ceil_x + (int32)ceil_y * source_bpr));
					} else
						p2 = background.word;

					// third
					if ((floor_x <= right) && (ceil_y <= bottom) && (floor_x >= left)
						&& (ceil_y >= top)) {
						p3 = src_over_fixed(*(target_bits + (int32)x + (int32)y * target_bpr),
							*(source_bits + (int32)floor_x + (int32)ceil_y * source_bpr));
					} else
						p3 = background.word;

					// fourth
					if ((ceil_x <= right) && (ceil_y <= bottom) && (ceil_x >= left)
						&& (ceil_y >= top)) {
						p4 = src_over_fixed(*(target_bits + (int32)x + (int32)y * target_bpr),
							*(source_bits + (int32)ceil_x + (int32)ceil_y * source_bpr));
					} else
						p4 = background.word;

					*(target_bits + (int32)x + (int32)y * target_bpr)
						= bilinear_interpolation(p1, p2, p3, p4, u, v);
				}
				x_times_sin += sin_angle;
				x_times_cos += cos_angle;
			}
			y_times_sin += sin_angle;
			y_times_cos += cos_angle;
			if ((status_bar != NULL) && (status_bar->Window() != NULL)) {
				BMessage* a_message = new BMessage(B_UPDATE_STATUS_BAR);
				a_message->AddFloat("delta", 100.0 / (float)(sel_bottom - sel_top));
				status_bar->Window()->PostMessage(a_message, status_bar);
				delete a_message;
			}
		}
	}

	return new_bitmap;
}


int32
RotationManipulator::PreviewBitmap(bool full_quality, BRegion* updated_region)
{
	if (transform_selection_only == true && (selection == NULL || selection->IsEmpty() == true))
		return 0;

	// First decide the resolution of the bitmap
	if ((previous_angle == settings->angle) && (full_quality == FALSE)) {
		if (last_calculated_resolution <= highest_available_quality) {
			last_calculated_resolution = 0;
			if (previous_origo != settings->origo) {
				previous_origo = settings->origo;
				return DRAW_ONLY_GUI;
			} else
				return 0;
		} else
			last_calculated_resolution = last_calculated_resolution / 2;
	} else if (full_quality == TRUE)
		last_calculated_resolution = 1;
	else
		last_calculated_resolution = lowest_available_quality;

	union color_conversion background;
	// Transparent background.
	background.bytes[0] = 0xFF;
	background.bytes[1] = 0xFF;
	background.bytes[2] = 0xFF;
	background.bytes[3] = 0x00;

	// Then calculate the preview
	BPoint center = settings->origo;
	float the_angle = settings->angle;
	float sin_angle = sin(-the_angle / 360 * 2 * PI);
	float cos_angle = cos(-the_angle / 360 * 2 * PI);

	int32* target_bits = (int32*)preview_bitmap->Bits();
	int32* source_bits = (int32*)copy_of_the_preview_bitmap->Bits();
	int32 source_bpr = copy_of_the_preview_bitmap->BytesPerRow() / 4;
	int32 target_bpr = preview_bitmap->BytesPerRow() / 4;

	// copy the points according to angle
	float height = preview_bitmap->Bounds().Height();
	float width = preview_bitmap->Bounds().Width();
	float left = copy_of_the_preview_bitmap->Bounds().left;
	float right = copy_of_the_preview_bitmap->Bounds().right;
	float top = copy_of_the_preview_bitmap->Bounds().top;
	float bottom = copy_of_the_preview_bitmap->Bounds().bottom;
	float center_x = center.x;
	float center_y = center.y;
	float source_x, source_y;
	float y_times_sin = (-center_y) * sin_angle;
	float y_times_cos = (-center_y) * cos_angle;
	if (selection == NULL || selection->IsEmpty()) {
		for (int32 y = 0; y <= height; y += last_calculated_resolution) {
			float x_times_sin = (-center_x) * sin_angle;
			float x_times_cos = (-center_x) * cos_angle;
			for (int32 x = 0; x <= width; x += last_calculated_resolution) {
				// rotete here around the origin
				source_x = x_times_cos - y_times_sin;
				source_y = x_times_sin + y_times_cos;
				// translate back to correct position
				source_x += center_x;
				source_y += center_y;
				if ((source_x <= right) && (source_y <= bottom) && (source_x >= left)
					&& (source_y >= top)) {
					*(target_bits + x + y * target_bpr)
						= *(source_bits + (int32)source_x + (int32)source_y * source_bpr);
				} else
					*(target_bits + x + y * target_bpr) = background.word;

				x_times_sin += last_calculated_resolution * sin_angle;
				x_times_cos += last_calculated_resolution * cos_angle;
			}
			y_times_sin += last_calculated_resolution * sin_angle;
			y_times_cos += last_calculated_resolution * cos_angle;
		}
	} else {
		// Rotate the selection also
		selection->RotateTo(center, the_angle);

		if (transform_selection_only == false) {
			for (int32 y = 0; y <= height; y += last_calculated_resolution) {
				float x_times_sin = (-center_x) * sin_angle;
				float x_times_cos = (-center_x) * cos_angle;
				for (int32 x = 0; x <= width; x += last_calculated_resolution) {
					// rotete here around the origin
					source_x = x_times_cos - y_times_sin;
					source_y = x_times_sin + y_times_cos;
					// translate back to correct position
					source_x += center_x;
					source_y += center_y;
					if ((source_x <= right) && (source_y <= bottom) && (source_x >= left)
						&& (source_y >= top)
						&& selection->ContainsPoint(int32(source_x), int32(source_y))) {
						if (selection->ContainsPoint(x, y)) {
							*(target_bits + x + y * target_bpr) = src_over_fixed(background.word,
								*(source_bits + (int32)source_x + (int32)source_y * source_bpr));
						} else {
							*(target_bits + x + y * target_bpr) = src_over_fixed(
								*(source_bits + (int32)x + (int32)y * source_bpr),
								*(source_bits + (int32)source_x + (int32)source_y * source_bpr));
						}
					} else if (selection->ContainsPoint(BPoint(source_x, source_y)))
						*(target_bits + x + y * target_bpr) = background.word; // Transparent.
					else if (selection->ContainsPoint(x, y))
						*(target_bits + x + y * target_bpr) = background.word;
					else { // outside selection
						*(target_bits + x + y * target_bpr)
							= *(source_bits + (int32)x + (int32)y * source_bpr);
					}
					x_times_sin += last_calculated_resolution * sin_angle;
					x_times_cos += last_calculated_resolution * cos_angle;
				}
				y_times_sin += last_calculated_resolution * sin_angle;
				y_times_cos += last_calculated_resolution * cos_angle;
			}
		}
	}

	updated_region->Set(preview_bitmap->Bounds());

	return last_calculated_resolution;
}


void
RotationManipulator::Reset()
{
	selection->RotateTo(settings->origo, 0);
	settings->angle = 0;
	previous_angle = 0;

	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with loop.
		uint32* source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32* target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target, source, bits_length);
	}

	if (orig_selection_map != NULL && selection != NULL)
		selection->ReplaceSelection(orig_selection_map);
}


BView*
RotationManipulator::MakeConfigurationView(const BMessenger& target)
{
	config_view = new RotationManipulatorConfigurationView(this, target);
	config_view->SetAngle(settings->angle);
	return config_view;
}


void
RotationManipulator::SetAngle(float angle)
{
	previous_angle = settings->angle;
	settings->angle = angle;
}


const char*
RotationManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Rotate: Left-drag to rotate, "
			"right-click to set rotation center, "
			"SHIFT to snap angle.");
}


const char*
RotationManipulator::ReturnName()
{
	if (transform_selection_only == true)
		return B_TRANSLATE("Rotate selection");

	return B_TRANSLATE("Rotate");
}


void
RotationManipulator::SetSelection(Selection* new_selection)
{
	selection = new_selection;
	if (selection != NULL && selection->IsEmpty() == false) {
		orig_selection_map = new BBitmap(selection->ReturnSelectionMap());

		BRect bounds = selection->GetBoundingRect();

		settings->origo.x = (bounds.right + bounds.left) / 2;
		settings->origo.y = (bounds.bottom + bounds.top) / 2;
	}
}


void
RotationManipulator::UpdateSettings()
{
	if (config_view) {
		float angle = 0;
		config_view->GetControlValues(angle);
		SetAngle(angle);
	}
}


// #pragma mark -- RotationManipulatorConfigurationView


RotationManipulatorConfigurationView::RotationManipulatorConfigurationView(
	RotationManipulator* manipulator, const BMessenger& target)
	:
	WindowGUIManipulatorView(),
	fTarget(target),
	fManipulator(manipulator)
{
	fTextControl = new BTextControl("rotation", B_TRANSLATE("Angle:"), "9999.9˚",
		new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED));
	BRect bounds = fTextControl->TextView()->Bounds();
	fTextControl->TextView()->SetExplicitMaxSize(BSize(StringWidth("-999.99'"), bounds.Height()));

	SetLayout(BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fTextControl));

	RotationManipulatorSettings* settings
		= (RotationManipulatorSettings*)manipulator->ReturnSettings();
	char text[256];
	sprintf(text, "%.1f˚", settings->angle);
	fTextControl->TextView()->SetText(text);

	delete settings;
}


void
RotationManipulatorConfigurationView::AttachedToWindow()
{
	fTextControl->SetTarget(this);

	if (BView* parent = Parent()) {
		SetLowColor(parent->LowColor());
		SetViewColor(parent->ViewColor());
	}

	fTextControl->MakeFocus(true);

	WindowGUIManipulatorView::AttachedToWindow();
}


void
RotationManipulatorConfigurationView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case HS_MANIPULATOR_ADJUSTING_FINISHED:
		{
			BTextView* text_view = fTextControl->TextView();
			char float_text[256];
			const char* text = text_view->Text();
			float angle = 0;
			int decimal_place = 0;
			float sign = 1;

			for (uint32 i = 0; i < strlen(text); i++) {
				if (isdigit(text[i]) != 0) {
					float new_number = text[i] - '0';
					if (decimal_place <= 0)
						angle = 10 * angle + new_number;
					else {
						angle = angle + new_number / pow(10, decimal_place);
						decimal_place++;
					}
				} else if (((text[i] == '.') || (text[i] == ',')) && (decimal_place <= 0))
					decimal_place = 1;
				else if ((decimal_place <= 0) && (angle == 0) && (text[i] == '-'))
					sign = -1;
			}

			angle = sign * angle;

			while (angle > 360)
				angle -= 360;
			if (angle > 180)
				angle -= 360;

			while (angle < -360)
				angle += 360;
			if (angle < -180)
				angle += 360;

			sprintf(float_text, "%.1f˚", angle);
			text_view->SetText(float_text);
			fManipulator->SetAngle(angle);
			if (fTarget.IsValid())
				fTarget.SendMessage(message);
		} break;
		default:
			WindowGUIManipulatorView::MessageReceived(message);
	}
}


void
RotationManipulatorConfigurationView::SetAngle(float angle)
{
	BWindow* window = Window();
	if (window && window->Lock()) {
		char text[256];
		sprintf(text, "%.1f˚", angle);
		fTextControl->TextView()->SetText(text);
		window->Unlock();
	}
}


void
RotationManipulatorConfigurationView::SetTarget(const BMessenger& target)
{
	fTarget = target;
}


void
RotationManipulatorConfigurationView::GetControlValues(float& angle)
{
	BTextView* text_view = fTextControl->TextView();
	char float_text[256];
	const char* text = text_view->Text();
	int decimal_place = 0;
	float sign = 1;

	for (uint32 i = 0; i < strlen(text); i++) {
		if (isdigit(text[i]) != 0) {
			float new_number = text[i] - '0';
			if (decimal_place <= 0)
				angle = 10 * angle + new_number;
			else {
				angle = angle + new_number / pow(10, decimal_place);
				decimal_place++;
			}
		} else if (((text[i] == '.') || (text[i] == ',')) && (decimal_place <= 0))
			decimal_place = 1;
		else if ((decimal_place <= 0) && (angle == 0) && (text[i] == '-'))
			sign = -1;
	}

	angle = sign * angle;
}
