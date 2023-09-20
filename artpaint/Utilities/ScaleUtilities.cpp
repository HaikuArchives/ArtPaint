/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "ScaleUtilities.h"


void
ScaleUtilities::ScaleHorizontally(float width, float height, BPoint offset, BBitmap* source,
	BBitmap* target, float ratio, interpolation_type method)
{
	uint32* target_bits = (uint32*)target->Bits();
	int32 target_bpr = target->BytesPerRow() / 4;
	uint32* source_bits = (uint32*)source->Bits();
	int32 source_bpr = source->BytesPerRow() / 4;

	for (int32 y = 0; y < (int32)ceil(height + 0.5); y++) {
		uint32* src_bits = source_bits + (int32)offset.x + (y + (int32)offset.y) * source_bpr;

		for (int32 x = 0; x <= (int32)width; x++) {
			int32 low = floor(ratio * x);
			int32 high = ceil(ratio * x);
			float weight = ratio * x - low;

			switch (method) {
				case NEAREST_NEIGHBOR:
				{
					*(target_bits + x + y * target_bpr)
						= nearest_neighbor(*(src_bits + low),
						*(src_bits + high), weight);
				} break;
				case BILINEAR:
				{
					*(target_bits + x + y * target_bpr)
						= linear_interpolation(*(src_bits + low),
						*(src_bits + high), weight);
				} break;
				case BICUBIC:
				case BICUBIC_CATMULL_ROM:
				case BICUBIC_BSPLINE:
				case MITCHELL:
				{
					int32 low0 = ceil(ratio * (x - 1));
					if (x == 0)
						low0 = low;

					int32 high2 = floor(ratio * (x + 1));
					if (x == width)
						high2 = high;

					// default MITCHELL
					float B = 1. / 3.;
					float C = 1. / 3.;
					if (method == BICUBIC) {
						B = 0.;
						C = 0.75;
					} else if (method == BICUBIC_CATMULL_ROM) {
						B = 0.;
						C = 0.5;
					} else if (method == BICUBIC_BSPLINE) {
						B = 1.0;
						C = 0.;
					}

					*(target_bits + x + y * target_bpr)
						= mitchell_netravali(*(src_bits + low0),
							*(src_bits + low),
							*(src_bits + high),
							*(src_bits + high2),
							weight, B, C);
				} break;
			}
		}
	}
}


void
ScaleUtilities::ScaleVertically(float width, float height, BPoint offset, BBitmap* source,
	BBitmap* target, float ratio, interpolation_type method)
{
	uint32* target_bits = (uint32*)target->Bits();
	int32 target_bpr = target->BytesPerRow() / 4;
	uint32* source_bits = (uint32*)source->Bits();
	int32 source_bpr = source->BytesPerRow() / 4;

	for (int32 y = 0; y <= (int32)height; y++) {
		int32 low = floor(ratio * y);
		int32 high = ceil(ratio * y);
		float weight = (ratio * y) - low;
		uint32* src_bits_low = source_bits + (int32)offset.x + (low + (int32)ceil(offset.y + 0.5)) * source_bpr;
		uint32* src_bits_high = source_bits + (int32)offset.x + (high + (int32)ceil(offset.y + 0.5)) * source_bpr;

		for (int32 x = 0; x <= width; x++) {
			switch (method) {
				case NEAREST_NEIGHBOR:
				{
					*(target_bits + x + y * target_bpr)
						= nearest_neighbor(*(src_bits_low + x),
							*(src_bits_high + x),
							weight);
				} break;
				case BILINEAR:
				{
					*(target_bits + x + y * target_bpr)
						= linear_interpolation(*(src_bits_low + x),
							*(src_bits_high + x),
							weight);
				} break;
				case BICUBIC:
				case BICUBIC_CATMULL_ROM:
				case BICUBIC_BSPLINE:
				case MITCHELL:
				{
					int32 low0;

					low0 = ceil(ratio * (y - 1));
					if (y == 0)
						low0 = low;

					int32 high2 = floor(ratio * (y + 1));
					if (y == height)
						high2 = high;

					uint32* src_bits_low0 = source_bits + (int32)offset.x
						+ (low0 + (int32)ceil(offset.y + 0.5)) * source_bpr;
					uint32* src_bits_high2 = source_bits + (int32)offset.x
						+ (high2 + (int32)ceil(offset.y + 0.5)) * source_bpr;

					// default MITCHELL
					float B = 1. / 3.;
					float C = 1. / 3.;
					if (method == BICUBIC) {
						B = 0.;
						C = 0.75;
					} else if (method == BICUBIC_CATMULL_ROM) {
						B = 0.;
						C = 0.5;
					} else if (method == BICUBIC_BSPLINE) {
						B = 1.0;
						C = 0.;
					}

					*(target_bits + x + y * target_bpr)
						= mitchell_netravali(*(src_bits_low0 + x),
							*(src_bits_low + x),
							*(src_bits_high + x),
							*(src_bits_high2 + x),
							weight, B, C);
				} break;
			}
		}
	}
}


void
ScaleUtilities::ScaleHorizontallyGray(float width, float height, BPoint offset, BBitmap* source,
	BBitmap* target, float ratio)
{
	uint8* target_bits = (uint8*)target->Bits();
	int32 target_bpr = target->BytesPerRow();
	uint8* source_bits = (uint8*)source->Bits();
	int32 source_bpr = source->BytesPerRow();

	for (int32 y = 0; y < (int32)ceil(height + 0.5); y++) {
		uint8* src_bits = source_bits + (int32)offset.x + (y + (int32)offset.y) * source_bpr;

		for (int32 x = 0; x < (int32)width; x++) {
			int32 low = floor(ratio * x);
			int32 high = ceil(ratio * x);
			float weight = ratio * x - low;

			*(target_bits + x + y * target_bpr)
				= linear_interpolation(*(src_bits + low),
					*(src_bits + high), weight);
		}
	}
}


void
ScaleUtilities::ScaleVerticallyGray(float width, float height, BPoint offset, BBitmap* source,
	BBitmap* target, float ratio)
{
	uint8* target_bits = (uint8*)target->Bits();
	int32 target_bpr = target->BytesPerRow();
	uint8* source_bits = (uint8*)source->Bits();
	int32 source_bpr = source->BytesPerRow();

	for (int32 y = 0; y <= (int32)height; y++) {
		int32 low = floor(ratio * y);
		int32 high = ceil(ratio * y);
		float weight = (ratio * y) - low;
		uint8* src_bits_low = source_bits + (int32)offset.x + (low + (int32)ceil(offset.y + 0.5)) * source_bpr;
		uint8* src_bits_high = source_bits + (int32)offset.x + (high + (int32)ceil(offset.y + 0.5)) * source_bpr;

		for (int32 x = 0; x <= width; x++) {
			*(target_bits + x + y * target_bpr)
				= linear_interpolation(*(src_bits_low + x),
					*(src_bits_high + x), weight);
		}
	}
}


void
ScaleUtilities::MoveGrabbers(BPoint point, BPoint& previous, float& left, float& top, float& right,
	float& bottom, float aspect_ratio, bool& move_left, bool& move_top, bool& move_right,
	bool& move_bottom, bool& move_all, bool first_click, bool lock_aspect)
{
	if (first_click == TRUE) {
		// Here we select which grabbles to move
		move_left = move_right = move_top = move_bottom = FALSE;
		if (fabs(point.x - left) < 50) {
			if (fabs(point.x - left) < fabs(point.x - (left + (right - left) / 2)))
				move_left = TRUE;
		}
		if (fabs(point.x - right) < 50) {
			if (fabs(point.x - right) < fabs(point.x - (left + (right - left) / 2)))
				move_right = TRUE;
		}
		if ((move_left == TRUE) && (move_right == TRUE)) {
			if (fabs(point.x - left) > fabs(point.x - right))
				move_left = FALSE;
			else
				move_right = FALSE;
		}

		if (fabs(point.y - top) < 50) {
			if (fabs(point.y - top) < fabs(point.y - (top + (bottom - top) / 2)))
				move_top = TRUE;
		}
		if (fabs(point.y - bottom) < 50) {
			if (fabs(point.y - bottom) < fabs(point.y - (top + (bottom - top) / 2)))
				move_bottom = TRUE;
		}
		if ((move_top == TRUE) && (move_bottom == TRUE)) {
			if (fabs(point.y - top) > fabs(point.y - bottom))
				move_top = FALSE;
			else
				move_bottom = FALSE;
		}

		if (move_left == FALSE && move_top == FALSE && move_right == FALSE && move_bottom == FALSE)
			move_all = TRUE;
		else
			move_all = FALSE;

		previous.x = point.x;
		previous.y = point.y;
	} else {
		if (move_all == TRUE) {
			float width = right - left;
			float height = bottom - top;

			float delta_x = previous.x - point.x;
			float delta_y = previous.y - point.y;

			float new_left, new_top;

			new_left = left - delta_x;
			new_top = top - delta_y;

			left = new_left;
			top = new_top;
			right = new_left + width;
			bottom = new_top + height;

			previous.x = point.x;
			previous.y = point.y;
		} else {
			float old_width = right - left;
			float old_height = bottom - top;

			if (move_left == TRUE)
				left = min_c(point.x, right);
			if (move_right == TRUE)
				right = max_c(left, point.x);

			if (move_top == TRUE)
				top = min_c(point.y, bottom);
			if (move_bottom == TRUE)
				bottom = max_c(top, point.y);

			if (lock_aspect == TRUE || modifiers() & B_LEFT_SHIFT_KEY) {
				float new_width = right - left;
				float new_height = bottom - top;

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
						top = bottom - new_height;
					else
						bottom = top + new_height;
					right = left + new_width;
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
						left = right - new_width;
					else
						right = left + new_width;
					bottom = top + new_height;
				} else if (move_top == TRUE) {
					if (new_aspect < aspect_ratio) {
						new_height = new_width / aspect_ratio;
						top = bottom - new_height;
					} else {
						new_width = new_height * aspect_ratio;
						if (move_left == TRUE)
							left = right - new_width;
						else
							right = left + new_width;
					}
				} else if (move_bottom == TRUE) {
					if (new_aspect < aspect_ratio) {
						new_height = new_width / aspect_ratio;
						bottom = top + new_height;
					} else {
						new_width = new_height * aspect_ratio;
						if (move_left == TRUE)
							left = right - new_width;
						else
							right = left + new_width;
					}
				}
			}
		}
	}

	if (left >= right)
		left = right - 1;

	if (top >= bottom)
		top = bottom - 1;
}
