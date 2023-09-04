/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include <algorithm>
#include <stdio.h>


#include "BitmapDrawer.h"
#include "HSPolygon.h"
#include "PixelOperations.h"
#include "Selection.h"


BitmapDrawer::BitmapDrawer(BBitmap* bitmap)
{
	bitmap_bounds = bitmap->Bounds();

	bitmap_bits = (uint32*)bitmap->Bits();
	bitmap_bpr = bitmap->BytesPerRow() / 4;
	bitmap_data_length = bitmap->BitsLength();
}


status_t
BitmapDrawer::DrawHairLine(BPoint start, BPoint end, uint32 color, bool anti_alias, Selection* sel,
	uint32 (*composite_func)(uint32, uint32))
{
	// This function only draws lines with width of 1.
	// Always draw the lines from left to right or from top to bottom.
	float start_x = min_c(start.x, end.x);
	float start_y = min_c(start.y, end.y);

	float end_x = max_c(start.x, end.x);
	float end_y = max_c(start.y, end.y);
	float dx = end_x - start_x;
	float dy = end_y - start_y;
	float sign_y = 0.0, sign_x = 0.0;
	if (dx > dy) {
		if (end.y != start.y)
			sign_y = (start_x == start.x ? (end.y - start.y) / fabs(start.y - end.y)
										 : (start.y - end.y) / fabs(end.y - start.y));
		else
			sign_y = 0;
	} else {
		if (start.x != end.x)
			sign_x = (start_y == start.y ? (end.x - start.x) / fabs(end.x - start.x)
										 : (start.x - end.x) / fabs(start.x - end.x));
		else
			sign_x = 0;
	}
	uint32 old_value;
	uint32 new_value;

	if (anti_alias == TRUE) {
		// Here we should take the line's equation and draw with Wu's algorithm.
		if (dx > dy) {
			// Draw the line horizontally. For every x-coordinate we mix the color with
			// two pixels at positions x,floor(y) and x,ceil(y).
			float step_y = sign_y * dy / dx;
			float y = ((start_x == start.x) ? start.y : end.y);
			float y_mix_upper, y_mix_lower;

			for (float x = start_x; x <= end_x; x++) {
				y_mix_upper = y - floor(y);
				y_mix_lower = 1.0 - y_mix_upper;

				// This is the real Wu's two point anti-aliasing scheme.
				union color_conversion color1, color2;
				color1.word = color2.word = color;

				uint8 alpha = round((y_mix_lower) * color1.bytes[3]);
				color1.bytes[3] = alpha;
				color2.bytes[3] += -alpha;

				SetPixel(BPoint(x, ceil(y)), color2.word, sel, composite_func);
				SetPixel(BPoint(x, floor(y)), color1.word, sel, composite_func);
				y += step_y;
			}
		} else {
			// Draw the line vertically.
			float step_x = sign_x * dx / dy;
			float x = ((start_y == start.y) ? start.x : end.x);
			float x_mix_left, x_mix_right;

			for (float y = start_y; y <= end_y; y++) {
				x_mix_left = ceil(x) - x;
				x_mix_right = 1.0 - x_mix_left;

				// This is the real Wu's two point anti-aliasing scheme.
				union color_conversion color1, color2;
				color1.word = color2.word = color;

				uint8 alpha = round((x_mix_right) * color1.bytes[3]);
				color1.bytes[3] = alpha;
				color2.bytes[3] += -alpha;

				SetPixel(BPoint(ceil(x), y), color1.word, sel, composite_func);
				SetPixel(BPoint(floor(x), y), color2.word, sel, composite_func);

				x += step_x;
			}
		}
	} else {
		// use DDA-algorithm to calculate line between the two argument points
		// first check whether the line is longer in x direction than y
		bool increase_x = fabs(start.x - end.x) >= fabs(start.y - end.y);
		// check which direction the line is going
		float sign_x;
		float sign_y;

		if ((end.x - start.x) != 0)
			sign_x = (end.x - start.x) / fabs(end.x - start.x);
		else
			sign_x = 0;

		if ((end.y - start.y) != 0)
			sign_y = (end.y - start.y) / fabs(end.y - start.y);
		else
			sign_y = 0;

		if (increase_x) {
			// allocate the point-list
			int32 number_of_points = (int32)fabs(start.x - end.x) + 1;
			float y_add = ((float)fabs(start.y - end.y)) / ((float)fabs(start.x - end.x));
			for (int32 i = 0; i < number_of_points; i++) {
				SetPixel(start, color, sel, composite_func);

				start.x += sign_x;
				start.y += sign_y * y_add;
			}
		} else {
			int32 number_of_points = (int32)fabs(start.y - end.y) + 1;
			float x_add = ((float)fabs(start.x - end.x)) / ((float)fabs(start.y - end.y));
			for (int32 i = 0; i < number_of_points; i++) {
				SetPixel(start, color, sel, composite_func);

				start.y += sign_y;
				start.x += sign_x * x_add;
			}
		}
	}

	return B_OK;
}


status_t
BitmapDrawer::DrawLine(BPoint start, BPoint end, uint32 color, float width, bool anti_alias,
	Selection* sel, uint32 (*composite_func)(uint32, uint32))
{
	// The line width is split in to two parts
	float distance1_from_center = ceil((width - 1.0) / 2.0);
	float distance2_from_center = floor((width - 1.0) / 2.0);

	// This function hangs when the start and end points were the same.
	if (start == end)
		return B_ERROR;

	// Then we take the normal vector of (end - start) by using cross product.
	// Let's pretend that a = (end-start) is a vector in three dimensional
	// vector space. The normal for it (in two dimensions) can be obtained by:
	//
	//			|	i	j	k	|
	//	a'xa =	| a'.x a'.y	1	| 	=	(a'.y*0 - 1*a.y)i - (a'.x*0 - 1*a.x)k + (a'.x*a.y - a'.y*a.x)k
	//			|	a.x	a.y	0	|
	//
	// We then ignore the k component.
	BPoint normal;
	normal.x = -(end - start).y;
	normal.y = (end - start).x;

	// Then we normalize the normal vector
	float normal_vector_length = sqrt((normal.x * normal.x) + (normal.y * normal.y));
	normal.x = normal.x / normal_vector_length;
	normal.y = normal.y / normal_vector_length;

	// Change the normal vector to point upward.
	if (normal.y < 0) {
		normal = BPoint(0, 0) - normal;
	}

	BPoint normal1;
	BPoint normal2;

	normal1.x = distance1_from_center * normal.x;
	normal1.y = distance1_from_center * normal.y;
	normal2.x = distance2_from_center * normal.x;
	normal2.y = distance2_from_center * normal.y;

	// Then we add and subtract the normal vector multiplied by scalars of width from
	// the end and start-points to get the four corners of our rectangle.
	BPoint point_list[4];
	point_list[0] = start + normal1;
	point_list[1] = end + normal1;
	point_list[2] = end - normal2;
	point_list[3] = start - normal2;

	// Then we fill the rectangle.
	DrawRectanglePolygon(point_list, color, TRUE, anti_alias, sel, composite_func);

	return B_OK;
}


status_t
BitmapDrawer::DrawCircle(BPoint center, float radius, uint32 color, bool fill, bool anti_alias,
	Selection* sel, uint32(*composite_func)(uint32, uint32))
{
	// For the moment we only do non-anti-aliased circles. So radius should be a whole number.

	BRect circleRect(center.x - radius, center.y - radius, center.x + radius, center.y + radius);
	DrawEllipse(circleRect, color, fill, anti_alias, sel, 0, composite_func);

	return B_OK;
}


status_t
BitmapDrawer::DrawEllipse(BRect rect, uint32 color,
	bool fill, bool anti_alias, Selection *sel, float angle,
	uint32 (*composite_func)(uint32, uint32))
{
	BPoint center;
	center.x = floor(rect.left + (rect.right-rect.left) / 2.0);
	center.y = floor(rect.top + (rect.bottom-rect.top) / 2.0);

	float radius1 = rect.Width() / 2.;
	float radius2 = rect.Height() / 2.;

	if (radius1 == radius2 || (int32)angle % 180 == 0)
		_DrawShearedEllipse(center, radius1, radius2, color, fill, anti_alias, 1, 0, sel, composite_func);
	else {
		if (abs(angle) > 45) {
			uint32 temp = radius1;
			radius1 = radius2;
			radius2 = temp;

			angle += 90;
		}
		angle = angle * M_PI / 180.;
		float theta = atan((radius2 / radius1) * (-1. * tan(angle)));
		float shear_dx = (radius1 * cos(theta) * cos(angle)) - (radius2 * sin(theta) * sin(angle));
		float shear_dy = (radius1 * cos(theta) * sin(angle)) + (radius2 * sin(theta) * cos(angle));
		float shear_x = abs(shear_dx);
		float shear_y = (radius2 * radius1) / shear_x;
		_DrawShearedEllipse(center, floor(shear_x), floor(shear_y), color, fill, anti_alias, shear_dx, shear_dy, sel, composite_func);
	}

	return B_OK;
}


status_t
BitmapDrawer::_DrawShearedEllipse(BPoint center, float width, float height, uint32 color,
	bool fill, bool anti_alias, float shear_dx, float shear_dy, Selection* sel,
	uint32 (*composite_func)(uint32, uint32))
{
	if (shear_dx != 0) {
		float a_squared = width * width;
		float b_squared = height * height;

		float x = 0;
		float y = height;

		float ffd = a_squared / sqrt(b_squared + a_squared);
		for (x = 0; x < ffd; ++x) {
			y = height * sqrt(1 - (x * x)/a_squared);

			int32 fy = floor(y);

			union color_conversion color1, color2;

			color1.word = color;
			if (anti_alias == true) {
				float error = y - fy;

				color2.word = color;

				uint8 alpha = round((1.0 - error) * color1.bytes[3]);

				color1.bytes[3] = alpha;
				color2.bytes[3] -= alpha;
			}

			if (fill == true) {
				_FillShearedColumn(center.x, center.y, x, 0 - fy, fy, shear_dx, shear_dy, color, sel, composite_func);
				if (x != 0)
					_FillShearedColumn(center.x, center.y, 0 - x, 0 - fy, fy, shear_dx, shear_dy, color, sel, composite_func);

				if (anti_alias == true) {
					_SetShearedPixel(center.x, center.y, x, 0 - fy - 1, shear_dx, shear_dy, color2.word, sel, composite_func);
					_SetShearedPixel(center.x, center.y, 0 - x, 0 - fy - 1, shear_dx, shear_dy, color2.word, sel, composite_func);
					_SetShearedPixel(center.x, center.y, x, fy + 1, shear_dx, shear_dy, color2.word, sel, composite_func);
					_SetShearedPixel(center.x, center.y, 0 - x, fy + 1, shear_dx, shear_dy, color2.word, sel, composite_func);
				}
			} else {
				_SetShearedPixel(center.x, center.y, x, fy, shear_dx, shear_dy, color1.word, sel, composite_func);
				_SetShearedPixel(center.x, center.y, 0 - x, fy, shear_dx, shear_dy, color1.word, sel, composite_func);
				_SetShearedPixel(center.x, center.y, x, 0 - fy, shear_dx, shear_dy, color1.word, sel, composite_func);
				_SetShearedPixel(center.x, center.y, 0 - x, 0 - fy, shear_dx, shear_dy, color1.word, sel, composite_func);

				if (anti_alias == true) {
					_SetShearedPixel(center.x, center.y, x, fy + 1, shear_dx, shear_dy, color2.word, sel, composite_func);
					_SetShearedPixel(center.x, center.y, 0 - x, fy + 1, shear_dx, shear_dy, color2.word, sel, composite_func);
					_SetShearedPixel(center.x, center.y, x, 0 - fy - 1, shear_dx, shear_dy, color2.word, sel, composite_func);
					_SetShearedPixel(center.x, center.y, 0 - x, 0 - fy - 1, shear_dx, shear_dy, color2.word, sel, composite_func);
				}
			}
		}

		int32 pfx = floor(ffd);
		float ffdy = b_squared / sqrt(b_squared + a_squared);
		for (y = ffdy; y > 0; --y) {
			x = width * sqrt(1 - (y * y) / b_squared);

			int32 fx = floor(x);
			union color_conversion color1, color2;

			color1.word = color;
			if (anti_alias == true) {
				float error = x - fx;

				color2.word = color;

				uint8 alpha = round((1.0 - error) * color1.bytes[3]);

				color1.bytes[3] = alpha;
				color2.bytes[3] -= alpha;
			}

			if (fill == true) {
				if (pfx != fx) {
					_FillShearedColumn(center.x, center.y, fx, 0 - y, y, shear_dx, shear_dy, color, sel, composite_func);
					_FillShearedColumn(center.x, center.y, 0 - fx, 0 - y, y, shear_dx, shear_dy, color, sel, composite_func);
					pfx = fx;
				}

				if (anti_alias == true) {
					_SetShearedPixel(center.x, center.y, fx + 1, y, shear_dx, shear_dy, color2.word, sel, composite_func);
					_SetShearedPixel(center.x, center.y, 0 - fx - 1, y, shear_dx, shear_dy, color2.word, sel, composite_func);
					_SetShearedPixel(center.x, center.y, fx + 1, 0 - y, shear_dx, shear_dy, color2.word, sel, composite_func);
					_SetShearedPixel(center.x, center.y, 0 - fx - 1, 0 - y, shear_dx, shear_dy, color2.word, sel, composite_func);
				}
			} else {
				_SetShearedPixel(center.x, center.y, fx, y, shear_dx, shear_dy, color1.word, sel, composite_func);
				_SetShearedPixel(center.x, center.y, 0 - fx, y, shear_dx, shear_dy, color1.word, sel, composite_func);
				_SetShearedPixel(center.x, center.y, fx, 0 - y, shear_dx, shear_dy, color1.word, sel, composite_func);
				_SetShearedPixel(center.x, center.y, 0 - fx, 0 - y, shear_dx, shear_dy, color1.word, sel, composite_func);

				if (anti_alias == true) {
					_SetShearedPixel(center.x, center.y, fx + 1, y, shear_dx, shear_dy, color2.word, sel, composite_func);
					_SetShearedPixel(center.x, center.y, 0 - fx - 1, y, shear_dx, shear_dy, color2.word, sel, composite_func);
					_SetShearedPixel(center.x, center.y, fx + 1, 0 - y, shear_dx, shear_dy, color2.word, sel, composite_func);
					_SetShearedPixel(center.x, center.y, 0 - fx - 1, 0 - y, shear_dx, shear_dy, color2.word, sel, composite_func);
				}
			}
		}
	}

	return B_OK;
}


void
BitmapDrawer::_SetShearedPixel(int32 x, int32 y, int32 dx, int32 dy,
	float shear_dx, float shear_dy, uint32 color, Selection* sel,
	uint32 (*composite_func)(uint32, uint32))
{
	float shear_delta = ((float)dx * shear_dy) / shear_dx;
	int32 x0 = x + dx;
	int32 y0 = y + dy + floor(shear_delta);

	if (x0 < 0)
		x0 = 0;

	if (y0 < 0)
		y0 = 0;

	SetPixel(x0, y0, color, sel, composite_func);
}


void
BitmapDrawer::_FillShearedColumn(int32 x, int32 y, int32 dx, int32 dy0,
	int32 dy1, float shear_dx, float shear_dy, uint32 color,
	Selection* sel, uint32 (*composite_func)(uint32, uint32))
{
	float shear_delta = ((float)dx * shear_dy) / shear_dx;
	int32 x0 = x + dx;
	int32 y0 = y + dy0 + floor(shear_delta);
	int32 y1 = y + dy1 + floor(shear_delta);

	if (x0 < 0)
		x0 = 0;

	if (y0 < 0)
		y0 = 0;

	if (y0 > y1) {
		int32 temp = y1;
		y1 = y0;
		y0 = temp;
	}

	for (uint32 yy = y0; yy <= y1; ++yy)
		SetPixel(x0, yy, color, sel, composite_func);
}


// This function is quite useless unless we want draw a rectangular bitmap such that the data
// reaches borders at every position and it is binary data.
status_t
BitmapDrawer::DrawBitmap(BBitmap* bitmap, BRect bounds, BRect exclude, bool)
{
	// The exclude rect should be the same size as bounds but with a different offset.
	// This will copy a bitmap to the actual bitmap
	uint32* target_bits;
	uint32* bits = (uint32*)bitmap->Bits();
	int32 bitmap_width = bitmap->Bounds().IntegerWidth();

	// Divide the bounds into two rectangles.
	BRect area1 = bounds;
	if (exclude.IsValid() == TRUE) {
		if (exclude.top > bounds.top) {
			area1.top = bounds.top;
			area1.bottom = exclude.top - 1;
		} else {
			area1.top = exclude.bottom + 1;
			area1.bottom = bounds.bottom;
		}
		area1.left = bounds.left;
		area1.right = bounds.right;
	}
	area1 = area1 & bitmap_bounds;

	target_bits = bitmap_bits + (int32)area1.left + (int32)area1.top * bitmap_bpr;
	int32 area_width = area1.IntegerWidth();
	int32 area_height = area1.IntegerHeight();
	for (int32 y = 0; y <= area_height; y++) {
		for (int32 x = 0; x <= area_width; x++)
			*target_bits++ = *bits++;

		target_bits += bitmap_bpr - area_width - 1;
		bits += bitmap_width - area_width;
	}

	BRect area2 = bounds;
	if (area1 != (bounds & bitmap_bounds)) {
		if (exclude.IsValid() == TRUE) {
			if (exclude.left > bounds.left) {
				area2.left = bounds.left;
				area2.right = exclude.left - 1;
			} else {
				area2.left = exclude.right + 1;
				area2.right = bounds.right;
			}
			area2.top = (area1.bottom < exclude.top ? area1.bottom + 1 : bounds.top);
			area2.bottom = (area1.top > exclude.bottom ? area1.top - 1 : bounds.bottom);
		}
		area2 = area2 & bitmap_bounds;

		target_bits = bitmap_bits + (int32)area2.left + (int32)area2.top * bitmap_bpr;
		bits = (uint32*)bitmap->Bits();
		area_width = area2.IntegerWidth();
		area_height = area2.IntegerHeight();
		for (int32 y = 0; y <= area_height; y++) {
			for (int32 x = 0; x <= area_width; x++)
				*target_bits++ = *bits++;

			target_bits += bitmap_bpr - area_width - 1;
			bits += bitmap_width - area_width;
		}
	}
	return B_OK;
}


status_t
BitmapDrawer::DrawConvexPolygon(
	BPoint* point_list, int32 point_count, uint32 color, bool fill, bool anti_alias)
{
	if (fill == FALSE) {
		// Just connect the corners with lines.
		for (int32 i = 0; i < point_count - 1; i++)
			DrawHairLine(point_list[i], point_list[i + 1], color, anti_alias);

		DrawHairLine(point_list[point_count - 1], point_list[0], color, anti_alias);
	} else /*if (anti_aliasing == FALSE) */ {
		// We have to fill the polygon.
		// We use the following method:
		//	1.	Determine the minimum y and maximum y coordinates of the polygon.
		//	2.	For each y-coordinate within the polygon find the two lines
		//		that cross the y-coordinate.
		//	3.	For those two found lines calculate the x-coordinates at y-position.
		//	4.	Select the minimum and maximum of those x-coordinates.
		//	5.	Fill the line at y-coordinate between minimum x and maximum x.
		float min_y = 1000000, max_y = -1000000;
		for (int32 i = 0; i < point_count; i++) {
			min_y = min_c(point_list[i].y, min_y);
			max_y = max_c(point_list[i].y, max_y);
		}

		// Round the mimimum and maximum y to the nearest integer.
		min_y = (((min_y - floor(min_y)) < 0.5) ? floor(min_y) : ceil(min_y));
		max_y = (((max_y - floor(min_y)) < 0.5) ? floor(max_y) : ceil(max_y));

		for (int32 y = (int32)min_y; y <= max_y; y++) {
			float min_x = 1000000, max_x = -1000000;
			int32 i = 0, found_lines = 0;

			// We have to examine at least three lines because otherwise lines beginning at
			// a the same corner-point might be the only found lines that cross certain y-row.
			while ((found_lines < 3) && (i < (point_count - 1))) {
				if (((point_list[i].y <= y) && (point_list[i + 1].y >= y))
					|| ((point_list[i].y >= y) && (point_list[i + 1].y <= y))) {
					found_lines++;
					min_x = min_c(min_x, MinimumCrossingPoint(point_list[i], point_list[i + 1], y));
					max_x = max_c(max_x, MaximumCrossingPoint(point_list[i], point_list[i + 1], y));
				}
				i++;
			}
			if (found_lines < 3) {
				if (((point_list[0].y <= y) && (point_list[point_count - 1].y >= y))
					|| ((point_list[0].y >= y) && (point_list[point_count - 1].y <= y))) {
					found_lines++;
					min_x = min_c(
						min_x, MinimumCrossingPoint(point_list[0], point_list[point_count - 1], y));
					max_x = max_c(
						max_x, MaximumCrossingPoint(point_list[0], point_list[point_count - 1], y));
				}
			}
			if (found_lines == 0) {
				// We should not fill any pixels in this line
				min_x = 1;
				max_x = 0;
			}
			// Then round the minimum and maximum x to nearest integers.
			min_x = (((min_x - floor(min_x)) < 0.5) ? floor(min_x) : ceil(min_x));
			max_x = (((max_x - floor(min_x)) < 0.5) ? floor(max_x) : ceil(max_x));

			// Then fill the span
			// First round the minimum and maximum x to nearest integers.
			min_x = (((min_x - floor(min_x)) < 0.5) ? floor(min_x) : ceil(min_x));
			max_x = (((max_x - floor(min_x)) < 0.5) ? floor(max_x) : ceil(max_x));
			for (int32 x = (int32)min_x; x <= max_x; x++)
				SetPixel(BPoint(x, y), color);
		}
	}
	/* else {

	}*/

	return B_OK;
}


float
BitmapDrawer::MinimumCrossingPoint(BPoint& start, BPoint& end, int32 y)
{
	// If the line is more vertical than horizontal there is only one point
	// at coordinate y.
	if (fabs(start.y - end.y) >= fabs(start.x - end.x)) {
		// Now we just have to return the point at that location.
		float step_x = (end.x - start.x) / fabs(start.y - end.y);
		return start.x + fabs(start.y - y) * step_x;
	} else {
		float step_y = (start.y - end.y) / fabs(start.x - end.x);
		if (step_y != 0) {
			float dir_x = (end.x - start.x) / fabs(start.x - end.x);
			float x_add = fabs((fabs(start.y - y) - 0.5) / step_y);
			float x1 = start.x + dir_x * x_add;
			x_add = fabs((fabs(start.y - y) + 0.5) / step_y);
			float x2 = start.x + dir_x * x_add;
			return max_c(min_c(start.x, end.x), min_c(x1, x2));
		} else
			return min_c(start.x, end.x);
	}
}


float
BitmapDrawer::MaximumCrossingPoint(BPoint& start, BPoint& end, int32 y)
{
	// If the line is more vertical than horizontal there is only one point
	// at coordinate y.
	if (fabs(start.y - end.y) >= fabs(start.x - end.x)) {
		// Now we just have to return the point at that location.
		float step_x = (end.x - start.x) / fabs(start.y - end.y);
		return start.x + fabs(start.y - y) * step_x;
	} else {
		float step_y = (start.y - end.y) / fabs(start.x - end.x);
		if (step_y != 0) {
			float dir_x = (end.x - start.x) / fabs(start.x - end.x);
			float x_add = fabs((fabs(start.y - y) - 0.5) / step_y);
			float x1 = start.x + dir_x * x_add;
			x_add = fabs((fabs(start.y - y) + 0.5) / step_y);
			float x2 = start.x + dir_x * x_add;
			return min_c(max_c(start.x, end.x), max_c(x1, x2));
		} else
			return max_c(start.x, end.x);
	}
}


status_t
BitmapDrawer::DrawRectanglePolygon(BPoint* corners, uint32 color, bool fill, bool anti_alias,
	Selection* sel, uint32 (*composite_func)(uint32, uint32))
{
	// This is a special-case of the convex-polygon function that draws polygons that are
	// rectangular. In addition to the convex polygon case we can use the following simplifications:
	//	1.	The rectangle has two pairs of parallel lines.
	//	2.	Rectangle is symmetric; point at (x,y) is same as point at (-x,-y)
	//		when the origo is at polygons center.
	//	3.	If one of the polygon's lines has the same x or y coordinate at both ends,
	//		the polygon can be drawn very quickly.
	//	4.	If the case 3 does not apply we can select two lines that begin from top point
	//		and fill the spans between them. When the other line ends we can select the other line
	//		that begins at the point where the line ended. If we draw symmetrically we can stop when
	//		we have reached the middle-height of rectangle.
	//	5.	If case 3 does not apply then every corner has one extreme value.

	if (fill == FALSE) {
		// Just connect the corners with lines.
		// First round the corners to proper values.
		corners[0].x = round(corners[0].x);
		corners[0].y = round(corners[0].y);
		corners[1].x = round(corners[1].x);
		corners[1].y = round(corners[1].y);
		corners[2].x = round(corners[2].x);
		corners[2].y = round(corners[2].y);
		corners[3].x = round(corners[3].x);
		corners[3].y = round(corners[3].y);

		DrawHairLine(corners[0], corners[1], color, anti_alias, sel, composite_func);
		DrawHairLine(corners[1], corners[2], color, anti_alias, sel, composite_func);
		DrawHairLine(corners[2], corners[3], color, anti_alias, sel, composite_func);
		DrawHairLine(corners[3], corners[0], color, anti_alias, sel, composite_func);
	} else {
		if (anti_alias == TRUE)
			FillAntiAliasedRectangle(corners, color, sel, composite_func);
		else
			FillRectangle(corners, color, sel, composite_func);
	}

	return B_OK;
}


status_t
BitmapDrawer::FillAntiAliasedRectangle(
	BPoint* corners, uint32 color, Selection* sel, uint32 (*composite_func)(uint32, uint32))
{
	// If the rectangle is aligned with the coordinate axis we do not need to
	// do much.
	if ((corners[0].x == corners[1].x) || (corners[0].y == corners[1].y)) {
		// Select minimum and maximum coordinates.
		int32 min_x = (int32)round(min_c(corners[0].x, corners[2].x));
		int32 max_x = (int32)round(max_c(corners[0].x, corners[2].x));
		int32 min_y = (int32)round(min_c(corners[0].y, corners[2].y));
		int32 max_y = (int32)round(max_c(corners[0].y, corners[2].y));

		for (int32 y = min_y; y <= max_y; y++) {
			for (int32 x = min_x; x <= max_x; x++)
				SetPixel(BPoint(x, y), color, sel, composite_func);
		}
	}
	// If the rectangle is not rectilinear we must sort the points
	// and then fill the resulting rectangular polygon. This is almost
	// exact copy of non-antialiased fill, but we use a subppixel mask to
	// determine the coverage of each pixel.
	else {
		union {
			unsigned char bytes[4];
			uint32 word;
		} norm_color;

		norm_color.word = color;
		uint8 alpha = norm_color.bytes[3];

		// First we must sort the points.
		//
		//		Top
		// Left
		//			Right
		//   Bottom
		BPoint left, right, top, bottom;
		left = right = top = bottom = corners[0];
		for (int32 i = 0; i < 4; i++) {
			if (corners[i].x < left.x)
				left = corners[i];
			if (corners[i].x > right.x)
				right = corners[i];
			if (corners[i].y < top.y)
				top = corners[i];
			if (corners[i].y > bottom.y)
				bottom = corners[i];
		}
		float span_left;
		float span_right;
		float left_diff;
		float right_diff;
		left_diff = (left.x - top.x) / (left.y - top.y);
		right_diff = (right.x - top.x) / (right.y - top.y);

		// These two values are used when calculating the subpixel coverages.
		// These y-differences are positive.
		float left_y_diff = (left.y - top.y) / (top.x - left.x);
		float right_y_diff = (right.y - top.y) / (right.x - top.x);

		float y = floor(top.y);
		float bottom_y;
		span_left = (y - top.y) * left_diff + top.x;
		span_right = (y - top.y) * right_diff + top.x;

		float absolute_left = 0;
		float absolute_right = bitmap_bounds.right;
		float absolute_top = 0;
		float absolute_bottom = bitmap_bounds.bottom;

		bottom_y = min_c(left.y, right.y);

		// This constant determines how many subpixels are used in one direction.
		const float SUBPIXEL_AMOUNT = 13;

		int32 coverage;
		int32 max_coverage = 0;
		// This loop is split into three parts to avoid
		// having too many if-clauses inside the loop.
		// Then there are also two versions of it: one without selections
		// and one with selections.
		if ((sel == NULL) || (sel->IsEmpty() == TRUE)) {
			while (y <= bottom_y) {

				if ((y <= absolute_bottom) && (y >= absolute_top)) {
					int32 left_bound = (int32)max_c(
						floor(span_left + left_diff), max_c(absolute_left, floor(left.x)));
					int32 right_bound = (int32)min_c(
						min_c(absolute_right, ceil(right.x)), ceil(span_right + right_diff));
					for (int32 x = left_bound; x <= right_bound; x++) {
						coverage = 0;
						if ((((top.y) + (top.x - (x - 0.5)) * left_y_diff) > (y - 0.5))
							|| (((top.y) + ((x + 0.5) - top.x) * right_y_diff) > (y - 0.5))) {
							// First calculate the subpixel coverages for the point x,y.
							for (float sub_y = y - 0.51; sub_y <= y + 0.51;
								sub_y += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
								for (float sub_x = x - 0.51; sub_x <= x + 0.51;
									sub_x += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
									if ((((top.y) + (top.x - sub_x) * left_y_diff) <= sub_y)
										&& (((top.y) + (sub_x - top.x) * right_y_diff) <= sub_y)) {
										coverage++;
									}
								}
							}
							norm_color.bytes[3] = (uint8)(
								alpha * ((float(coverage)) / (SUBPIXEL_AMOUNT * SUBPIXEL_AMOUNT)));
							SetPixel(x, int32(y), norm_color.word, sel, composite_func);
							max_coverage = max_c(coverage, max_coverage);
						} else
							SetPixel(x, int32(y), color, sel, composite_func);
					}
				}
				y++;
				span_left += left_diff;
				span_right += right_diff;
			}

			bottom_y = max_c(left.y, right.y);

			if ((y >= left.y) && (left_diff <= 0)) {
				left_diff = (bottom.x - left.x) / (bottom.y - left.y);
				span_left = (y - left.y) * left_diff + left.x;
				left_y_diff = (bottom.y - left.y) / (bottom.x - left.x);

				while (y <= bottom_y) {

					if ((y <= absolute_bottom) && (y >= absolute_top)) {
						int32 left_bound = (int32)max_c(
							floor(span_left - left_diff), max_c(absolute_left, floor(left.x)));
						int32 right_bound = (int32)min_c(
							min_c(absolute_right, ceil(right.x)), ceil(span_right + right_diff));
						for (int32 x = left_bound; x <= right_bound; x++) {
							coverage = 0;

							// We can calculate y-coordinates of the boundaries at any given
							// x-coordinate by using the y_diffs
							if ((((left.y) + ((x - 0.5) - left.x) * left_y_diff) < (y + 0.5))
								|| (((top.y) + ((x + 0.5) - top.x) * right_y_diff) > (y - 0.5))) {
								// First calculate the subpixel coverages for the point x,y.
								for (float sub_y = y - 0.51; sub_y <= y + 0.51;
									sub_y += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
									for (float sub_x = x - 0.51; sub_x <= x + 0.51;
										sub_x += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
										if ((((left.y) + (sub_x - left.x) * left_y_diff) >= sub_y)
											&& (((top.y) + (sub_x - top.x) * right_y_diff)
												<= sub_y)) {
											coverage++;
										}
									}
								}
								norm_color.bytes[3] = (uint8)(alpha
									* ((float(coverage)) / (SUBPIXEL_AMOUNT * SUBPIXEL_AMOUNT)));
								SetPixel(x, int32(y), norm_color.word, sel, composite_func);
								max_coverage = max_c(coverage, max_coverage);
							} else
								SetPixel(x, int32(y), color, sel, composite_func);
						}
					}
					y++;
					span_left += left_diff;
					span_right += right_diff;
				}
			} else if ((y >= right.y) && (right_diff >= 0)) {
				right_diff = (bottom.x - right.x) / (bottom.y - right.y);
				span_right = (y - right.y) * right_diff + right.x;
				right_y_diff = (bottom.y - right.y) / (right.x - bottom.x);

				while (y <= bottom_y) {

					if ((y <= absolute_bottom) && (y >= absolute_top)) {
						int32 left_bound = (int32)max_c(
							floor(span_left + left_diff), max_c(absolute_left, floor(left.x)));
						int32 right_bound = (int32)min_c(
							min_c(absolute_right, ceil(right.x)), ceil(span_right - right_diff));
						for (int32 x = left_bound; x <= right_bound; x++) {
							coverage = 0;
							if ((((top.y) + (top.x - (x - 0.5)) * left_y_diff) > (y - 0.5))
								|| (((right.y) + (right.x - (x + 0.5)) * right_y_diff)
									< (y + 0.5))) {
								// First calculate the subpixel coverages for the point x,y.
								for (float sub_y = y - 0.51; sub_y <= y + 0.51;
									sub_y += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
									for (float sub_x = x - 0.51; sub_x <= x + 0.51;
										sub_x += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
										if ((((top.y) + (top.x - sub_x) * left_y_diff) <= sub_y)
											&& (((right.y) + (right.x - sub_x) * right_y_diff)
												> sub_y)) {
											coverage++;
										}
									}
								}
								norm_color.bytes[3] = (uint8)(alpha
									* ((float(coverage)) / (SUBPIXEL_AMOUNT * SUBPIXEL_AMOUNT)));
								SetPixel(x, int32(y), norm_color.word, sel, composite_func);
								max_coverage = max_c(coverage, max_coverage);
							} else
								SetPixel(x, int32(y), color, sel, composite_func);
						}
					}
					y++;
					span_left += left_diff;
					span_right += right_diff;
				}
			}

			bottom_y = ceil(bottom.y);
			if ((y >= left.y) && (left_diff <= 0)) {
				left_diff = (bottom.x - left.x) / (bottom.y - left.y);
				span_left = (y - left.y) * left_diff + left.x;
				left_y_diff = (bottom.y - left.y) / (bottom.x - left.x);
			}
			if ((y >= right.y) && (right_diff >= 0)) {
				right_diff = (bottom.x - right.x) / (bottom.y - right.y);
				span_right = (y - right.y) * right_diff + right.x;
				right_y_diff = (bottom.y - right.y) / (right.x - bottom.x);
			}

			while (y <= bottom_y) {

				if ((y <= absolute_bottom) && (y >= absolute_top)) {
					int32 left_bound = (int32)max_c(
						floor(span_left - left_diff), max_c(absolute_left, floor(left.x)));
					int32 right_bound = (int32)min_c(
						min_c(absolute_right, ceil(right.x)), ceil(span_right - right_diff));
					for (int32 x = left_bound; x <= right_bound; x++) {
						coverage = 0;
						if ((((left.y) + ((x - 0.5) - left.x) * left_y_diff) < (y + 0.5))
							|| (((right.y) + (right.x - (x + 0.5)) * right_y_diff) < (y + 0.5))) {
							// First calculate the subpixel coverages for the point x,y.
							for (float sub_y = y - 0.51; sub_y <= y + 0.51;
								sub_y += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
								for (float sub_x = x - 0.51; sub_x <= x + 0.51;
									sub_x += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
									if ((((left.y) + (sub_x - left.x) * left_y_diff) >= sub_y)
										&& (((right.y) + (right.x - sub_x) * right_y_diff)
											> sub_y)) {
										coverage++;
									}
								}
							}
							norm_color.bytes[3] = (uint8)(
								alpha * ((float(coverage)) / (SUBPIXEL_AMOUNT * SUBPIXEL_AMOUNT)));
							SetPixel(x, int32(y), norm_color.word, sel, composite_func);
							max_coverage = max_c(coverage, max_coverage);
						} else
							SetPixel(x, int32(y), color, sel, composite_func);
					}
				}
				y++;
				span_left += left_diff;
				span_right += right_diff;
			}
		} else {
			while (y <= bottom_y) {

				if ((y <= absolute_bottom) && (y >= absolute_top)) {
					int32 left_bound = (int32)max_c(
						floor(span_left + left_diff), max_c(absolute_left, floor(left.x)));
					int32 right_bound = (int32)min_c(
						min_c(absolute_right, ceil(right.x)), ceil(span_right + right_diff));
					for (int32 x = left_bound; x <= right_bound; x++) {
						coverage = 0;
						if ((((top.y) + (top.x - (x - 0.5)) * left_y_diff) > (y - 0.5))
							|| (((top.y) + ((x + 0.5) - top.x) * right_y_diff) > (y - 0.5))) {
							// First calculate the subpixel coverages for the point x,y.
							for (float sub_y = y - 0.51; sub_y <= y + 0.51;
								sub_y += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
								for (float sub_x = x - 0.51; sub_x <= x + 0.51;
									sub_x += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
									if ((((top.y) + (top.x - sub_x) * left_y_diff) <= sub_y)
										&& (((top.y) + (sub_x - top.x) * right_y_diff) <= sub_y)) {
										coverage++;
									}
								}
							}
							norm_color.bytes[3] = (uint8)(
								alpha * ((float(coverage)) / (SUBPIXEL_AMOUNT * SUBPIXEL_AMOUNT)));
							SetPixel(x, int32(y), norm_color.word, sel, composite_func);
							max_coverage = max_c(coverage, max_coverage);
						} else
							SetPixel(x, int32(y), color, sel, composite_func);
					}
				}
				y++;
				span_left += left_diff;
				span_right += right_diff;
			}

			bottom_y = max_c(left.y, right.y);

			if ((y >= left.y) && (left_diff <= 0)) {
				left_diff = (bottom.x - left.x) / (bottom.y - left.y);
				span_left = (y - left.y) * left_diff + left.x;
				left_y_diff = (bottom.y - left.y) / (bottom.x - left.x);

				while (y <= bottom_y) {
					if ((y <= absolute_bottom) && (y >= absolute_top)) {
						int32 left_bound = (int32)max_c(
							floor(span_left - left_diff), max_c(absolute_left, floor(left.x)));
						int32 right_bound = (int32)min_c(
							min_c(absolute_right, ceil(right.x)), ceil(span_right + right_diff));
						for (int32 x = left_bound; x <= right_bound; x++) {
							coverage = 0;

							// We can calculate y-coordinates of the boundaries at any given
							// x-coordinate by using the y_diffs
							if ((((left.y) + ((x - 0.5) - left.x) * left_y_diff) < (y + 0.5))
								|| (((top.y) + ((x + 0.5) - top.x) * right_y_diff) > (y - 0.5))) {
								// First calculate the subpixel coverages for the point x,y.
								for (float sub_y = y - 0.51; sub_y <= y + 0.51;
									sub_y += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
									for (float sub_x = x - 0.51; sub_x <= x + 0.51;
										sub_x += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
										if ((((left.y) + (sub_x - left.x) * left_y_diff) >= sub_y)
											&& (((top.y) + (sub_x - top.x) * right_y_diff)
												<= sub_y)) {
											coverage++;
										}
									}
								}
								norm_color.bytes[3] = (uint8)(alpha
									* ((float(coverage)) / (SUBPIXEL_AMOUNT * SUBPIXEL_AMOUNT)));
								SetPixel(x, int32(y), norm_color.word, sel, composite_func);

								max_coverage = max_c(coverage, max_coverage);
							} else
								SetPixel(x, int32(y), color, sel, composite_func);
						}
					}
					y++;
					span_left += left_diff;
					span_right += right_diff;
				}
			} else if ((y >= right.y) && (right_diff >= 0)) {
				right_diff = (bottom.x - right.x) / (bottom.y - right.y);
				span_right = (y - right.y) * right_diff + right.x;
				right_y_diff = (bottom.y - right.y) / (right.x - bottom.x);

				while (y <= bottom_y) {

					if ((y <= absolute_bottom) && (y >= absolute_top)) {
						int32 left_bound = (int32)max_c(
							floor(span_left + left_diff), max_c(absolute_left, floor(left.x)));
						int32 right_bound = (int32)min_c(
							min_c(absolute_right, ceil(right.x)), ceil(span_right - right_diff));
						for (int32 x = left_bound; x <= right_bound; x++) {
							coverage = 0;
							if ((((top.y) + (top.x - (x - 0.5)) * left_y_diff) > (y - 0.5))
								|| (((right.y) + (right.x - (x + 0.5)) * right_y_diff)
									< (y + 0.5))) {
								// First calculate the subpixel coverages for the point x,y.
								for (float sub_y = y - 0.51; sub_y <= y + 0.51;
									sub_y += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
									for (float sub_x = x - 0.51; sub_x <= x + 0.51;
										sub_x += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
										if ((((top.y) + (top.x - sub_x) * left_y_diff) <= sub_y)
											&& (((right.y) + (right.x - sub_x) * right_y_diff)
												> sub_y)) {
											coverage++;
										}
									}
								}
								norm_color.bytes[3] = (uint8)(alpha
									* ((float(coverage)) / (SUBPIXEL_AMOUNT * SUBPIXEL_AMOUNT)));
								SetPixel(x, int32(y), norm_color.word, sel, composite_func);
								max_coverage = max_c(coverage, max_coverage);
							} else
								SetPixel(x, int32(y), color, sel, composite_func);
						}
					}
					y++;
					span_left += left_diff;
					span_right += right_diff;
				}
			}

			bottom_y = ceil(bottom.y);
			if ((y >= left.y) && (left_diff <= 0)) {
				left_diff = (bottom.x - left.x) / (bottom.y - left.y);
				span_left = (y - left.y) * left_diff + left.x;
				left_y_diff = (bottom.y - left.y) / (bottom.x - left.x);
			}
			if ((y >= right.y) && (right_diff >= 0)) {
				right_diff = (bottom.x - right.x) / (bottom.y - right.y);
				span_right = (y - right.y) * right_diff + right.x;
				right_y_diff = (bottom.y - right.y) / (right.x - bottom.x);
			}

			while (y <= bottom_y) {

				if ((y <= absolute_bottom) && (y >= absolute_top)) {
					int32 left_bound = (int32)max_c(
						floor(span_left - left_diff), max_c(absolute_left, floor(left.x)));
					int32 right_bound = (int32)min_c(
						min_c(absolute_right, ceil(right.x)), ceil(span_right - right_diff));
					for (int32 x = left_bound; x <= right_bound; x++) {
						coverage = 0;
						if ((((left.y) + ((x - 0.5) - left.x) * left_y_diff) < (y + 0.5))
							|| (((right.y) + (right.x - (x + 0.5)) * right_y_diff) < (y + 0.5))) {
							// First calculate the subpixel coverages for the point x,y.
							for (float sub_y = y - 0.51; sub_y <= y + 0.51;
								sub_y += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
								for (float sub_x = x - 0.51; sub_x <= x + 0.51;
									sub_x += 1.0 / (SUBPIXEL_AMOUNT - 1)) {
									if ((((left.y) + (sub_x - left.x) * left_y_diff) >= sub_y)
										&& (((right.y) + (right.x - sub_x) * right_y_diff)
											> sub_y)) {
										coverage++;
									}
								}
							}
							norm_color.bytes[3] = (uint8)(
								alpha * ((float(coverage)) / (SUBPIXEL_AMOUNT * SUBPIXEL_AMOUNT)));
							SetPixel(x, int32(y), norm_color.word, sel, composite_func);
							max_coverage = max_c(coverage, max_coverage);
						} else
							SetPixel(x, int32(y), color, sel, composite_func);
					}
				}
				y++;
				span_left += left_diff;
				span_right += right_diff;
			}
		}
	}
	return B_OK;
}


status_t
BitmapDrawer::FillRectangle(
	BPoint* corners, uint32 color, Selection* sel, uint32 (*composite_func)(uint32, uint32))
{
	// If the rectangle is aligned with the coordinate axis we do not need to
	// do much.
	if ((corners[0].x == corners[1].x) || (corners[0].y == corners[1].y)) {
		// Select minimum and maximum coordinates.
		int32 min_x = (int32)round(min_c(corners[0].x, corners[2].x));
		int32 max_x = (int32)round(max_c(corners[0].x, corners[2].x));
		int32 min_y = (int32)round(min_c(corners[0].y, corners[2].y));
		int32 max_y = (int32)round(max_c(corners[0].y, corners[2].y));
		// Then fill the rectangle.
		for (int32 y = min_y; y <= max_y; y++) {
			for (int32 x = min_x; x <= max_x; x++)
				SetPixel(BPoint(x, y), color, sel, composite_func);
		}
	}
	// If the rectanle is not rectilinear we must sort the points
	// and then fill the resulting rectangular polygon.
	else {
		// First we must sort the points.
		//
		//		Top
		// Left
		//			Right
		//   Bottom
		BPoint left, right, top, bottom;
		left = right = top = bottom = corners[0];
		for (int32 i = 0; i < 4; i++) {
			if (corners[i].x < left.x)
				left = corners[i];
			if (corners[i].x > right.x)
				right = corners[i];
			if (corners[i].y < top.y)
				top = corners[i];
			if (corners[i].y > bottom.y)
				bottom = corners[i];
		}

		float span_left;
		float span_right;
		float left_diff;
		float right_diff;
		left_diff = (left.x - top.x) / (left.y - top.y);
		right_diff = (right.x - top.x) / (right.y - top.y);

		float y = ceil(top.y);
		float bottom_y;
		span_left = (y - top.y) * left_diff + top.x;
		span_right = (y - top.y) * right_diff + top.x;

		float absolute_left = 0;
		float absolute_right = bitmap_bounds.right;
		float absolute_top = 0;
		float absolute_bottom = bitmap_bounds.bottom;

		bottom_y = min_c(left.y, right.y);

		// This loop is split into three parts to avoid
		// having too many if-clauses inside the loop.

		while (y <= bottom_y) {

			if ((y <= absolute_bottom) && (y >= absolute_top)) {
				int32 left_bound = (int32)max_c(ceil(span_left), absolute_left);
				int32 right_bound = (int32)min_c(absolute_right, floor(span_right));

				for (int32 x = left_bound; x <= right_bound; x++)
					SetPixel(x, int32(y), color, sel, composite_func);
			}
			y++;
			span_left += left_diff;
			span_right += right_diff;
		}

		bottom_y = max_c(left.y, right.y);

		if ((y >= left.y) && (left_diff <= 0)) {
			left_diff = (bottom.x - left.x) / (bottom.y - left.y);
			span_left = (y - left.y) * left_diff + left.x;
		}
		if ((y >= right.y) && (right_diff >= 0)) {
			right_diff = (bottom.x - right.x) / (bottom.y - right.y);
			span_right = (y - right.y) * right_diff + right.x;
		}

		while (y <= bottom_y) {
			if ((y <= absolute_bottom) && (y >= absolute_top)) {
				int32 left_bound = (int32)max_c(ceil(span_left), absolute_left);
				int32 right_bound = (int32)min_c(absolute_right, floor(span_right));

				for (int32 x = left_bound; x <= right_bound; x++)
					SetPixel(x, int32(y), color, sel, composite_func);
			}
			y++;
			span_left += left_diff;
			span_right += right_diff;
		}

		bottom_y = floor(bottom.y);
		if ((y >= left.y) && (left_diff <= 0)) {
			left_diff = (bottom.x - left.x) / (bottom.y - left.y);
			span_left = (y - left.y) * left_diff + left.x;
		}
		if ((y >= right.y) && (right_diff >= 0)) {
			right_diff = (bottom.x - right.x) / (bottom.y - right.y);
			span_right = (y - right.y) * right_diff + right.x;
		}

		while (y <= bottom_y) {
			if ((y <= absolute_bottom) && (y >= absolute_top)) {
				int32 left_bound = (int32)max_c(ceil(span_left), absolute_left);
				int32 right_bound = (int32)min_c(absolute_right, floor(span_right));

				for (int32 x = left_bound; x <= right_bound; x++)
					SetPixel(x, int32(y), color, sel, composite_func);
			}
			y++;
			span_left += left_diff;
			span_right += right_diff;
		}
	}

	return B_OK;
}


status_t
BitmapDrawer::SetPixel(
	BPoint location, uint32 color, Selection* sel, uint32 (*composite_func)(uint32, uint32))
{
	if (sel == NULL || sel->ContainsPoint(location)) {
		float sel_alpha = 1.0;
		if (sel != NULL && sel->IsEmpty() == false)
			sel_alpha = sel->Value(location) / 255.;

		if (bitmap_bounds.Contains(location)) {
			if (composite_func) {
				union {
					unsigned char bytes[4];
					uint32 word;
				} norm_color, target_color;

				norm_color.word = color;
				norm_color.bytes[3] *= sel_alpha;

				uint32 target = GetPixel(location);
				target_color.word = target;

				*(bitmap_bits + (int32)location.x + (int32)location.y * bitmap_bpr)
					= (*composite_func)(target_color.word, norm_color.word);
			} else {
				union {
					unsigned char bytes[4];
					uint32 word;
				} norm_color;

				norm_color.word = color;
				norm_color.bytes[3] *= sel_alpha;

				*(bitmap_bits + (int32)location.x + (int32)location.y * bitmap_bpr) = norm_color.word;
			}

			return B_OK;
		} else
			return B_ERROR;
	} else
		return B_ERROR;
}


status_t
BitmapDrawer::SetPixel(
	int32 x, int32 y, uint32 color, Selection* sel, uint32 (*composite_func)(uint32, uint32))
{
	return SetPixel(BPoint(x, y), color, sel, composite_func);
}


uint32
BitmapDrawer::GetPixel(BPoint location)
{
	if (bitmap_bounds.Contains(location))
		return *(bitmap_bits + (int32)location.x + (int32)location.y * bitmap_bpr);
	else {
		union {
			unsigned char bytes[4];
			uint32 word;
		} color;

		color.bytes[0] = 0xFF;
		color.bytes[1] = 0xFF;
		color.bytes[2] = 0xFF;
		color.bytes[3] = 0x00;

		return color.word; // If out of bounds return transparent white.
	}
}


uint32
BitmapDrawer::GetPixel(int32 x, int32 y)
{
	return GetPixel(BPoint(x, y));
}
