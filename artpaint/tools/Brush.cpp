/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#include "Brush.h"


#include <Bitmap.h>
#include <Point.h>


#include <math.h>
#include <stdio.h>


#define PI M_PI


Brush::Brush(brush_info &info, bool create_diff_brushes)
{
	// First record the data for the brush
	shape_ = info.shape;
	hardness_ = info.hardness;

	actual_width = width_ = info.width;
	actual_height = height_ = info.height;
	angle_ = info.angle;

	brush = diff10 = diff11 = diff1_1 = NULL;
	diff01 = diff0_1 = NULL;
	diff_11 = diff_10 = diff_1_1 = NULL;

	brush_span = diff10_span = diff11_span = diff1_1_span = NULL;
	diff01_span = diff0_1_span = NULL;
	diff_11_span = diff_10_span = diff_1_1_span = NULL;

	// Here call the function that makes the brush.
	switch (shape_) {
		case HS_RECTANGULAR_BRUSH:
			make_rectangular_brush();
			break;
		case HS_ELLIPTICAL_BRUSH:
			make_elliptical_brush();
			break;
		case HS_IRREGULAR_BRUSH:
			break;
	}

	if (create_diff_brushes) {
		// Make the difference brushes here also.
		diff10 = make_diff_brush(brush,1,0);
		diff11 = make_diff_brush(brush,1,1);
		diff1_1 = make_diff_brush(brush,1,-1);
		diff01 = make_diff_brush(brush,0,1);
		diff0_1 = make_diff_brush(brush,0,-1);
		diff_10 = make_diff_brush(brush,-1,0);
		diff_11 = make_diff_brush(brush,-1,1);
		diff_1_1 = make_diff_brush(brush,-1,-1);

		// Then make the span-lists for the brushes
		brush_span = make_span_list(brush);
		diff10_span = make_span_list(diff10);
		diff11_span = make_span_list(diff11);
		diff1_1_span = make_span_list(diff1_1);
		diff01_span = make_span_list(diff01);
		diff0_1_span = make_span_list(diff0_1);
		diff_10_span = make_span_list(diff_10);
		diff_11_span = make_span_list(diff_11);
		diff_1_1_span = make_span_list(diff_1_1);
	}
}


Brush::~Brush()
{
	delete_all_data();
}


void
Brush::ModifyBrush(brush_info &info)
{
	delete_all_data();

	// First record the data for the brush
	shape_ = info.shape;
	hardness_ = info.hardness;

	actual_width = width_ = info.width;
	actual_height = height_ = info.height;
	angle_ = info.angle;

	brush = diff10 = diff11 = diff1_1 = NULL;
	diff01 = diff0_1 = NULL;
	diff_11 = diff_10 = diff_1_1 = NULL;

	brush_span = diff10_span = diff11_span = diff1_1_span = NULL;
	diff01_span = diff0_1_span = NULL;
	diff_11_span = diff_10_span = diff_1_1_span = NULL;

	// Here call the function that makes the brush.
	switch (shape_) {
		case HS_RECTANGULAR_BRUSH:
			make_rectangular_brush();
			break;
		case HS_ELLIPTICAL_BRUSH:
			make_elliptical_brush();
			break;
		case HS_IRREGULAR_BRUSH:
			break;
	}

	maximum_width = max_c(height_, width_);
	maximum_height = maximum_width;
}


void
Brush::CreateDiffBrushes()
{
	// Make the difference brushes here.
	diff10 = make_diff_brush(brush,1,0);
	diff11 = make_diff_brush(brush,1,1);
	diff1_1 = make_diff_brush(brush,1,-1);
	diff01 = make_diff_brush(brush,0,1);
	diff0_1 = make_diff_brush(brush,0,-1);
	diff_10 = make_diff_brush(brush,-1,0);
	diff_11 = make_diff_brush(brush,-1,1);
	diff_1_1 = make_diff_brush(brush,-1,-1);

	// Then make the span-lists for the brushes.
	brush_span = make_span_list(brush);
	diff10_span = make_span_list(diff10);
	diff11_span = make_span_list(diff11);
	diff1_1_span = make_span_list(diff1_1);
	diff01_span = make_span_list(diff01);
	diff0_1_span = make_span_list(diff0_1);
	diff_10_span = make_span_list(diff_10);
	diff_11_span = make_span_list(diff_11);
	diff_1_1_span = make_span_list(diff_1_1);
}


uint32**
Brush::GetData(span **sp,int32 dx,int32 dy)
{
	if (dx == 0) {
		if (dy == -1) {
			*sp = diff0_1_span;
			return diff0_1;
		}
		if (dy == 0) {
			*sp = brush_span;
			return brush;
		}
		if (dy == 1) {
			*sp = diff01_span;
			return diff01;
		}
	}
	if (dx == 1) {
		if (dy == -1) {
			*sp = diff1_1_span;
			return diff1_1;
		}
		if (dy == 0) {
			*sp = diff10_span;
			return diff10;
		}
		if (dy == 1) {
			*sp = diff11_span;
			return diff11;
		}
	}
	if (dx == -1) {
		if (dy == -1) {
			*sp = diff_1_1_span;
			return diff_1_1;
		}
		if (dy == 0) {
			*sp = diff_10_span;
			return diff_10;
		}
		if (dy == 1) {
			*sp = diff_11_span;
			return diff_11;
		}
	}
	return NULL;
}


brush_info
Brush::GetInfo()
{
	// Because the brush-generating operations alter width_ and height_, we should
	// use some other variables to store the actual width and height parameters
	// of the brush.
	brush_info info;
	info.shape = shape_;
	info.width = actual_width;
	info.height = actual_height;
	info.angle = angle_;
	info.hardness = hardness_;

	return info;
}


void
Brush::make_rectangular_brush()
{
	// We make a rectangular brush that has the fade length maximum of edge_hardness.
	// If hardness is greater than width/2 or height/2, we will not fade to maximum
	// value in that direction. At the moment the fade is linear.
	float angle_rad = angle_*PI/180.0;
	float half_width = actual_width / 2;
	float half_height = actual_height / 2;

	width_ = ceil(fabs(cos(angle_rad) * half_width) +
		fabs(cos(PI / 2 - angle_rad) * half_height)) * 2 + 1;
	height_ = ceil(fabs(sin(angle_rad) * half_width) +
		fabs(sin(PI / 2 - angle_rad) * half_height)) * 2 + 1;

	uint32 min_dimension = min_c(half_width, half_height);
	uint32 fade_pixels = min_dimension;

	if (hardness_ != 0)
		fade_pixels *= (100. - hardness_) / 100;

	float diff = 1.0 / fade_pixels;

	brush = reserve_brush();
	BPoint p1;
	BPoint c = BPoint(floor(width_ / 2),floor(height_ / 2));
	float x_distance, y_distance;
	float x_value, y_value;
	float value;
	float cos_minus_angle = cos(-angle_rad);
	float sin_minus_angle = sin(-angle_rad);

	for (int32 y = 0;y < height_;y++) {
		for (int32 x = 0;x < width_;x++) {
			p1.x = cos_minus_angle * (x - c.x) - sin_minus_angle * (y - c.y);
			p1.y = sin_minus_angle * (x - c.x) + cos_minus_angle * (y - c.y);

			x_distance = fabs(p1.x);
			if (x_distance <= (half_width - fade_pixels))
				x_value = 1;
			else {
				x_value = 1.0 - (x_distance - (half_width - fade_pixels)) * diff;
			}

			y_distance = fabs(p1.y);
			if (y_distance <= (half_height - fade_pixels))
				y_value = 1;
			else {
				y_value = 1.0 - (y_distance - (half_height - fade_pixels)) * diff;
			}

			value = min_c(x_value, y_value);
			if (value >= 0)
				brush[y][x] = (uint32)(value * 32768);
			else
				brush[y][x] = 0;
		}
	}
}


void
Brush::make_elliptical_brush()
{
	float dimension = max_c(width_, height_) / 2.;
	float ratio;

	if (height_ == width_) {
		ratio = 1.0;
	} else if (height_ > width_) {
		ratio = height_ / width_;
	} else {
		ratio = width_ / height_;
	}

	float w = width_;
	float h = height_;

	// Then we change the width and height to 2*X and the center to X,X.
	width_ = 2 * dimension + 1;
	height_ = 2 * dimension + 1;

	float new_angle = -angle_ * PI / 180.;

	// Then we reserve the space for brush and calculate the entries.
	brush = reserve_brush();

	float diff = 1.0;
	if (hardness_ > 0)
		diff /= (100. - hardness_) / 100.;

	float value = 0;

	float distance;

	float cos_val = cos(new_angle);
	float sin_val = sin(new_angle);

	for (int32 y = 0;y < height_;y++) {
		float sinY = sin_val * (y - dimension);
		float cosY = cos_val * (y - dimension);

		for (int32 x = 0;x < width_;x++) {
			float rotX, rotY;

			rotX = cos_val * (x - dimension) - sinY;
			rotY = sin_val * (x - dimension) + cosY;

			if (w < h)
				rotX = rotX * ratio;
			else
				rotY = rotY * ratio;

			distance = sqrt(pow(rotX, 2) + pow(rotY, 2));
			value = diff * (1 - (distance / dimension));

			value = max_c(value, 0);
			value = min_c(value, 1.0);

			brush[y][x] = (uint32)(value * 32768);
		}
	}
}



uint32**
Brush::reserve_brush()
{
	uint32 **b = NULL;
	try {
		b = new uint32*[(int32)height_];
		for (int32 y=0;y<height_;y++)
			b[y] = new uint32[(int32)width_];
	}
	catch (...) {

	}
	return b;
}


uint32**
Brush::make_diff_brush(uint32 **b,int32 dx, int32 dy)
{
	// How does this function work? Does it even work correctly.

	uint32 **d;
	d = reserve_brush();

	int32 new_x;
	int32 new_y;
	for (int32 y=0;y<height_;y++) {
		for (int32 x=0;x<width_;x++) {
			new_x = x+dx;
			new_y = y+dy;
			if ((new_x<width_) && (new_x>=0) && (new_y < height_) && (new_y >= 0)) {
				float new_value;
				if (b[new_y][new_x] != 1) {
					new_value = (((float)b[y][x]-(float)b[new_y][new_x])/32768.0)/(((float)32768-(float)b[new_y][new_x])/32768.0);
					new_value = max_c(new_value,0.0);
					new_value = min_c(new_value,1.0);
				}
				else
					new_value = 0;
				d[y][x] = (uint32)(new_value*32768);
			}
			else
				d[y][x] = b[y][x];
		}
	}

	return d;
}


span*
Brush::make_span_list(uint32 **b)
{
	// Spans are not optimal solution, because the brush might have just one
	// pixel at both ends. In that case checking the span would mean a lot of
	// useless work.
	bool inside_span;
	span *first = NULL;
	span *current = NULL;

	for (int32 y=0;y<height_;y++) {
		inside_span = FALSE;
		for (int32 x=0;x<width_;x++) {
			if (b[y][x] != 0) {
				if (inside_span == FALSE) {
					if (first == NULL) {
						first = new span(y,x,x);
						current = first;
					}
					else {
						current->next = new span(y,x,x);
						current = current->next;
					}
					inside_span = TRUE;
				}
				else {
					current->span_end = x;
				}
			}
			else {
				if (inside_span == TRUE) {
					inside_span = FALSE;
				}
			}
		}
	}
	return first;
}


void
Brush::delete_all_data()
{
	span *c;
	span *help;
	if (brush != NULL) {
		for (int32 y=0;y<height_;y++)
			delete[] brush[y];

		delete[] brush;
		c = brush_span;
		while (c!=NULL) {
			help = c->next;
			delete c;
			c = help;
		}
	}
	if (diff10 != NULL) {
		for (int32 y=0;y<height_;y++)
			delete[] diff10[y];

		delete[] diff10;
		c = diff10_span;
		while (c!=NULL) {
			help = c->next;
			delete c;
			c = help;
		}
	}
	if (diff11 != NULL) {
		for (int32 y=0;y<height_;y++)
			delete[] diff11[y];

		delete[] diff11;
		c = diff11_span;
		while (c!=NULL) {
			help = c->next;
			delete c;
			c = help;
		}
	}
	if (diff1_1 != NULL) {
		for (int32 y=0;y<height_;y++)
			delete[] diff1_1[y];

		delete[] diff1_1;
		c = diff1_1_span;
		while (c!=NULL) {
			help = c->next;
			delete c;
			c = help;
		}
	}
	if (diff01 != NULL) {
		for (int32 y=0;y<height_;y++)
			delete[] diff01[y];

		delete[] diff01;
		c = diff01_span;
		while (c!=NULL) {
			help = c->next;
			delete c;
			c = help;
		}
	}
	if (diff0_1 != NULL) {
		for (int32 y=0;y<height_;y++)
			delete[] diff0_1[y];

		delete[] diff0_1;
		c = diff0_1_span;
		while (c!=NULL) {
			help = c->next;
			delete c;
			c = help;
		}
	}
	if (diff_10 != NULL) {
		for (int32 y=0;y<height_;y++)
			delete[] diff_10[y];

		delete[] diff_10;
		c = diff_10_span;
		while (c!=NULL) {
			help = c->next;
			delete c;
			c = help;
		}
	}
	if (diff_11 != NULL) {
		for (int32 y=0;y<height_;y++)
			delete[] diff_11[y];

		delete[] diff_11;
		c = diff_11_span;
		while (c!=NULL) {
			help = c->next;
			delete c;
			c = help;
		}
	}
	if (diff_1_1 != NULL) {
		for (int32 y=0;y<height_;y++)
			delete[] diff_1_1[y];

		delete[] diff_1_1;
		c = diff_1_1_span;
		while (c!=NULL) {
			help = c->next;
			delete c;
			c = help;
		}
	}
}


float
Brush::PreviewBrush(BBitmap *preview_bitmap)
{
	float preview_width = width_;
	float preview_height = height_;

	float bmap_width = preview_bitmap->Bounds().Width()+1;
	float bmap_height = preview_bitmap->Bounds().Height()+1;

	while ((preview_width > bmap_width) || (preview_height > bmap_height)) {
		preview_width /= 2.0;
		preview_height /= 2.0;
	}
	int32 scale = (int32)(width_/preview_width);

	int32 top = (int32)((bmap_height-preview_height) / 2.0);
	int32 left = (int32)((bmap_width-preview_width) / 2.0);

	uint32 *bits = (uint32*)preview_bitmap->Bits();
	int32 bpr = preview_bitmap->BytesPerRow()/4;
	int32 bits_length = preview_bitmap->BitsLength()/4;
	// Here we clear the bitmap.
	union {
		char bytes[4];
		uint32 word;
	} color;

	color.bytes[0] = 0xFF;
	color.bytes[1] = 0xFF;
	color.bytes[2] = 0xFF;
	color.bytes[3] = 0x00;

	for (int32 i=0;i<bits_length;i++) {
		*bits++ = color.word;
	}
	bits = (uint32*)preview_bitmap->Bits();

	// Here we draw the brush to the bitmap.
	for (int32 y = 0; y < preview_height; ++y) {
		for (int32 x = 0; x < preview_width; ++x) {
			uchar value = (255 - ((brush[y * scale][x * scale] * 255) >> 15)) & 0xFF;
			color.bytes[0] = value;
			color.bytes[1] = value;
			color.bytes[2] = value;
			color.bytes[3] = 0xFF;
//			value = value << 24 | value << 16 | value << 8 | 0xFF;
			*(bits + (top+y)*bpr + left+x) = color.word;
		}
	}

	// Here we draw a line indicating the relative size of the brush
	color.bytes[0] = 0x00;
	color.bytes[1] = 0x00;
	color.bytes[2] = 0xFF;
	color.bytes[3] = 0xFF;
	bits = (uint32*)preview_bitmap->Bits();
	bits += bpr * (int32)(bmap_height-3)+1;
	*(bits - bpr) = color.word;
	*(bits + bpr) = color.word;
	for (int32 i=0;i<bmap_width*(preview_width/width_)-1;i++)
		*bits++ = color.word;
	bits--;
	*(bits - bpr) = color.word;
	*(bits + bpr) = color.word;

	return preview_width/width_;
}


void
Brush::print_brush(uint32 **b)
{
	printf("Brush:\n");
	for (int32 y=0;y<height_;y++) {
		for (int32 x=0;x<width_;x++) {
			printf("%ld ",b[y][x]);
		}
		printf("\n");
	}
}
