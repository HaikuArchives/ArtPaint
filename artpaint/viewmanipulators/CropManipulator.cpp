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

#include "CropManipulator.h"

#include "MessageConstants.h"
#include "NumberControl.h"
#include "PixelOperations.h"
#include "Selection.h"


#include <Bitmap.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <StatusBar.h>
#include <Window.h>


#include <new>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Manipulators"


using ArtPaint::Interface::NumberControl;


CropManipulator::CropManipulator(BBitmap* bm)
	: WindowGUIManipulator(),
	selection(NULL)
{
	settings = new CropManipulatorSettings();
	config_view = NULL;
	preview_bitmap = NULL;
	copy_of_the_preview_bitmap = NULL;
	use_selected = TRUE;
	lock_aspect = FALSE;

	min_x = 0;
	max_x = 0;
	min_y = 0;
	max_y = 0;

	if (bm != NULL) {
		BRect rect = bm->Bounds();
		settings->left = min_x = rect.left;
		settings->right = max_x = rect.right + 1;
		settings->top = min_y = rect.top;
		settings->bottom = max_y = rect.bottom + 1;
	}

	SetPreviewBitmap(bm);
}


CropManipulator::~CropManipulator()
{
	if (config_view != NULL) {
		config_view->RemoveSelf();
		delete config_view;
	}

	delete settings;
}


void
CropManipulator::MouseDown(BPoint point, uint32, BView*, bool first)
{
	previous_left = settings->left;
	previous_right = settings->right;
	previous_top = settings->top;
	previous_bottom = settings->bottom;

	point.x = floor(point.x);
	point.y = floor(point.y);

	float aspect_ratio = (previous_right - previous_left) /
		(previous_bottom - previous_top);

	if (first == TRUE) {
		// Here we select which grabbles to move
		move_left = move_right = move_top = move_bottom = FALSE;
		if (fabs(point.x - settings->left) < 50)
			if (fabs(point.x - settings->left) <
				fabs(point.x - (settings->left +
				(settings->right - settings->left) / 2)))
				move_left = TRUE;
		if (fabs(point.x - settings->right) < 50)
			if (fabs(point.x - settings->right) <
				fabs(point.x - (settings->left +
				(settings->right - settings->left) / 2)))
				move_right = TRUE;
		if ((move_left == TRUE) && (move_right == TRUE)) {
			if (fabs(point.x - settings->left) >
				fabs(point.x - settings->right))
				move_left = FALSE;
			else
				move_right = FALSE;
		}

		if (fabs(point.y - settings->top) < 50)
			if (fabs(point.y - settings->top) <
				fabs(point.y - (settings->top +
				(settings->bottom - settings->top) / 2)))
				move_top = TRUE;
		if (fabs(point.y - settings->bottom) < 50)
			if (fabs(point.y - settings->bottom) <
				fabs(point.y - (settings->top +
				(settings->bottom - settings->top) / 2)))
				move_bottom = TRUE;
		if ((move_top == TRUE) && (move_bottom == TRUE)) {
			if (fabs(point.y - settings->top) >
				fabs(point.y - settings->bottom))
				move_top = FALSE;
			else
				move_bottom = FALSE;
		}

		if (move_left == FALSE && move_top == FALSE &&
			move_right == FALSE && move_bottom == FALSE)
			move_all = TRUE;
		else
			move_all = FALSE;

		last_x = point.x;
		last_y = point.y;
	} else {
		if (move_all == TRUE) {
			float width = settings->right - settings->left;
			float height = settings->bottom - settings->top;
			
			float delta_x = last_x - point.x;
			float delta_y = last_y - point.y;

			float new_left, new_top;

			new_left = settings->left - delta_x;
			new_top = settings->top - delta_y;

			settings->left = new_left;
			settings->top = new_top;
			settings->right = new_left + width;
			settings->bottom = new_top + height;

			last_x = point.x;
			last_y = point.y;
		} else {
			float old_width = settings->right - settings->left;
			float old_height = settings->bottom - settings->top;

			if (move_left == TRUE)
				settings->left = min_c(point.x, settings->right);
			if (move_right == TRUE)
				settings->right = max_c(settings->left, point.x);

			if (move_top == TRUE)
				settings->top = min_c(point.y, settings->bottom);
			if (move_bottom == TRUE)
				settings->bottom = max_c(settings->top, point.y);

			if (lock_aspect == TRUE || modifiers() & B_LEFT_SHIFT_KEY) {
				float new_width = settings->right - settings->left;
				float new_height = settings->bottom - settings->top;

				if (new_height == 0)
					new_height = 1;

				float new_aspect = new_width / new_height;

				if (move_right == FALSE && move_left == FALSE) {
					if (new_aspect < aspect_ratio) {
						if (new_height <= old_height)
							new_height = new_width / aspect_ratio;
						else
							new_width = new_height * aspect_ratio;
					} else {
						if (new_width <= old_width)
							new_width = new_height * aspect_ratio;
						else
							new_height = new_width / aspect_ratio;
					}
					if (move_top == TRUE)
						settings->top = settings->bottom - new_height;
					else
						settings->bottom = settings->top + new_height;
					settings->right = settings->left + new_width;
				} else if (move_top == FALSE && move_bottom == FALSE) {
					if (new_aspect < aspect_ratio) {
						if (new_height <= old_height)
							new_height = new_width / aspect_ratio;
						else
							new_width = new_height * aspect_ratio;
					} else {
						if (new_width <= old_width)
							new_width = new_height * aspect_ratio;
						else
							new_height = new_width / aspect_ratio;
					}
					if (move_left == TRUE)
						settings->left = settings->right - new_width;
					else
						settings->right = settings->left + new_width;
					settings->bottom = settings->top + new_height;
				} else if (move_top == TRUE) {
					if (new_aspect < aspect_ratio) {
						new_height = new_width / aspect_ratio;
						settings->top = settings->bottom - new_height;
					} else {
						new_width = new_height * aspect_ratio;
						if (move_left == TRUE)
							settings->left = settings->right - new_width;
						else
							settings->right = settings->left + new_width;
					}
				} else if (move_bottom == TRUE) {
					if (new_aspect < aspect_ratio) {
						new_height = new_width / aspect_ratio;
						settings->bottom = settings->top + new_height;
					} else {
						new_width = new_height * aspect_ratio;
						if (move_left == TRUE)
							settings->left = settings->right - new_width;
						else
							settings->right = settings->left + new_width;
					}
				}
			}
		}
	}

	if (settings->left >= settings->right)
		settings->left = settings->right - 1;

	if (settings->top >= settings->bottom)
		settings->top - settings->bottom - 1;

	if ((previous_left != settings->left) ||
		(previous_right != settings->right) ||
		(previous_top != settings->top) ||
		(previous_bottom != settings->bottom))
		if (config_view != NULL)
			config_view->SetValues(settings->left, settings->right,
				settings->top, settings->bottom);
}


BRegion
CropManipulator::Draw(BView* view, float mag_scale)
{
	int32 DRAGGER_SIZE = 10;
	// Draw all the data that needs to be drawn
	BRect bounds = BRect(mag_scale * settings->left,
		mag_scale * settings->top,
		mag_scale * (settings->right + 1) - 1,
		mag_scale * (settings->bottom + 1) - 1);
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

	if ((f_bottom - f_top > 3 * DRAGGER_SIZE + 10) &&
		(f_right - f_left > 3 * DRAGGER_SIZE + 10))
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

		dragger_rect.OffsetTo(bounds.LeftTop() +
			BPoint(width / 2 - DRAGGER_SIZE / 2, 0));
		view->FillRect(dragger_rect, B_SOLID_HIGH);
		view->StrokeRect(dragger_rect, B_SOLID_LOW);

		dragger_rect.OffsetTo(bounds.LeftTop() +
			BPoint(width - DRAGGER_SIZE + 1, 0));
		view->FillRect(dragger_rect, B_SOLID_HIGH);
		view->StrokeRect(dragger_rect, B_SOLID_LOW);

		dragger_rect.OffsetTo(bounds.LeftTop() +
			BPoint(0, height / 2 - DRAGGER_SIZE / 2));
		view->FillRect(dragger_rect, B_SOLID_HIGH);
		view->StrokeRect(dragger_rect, B_SOLID_LOW);

		dragger_rect.OffsetTo(bounds.RightTop() +
			BPoint(-DRAGGER_SIZE + 1, height / 2 - DRAGGER_SIZE / 2));
		view->FillRect(dragger_rect, B_SOLID_HIGH);
		view->StrokeRect(dragger_rect, B_SOLID_LOW);

		dragger_rect.OffsetTo(bounds.LeftBottom() +
			BPoint(0, -DRAGGER_SIZE + 1));
		view->FillRect(dragger_rect, B_SOLID_HIGH);
		view->StrokeRect(dragger_rect, B_SOLID_LOW);

		dragger_rect.OffsetTo(bounds.LeftBottom() +
			BPoint(width / 2 - DRAGGER_SIZE / 2, -DRAGGER_SIZE + 1));
		view->FillRect(dragger_rect, B_SOLID_HIGH);
		view->StrokeRect(dragger_rect, B_SOLID_LOW);

		dragger_rect.OffsetTo(bounds.LeftBottom() +
			BPoint(width - DRAGGER_SIZE + 1, -DRAGGER_SIZE + 1));
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


BBitmap*
CropManipulator::ManipulateBitmap(ManipulatorSettings* set,
	BBitmap* original, BStatusBar* status_bar)
{
	CropManipulatorSettings* new_settings =
		dynamic_cast<CropManipulatorSettings*> (set);

	if (new_settings == NULL)
		return NULL;

	float left = new_settings->left;
	float right = new_settings->right;
	float top = new_settings->top;
	float bottom = new_settings->bottom;

	float width = right - left;
	float height = bottom - top;

	if (width == original->Bounds().Width() &&
		height == original->Bounds().Height() &&
		top == original->Bounds().top &&
		left == original->Bounds().left)
		return NULL;

	// Create a new bitmap here and copy the appropriate part from original.
	// Return the new bitmap.
	BBitmap* new_bitmap;
	new_bitmap = new BBitmap(BRect(0, 0, right - left, bottom - top), B_RGBA32);
	if (new_bitmap->IsValid() == FALSE)
		throw std::bad_alloc();

	uint32* orig_bits = (uint32*)original->Bits();
	uint32* new_bits = (uint32*)new_bitmap->Bits();
	int32 original_bpr = original->BytesPerRow() / 4;
	int32 new_bpr = new_bitmap->BytesPerRow() / 4;
	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta", 0.0);

	union color_conversion background;

	background.bytes[0] = 0xFF;
	background.bytes[1] = 0xFF;
	background.bytes[2] = 0xFF;
	background.bytes[3] = 0x00;

	int32 new_y = 0;
	for (int32 y = (int32)top; y <= bottom; y++) {
		int32 new_x = 0;
		for (int32 x = (int32)left; x <= right; x++) {
			if (x >= 0 && y >= 0 &&
				x <= original->Bounds().right &&
				y <= original->Bounds().bottom)
				*(new_bits + new_x + new_y * new_bpr) =
					*(orig_bits + x + y * original_bpr);
			else
				*(new_bits + new_x + new_y * new_bpr) = background.word;
			++new_x;
		}
		if ((y % 10 == 0) && (status_bar != NULL)) {
			progress_message.ReplaceFloat("delta", 100.0 / (height + 1) * 10.0);
			status_bar->Window()->PostMessage(&progress_message, status_bar);
		}
		++new_y;
	}

	return new_bitmap;
}


int32
CropManipulator::PreviewBitmap(bool, BRegion* updated_region)
{
	int32* target_bits = (int32*)preview_bitmap->Bits();
	int32* source_bits = (int32*)copy_of_the_preview_bitmap->Bits();
	int32 target_bpr = preview_bitmap->BytesPerRow() / 4;
	int32 source_bpr = copy_of_the_preview_bitmap->BytesPerRow() / 4;

	union color_conversion mask_color;

	mask_color.bytes[0] = 0x00;
	mask_color.bytes[1] = 0x00;
	mask_color.bytes[2] = 0x00;
	mask_color.bytes[3] = 0x5F;

	float height = preview_bitmap->Bounds().Height();
	float width = preview_bitmap->Bounds().Width();

	if (use_selected == TRUE && selection != NULL && !selection->IsEmpty()) {
		BRect selection_bounds = selection->GetBoundingRect();
		SetValues(selection_bounds.left, selection_bounds.Width(),
			selection_bounds.top, selection_bounds.Height());
	}

	use_selected = FALSE;

	BRect bounds = BRect(settings->left,
		settings->top,
		(settings->right + 1) - 1,
		(settings->bottom + 1) - 1);
	bounds.left = floor(bounds.left);
	bounds.top = floor(bounds.top);
	bounds.right = ceil(bounds.right);
	bounds.bottom = ceil(bounds.bottom);

	for (int32 y = 0; y < height; ++y) {
		for (int32 x = 0; x < width; ++x) {
			if (bounds.Contains(BPoint(x, y)) == TRUE)
				*(target_bits + x + y * target_bpr) =
					*(source_bits + x + y * source_bpr);
			else
				*(target_bits + x + y * target_bpr) =
					src_over_fixed(*(source_bits + x + y * source_bpr),
					mask_color.word);
		}
	}

	updated_region->Set(preview_bitmap->Bounds());

	return 1;
}


void
CropManipulator::SetPreviewBitmap(BBitmap* bm)
{
	if (bm != NULL) {
		BRect rect = bm->Bounds();
		settings->left = min_x = rect.left;
		settings->right = max_x = rect.right + 1;
		settings->top = min_y = rect.top;
		settings->bottom = max_y = rect.bottom + 1;
	} else {
		settings->left = min_x = 0;
		settings->right = max_x = 0;
		settings->top = min_y = 0;
		settings->bottom = max_y = 0;
	}

	if ((bm == NULL) || (preview_bitmap == NULL) ||
		(bm->Bounds() != preview_bitmap->Bounds())) {
		try {
			if (preview_bitmap != NULL) {
				delete copy_of_the_preview_bitmap;
			}
			if (bm != NULL) {
				preview_bitmap = bm;
				copy_of_the_preview_bitmap = DuplicateBitmap(preview_bitmap);
				BRect bounds = preview_bitmap->Bounds();
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
		preview_bitmap = bm;
		uint32* source = (uint32*)preview_bitmap->Bits();
		uint32* target = (uint32*)copy_of_the_preview_bitmap->Bits();
		int32 bitslength = min_c(preview_bitmap->BitsLength(),
			copy_of_the_preview_bitmap->BitsLength());
		memcpy(target, source, bitslength);
	}

	if (config_view != NULL) {
		config_view->SetValues(settings->left, settings->right,
			settings->top, settings->bottom);
	}
}


void
CropManipulator::Reset()
{
	BRect bounds = preview_bitmap->Bounds();

	settings->left = bounds.left;
	settings->right = bounds.right + 1;
	settings->top = bounds.top;
	settings->bottom = bounds.bottom + 1;

	previous_left = settings->left;
	previous_right = settings->right;
	previous_top = settings->top;
	previous_bottom = settings->bottom;

	if (config_view != NULL) {
		config_view->SetValues(settings->left, settings->right,
			settings->top, settings->bottom);
	}
}


void
CropManipulator::SetValues(float x, float width, float y, float height)
{
	previous_left = settings->left;
	previous_right = settings->right;
	previous_top = settings->top;
	previous_bottom = settings->bottom;

	settings->left = x;
	settings->right = x + width;
	settings->top = y;
	settings->bottom = y + height;

	settings->left = min_c(settings->left, settings->right);
	settings->right = max_c(settings->left, settings->right);

	settings->top = min_c(settings->top, settings->bottom);
	settings->bottom = max_c(settings->top, settings->bottom);

	if (config_view != NULL) {
		config_view->SetValues(settings->left, settings->right,
			settings->top, settings->bottom);
	}
}


ManipulatorSettings*
CropManipulator::ReturnSettings()
{
	return new CropManipulatorSettings(settings);
}


BView*
CropManipulator::MakeConfigurationView(const BMessenger& target)
{
	config_view = new CropManipulatorView(this, target);
	config_view->SetValues(settings->left, settings->right, settings->top,
		settings->bottom);
	return config_view;
}


const char*
CropManipulator::ReturnName()
{
	return B_TRANSLATE("Crop" B_UTF8_ELLIPSIS);
}


const char*
CropManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Crop: Use handles or number-fields to set new borders. SHIFT locks aspect ratio.");
}


// #pragma mark -- CropManipulatorView


CropManipulatorView::CropManipulatorView(CropManipulator* manipulator,
		const BMessenger& target)
	: WindowGUIManipulatorView(),
	fTarget(target),
	fManipulator(manipulator)
{
	fTopCrop = new NumberControl(B_TRANSLATE("Top:"), "",
		new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), 5, true);
	fLeftCrop = new NumberControl(B_TRANSLATE("Left:"), "",
		new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), 5, true);
	fRightCrop = new NumberControl(B_TRANSLATE("Width:"),
		"", new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), 5, false);
	fBottomCrop = new NumberControl(B_TRANSLATE("Height:"),
		"", new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), 5, false);

	BFont font;
	font_height height;
	font.GetHeight(&height);
	BString selectString = B_TRANSLATE("Use selection");

	float buttonWidth = font.StringWidth(selectString);
	BSize buttonSize = BSize(buttonWidth * 1.5,
		(height.ascent + height.descent + height.leading) * 1.5);

	fSelected = new BButton(B_TRANSLATE(selectString),
		new BMessage(CROP_TO_SELECTION));
	fReset = new BButton(B_TRANSLATE("Reset to canvas"),
		new BMessage(RESET_CROP));

	fSelected->SetExplicitMaxSize(buttonSize);
	fSelected->SetExplicitMinSize(buttonSize);
	fReset->SetExplicitMaxSize(buttonSize);
	fReset->SetExplicitMinSize(buttonSize);

	fLockAspect = new BCheckBox(B_TRANSLATE("Lock"),
		new BMessage(TOGGLE_LOCK_ASPECT));

	BGridLayout* mainLayout = BLayoutBuilder::Grid<>(this, 5.0, 5.0)
			.Add(fLeftCrop->CreateLabelLayoutItem(), 0, 0)
			.Add(fLeftCrop->CreateTextViewLayoutItem(), 1, 0)
			.Add(fTopCrop->CreateLabelLayoutItem(), 0, 1)
			.Add(fTopCrop->CreateTextViewLayoutItem(), 1, 1)
			.Add(fRightCrop->CreateLabelLayoutItem(), 0, 2)
			.Add(fRightCrop->CreateTextViewLayoutItem(), 1, 2)
			.Add(fBottomCrop->CreateLabelLayoutItem(), 0, 3)
			.Add(fBottomCrop->CreateTextViewLayoutItem(), 1, 3)
			.Add(fSelected, 2, 0)
			.Add(fReset, 2, 1)
			.Add(fLockAspect, 2, 2, 1, 2);
	mainLayout->SetMaxColumnWidth(0, font.StringWidth("LABELLABELLABEL"));
	mainLayout->SetMinColumnWidth(1, font.StringWidth("01234"));
	mainLayout->SetMaxColumnWidth(2, buttonWidth);

	CropManipulatorSettings* settings
		= (CropManipulatorSettings*)manipulator->ReturnSettings();
	fTopCrop->SetValue(settings->top);
	fLeftCrop->SetValue(settings->left);
	fRightCrop->SetValue(settings->right - settings->left);
	fBottomCrop->SetValue(settings->bottom - settings->top);

	delete settings;
}


void
CropManipulatorView::AttachedToWindow()
{
	fTopCrop->SetTarget(this);
	fLeftCrop->SetTarget(this);
	fRightCrop->SetTarget(this);
	fBottomCrop->SetTarget(this);

	fSelected->SetTarget(this);
	fReset->SetTarget(this);
	fLockAspect->SetTarget(this);

	fLeftCrop->MakeFocus(true);

	WindowGUIManipulatorView::AttachedToWindow();
}


void
CropManipulatorView::SetValues(float x1, float x2, float y1, float y2)
{
	BWindow* window = Window();
	if (window && window->Lock()) {
		left = x1;
		right = x2;
		top = y1;
		bottom = y2;

		if (left != fLeftCrop->Value())
			fLeftCrop->SetValue(int32(left));

		if (top != fTopCrop->Value())
			fTopCrop->SetValue(int32(top));

		if ((right - left) != fRightCrop->Value())
			fRightCrop->SetValue(int32(right - left));

		if ((bottom - top) != fBottomCrop->Value())
			fBottomCrop->SetValue(int32(bottom - top));

		window->Unlock();
	}
}


void
CropManipulatorView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case HS_MANIPULATOR_ADJUSTING_FINISHED: {
			if (fLockAspect->Value() == B_CONTROL_ON) {
				CropManipulatorSettings* settings = (CropManipulatorSettings*)(fManipulator->ReturnSettings());

				float width = settings->right - settings->left;
				float height = settings->bottom - settings->top;
				float aspect = width / height;
				int32 rightValue = (int32)fRightCrop->Value();
				int32 bottomValue = (int32)fBottomCrop->Value();

				if ((int32)settings->right != rightValue)
					fBottomCrop->SetValue(int32(rightValue / aspect));
				else if ((int32)settings->bottom != bottomValue)
					fRightCrop->SetValue(int32(bottomValue * aspect));
			}

			fManipulator->SetValues(fLeftCrop->Value(),
				fRightCrop->Value(), fTopCrop->Value(),
				fBottomCrop->Value());
			fTarget.SendMessage(message);
		} break;

		case CROP_TO_SELECTION: {
			fManipulator->UseSelected();
			fTarget.SendMessage(
				new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED));
		} break;

		case RESET_CROP: {
			fManipulator->Reset();
			fTarget.SendMessage(
				new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED));
		} break;

		case TOGGLE_LOCK_ASPECT: {
			if (fLockAspect->Value() == B_CONTROL_ON)
				fManipulator->LockAspect(TRUE);
			else
				fManipulator->LockAspect(FALSE);
		}

		default: {
			WindowGUIManipulatorView::MessageReceived(message);
		} break;
	}
}
