/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "FlipManipulator.h"
#include "ImageView.h"
#include "MessageConstants.h"
#include "Selection.h"


#include <AppDefs.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Message.h>
#include <StatusBar.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Manipulators"


HorizFlipManipulator::HorizFlipManipulator()
	:
	Manipulator(),
	transform_selection_only(false)
{
}


BBitmap*
HorizFlipManipulator::ManipulateBitmap(BBitmap* original, BStatusBar* status_bar)
{
	BRect bounds = original->Bounds();

	if (selection != NULL && selection->IsEmpty() == false)
		bounds = selection->GetBoundingRect();

	int32 height = bounds.IntegerHeight();
	int32 width = bounds.IntegerWidth();
	int32 width_per_2 = width / 2;
	uint32 left_bits, right_bits;
	uint32* bits = (uint32*)original->Bits();
	uint32 bpr = original->BytesPerRow() / 4;

	int32 left = bounds.left;
	int32 right = bounds.right;
	int32 right_per_2 = bounds.left + width_per_2;
	int32 top = bounds.top;
	int32 bottom = bounds.bottom;

	bool has_selection = true;
	if (selection == NULL || selection->IsEmpty() == true)
		has_selection = false;

	if (transform_selection_only == false) {
		for (int32 y = top; y <= bottom; y++) {
			for (int32 x = left; x <= right_per_2; x++) {
				uint32* bit = bits + y * bpr;
				int32 right_x = left + right - x;
				right_bits = *(bit + right_x);
				left_bits = *(bit + x);
				if (has_selection == true) {
					if (selection->ContainsPoint(x, y))
						*(bit + x) = 0;
					if (selection->ContainsPoint(right_x, y))
						*(bit + right_x) = 0;

					if (selection->ContainsPoint(x, y))
						*(bit + right_x) = left_bits;
					if (selection->ContainsPoint(right_x, y))
						*(bit + x) = right_bits;
				} else {
					*(bit + x) = right_bits;
					*(bit + right_x) = left_bits;
				}
			}

			if ((y % 40 == 0) && (status_bar != NULL) && (status_bar->Window() != NULL)) {
				BMessage* a_message = new BMessage(B_UPDATE_STATUS_BAR);
				a_message->AddFloat("delta", 40.0 * 100.0 / (float)height);
				status_bar->Window()->PostMessage(a_message, status_bar);
				delete a_message;
			}
		}
	}

	if (has_selection == true) {
		BBitmap* sel_map = ManipulateSelectionBitmap();
		selection->ReplaceSelection(sel_map);
	}

	return original;
}


BBitmap*
HorizFlipManipulator::ManipulateSelectionBitmap()
{
	BRect bounds = selection->GetBoundingRect();
	BBitmap* selection_map = new BBitmap(selection->ReturnSelectionMap());

	//int32 height = bounds.IntegerHeight();
	int32 width = bounds.IntegerWidth();
	int32 width_per_2 = width / 2;
	uint8 left_bits, right_bits;
	uint8* bits = (uint8*)selection_map->Bits();
	uint32 bpr = selection_map->BytesPerRow();

	int32 left = bounds.left;
	int32 right = bounds.right;
	int32 right_per_2 = bounds.left + width_per_2;
	int32 top = bounds.top;
	int32 bottom = bounds.bottom;

	for (int32 y = top; y <= bottom; y++) {
		for (int32 x = left; x <= right_per_2; x++) {
			uint8* bit = bits + y * bpr;
			int32 right_x = left + right - x;
			right_bits = *(bit + right_x);
			left_bits = *(bit + x);
			*(bit + x) = right_bits;
			*(bit + right_x) = left_bits;

			if (selection->ContainsPoint(x, y))
				*(bit + x) = 0;
			if (selection->ContainsPoint(right_x, y))
				*(bit + right_x) = 0;

			if (selection->ContainsPoint(x, y))
				*(bit + right_x) = left_bits;
			if (selection->ContainsPoint(right_x, y))
				*(bit + x) = right_bits;
		}
	}

	return selection_map;
}


const char*
HorizFlipManipulator::ReturnName()
{
	if (transform_selection_only == true)
		return B_TRANSLATE("Flip selection horizontally");

	return B_TRANSLATE("Flip horizontally");
}


VertFlipManipulator::VertFlipManipulator()
	:
	Manipulator(),
	transform_selection_only(false)
{
}


BBitmap*
VertFlipManipulator::ManipulateBitmap(BBitmap* original, BStatusBar* status_bar)
{
	BRect bounds = original->Bounds();

	if (selection != NULL && selection->IsEmpty() == false)
		bounds = selection->GetBoundingRect();

	int32 height = bounds.IntegerHeight();
	int32 height_per_2 = height / 2;
	uint32 top_bits, bottom_bits;
	uint32* bits = (uint32*)original->Bits();
	uint32 bpr = original->BytesPerRow() / 4;

	int32 left = bounds.left;
	int32 right = bounds.right;
	int32 top = bounds.top;
	int32 bottom = bounds.bottom;
	int32 bottom_per_2 = bounds.top + height_per_2;

	bool has_selection = true;
	if (selection == NULL || selection->IsEmpty() == true)
		has_selection = false;

	if (transform_selection_only == false) {
		for (int32 y = top; y <= bottom_per_2; y++) {
			for (int32 x = left; x <= right; x++) {
				uint32* bit = bits; // + y * bpr;
				int32 bottom_y = top + bottom - y;
				bottom_bits = *(bit + x + bottom_y * bpr);
				top_bits = *(bit + x + y * bpr);
				if (has_selection == true) {
					if (selection->ContainsPoint(x, y))
						*(bit + x + y * bpr) = 0;
					if (selection->ContainsPoint(x, bottom_y))
						*(bit + x + bottom_y * bpr) = 0;

					if (selection->ContainsPoint(x, y))
						*(bit + x + bottom_y * bpr) = top_bits;
					if (selection->ContainsPoint(x, bottom_y))
						*(bit + x + y * bpr) = bottom_bits;
				} else {
					*(bit + x + y * bpr) = bottom_bits;
					*(bit + x + bottom_y * bpr) = top_bits;
				}
			}

			if ((y % 40 == 0) && (status_bar != NULL) && (status_bar->Window() != NULL)) {
				BMessage* a_message = new BMessage(B_UPDATE_STATUS_BAR);
				a_message->AddFloat("delta", 40.0 * 100.0 / (float)height_per_2);
				status_bar->Window()->PostMessage(a_message, status_bar);
				delete a_message;
			}
		}
	}

	if (has_selection == true) {
		BBitmap* sel_map = ManipulateSelectionBitmap();
		selection->ReplaceSelection(sel_map);
	}

	return original;
}


BBitmap*
VertFlipManipulator::ManipulateSelectionBitmap()
{
	BRect bounds = selection->GetBoundingRect();

	BBitmap* selection_map = new BBitmap(selection->ReturnSelectionMap());
	int32 height = bounds.IntegerHeight();
	int32 height_per_2 = height / 2;
	uint8 top_bits, bottom_bits;
	uint8* bits = (uint8*)selection_map->Bits();
	uint32 bpr = selection_map->BytesPerRow();

	int32 left = bounds.left;
	int32 right = bounds.right;
	int32 top = bounds.top;
	int32 bottom = bounds.bottom;
	int32 bottom_per_2 = bounds.top + height_per_2;

	for (int32 y = top; y <= bottom_per_2; y++) {
		for (int32 x = left; x <= right; x++) {
			uint8* bit = bits; // + y * bpr;
			int32 bottom_y = top + bottom - y;
			bottom_bits = *(bit + x + bottom_y * bpr);
			top_bits = *(bit + x + y * bpr);
			*(bit + x + y * bpr) = bottom_bits;
			*(bit + x + bottom_y * bpr) = top_bits;

			if (selection->ContainsPoint(x, y))
				*(bit + x + y * bpr) = 0;
			if (selection->ContainsPoint(x, bottom_y))
				*(bit + x + bottom_y * bpr) = 0;

			if (selection->ContainsPoint(x, y))
				*(bit + x + bottom_y * bpr) = top_bits;
			if (selection->ContainsPoint(x, bottom_y))
				*(bit + x + y * bpr) = bottom_bits;
		}
	}

	return selection_map;
}


const char*
VertFlipManipulator::ReturnName()
{
	if (transform_selection_only == true)
		return B_TRANSLATE("Flip selection vertically");

	return B_TRANSLATE("Flip vertically");
}
