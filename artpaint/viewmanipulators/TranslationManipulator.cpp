/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "BitmapUtilities.h"
#include "ImageView.h"
#include "MessageConstants.h"
#include "NumberControl.h"
#include "PixelOperations.h"
#include "Selection.h"
#include "TranslationManipulator.h"


#include <Catalog.h>
#include <ClassInfo.h>
#include <LayoutBuilder.h>
#include <StatusBar.h>
#include <Window.h>


#include <new>
#include <stdlib.h>
#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Manipulators"


using ArtPaint::Interface::NumberControl;


TranslationManipulator::TranslationManipulator(BBitmap* bm)
	:
	WindowGUIManipulator(),
	copy_of_the_preview_bitmap(NULL),
	selection(NULL),
	transform_selection_only(false)
{
	preview_bitmap = bm;
	if (preview_bitmap != NULL)
		copy_of_the_preview_bitmap = DuplicateBitmap(preview_bitmap);

	settings = new TranslationManipulatorSettings();
	previous_x_translation = 0;
	previous_y_translation = 0;

	last_calculated_resolution = 0;
	lowest_available_quality = 4;
}


TranslationManipulator::~TranslationManipulator()
{
	delete settings;
	delete copy_of_the_preview_bitmap;

	if (config_view != NULL) {
		config_view->RemoveSelf();
		delete config_view;
	}
}


void
TranslationManipulator::SetSelection(Selection* new_selection)
{
	selection = new_selection;

	if (selection != NULL && selection->IsEmpty() == false)
		orig_selection_map = new BBitmap(selection->ReturnSelectionMap());
	else
		orig_selection_map = NULL;
}


void
TranslationManipulator::MouseDown(BPoint point, uint32, BView*, bool first)
{
	if (first)
		previous_point = point;
	else {
		settings->x_translation += point.x - previous_point.x;
		settings->y_translation += point.y - previous_point.y;

		previous_point = point;

		if (config_view != NULL)
			config_view->SetValues(settings->x_translation, settings->y_translation);
	}
}


BBitmap*
TranslationManipulator::ManipulateBitmap(
	ManipulatorSettings* set, BBitmap* original, BStatusBar* status_bar)
{
	TranslationManipulatorSettings* new_settings = cast_as(set, TranslationManipulatorSettings);

	if (transform_selection_only == true)
		return NULL;

	if ((status_bar != NULL) && (status_bar->Window() != NULL)) {
		if (status_bar->Window()->LockWithTimeout(0) == B_OK) {
			status_bar->Update(100);
			status_bar->Window()->Unlock();
		}
	}

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	if ((new_settings->x_translation == 0) && (new_settings->y_translation == 0))
		return NULL;

	BBitmap* new_bitmap;
	if (original != preview_bitmap) {
		original->Lock();
		BRect bitmap_frame = original->Bounds();
		new_bitmap = new BBitmap(bitmap_frame, B_RGB_32_BIT);
		original->Unlock();
		if (new_bitmap->IsValid() == FALSE)
			throw std::bad_alloc();
	} else {
		new_bitmap = original;
		original = copy_of_the_preview_bitmap;
	}

	union color_conversion background;
	// Transparent background.
	background.bytes[0] = 0xFF;
	background.bytes[1] = 0xFF;
	background.bytes[2] = 0xFF;
	background.bytes[3] = 0x00;

	// This function assumes that the both bitmaps are of same size.
	uint32* target_bits = (uint32*)new_bitmap->Bits();
	uint32* source_bits = (uint32*)original->Bits();
	uint32 target_bpr = new_bitmap->BytesPerRow() / 4;
	uint32 source_bpr = original->BytesPerRow() / 4;
	int32 width = (int32)min_c(new_bitmap->Bounds().Width() + 1, original->Bounds().Width() + 1);
	int32 height = (int32)min_c(new_bitmap->Bounds().Height() + 1, original->Bounds().Height() + 1);

	// We have to copy translations so that we do translation for all pixels
	// with the same values
	int32 x_translation_local = ((int32)new_settings->x_translation);
	int32 y_translation_local = ((int32)new_settings->y_translation);
	if (selection == NULL || selection->IsEmpty()) {
		// First clear the target bitmap. Here the clearing of the whole bitmap is not usually
		// necessary. Actually this loop combined with the next should only set each pixel in the
		// bitmap exactly once.
		for (int32 y = 0; y < height; y++) {
			for (int32 x = 0; x < width; x++)
				*(target_bits + x + y * target_bpr) = background.word;
		}
		// Copy only the points that are within the intersection of
		// original_bitmap->Bounds() and
		// original_bitmap->Bounds().OffsetBy(x_translation,y_translation)
		int32 target_x_offset = max_c(0, x_translation_local);
		int32 target_y_offset = max_c(0, y_translation_local);
		int32 source_x_offset = max_c(0, -x_translation_local);
		int32 source_y_offset = max_c(0, -y_translation_local);

		for (int32 y = 0; y < height - abs(y_translation_local); y++) {
			for (int32 x = 0; x < width - abs(x_translation_local); x++) {
				*(target_bits + x + target_x_offset + (y + target_y_offset) * target_bpr)
					= *(source_bits + x + source_x_offset + (y + source_y_offset) * source_bpr);
			}
		}
	} else {
		// First reset the picture
		for (int32 y = 0; y < height; y++) {
			for (int32 x = 0; x < width; x++)
				*(target_bits + x + y * target_bpr) = *(source_bits + x + y * source_bpr);
		}

		selection->ReplaceSelection(orig_selection_map);
		// Then clear the selection
		BRect selection_bounds = selection->GetBoundingRect();
		int32 left = (int32)selection_bounds.left;
		int32 right = (int32)selection_bounds.right;
		int32 top = (int32)selection_bounds.top;
		int32 bottom = (int32)selection_bounds.bottom;
		for (int32 y = top; y <= bottom; y++) {
			for (int32 x = left; x <= right; x++) {
				if (selection->ContainsPoint(x, y))
					*(target_bits + x + y * target_bpr) = background.word;
			}
		}

		BBitmap* selmap = ManipulateSelectionMap(settings);
		selection->ReplaceSelection(selmap);

		selection_bounds = selection->GetBoundingRect();
		selection_bounds = selection_bounds & original->Bounds() & new_bitmap->Bounds();
		left = (int32)selection_bounds.left;
		right = (int32)selection_bounds.right;
		top = (int32)selection_bounds.top;
		bottom = (int32)selection_bounds.bottom;
		for (int32 y = top; y <= bottom; y++) {
			for (int32 x = left; x <= right; x++) {
				int32 new_x = (int32)(x - new_settings->x_translation);
				int32 new_y = (int32)(y - new_settings->y_translation);
				if (selection->ContainsPoint(x, y)) {
					*(target_bits + x + y * target_bpr)
						= src_over_fixed(*(target_bits + x + y * target_bpr),
							*(source_bits + new_x + new_y * source_bpr));
				}
			}
		}
	}

	return new_bitmap;
}


BBitmap*
TranslationManipulator::ManipulateSelectionMap(ManipulatorSettings* set)
{
	TranslationManipulatorSettings* new_settings = cast_as(set, TranslationManipulatorSettings);

	if (new_settings == NULL)
		return NULL;

	if ((new_settings->x_translation == 0) && (new_settings->y_translation == 0))
		return NULL;

	BRect bitmap_frame;
	if (orig_selection_map != NULL) {
		selection->ReplaceSelection(orig_selection_map);
		orig_selection_map->Lock();
		bitmap_frame = orig_selection_map->Bounds();
		orig_selection_map->Unlock();
	}

	BBitmap* selection_map = new BBitmap(selection->ReturnSelectionMap());
	BBitmap* new_bitmap;

	new_bitmap = new BBitmap(bitmap_frame, B_GRAY8);
	if (new_bitmap->IsValid() == FALSE)
		throw std::bad_alloc();

	uint8 background = 0x00;

	// This function assumes that the both bitmaps are of same size.
	uint8* target_bits = (uint8*)new_bitmap->Bits();
	uint8* source_bits = (uint8*)selection_map->Bits();
	uint32 target_bpr = new_bitmap->BytesPerRow();
	uint32 source_bpr = selection_map->BytesPerRow();

	int32 width = (int32)min_c(new_bitmap->Bounds().Width() + 1, bitmap_frame.Width() + 1);
	int32 height = (int32)min_c(new_bitmap->Bounds().Height() + 1, bitmap_frame.Height() + 1);

	// We have to copy translations so that we do translation for all pixels
	// with the same values
	int32 x_translation_local = ((int32)new_settings->x_translation);
	int32 y_translation_local = ((int32)new_settings->y_translation);

	// First clear the target bitmap. Here the clearing of the whole bitmap is not usually
	// necessary. Actually this loop combined with the next should only set each pixel in the
	// bitmap exactly once.
	for (int32 y = 0; y < height; y++) {
		for (int32 x = 0; x < width; x++)
			*(target_bits + x + y * target_bpr) = background;
	}

	// Copy only the points that are within the intersection of
	// original_bitmap->Bounds() and
	// original_bitmap->Bounds().OffsetBy(x_translation,y_translation)
	int32 target_x_offset = max_c(0, x_translation_local);
	int32 target_y_offset = max_c(0, y_translation_local);
	int32 source_x_offset = max_c(0, -x_translation_local);
	int32 source_y_offset = max_c(0, -y_translation_local);

	for (int32 y = 0; y < height - abs(y_translation_local); y++) {
		for (int32 x = 0; x < width - abs(x_translation_local); x++) {
			*(target_bits + x + target_x_offset + (y + target_y_offset) * target_bpr)
				= *(source_bits + x + source_x_offset + (y + source_y_offset) * source_bpr);
		}
	}

	return new_bitmap;
}


int32
TranslationManipulator::PreviewBitmap(bool full_quality, BRegion* updated_region)
{
	if (preview_bitmap == NULL || copy_of_the_preview_bitmap == NULL)
		return 0;

	if (transform_selection_only == true
		&& ((selection == NULL || selection->IsEmpty() == true) && orig_selection_map == NULL))
		return 0;

	// First decide the resolution of the bitmap
	if ((previous_x_translation == settings->x_translation)
		&& (previous_y_translation == settings->y_translation) && (full_quality == FALSE)) {
		if (last_calculated_resolution <= highest_available_quality) {
			last_calculated_resolution = 0;
			if (full_quality == TRUE) {
				updated_region->Set(preview_bitmap->Bounds());
				return 1;
			} else
				return 0;
		} else {
			if (full_quality == FALSE)
				last_calculated_resolution = last_calculated_resolution / 2;
			else
				last_calculated_resolution = min_c(1, last_calculated_resolution / 2);
		}
	} else if (full_quality == TRUE)
		last_calculated_resolution = 1;
	else
		last_calculated_resolution = lowest_available_quality;

	if (last_calculated_resolution > 0) {
		union color_conversion background;
		// Transparent background.
		background.bytes[0] = 0xFF;
		background.bytes[1] = 0xFF;
		background.bytes[2] = 0xFF;
		background.bytes[3] = 0x00;

		// This function assumes that the both bitmaps are of same size.
		uint32* target_bits = (uint32*)preview_bitmap->Bits();
		uint32* source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 target_bpr = preview_bitmap->BytesPerRow() / 4;
		uint32 source_bpr = copy_of_the_preview_bitmap->BytesPerRow() / 4;

		int32 width = (int32)min_c(
			preview_bitmap->Bounds().Width() + 1, copy_of_the_preview_bitmap->Bounds().Width() + 1);
		int32 height = (int32)min_c(preview_bitmap->Bounds().Height() + 1,
			copy_of_the_preview_bitmap->Bounds().Height() + 1);

		// We have to copy translations so that we do translation for all pixels
		// with the same values
		int32 x_translation_local = ((int32)settings->x_translation) / last_calculated_resolution
			* last_calculated_resolution;
		int32 y_translation_local = ((int32)settings->y_translation) / last_calculated_resolution
			* last_calculated_resolution;

		if (orig_selection_map != NULL && selection != NULL)
			selection->ReplaceSelection(orig_selection_map);

		if (selection == NULL || selection->IsEmpty() == true) {
			// First clear the target bitmap.
			BRegion to_be_cleared;
			to_be_cleared.Set(uncleared_rect);
			uncleared_rect.left = 0;
			uncleared_rect.top = 0;
			uncleared_rect.right = width;
			uncleared_rect.bottom = height;
			uncleared_rect.OffsetBy(x_translation_local, y_translation_local);
			uncleared_rect.InsetBy(-10, -10);
			to_be_cleared.Exclude(uncleared_rect);
			for (int32 i = 0; i < to_be_cleared.CountRects(); i++) {
				BRect rect = to_be_cleared.RectAt(i);
				rect.InsetBy(-10, -10);
				int32 left = (int32)(floor(rect.left
					/ last_calculated_resolution) * last_calculated_resolution);
				int32 top = (int32)(floor(rect.top
					/ last_calculated_resolution) * last_calculated_resolution);
				int32 right = (int32)(ceil(rect.right
					/ last_calculated_resolution) * last_calculated_resolution);
				int32 bottom = (int32)(ceil(rect.bottom
					/ last_calculated_resolution) * last_calculated_resolution);
				for (int32 y = top; y <= bottom; y += last_calculated_resolution) {
					for (int32 x = left; x <= right; x += last_calculated_resolution) {
						if (x >= 0 && y >= 0 && x < width && y < height)
							*(target_bits + x + y * target_bpr) = background.word;
					}
				}
			}
			// Copy only the points that are within the intersection of
			// original_bitmap->Bounds() and
			// original_bitmap->Bounds().OffsetBy(x_translation,y_translation)
			int32 target_x_offset = max_c(0, x_translation_local);
			int32 target_y_offset = max_c(0, y_translation_local);
			int32 source_x_offset = max_c(0, -x_translation_local);
			int32 source_y_offset = max_c(0, -y_translation_local);

			for (int32 y = 0; y < height - abs(y_translation_local);
				y += last_calculated_resolution) {
				for (int32 x = 0; x < width - abs(x_translation_local);
					x += last_calculated_resolution) {
					*(target_bits + x + target_x_offset + (y + target_y_offset) * target_bpr)
						= *(source_bits + x + source_x_offset + (y + source_y_offset) * source_bpr);
				}
			}

			updated_region->Set(uncleared_rect);
			updated_region->Include(&to_be_cleared);
		} else {
			BRect orig_selection_bounds = selection->GetBoundingRect();

			BRegion to_be_cleared;
			to_be_cleared.Set(uncleared_rect);

			if (transform_selection_only == false) {
				// First reset the picture
				for (int32 i = 0; i < to_be_cleared.CountRects(); i++) {
					BRect rect = to_be_cleared.RectAt(i);
					int32 left = (int32)(floor(rect.left
						/ last_calculated_resolution) * last_calculated_resolution);
					int32 top = (int32)(floor(rect.top
						/ last_calculated_resolution) * last_calculated_resolution);
					int32 right = (int32)(floor(rect.right
						/ last_calculated_resolution) * last_calculated_resolution);
					int32 bottom = (int32)(floor(rect.bottom
						/ last_calculated_resolution) * last_calculated_resolution);

					for (int32 y = top; y <= bottom; y += last_calculated_resolution) {
						for (int32 x = left; x <= right; x += last_calculated_resolution) {
							*(target_bits + x + y * target_bpr)
								= *(source_bits + x + y * source_bpr);
						}
					}
				}

				// Then do the selection
				BRect selection_bounds(orig_selection_bounds);

				int32 left = (int32)ceil(selection_bounds.left / last_calculated_resolution)
					* last_calculated_resolution;
				int32 right = (int32)selection_bounds.right;
				int32 top = (int32)ceil(selection_bounds.top / last_calculated_resolution)
					* last_calculated_resolution;
				int32 bottom = (int32)selection_bounds.bottom;
				for (int32 y = top; y <= bottom; y += last_calculated_resolution)
					for (int32 x = left; x <= right; x += last_calculated_resolution)
						if (selection->ContainsPoint(x, y))
							*(target_bits + x + y * target_bpr) = background.word;
			}

			BBitmap* selmap = ManipulateSelectionMap(settings);
			selection->ReplaceSelection(selmap);

			uncleared_rect = selection->GetBoundingRect();
			uncleared_rect.InsetBy(-10, -10);
			uncleared_rect = uncleared_rect & preview_bitmap->Bounds();

			if (transform_selection_only == false) {
				BRect selection_bounds = selection->GetBoundingRect();
				selection_bounds = selection_bounds & preview_bitmap->Bounds();
				int32 left = (int32)ceil(selection_bounds.left / last_calculated_resolution)
					* last_calculated_resolution;
				int32 right = (int32)selection_bounds.right;
				int32 top = (int32)ceil(selection_bounds.top / last_calculated_resolution)
					* last_calculated_resolution;
				int32 bottom = (int32)selection_bounds.bottom;

				if (selection_bounds != preview_bitmap->Bounds() ||
					(orig_selection_bounds == preview_bitmap->Bounds()
						&& (settings->x_translation >= 0 && settings->y_translation >= 0))) {
					for (int32 y = top; y <= bottom; y += last_calculated_resolution) {
						for (int32 x = left; x <= right; x += last_calculated_resolution) {
							int32 new_x = (int32)(x - settings->x_translation);
							int32 new_y = (int32)(y - settings->y_translation);
							if (selection->ContainsPoint(x, y) && new_x >= 0 && new_y >= 0)
								*(target_bits + x + y * target_bpr)
									= *(source_bits + new_x + new_y * source_bpr);
						}
					}
				}
			}

			updated_region->Set(uncleared_rect);
			updated_region->Include(&to_be_cleared);
		}
		previous_x_translation = x_translation_local;
		previous_y_translation = y_translation_local;
	}
	return last_calculated_resolution;
}


ManipulatorSettings*
TranslationManipulator::ReturnSettings()
{
	return new TranslationManipulatorSettings(settings);
}


void
TranslationManipulator::Reset()
{
	settings->x_translation = 0;
	settings->y_translation = 0;
	previous_x_translation = 0;
	previous_y_translation = 0;

	if (preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with loop.
		uint32* source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32* target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target, source, bits_length);

		uncleared_rect = preview_bitmap->Bounds();
	}

	if (orig_selection_map != NULL && selection != NULL)
		selection->ReplaceSelection(orig_selection_map);
}


void
TranslationManipulator::SetPreviewBitmap(BBitmap* bm)
{
	delete copy_of_the_preview_bitmap;
	preview_bitmap = bm;
	if (preview_bitmap != NULL)
		copy_of_the_preview_bitmap = DuplicateBitmap(preview_bitmap);
	else
		copy_of_the_preview_bitmap = NULL;

	if (preview_bitmap != NULL) {
		double speed = GetSystemClockSpeed() / 1000;

		BRect bounds = preview_bitmap->Bounds();
		float num_pixels = (bounds.Width() + 1) * (bounds.Height() + 1);
		lowest_available_quality = 1;

		while ((2 * num_pixels / lowest_available_quality / lowest_available_quality) > speed)
			lowest_available_quality *= 2;

		highest_available_quality = max_c(lowest_available_quality / 2, 1);

		uncleared_rect = preview_bitmap->Bounds();
	} else {
		lowest_available_quality = 1;
		highest_available_quality = 1;
	}
	last_calculated_resolution = lowest_available_quality;
}


BView*
TranslationManipulator::MakeConfigurationView(const BMessenger& target)
{
	config_view = new TranslationManipulatorView(this, target);
	config_view->SetValues(settings->x_translation, settings->y_translation);
	return config_view;
}


void
TranslationManipulator::SetValues(float x, float y)
{
	settings->x_translation = x;
	settings->y_translation = y;
}


const char*
TranslationManipulator::ReturnName()
{
	if (transform_selection_only == true)
		return B_TRANSLATE("Translate selection");

	return B_TRANSLATE("Translate");
}


const char*
TranslationManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Translate: Drag to move or use number-fields.");
}


void
TranslationManipulator::UpdateSettings()
{
	if (config_view)
		config_view->GetControlValues(settings->x_translation, settings->y_translation);
}


// #pragma mark -- TranslationManipulatorView


TranslationManipulatorView::TranslationManipulatorView(
	TranslationManipulator* manipulator, const BMessenger& target)
	:
	WindowGUIManipulatorView(),
	fTarget(target),
	fManipulator(manipulator)
{
	fXControl = new NumberControl(
		"X:", "9999˚", new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), 5, true);

	fYControl = new NumberControl(
		"Y:", "9999˚", new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), 5, true);

	SetLayout(BLayoutBuilder::Group<>(this, B_HORIZONTAL).Add(fXControl).Add(fYControl));

	TranslationManipulatorSettings* settings
		= (TranslationManipulatorSettings*)manipulator->ReturnSettings();
	fXControl->SetValue(settings->x_translation);
	fYControl->SetValue(settings->y_translation);

	delete settings;
}


void
TranslationManipulatorView::AttachedToWindow()
{
	fXControl->SetTarget(this);
	fYControl->SetTarget(this);

	if (BView* parent = Parent()) {
		SetLowColor(parent->LowColor());
		SetViewColor(parent->ViewColor());
	}

	fXControl->MakeFocus(true);

	WindowGUIManipulatorView::AttachedToWindow();
}


void
TranslationManipulatorView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case HS_MANIPULATOR_ADJUSTING_FINISHED:
		{
			if (fManipulator)
				fManipulator->SetValues(fXControl->Value(), fYControl->Value());

			if (fTarget.IsValid())
				fTarget.SendMessage(message);
		} break;
		default:
			WindowGUIManipulatorView::MessageReceived(message);
	}
}


void
TranslationManipulatorView::SetValues(float x, float y)
{
	BWindow* window = Window();
	if (window && window->Lock()) {
		fXControl->SetValue(int32(x));
		fYControl->SetValue(int32(y));

		window->Unlock();
	}
}


void
TranslationManipulatorView::SetTarget(const BMessenger& target)
{
	fTarget = target;
}


void
TranslationManipulatorView::GetControlValues(float& x, float& y)
{
	x = fXControl->Value();
	y = fYControl->Value();
}
