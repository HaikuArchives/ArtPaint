/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "ScaleManipulator.h"

#include "MessageConstants.h"
#include "NumberControl.h"
#include "PixelOperations.h"
#include "Selection.h"


#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <GridLayout.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <PopUpMenu.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>
#include <Window.h>


#include <new>
#include <string.h>

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Manipulators"


using ArtPaint::Interface::NumberControl;


ScaleManipulator::ScaleManipulator(BBitmap* bm)
	:
	WindowGUIManipulator(),
	selection(NULL),
	orig_selection_map(NULL),
	transform_selection_only(false)
{
	configuration_view = NULL;
	settings = new ScaleManipulatorSettings();

	preview_bitmap = NULL;
	copy_of_the_preview_bitmap = NULL;

	SetPreviewBitmap(bm);
	reject_mouse_input = false;
	previous_point.x = 0;
	previous_point.y = 0;

	previous_left = -1;
	previous_top = -1;
	previous_bottom = -1;
	previous_right = -1;

	method = BILINEAR;
}


ScaleManipulator::~ScaleManipulator()
{
	if (configuration_view != NULL) {
		configuration_view->RemoveSelf();
		delete configuration_view;
	}

	if (orig_selection_map != NULL)
		delete orig_selection_map;

	delete copy_of_the_preview_bitmap;
}


BBitmap*
ScaleManipulator::ManipulateBitmap(
	ManipulatorSettings* set, BBitmap* original, BStatusBar* status_bar)
{
	ScaleManipulatorSettings* new_settings = dynamic_cast<ScaleManipulatorSettings*>(set);
	if (new_settings == NULL)
		return NULL;

	if (transform_selection_only == true)
		return NULL;

	BBitmap* final_bitmap = NULL;

	BRect bounds = original->Bounds();
	BRect orig_bounds = original->Bounds();

	if (bounds.IsValid() == FALSE)
		return NULL;

	union color_conversion background;
	// Transparent background.
	background.bytes[0] = 0xFF;
	background.bytes[1] = 0xFF;
	background.bytes[2] = 0xFF;
	background.bytes[3] = 0x00;

	if (original == preview_bitmap) {
		final_bitmap = original;
		original = copy_of_the_preview_bitmap;
	}

	uint32* source_bits;
	uint32* target_bits;

	if (selection != NULL && selection->IsEmpty() == FALSE) {
		if (orig_selection_map != NULL) {
			BBitmap* new_selection_map = ManipulateSelectionMap(settings);

			// copy non-selected data to result bitmap

			final_bitmap
				= new BBitmap(BRect(0, 0, orig_bounds.Width(), orig_bounds.Height()), B_RGBA32);
			target_bits = (uint32*)final_bitmap->Bits();
			uint32 target_bpr = final_bitmap->BytesPerRow() / 4;
			source_bits = (uint32*)original->Bits();
			uint32 source_bpr = original->BytesPerRow() / 4;
			for (int32 y = 0; y <= orig_bounds.bottom; y++) {
				for (int32 x = 0; x < orig_bounds.right; x++) {
					if (selection->ContainsPoint(x, y) == false)
						*(target_bits + x + y * target_bpr) = *(source_bits + x + y * source_bpr);
					else
						*(target_bits + x + y * target_bpr) = background.word;
				}
			}

			// set the "original" bounds to the size of the
			// selection before scaling
			orig_bounds = selection->GetBoundingRect();

			// and reset selection back to the new selection
			selection->ReplaceSelection(new_selection_map);

			delete new_selection_map;
		}

		bounds = selection->GetBoundingRect();
	}

	if (bounds.IsValid() == FALSE || orig_bounds.IsValid() == FALSE)
		return NULL;

	float starting_width = bounds.Width() + 1;
	float starting_height = bounds.Height() + 1;

	if (orig_bounds != bounds) {
		starting_width = orig_bounds.Width() + 1;
		starting_height = orig_bounds.Height() + 1;
	}

	float new_width = new_settings->right - new_settings->left + 1;
	float new_height = new_settings->bottom - new_settings->top + 1;

	// Create a new bitmap here and copy it applying scaling.
	// But first create an intermediate bitmap for scaling in one direction only.
	// Remember that the returned bitmap must accept views
	// First scale the width.
	// If the new size is the same as old return the original
	if (new_width == starting_width && new_height == starting_height
		&& new_settings->left == original_left && new_settings->top == original_top)
		return NULL;

	if (new_width == starting_width && new_height == starting_height) {
		if (final_bitmap != NULL)
			delete final_bitmap;
		final_bitmap = new BBitmap(preview_bitmap, B_RGBA32);
		return final_bitmap;
	}

	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta", 0.0);

	BBitmap* scale_x_bitmap = NULL;
	BBitmap* scale_y_bitmap = NULL;

	if (new_width != starting_width) {
		float bitmapWidth = new_width;
		float bitmapHeight = starting_height;
		scale_x_bitmap = new BBitmap(BRect(0, 0, bitmapWidth, bitmapHeight), B_RGBA32);
		if (scale_x_bitmap->IsValid() == FALSE)
			throw std::bad_alloc();

		target_bits = (uint32*)scale_x_bitmap->Bits();
		int32 target_bpr = scale_x_bitmap->BytesPerRow() / 4;
		source_bits = (uint32*)original->Bits();
		int32 source_bpr = original->BytesPerRow() / 4;
		float diff = starting_width / new_width;
		float accumulation = 0;
		int32 bottom = (int32)bounds.bottom;

		for (int32 y = 0; y < starting_height; y++)
			for (int32 x = 0; x < target_bpr; x++)
				*(target_bits + x + y * target_bpr) = background.word;

		if (diff != 1) {
			ScaleUtilities::ScaleHorizontally(target_bpr, starting_height,
				BPoint(original_left, original_top), original, scale_x_bitmap, diff, method);
		} else {
			for (int32 y = 0; y <= starting_height; y++) {
				uint32* src_bits = source_bits + y * source_bpr;

				for (int32 x = 0; x <= target_bpr; x++) {
					// Just copy it straight
					*(target_bits + x + y * target_bpr) = *(src_bits + x);
				}
				if ((y % 10 == 0) && (status_bar != NULL)) {
					progress_message.ReplaceFloat("delta", (100.0) / bottom * 10.0 / 2);
					status_bar->Window()->PostMessage(&progress_message, status_bar);
				}
			}
		}
	} else {
		scale_x_bitmap = original;
	}

	if (new_height != starting_height) {
		float bitmapWidth = new_width;
		float bitmapHeight = new_height;

		scale_y_bitmap = new BBitmap(BRect(0, 0, bitmapWidth, bitmapHeight), B_RGBA32);
		if (scale_y_bitmap->IsValid() == FALSE)
			throw std::bad_alloc();

		target_bits = (uint32*)scale_y_bitmap->Bits();
		int32 target_bpr = scale_y_bitmap->BytesPerRow() / 4;
		source_bits = (uint32*)scale_x_bitmap->Bits();

		for (int32 y = 0; y < new_height; y++)
			for (int32 x = 0; x < new_width; x++)
				*(target_bits + x + y * target_bpr) = background.word;

		int32 top = (int32)bounds.top;
		int32 left = (int32)bounds.left;
		int32 bottom = (int32)scale_y_bitmap->Bounds().bottom;

		if (scale_x_bitmap != original) {
			left = 0;
			top = 0;
		}

		float diff = (starting_height - 1) / new_height;
		if (diff != 1) {
			ScaleUtilities::ScaleVertically(new_width, new_height, BPoint(left, top),
				scale_x_bitmap, scale_y_bitmap, diff, method);
		} else {
			for (int32 y = 0; y < bottom; y++) {
				for (int32 x = 0; x <= target_bpr; x++) {
					// Just copy it straight
					*target_bits++ = *source_bits++;
				}
				if ((y % 10 == 0) && (status_bar != NULL)) {
					progress_message.ReplaceFloat("delta", (100.0) / bottom * 10.0 / 2);
					status_bar->Window()->PostMessage(&progress_message, status_bar);
				}
			}
		}

		if (scale_x_bitmap != original)
			delete scale_x_bitmap;
	}

	source_bits = NULL;
	uint32 source_bpr;
	BRect final_bounds(orig_bounds);
	BRect source_bounds;

	if (scale_y_bitmap != NULL) {
		source_bits = (uint32*)scale_y_bitmap->Bits();
		source_bpr = scale_y_bitmap->BytesPerRow() / 4;
		source_bounds = scale_y_bitmap->Bounds();
	} else if (scale_x_bitmap != NULL) {
		source_bits = (uint32*)scale_x_bitmap->Bits();
		source_bpr = scale_x_bitmap->BytesPerRow() / 4;
		source_bounds = scale_x_bitmap->Bounds();
	}

	if (final_bitmap == NULL && final_bounds.IsValid()) {
		final_bitmap = new BBitmap(final_bounds, B_RGBA32);

		target_bits = (uint32*)final_bitmap->Bits();
		uint32 target_bpr = final_bitmap->BytesPerRow() / 4;

		for (int32 y = 0; y < final_bitmap->Bounds().Height(); y++)
			for (int32 x = 0; x < final_bitmap->Bounds().Width(); x++)
				*(target_bits + x + y * target_bpr) = background.word;
	}

	if (final_bitmap == preview_bitmap) {
		target_bits = (uint32*)final_bitmap->Bits();
		uint32 target_bpr = final_bitmap->BytesPerRow() / 4;

		for (int32 y = 0; y < final_bitmap->Bounds().Height(); y++)
			for (int32 x = 0; x < final_bitmap->Bounds().Width(); x++)
				*(target_bits + x + y * target_bpr) = background.word;
	}

	if (final_bitmap == NULL)
		return NULL;

	target_bits = (uint32*)final_bitmap->Bits();
	uint32 target_bpr = final_bitmap->BytesPerRow() / 4;

	final_bounds.top = settings->top;
	final_bounds.bottom = settings->bottom;
	final_bounds.left = settings->left;
	final_bounds.right = settings->right;

	if (source_bits != NULL) {
		for (int32 y = final_bounds.top; y <= final_bounds.bottom; y++) {
			int32 src_y = y - final_bounds.top;

			if (src_y >= source_bounds.bottom)
				break;

			if (y < 0)
				continue;

			for (int32 x = final_bounds.left; x <= final_bounds.right; x++) {
				if (selection == NULL || selection->IsEmpty() == TRUE
					|| selection->ContainsPoint(x, y)) {
					int32 src_x = x - final_bounds.left;
					if (src_x >= source_bpr)
						break;
					if (x < 0)
						continue;

					*(target_bits + x + y * target_bpr)
						= src_over_fixed(*(target_bits + x + y * target_bpr),
							*(source_bits + src_x + src_y * source_bpr));
				}
			}
		}
	}

	return final_bitmap;
}


BBitmap*
ScaleManipulator::ManipulateSelectionMap(ManipulatorSettings* set)
{
	ScaleManipulatorSettings* new_settings = dynamic_cast<ScaleManipulatorSettings*>(set);
	if (new_settings == NULL)
		return NULL;

	BBitmap* final_bitmap = NULL;

	BRect bitmap_frame;
	if (orig_selection_map != NULL) {
		selection->ReplaceSelection(orig_selection_map);
		orig_selection_map->Lock();
		bitmap_frame = orig_selection_map->Bounds();
		orig_selection_map->Unlock();
	}

	BBitmap* selection_map = new BBitmap(selection->ReturnSelectionMap());

	final_bitmap = new BBitmap(bitmap_frame, B_GRAY8);

	uint8* source_bits;
	uint8* target_bits;

	uint8 background = 0x00;

	target_bits = (uint8*)final_bitmap->Bits();
	uint32 target_bpr = final_bitmap->BytesPerRow();

	for (int32 y = 0; y <= final_bitmap->Bounds().Height(); y++)
		for (int32 x = 0; x <= final_bitmap->Bounds().Width(); x++)
			*(target_bits + x + y * target_bpr) = background;

	BRect bounds = selection_map->Bounds();
	BRect orig_bounds = selection_map->Bounds();

	orig_bounds = selection->GetBoundingRect();

	if (bounds.IsValid() == FALSE || orig_bounds.IsValid() == FALSE)
		return NULL;

	float starting_width = orig_bounds.Width();
	float starting_height = orig_bounds.Height();

	float new_width = new_settings->right - new_settings->left + 1;
	float new_height = new_settings->bottom - new_settings->top + 1;

	// Create a new bitmap here and copy it applying scaling.
	// But first create an intermediate bitmap for scaling in one direction only.
	// Remember that the returned bitmap must accept views
	// First scale the width.
	// If the new size is the same as old return the selection_map
	if (new_width == starting_width && new_height == starting_height
		&& new_settings->left == original_left && new_settings->top == original_top)
		return NULL;

	if (new_width == starting_width && new_height == starting_height) {
		if (final_bitmap != NULL)
			delete final_bitmap;
		final_bitmap = new BBitmap(selection_map, B_GRAY8);
		return final_bitmap;
	}

	BBitmap* scale_x_bitmap = NULL;
	BBitmap* scale_y_bitmap = NULL;

	BRect scale_x_frame(bitmap_frame);

	if (new_width != starting_width) {
		scale_x_frame.right = max_c(scale_x_frame.right, new_width);

		scale_x_bitmap = new BBitmap(scale_x_frame, B_GRAY8);
		if (scale_x_bitmap->IsValid() == FALSE)
			throw std::bad_alloc();

		target_bits = (uint8*)scale_x_bitmap->Bits();
		int32 target_bpr = scale_x_bitmap->BytesPerRow();
		source_bits = (uint8*)selection_map->Bits();
		int32 source_bpr = selection_map->BytesPerRow();
		float diff = starting_width / new_width;

		for (int32 y = 0; y < starting_height; y++)
			for (int32 x = 0; x < target_bpr; x++)
				*(target_bits + x + y * target_bpr) = background;

		if (diff != 1) {
			ScaleUtilities::ScaleHorizontallyGray(new_width, starting_height,
				BPoint(original_left, original_top), selection_map, scale_x_bitmap, diff);
		} else {
			for (int32 y = 0; y <= selection_map->Bounds().Height(); y++) {
				for (int32 x = 0; x <= selection_map->Bounds().Width(); x++) {
					// Just copy it straight
					*(target_bits + x + y * target_bpr) = *(source_bits + x + y * source_bpr);
				}
			}
		}
	} else {
		scale_x_bitmap = selection_map;
	}

	if (new_height != starting_height) {
		BRect scale_y_frame(scale_x_frame);

		scale_y_frame.bottom = max_c(scale_y_frame.bottom, new_height);
		scale_y_bitmap = new BBitmap(scale_y_frame, B_GRAY8);
		if (scale_y_bitmap->IsValid() == FALSE)
			throw std::bad_alloc();

		target_bits = (uint8*)scale_y_bitmap->Bits();
		int32 target_bpr = scale_y_bitmap->BytesPerRow();
		source_bits = (uint8*)scale_x_bitmap->Bits();
		int32 source_bpr = scale_x_bitmap->BytesPerRow();

		int32 max_height = min_c(new_height, scale_y_bitmap->Bounds().Height());
		int32 max_width = min_c(new_width, target_bpr);
		for (int32 y = 0; y < max_height; y++)
			for (int32 x = 0; x < max_width; x++)
				*(target_bits + x + y * target_bpr) = background;

		int32 top = (int32)bounds.top;
		int32 left = (int32)bounds.left;

		if (scale_x_bitmap != selection_map) {
			left = 0;
			top = 0;
		}

		float diff = (starting_height - 1) / new_height;

		if (diff != 1) {
			ScaleUtilities::ScaleVerticallyGray(new_width, new_height, BPoint(left, top),
				scale_x_bitmap, scale_y_bitmap, diff);
		} else {
			for (int32 y = 0; y < scale_x_bitmap->Bounds().Height(); y++) {
				for (int32 x = 0; x <= scale_x_bitmap->Bounds().Width(); x++) {
					// Just copy it straight
					*(target_bits + x + y * target_bpr) = *(source_bits + x + y * source_bpr);
				}
			}
		}

		if (scale_x_bitmap != selection_map) {
			delete scale_x_bitmap;
			scale_x_bitmap = NULL;
		}
	}

	source_bits = NULL;
	uint32 source_bpr;
	BRect final_bounds(selection_map->Bounds());
	BRect source_bounds;

	if (scale_y_bitmap != NULL) {
		source_bits = (uint8*)scale_y_bitmap->Bits();
		source_bpr = scale_y_bitmap->BytesPerRow();
		source_bounds = scale_y_bitmap->Bounds();
	} else if (scale_x_bitmap != NULL) {
		source_bits = (uint8*)scale_x_bitmap->Bits();
		source_bpr = scale_x_bitmap->BytesPerRow();
		source_bounds = scale_x_bitmap->Bounds();
	}

	if (final_bitmap == NULL)
		return NULL;

	target_bits = (uint8*)final_bitmap->Bits();
	target_bpr = final_bitmap->BytesPerRow();

	final_bounds.top = settings->top;
	final_bounds.bottom = min_c(settings->bottom, final_bitmap->Bounds().bottom);
	final_bounds.left = settings->left;
	final_bounds.right = min_c(settings->right, final_bitmap->Bounds().right);

	if (source_bits != NULL) {
		for (int32 y = final_bounds.top; y <= final_bounds.bottom; y++) {
			int32 src_y = y - final_bounds.top;

			if (src_y > source_bounds.bottom)
				break;

			if (y < 0)
				continue;

			for (int32 x = final_bounds.left; x <= final_bounds.right; x++) {
				int32 src_x = x - final_bounds.left;
				if (src_x > source_bpr)
					break;
				if (x < 0)
					continue;

				*(target_bits + x + y * target_bpr)
					= *(source_bits + src_x + src_y * source_bpr);
			}
		}
	}

	delete selection_map;

	return final_bitmap;
}


int32
ScaleManipulator::PreviewBitmap(bool, BRegion* region)
{
	if (preview_bitmap == NULL)
		return 0;

	if (transform_selection_only == true && (selection == NULL || selection->IsEmpty() == true))
		return 0;

	union color_conversion white;
	white.bytes[0] = 0xFF;
	white.bytes[1] = 0xFF;
	white.bytes[2] = 0xFF;
	white.bytes[3] = 0x00;

	preview_bitmap->Lock();
	// Here do a DDA-scaling from copy_of_the_preview_bitmap to preview_bitmap.
	uint32 width = preview_bitmap->Bounds().IntegerWidth();
	uint32 height = preview_bitmap->Bounds().IntegerHeight();
	uint32 source_width = copy_of_the_preview_bitmap->Bounds().IntegerWidth();
	uint32 source_height = copy_of_the_preview_bitmap->Bounds().IntegerHeight();

	if (width == 0 || height == 0)
		return 0;

	uint32* source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
	uint32* target_bits = (uint32*)preview_bitmap->Bits();
	int32 bpr = preview_bitmap->BytesPerRow() / 4;

	float new_width = settings->right - settings->left;
	float new_height = settings->bottom - settings->top;
	float old_width = original_right - original_left;
	float old_height = original_bottom - original_top;
	float width_coeff = old_width / new_width;
	float height_coeff = old_height / new_height;

	BRect selection_bounds;
	BRect updated_rect;

	if (selection != NULL) {
		selection_bounds = selection->GetBoundingRect();
		selection_bounds = selection_bounds & copy_of_the_preview_bitmap->Bounds();
	}

	if (selection == NULL || selection->IsEmpty() == true) {
		updated_rect = preview_bitmap->Bounds();

		int32 preview_width = width;
		int32 preview_height = height;
		if (new_width > width)
			preview_width = new_width;
		if (new_height > height)
			preview_height = new_height;

		int32* source_x_table = new int32[preview_width + 1];

		for (int32 i = 0; i <= preview_width; i++)
			source_x_table[i] = (int32)floor(i * width_coeff);

		for (int32 y = 0; y <= height; y++)
			for (int32 x = 0; x <= width; x++)
				*(target_bits + x + y * bpr) = white.word;

		for (int32 y = 0; y <= preview_height; y++) {
			int32 adj_y = y + settings->top;
			int32 source_y = (int32)floor(y * height_coeff);
			int32 y_adj_times_bpr = adj_y * bpr;
			int32 source_y_times_bpr = source_y * bpr;
			if (adj_y < 0)
				continue;
			if (adj_y > height)
				break;
			if (source_y > source_height)
				break;
			for (int32 x = 0; x <= preview_width; x++) {
				int32 adj_x = x + settings->left;
				if (adj_x < 0)
					continue;
				if (adj_x > width)
					break;
				int32 source_x = source_x_table[x];
				if (source_x > source_width)
					break;
				*(target_bits + adj_x + y_adj_times_bpr)
					= *(source_bits + source_x + source_y_times_bpr);
			}
		}

		delete[] source_x_table;
	} else {
		float new_x = new_width;
		float new_y = new_height;

		if (reject_mouse_input == true)
			return 1;

		if (previous_left < 0 && previous_top < 0
			&& settings->left > 0 && settings->top > 0) {
				previous_left = settings->left;
				previous_top = settings->top;
				previous_right = settings->right;
				previous_bottom = settings->bottom;
			}

		int32 sel_top = (int32)min_c(previous_top, settings->top);
		int32 sel_left = (int32)min_c(previous_left, settings->left);
		int32 sel_bottom = (int32)max_c(previous_bottom, settings->bottom);
		int32 sel_right = (int32)max_c(previous_right, settings->right);
		sel_top = (int32)max_c(0, sel_top);
		sel_left = (int32)max_c(0, sel_left);
		sel_bottom = (int32)min_c(preview_bitmap->Bounds().bottom, sel_bottom);
		sel_right = (int32)min_c(preview_bitmap->Bounds().right, sel_right);

		updated_rect.Set(sel_left, sel_top, sel_right, sel_bottom);
		updated_rect.InsetBy(-10, -10);

		// this code redraws the part of the image where the selection was moved
		if (transform_selection_only == false) {
			for (int32 y = sel_top; y <= sel_bottom; ++y) {
				for (int32 x = sel_left; x <= sel_right; ++x) {
					uint32 clear_bits = *(source_bits + x + y * bpr);
					if (selection->ContainsPoint(x, y))
						clear_bits = white.word;

					*(target_bits + x + y * bpr) = clear_bits;
				}
			}
		}

		selection->Translate(settings->left - previous_left,
			settings->top - previous_top);

		selection->ScaleTo(BPoint(settings->left, settings->top), new_width, new_height);
		selection_bounds = selection->GetBoundingRect();
		selection_bounds.OffsetBy(settings->left, settings->top);

		copy_of_the_preview_bitmap->Lock();
		selection_bounds = selection_bounds & copy_of_the_preview_bitmap->Bounds();
		copy_of_the_preview_bitmap->Unlock();

		if (transform_selection_only == false) {
			sel_top = (int32)settings->top;
			sel_bottom = min_c((int32)settings->bottom, preview_bitmap->Bounds().bottom);
			sel_left = (int32)settings->left;
			sel_right = min_c((int32)settings->right, preview_bitmap->Bounds().right);

			if (sel_right > 0 && sel_bottom > 0
				&& sel_left < preview_bitmap->Bounds().right
				&& sel_top < preview_bitmap->Bounds().bottom)  {
				for (int32 y = sel_top; y <= sel_bottom; y++) {
					if (y < 0)
						continue;

					int32 source_y = (int32)floor((y - sel_top) * height_coeff) + original_top;
					int32 y_times_bpr = y * bpr;
					int32 source_y_times_bpr = source_y * bpr;
					for (int32 x = sel_left; x <= sel_right; x++) {
						if (x < 0)
							continue;

						int32 source_x
							= (int32)floor((x - sel_left) * width_coeff) +
								original_left;

						if (selection->ContainsPoint(source_x, source_y)) {
							if (source_x < width && source_y < height &&
								source_x >= 0 && source_y >= 0) {
								*(target_bits + x + y_times_bpr)
									= src_over_fixed(*(target_bits + x + y_times_bpr),
										*(source_bits + source_x + source_y_times_bpr));
							}
						}
					}
				}
			}
		}
	}

	previous_left = settings->left;
	previous_top = settings->top;
	previous_right = settings->right;
	previous_bottom = settings->bottom;

	region->Set(updated_rect);
	preview_bitmap->Unlock();

	return 1;
}


BBitmap*
ScaleManipulator::PreviewSelectionMap(float width_coeff, float height_coeff)
{
	BBitmap* current_selection_map = selection->ReturnSelectionMap();

	BRect selection_frame(settings->left, settings->top, settings->right, settings->bottom);
	int32 width = selection_frame.IntegerWidth();
	int32 height = selection_frame.IntegerHeight();

	BRect selection_map_frame = BRect(preview_bitmap->Bounds());
	selection_map_frame.right = max_c(selection_map_frame.right, width);
	selection_map_frame.bottom = max_c(selection_map_frame.bottom, height);

	BBitmap* selection_map = new BBitmap(selection_map_frame, B_GRAY8);

	uint8* source_bits = (uint8*)current_selection_map->Bits();
	uint8* target_bits = (uint8*)selection_map->Bits();
	int32 bpr = selection_map->BytesPerRow();
	int32 source_bpr = current_selection_map->BytesPerRow();

	for (int32 y = 0; y <= selection_map->Bounds().IntegerHeight(); y++)
			for (int32 x = 0; x <= selection_map->Bounds().IntegerWidth(); x++)
				*(target_bits + x + y * bpr) = 0;

	int32 sel_top = (int32)settings->top;
	int32 sel_bottom = min_c((int32)selection_frame.bottom, preview_bitmap->Bounds().bottom);
	int32 sel_left = (int32)settings->left;
	int32 sel_right = min_c((int32)selection_frame.right, preview_bitmap->Bounds().right);

	for (int32 y = sel_top; y <= sel_bottom; y++) {
		int32 source_y = (int32)floor((y - sel_top) * height_coeff) + original_top;
		int32 y_times_bpr = y * bpr;
		int32 source_y_times_bpr = source_y * source_bpr;

		if (y < 0)
			continue;

		if (y > preview_bitmap->Bounds().bottom)
			break;

		for (int32 x = sel_left; x <= sel_right; x++) {
			if (x < 0)
				continue;

			if (x > preview_bitmap->Bounds().right)
				break;

			int32 source_x
				= (int32)floor((x - sel_left) * width_coeff) + original_left;
			if (source_x > 0 && source_y > 0
				&& source_x < current_selection_map->Bounds().right
				&& source_y < current_selection_map->Bounds().bottom) {
				*(target_bits + x + y_times_bpr)
					= *(source_bits + source_x + source_y_times_bpr);
			}
		}
	}

	return selection_map;
}


void
ScaleManipulator::MouseDown(BPoint point, uint32 buttons, BView* image_view, bool first_click)
{
	BPoint normPoint(point);

	if (reject_mouse_input == true) {
		if (normPoint.x > 0 && normPoint.y > 0 && normPoint.x >= previous_point.x
			&& normPoint.y >= previous_point.y)
			return;

		if (normPoint.x < 0 && normPoint.y < 0 && normPoint.x <= previous_point.x
			&& normPoint.y <= previous_point.y)
			return;

		if ((normPoint.x < settings->left && move_right == TRUE)
			|| (normPoint.y > settings->bottom && move_top == TRUE)
			|| (normPoint.x > settings->right && move_left == TRUE)
			|| (normPoint.y < settings->top && move_bottom == TRUE))
			return;

		reject_mouse_input = false;
	}

	bool lock_aspect = configuration_view->MaintainProportions();

	previous_left = settings->left;
	previous_right = settings->right;
	previous_top = settings->top;
	previous_bottom = settings->bottom;

	point.x = floor(point.x);
	point.y = floor(point.y);

	float aspect_ratio = (previous_right - previous_left) / (previous_bottom - previous_top);

	ScaleUtilities::MoveGrabbers(point, previous_point, settings->left, settings->top,
		settings->right, settings->bottom, aspect_ratio, move_left, move_top, move_right,
		move_bottom, move_all, first_click, lock_aspect);

	if ((previous_left != settings->left) || (previous_right != settings->right)
		|| (previous_top != settings->top) || (previous_bottom != settings->bottom)) {
		if (configuration_view != NULL)
			configuration_view->SetValues(settings->left, settings->top,
				settings->right - settings->left, settings->bottom - settings->top);
	}
}


void
ScaleManipulator::SetValues(float left, float top, float right, float bottom)
{
	settings->left = left;
	settings->top = top;
	settings->bottom = bottom;
	settings->right = right;
}


void
ScaleManipulator::Reset()
{
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with a loop.
		uint32* source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32* target = (uint32*)preview_bitmap->Bits();
		if (source != NULL)
			memcpy(target, source, preview_bitmap->BitsLength());
	}

	if (orig_selection_map != NULL && selection != NULL)
		selection->ReplaceSelection(orig_selection_map);

	SetValues(original_left, original_top, original_right, original_bottom);
	if (configuration_view != NULL)
		configuration_view->SetValues(original_left, original_top, original_right - original_left,
			original_bottom - original_top);
}


void
ScaleManipulator::SetPreviewBitmap(BBitmap* bitmap)
{
	if (bitmap) {
		original_left = bitmap->Bounds().left;
		original_top = bitmap->Bounds().top;
		original_right = bitmap->Bounds().right;
		original_bottom = bitmap->Bounds().bottom;

		SetValues(original_left, original_top, original_right, original_bottom);

		if (configuration_view)
			configuration_view->SetValues(original_left, original_top,
				original_right - original_left, original_bottom - original_top);
	}

	if (!bitmap || !preview_bitmap || bitmap->Bounds() != preview_bitmap->Bounds()) {
		try {
			if (preview_bitmap)
				delete copy_of_the_preview_bitmap;

			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;

			if (bitmap) {
				preview_bitmap = bitmap;
				copy_of_the_preview_bitmap = DuplicateBitmap(preview_bitmap);
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
}


void
ScaleManipulator::SetSelection(Selection* new_selection)
{
	selection = new_selection;

	if (selection != NULL && selection->IsEmpty() == false) {
		orig_selection_map = new BBitmap(selection->ReturnSelectionMap());

		BRect bounds = selection->GetBoundingRect();

		original_left = bounds.left;
		original_top = bounds.top;
		original_right = bounds.right;
		original_bottom = bounds.bottom;

		SetValues(original_left, original_top, original_right, original_bottom);
		if (configuration_view != NULL)
			configuration_view->SetValues(original_left, original_top,
				original_right - original_left, original_bottom - original_top);
	}
};


ManipulatorSettings*
ScaleManipulator::ReturnSettings()
{
	return new (std::nothrow) ScaleManipulatorSettings(settings);
}


BView*
ScaleManipulator::MakeConfigurationView(const BMessenger& target)
{
	configuration_view = new (std::nothrow) ScaleManipulatorView(this, target);
	if (configuration_view)
		configuration_view->SetValues(original_left, original_top, original_right - original_left,
			original_bottom - original_top);
	return configuration_view;
}


const char*
ScaleManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Scale: Drag to resize or use number-fields.");
}


const char*
ScaleManipulator::ReturnName()
{
	if (transform_selection_only == true)
		return B_TRANSLATE("Scale selection");

	return B_TRANSLATE("Scale");
}


BRegion
ScaleManipulator::Draw(BView* view, float mag_scale)
{
	int32 DRAGGER_SIZE = 10;
	// Draw all the data that needs to be drawn
	BRect bounds = BRect(mag_scale * settings->left, mag_scale * settings->top,
		mag_scale * (settings->right + 1) - 1, mag_scale * (settings->bottom + 1) - 1);
	bounds.left = floor(bounds.left);
	bounds.top = floor(bounds.top);
	bounds.right = ceil(bounds.right);
	bounds.bottom = ceil(bounds.bottom);

	bool draw_draggers = FALSE;
	BRect dragger_rect = BRect(0, 0, DRAGGER_SIZE - 1, DRAGGER_SIZE - 1);
	float f_bottom = bounds.bottom;
	float f_top = bounds.top;
	float f_left = bounds.left;
	float f_right = bounds.right;

	if ((f_bottom - f_top > 3 * DRAGGER_SIZE + 10) && (f_right - f_left > 3 * DRAGGER_SIZE + 10))
		draw_draggers = TRUE;

	rgb_color high = view->HighColor();
	rgb_color low = view->LowColor();
	view->SetHighColor(255, 255, 255, 255);
	view->SetLowColor(0, 0, 0, 255);
	view->StrokeRect(bounds.InsetByCopy(1, 1), B_SOLID_HIGH);
	view->StrokeRect(bounds, B_SOLID_LOW);
	if (draw_draggers == TRUE) {
		float height = f_bottom - f_top;
		float width = f_right - f_left;

		dragger_rect.OffsetTo(bounds.LeftTop());
		view->FillRect(dragger_rect, B_SOLID_HIGH);
		view->StrokeRect(dragger_rect, B_SOLID_LOW);

		dragger_rect.OffsetTo(bounds.LeftTop() + BPoint(width / 2 - DRAGGER_SIZE / 2, 0));
		view->FillRect(dragger_rect, B_SOLID_HIGH);
		view->StrokeRect(dragger_rect, B_SOLID_LOW);

		dragger_rect.OffsetTo(bounds.LeftTop() + BPoint(width - DRAGGER_SIZE + 1, 0));
		view->FillRect(dragger_rect, B_SOLID_HIGH);
		view->StrokeRect(dragger_rect, B_SOLID_LOW);

		dragger_rect.OffsetTo(bounds.LeftTop() + BPoint(0, height / 2 - DRAGGER_SIZE / 2));
		view->FillRect(dragger_rect, B_SOLID_HIGH);
		view->StrokeRect(dragger_rect, B_SOLID_LOW);

		dragger_rect.OffsetTo(
			bounds.RightTop() + BPoint(-DRAGGER_SIZE + 1, height / 2 - DRAGGER_SIZE / 2));
		view->FillRect(dragger_rect, B_SOLID_HIGH);
		view->StrokeRect(dragger_rect, B_SOLID_LOW);

		dragger_rect.OffsetTo(bounds.LeftBottom() + BPoint(0, -DRAGGER_SIZE + 1));
		view->FillRect(dragger_rect, B_SOLID_HIGH);
		view->StrokeRect(dragger_rect, B_SOLID_LOW);

		dragger_rect.OffsetTo(
			bounds.LeftBottom() + BPoint(width / 2 - DRAGGER_SIZE / 2, -DRAGGER_SIZE + 1));
		view->FillRect(dragger_rect, B_SOLID_HIGH);
		view->StrokeRect(dragger_rect, B_SOLID_LOW);

		dragger_rect.OffsetTo(
			bounds.LeftBottom() + BPoint(width - DRAGGER_SIZE + 1, -DRAGGER_SIZE + 1));
		view->FillRect(dragger_rect, B_SOLID_HIGH);
		view->StrokeRect(dragger_rect, B_SOLID_LOW);
	}
	view->SetHighColor(high);
	view->SetLowColor(low);

	BRegion updated_region;

	bounds.InsetBy(-1, -1);
	updated_region.Set(bounds);
	bounds.InsetBy(DRAGGER_SIZE + 2, DRAGGER_SIZE + 2);
	updated_region.Exclude(bounds);

	return updated_region;
}


void
ScaleManipulator::UpdateSettings()
{
	if (configuration_view != NULL) {
		float left, top, width, height;
		configuration_view->GetControlValues(left, top, width, height);

		settings->left = left;
		settings->top = top;
		settings->right = left + width;
		settings->bottom = top + height;
	}
}


// #pragma mark -- ScaleManipulatorView


ScaleManipulatorView::ScaleManipulatorView(ScaleManipulator* manipulator, const BMessenger& target)
	:
	WindowGUIManipulatorView(),
	fTarget(target),
	fManipulator(manipulator)
{
	original_left = -1;
	original_top = -1;
	original_width = -1;
	original_height = -1;
	maintain_proportions = true;

	left_control
		= new NumberControl(B_TRANSLATE("Left:"), "", new BMessage(SCALE_LEFT_CHANGED), 5, true);

	top_control
		= new NumberControl(B_TRANSLATE("Top:"), "", new BMessage(SCALE_TOP_CHANGED), 5, true);

	width_control
		= new NumberControl(B_TRANSLATE("Width:"), "", new BMessage(SCALE_WIDTH_CHANGED), 5);

	height_control
		= new NumberControl(B_TRANSLATE("Height:"), "", new BMessage(SCALE_HEIGHT_CHANGED), 5);

	BFont font;
	font_height height;
	font.GetHeight(&height);
	float buttonHeight = (height.ascent + height.descent + height.leading) * 1.5;

	BButton* restoreWidthButton
		= new BButton(B_TRANSLATE("Restore"), new BMessage(SCALE_RESTORE_WIDTH));
	restoreWidthButton->SetExplicitMinSize(BSize(B_SIZE_UNSET, buttonHeight));
	restoreWidthButton->SetExplicitMaxSize(BSize(B_SIZE_UNSET, buttonHeight));

	BButton* restoreHeightButton
		= new BButton(B_TRANSLATE("Restore"), new BMessage(SCALE_RESTORE_HEIGHT));
	restoreHeightButton->SetExplicitMinSize(BSize(B_SIZE_UNSET, buttonHeight));
	restoreHeightButton->SetExplicitMaxSize(BSize(B_SIZE_UNSET, buttonHeight));

	BButton* doubleWidthButton = _MakeButton("x2", SCALE_MULTIPLY_WIDTH, 2.0);
	doubleWidthButton->SetExplicitMinSize(BSize(buttonHeight, buttonHeight));
	doubleWidthButton->SetExplicitMaxSize(BSize(buttonHeight, buttonHeight));

	BButton* halveWidthButton = _MakeButton("/2", SCALE_MULTIPLY_WIDTH, 0.5);
	halveWidthButton->SetExplicitMinSize(BSize(buttonHeight, buttonHeight));
	halveWidthButton->SetExplicitMaxSize(BSize(buttonHeight, buttonHeight));

	BButton* doubleHeightButton = _MakeButton("x2", SCALE_MULTIPLY_HEIGHT, 2.0);
	doubleHeightButton->SetExplicitMinSize(BSize(buttonHeight, buttonHeight));
	doubleHeightButton->SetExplicitMaxSize(BSize(buttonHeight, buttonHeight));

	BButton* halveHeightButton = _MakeButton("/2", SCALE_MULTIPLY_HEIGHT, 0.5);
	halveHeightButton->SetExplicitMinSize(BSize(buttonHeight, buttonHeight));
	halveHeightButton->SetExplicitMaxSize(BSize(buttonHeight, buttonHeight));

	BCheckBox* proportion_box = new BCheckBox(B_TRANSLATE_COMMENT("Lock", "Keep the aspect ratio"),
		new BMessage(SCALE_PROPORTION_CHANGED));
	proportion_box->SetValue(B_CONTROL_ON);

	sample_mode_menu = new BPopUpMenu("blend_mode");

	BMessage* sample_msg = new BMessage(SCALE_METHOD_CHANGED);
	sample_msg->AddUInt8("sample_mode", NEAREST_NEIGHBOR);
	sample_mode_menu->AddItem(
		new BMenuItem(interpolation_type_to_string(NEAREST_NEIGHBOR), sample_msg));

	sample_msg = new BMessage(SCALE_METHOD_CHANGED);
	sample_msg->AddUInt8("sample_mode", BILINEAR);
	sample_mode_menu->AddItem(new BMenuItem(interpolation_type_to_string(BILINEAR), sample_msg));

	sample_msg = new BMessage(SCALE_METHOD_CHANGED);
	sample_msg->AddUInt8("sample_mode", BICUBIC);
	sample_mode_menu->AddItem(new BMenuItem(interpolation_type_to_string(BICUBIC), sample_msg));

	sample_msg = new BMessage(SCALE_METHOD_CHANGED);
	sample_msg->AddUInt8("sample_mode", BICUBIC_BSPLINE);
	sample_mode_menu->AddItem(
		new BMenuItem(interpolation_type_to_string(BICUBIC_BSPLINE), sample_msg));

	sample_msg = new BMessage(SCALE_METHOD_CHANGED);
	sample_msg->AddUInt8("sample_mode", BICUBIC_CATMULL_ROM);
	sample_mode_menu->AddItem(
		new BMenuItem(interpolation_type_to_string(BICUBIC_CATMULL_ROM), sample_msg));

	sample_msg = new BMessage(SCALE_METHOD_CHANGED);
	sample_msg->AddUInt8("sample_mode", MITCHELL);
	sample_mode_menu->AddItem(new BMenuItem(interpolation_type_to_string(MITCHELL), sample_msg));

	sample_mode_menu->ItemAt(1)->SetMarked(TRUE);

	BMenuField* sample_dropdown
		= new BMenuField("sample_dropdown", B_TRANSLATE("Filter:"), sample_mode_menu);

	BGridLayout* mainLayout = BLayoutBuilder::Grid<>(this, 5.0, 5.0)
	  .Add(left_control->CreateLabelLayoutItem(), 0, 0)
	  .Add(left_control->CreateTextViewLayoutItem(), 1, 0)
	  .Add(top_control->CreateLabelLayoutItem(), 0, 1)
	  .Add(top_control->CreateTextViewLayoutItem(), 1, 1)
	  .Add(width_control->CreateLabelLayoutItem(), 0, 2)
	  .Add(width_control->CreateTextViewLayoutItem(), 1, 2)
	  .Add(height_control->CreateLabelLayoutItem(), 0, 3)
	  .Add(height_control->CreateTextViewLayoutItem(), 1, 3)
	  .Add(proportion_box, 5, 2, 1, 2)
	  .Add(doubleWidthButton, 2, 2)
	  .Add(halveWidthButton, 3, 2)
	  .Add(restoreWidthButton, 4, 2)
	  .Add(doubleHeightButton, 2, 3)
	  .Add(halveHeightButton, 3, 3)
	  .Add(restoreHeightButton, 4, 3)
	  .Add(sample_dropdown->CreateLabelLayoutItem(), 0, 4)
	  .Add(sample_dropdown->CreateMenuBarLayoutItem(), 1, 4, 4);

	mainLayout->SetMaxColumnWidth(0, font.StringWidth("LABELLABELLABEL"));
	mainLayout->SetMinColumnWidth(1, font.StringWidth("01234"));

	if (manipulator->GetTransformSelectionOnly() == true)
		sample_dropdown->SetEnabled(false);
}


void
ScaleManipulatorView::AttachedToWindow()
{
	_SetTarget(this);
	sample_mode_menu->SetTargetForItems(this);

	left_control->MakeFocus(true);
	WindowGUIManipulatorView::AttachedToWindow();
}


void
ScaleManipulatorView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case SCALE_LEFT_CHANGED:
		case SCALE_TOP_CHANGED:
		case SCALE_WIDTH_CHANGED:
		case SCALE_HEIGHT_CHANGED:
		{
			//			message->PrintToStream();
			if (message->what == SCALE_LEFT_CHANGED) {
				current_left = left_control->Value();
			} else if (message->what == SCALE_WIDTH_CHANGED) {
				current_width = max_c(1.0, ceil(width_control->Value()));
				// Need to round the height correctly to the nearest pixel
				if (maintain_proportions) {
					current_height = max_c(
						1.0, floor(original_height * (current_width / original_width) + 0.5));
				}
			} else if (message->what == SCALE_TOP_CHANGED) {
				current_top = top_control->Value();
			} else {
				current_height = max_c(1.0, ceil(height_control->Value()));
				if (maintain_proportions) {
					current_width = max_c(
						1.0, floor(original_width * (current_height / original_height) + 0.5));
				}
			}

			if (fManipulator)
				fManipulator->SetValues(current_left, current_top, current_left + current_width,
					current_top + current_height);
			SetValues(current_left, current_top, current_width, current_height);
			fTarget.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		} break;
		case SCALE_MULTIPLY_WIDTH:
		{
			case SCALE_MULTIPLY_HEIGHT:
				float coefficient;
				//			message->PrintToStream();
				if (message->FindFloat("coefficient", &coefficient) == B_OK) {
					if (message->what == SCALE_MULTIPLY_WIDTH) {
						current_width = max_c(1.0, ceil(width_control->Value() * coefficient));
						if (maintain_proportions) {
							current_height = max_c(1.0,
								floor(original_height * (current_width / original_width) + 0.5));
							height_control->SetValue(int32(current_height));
						}
						width_control->SetValue(int32(current_width));
						Window()->PostMessage(SCALE_HEIGHT_CHANGED, this);
						Window()->PostMessage(SCALE_WIDTH_CHANGED, this);
					} else {
						current_height = max_c(1.0, ceil(height_control->Value() * coefficient));
						if (maintain_proportions) {
							current_width = max_c(1.0,
								floor(original_width * (current_height / original_height) + 0.5));
							width_control->SetValue(int32(current_width));
						}
						height_control->SetValue(int32(current_height));
						Window()->PostMessage(SCALE_HEIGHT_CHANGED, this);
						Window()->PostMessage(SCALE_WIDTH_CHANGED, this);
					}
				}
		} break;
		case SCALE_RESTORE_HEIGHT:
		{
			case SCALE_RESTORE_WIDTH:
				//			message->PrintToStream();
				if (message->what == SCALE_RESTORE_WIDTH) {
					SetValues(original_left, current_top, original_width, current_height);
					if (fManipulator)
						fManipulator->SetValues(original_left, current_top,
							original_left + original_width, current_top + current_height);
					fTarget.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
				} else if (message->what == SCALE_RESTORE_HEIGHT) {
					SetValues(current_left, original_top, current_width, original_height);
					if (fManipulator)
						fManipulator->SetValues(current_left, original_top,
							current_left + current_width, original_top + original_height);
					fTarget.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
				}
		} break;
		case SCALE_PROPORTION_CHANGED:
		{
			maintain_proportions = !maintain_proportions;
			if (maintain_proportions) {
				current_height
					= max_c(1.0, floor(original_height * (current_width / original_width) + 0.5));

				if (fManipulator) {
					fManipulator->SetValues(current_left, current_top, current_width + current_left,
						current_height + current_top);
					left_control->SetValue(int32(current_left));
					top_control->SetValue(int32(current_top));
					width_control->SetValue(int32(current_width));
					height_control->SetValue(int32(current_height));
				}
			}
			fTarget.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		} break;
		case SCALE_METHOD_CHANGED:
		{
			uint8 new_method;
			if (message->FindUInt8("sample_mode", &new_method) == B_OK) {
				method = (interpolation_type) new_method;
				if (fManipulator)
					fManipulator->SetInterpolationMethod(method);
			}
		} break;
		default:
			WindowGUIManipulatorView::MessageReceived(message);
	}
}


void
ScaleManipulatorView::SetValues(float left, float top, float width, float height)
{
	current_left = round(left);
	current_top = round(top);
	current_width = round(width);
	current_height = round(height);

	if (original_width <= 0) {
		original_left = current_left;
		original_top = current_top;
		original_width = current_width;
		original_height = current_height;
	}

	// TODO: check why we try locking first and on error just access
	if (LockLooper()) {
		_SetValues(current_left, current_top, current_width, current_height);
		UnlockLooper();
	} else {
		_SetValues(current_left, current_top, current_width, current_height);
	}
}


void
ScaleManipulatorView::GetControlValues(float& left, float& top, float& width, float& height)
{
	left = left_control->Value();
	top = top_control->Value();
	width = width_control->Value();
	height = height_control->Value();
}


void
ScaleManipulatorView::_SetTarget(BView* view)
{
	for (int32 i = 0; i < view->CountChildren(); ++i) {
		BView* child = view->ChildAt(i);
		if (child->CountChildren() > 0)
			_SetTarget(child);
		if (BControl* control = dynamic_cast<BControl*>(child))
			control->SetTarget(this);
	}
}


void
ScaleManipulatorView::_SetValues(float left, float top, float width, float height)
{
	// TODO: check why we cast to BControl here
	BControl* text_control = dynamic_cast<BControl*>(left_control);
	if (text_control)
		text_control->SetValue(int32(left));

	text_control = dynamic_cast<BControl*>(top_control);
	if (text_control != NULL)
		text_control->SetValue(int32(top));

	text_control = dynamic_cast<BControl*>(width_control);
	if (text_control)
		text_control->SetValue(int32(width));

	text_control = dynamic_cast<BControl*>(height_control);
	if (text_control != NULL)
		text_control->SetValue(int32(height));
}


BButton*
ScaleManipulatorView::_MakeButton(const char* label, uint32 what, float coefficient)
{
	BMessage* message = new BMessage(what);
	message->AddFloat("coefficient", coefficient);

	BButton* button = new BButton(label, message);

	float width, height;
	button->GetPreferredSize(&width, &height);

	// TODO: this sucks, file bugreport for HAIKU
	button->SetExplicitMaxSize(BSize(height, B_SIZE_UNSET));
	button->SetExplicitMinSize(BSize(height, B_SIZE_UNSET));

	return button;
}
