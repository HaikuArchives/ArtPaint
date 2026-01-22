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


#include "BitmapUtilities.h"
#include "HSPolygon.h"
#include "MessageConstants.h"
#include "PixelOperations.h"
#include "StatusView.h"
#include "UtilityClasses.h"


#include <math.h>
#include <stdio.h>


#define PI M_PI


bool
Brush::compare_brushes(brush_info one, brush_info two)
{
	if (one.shape == two.shape
		&& one.width == two.width
		&& one.height == two.height
		&& one.angle == two.angle
		&& one.hardness == two.hardness)
		return true;

	return false;
}


Brush::Brush(brush_info& info)
{
	// First record the data for the brush
	shape_ = info.shape;
	hardness_ = info.hardness;

	actual_width = width_ = info.width;
	actual_height = height_ = info.height;
	angle_ = info.angle;

	brush_span = NULL;

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

	brush_span = make_span_list();

	BList polygons;
	BitmapUtilities::RasterToPolygonsMoore(brush_bmap, brush_bmap->Bounds(), &polygons);

	num_shapes = polygons.CountItems();
	shapes = new HSPolygon*[num_shapes];

	for (uint32 i = 0; i < polygons.CountItems(); ++i) {
		HSPolygon* new_polygon = (HSPolygon*)polygons.ItemAt(i);
		shapes[i] = new_polygon;
	}
}


Brush::~Brush()
{
	delete_all_data();
}


void
Brush::ModifyBrush(brush_info& info, bool notify)
{
	delete_all_data();

	// First record the data for the brush
	shape_ = info.shape;
	hardness_ = info.hardness;

	actual_width = width_ = info.width;
	actual_height = height_ = info.height;
	angle_ = info.angle;

	brush_bmap = NULL;
	brush_span = NULL;

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

	if (brush_bmap == NULL)
		return;

	maximum_width = max_c(height_, width_);
	maximum_height = maximum_width;

	BList polygons;
	BitmapUtilities::RasterToPolygonsMoore(brush_bmap, brush_bmap->Bounds(), &polygons);

	if (shapes != NULL)
		delete[] shapes;
	num_shapes = polygons.CountItems();
	shapes = new HSPolygon*[num_shapes];

	for (uint32 i = 0; i < polygons.CountItems(); ++i) {
		HSPolygon* new_polygon = (HSPolygon*)polygons.ItemAt(i);
		shapes[i] = new_polygon;
	}

	if (notify == true)
		CurrentBrushView::SendMessageToAll(HS_BRUSH_CHANGED);
}


void
Brush::CreateDiffBrushes()
{
	brush_span = make_span_list();
}


uint32*
Brush::GetData(span** sp)
{
	*sp = brush_span;
	return (uint32*)brush_bmap->Bits();
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
	float angle_rad = angle_ * PI / 180.0;
	float half_width = actual_width / 2;
	float half_height = actual_height / 2;

	width_ = ceil(fabs(cos(angle_rad) * half_width)
		+ fabs(cos(PI / 2 - angle_rad) * half_height)) * 2 + 1;
	height_ = ceil(fabs(sin(angle_rad) * half_width)
		+ fabs(sin(PI / 2 - angle_rad) * half_height)) * 2 + 1;

	uint32 min_dimension = min_c(half_width, half_height);
	uint32 fade_pixels = min_dimension;

	if (hardness_ != 0)
		fade_pixels *= (100. - hardness_) / 100;

	float diff = 1.0 / fade_pixels;

	reserve_brush();
	BPoint p1;
	BPoint c = BPoint(ceil(width_ / 2), ceil(height_ / 2));
	float x_distance, y_distance;
	float x_value, y_value;
	float value;
	float cos_minus_angle = cos(-angle_rad);
	float sin_minus_angle = sin(-angle_rad);

	BitmapUtilities::ClearBitmap(brush_bmap, 0);
	uint32* bits = (uint32*)brush_bmap->Bits();
	uint32 bpr = brush_bmap->BytesPerRow() / 4;
	for (int32 y = 0; y < height_; y++) {
		for (int32 x = 0; x < width_; x++) {
			p1.x = cos_minus_angle * (x - c.x) - sin_minus_angle * (y - c.y);
			p1.y = sin_minus_angle * (x - c.x) + cos_minus_angle * (y - c.y);

			x_distance = fabs(p1.x);
			if (x_distance <= (half_width - fade_pixels))
				x_value = 1;
			else
				x_value = 1.0 - (x_distance - (half_width - fade_pixels)) * diff;

			y_distance = fabs(p1.y);
			if (y_distance <= (half_height - fade_pixels))
				y_value = 1;
			else
				y_value = 1.0 - (y_distance - (half_height - fade_pixels)) * diff;

			value = min_c(x_value, y_value);
			if (value >= 0) {
				union color_conversion color;
				color.bytes[0] = value * 255;
				color.bytes[1] = value * 255;
				color.bytes[2] = value * 255;
				color.bytes[3] = value * 255;

				*(bits + x + y * bpr) = color.word;
			} else
				*(bits + x + y * bpr) = 0;
		}
	}
}


void
Brush::make_elliptical_brush()
{
	float dimension = (max_c(width_, height_) + 1.) / 2.;
	float ratio;
	dimension = max_c(dimension, 1.0);

	if (height_ == width_)
		ratio = 1.0;
	else if (height_ > width_)
		ratio = height_ / width_;
	else
		ratio = width_ / height_;

	float w = width_;
	float h = height_;

	// Then we change the width and height to 2*X and the center to X,X.
	width_ = 2 * dimension + 1;
	height_ = 2 * dimension + 1;

	float new_angle = -angle_ * PI / 180.;

	// Then we reserve the space for brush and calculate the entries.
	reserve_brush();

	float diff = 1.0;
	if (hardness_ > 0)
		diff /= (100. - hardness_) / 100.;

	float value = 0;

	float distance;

	float cos_val = cos(new_angle);
	float sin_val = sin(new_angle);

	BitmapUtilities::ClearBitmap(brush_bmap, 0);
	uint32* bits = (uint32*)brush_bmap->Bits();
	uint32 bpr = brush_bmap->BytesPerRow() / 4;

	for (int32 y = 0; y < height_; y++) {
		float sinY = sin_val * (y - dimension);
		float cosY = cos_val * (y - dimension);

		for (int32 x = 0; x < width_; x++) {
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

			union color_conversion color;
			color.bytes[0] = value * 255;
			color.bytes[1] = value * 255;
			color.bytes[2] = value * 255;
			color.bytes[3] = value * 255;

			*(bits + x + y * bpr) = color.word;
		}
	}
}


void
Brush::reserve_brush()
{
	brush_bmap = new (std::nothrow) BBitmap(BRect(0, 0, width_ + 1, height_ + 1), B_RGBA32);
}


span*
Brush::make_span_list()
{
	// Spans are not optimal solution, because the brush might have just one
	// pixel at both ends. In that case checking the span would mean a lot of
	// useless work.
	bool inside_span;
	span* first = NULL;
	span* current = NULL;

	uint32* bits = (uint32*)brush_bmap->Bits();
	uint32 bpr = brush_bmap->BytesPerRow() / 4;
	for (int32 y = 0; y < height_; y++) {
		inside_span = FALSE;
		for (int32 x = 0; x < width_; x++) {
			union color_conversion color;
			color.word = *(bits + x + y * bpr);
			if (color.bytes[3] != 0) {
				if (inside_span == FALSE) {
					if (first == NULL) {
						first = new span(y, x, x);
						current = first;
					} else {
						current->next = new span(y, x, x);
						current = current->next;
					}
					inside_span = TRUE;
				} else
					current->span_end = x;
			} else {
				if (inside_span == TRUE)
					inside_span = FALSE;
			}
		}
	}
	return first;
}


void
Brush::delete_all_data()
{
	span* c;
	span* help;
	if (brush_bmap != NULL) {
		delete brush_bmap;
		c = brush_span;
		while (c != NULL) {
			help = c->next;
			delete c;
			c = help;
		}
	}
}


float
Brush::PreviewBrush(BBitmap* preview_bitmap)
{
	float bmap_width = (preview_bitmap->Bounds().Width() + 1);
	float bmap_height = (preview_bitmap->Bounds().Height() + 1);

	float preview_width = 0.9 * bmap_width;
	float preview_height = 0.9 * bmap_height;

	float max_dim = max_c(width_, height_);

	float scale = max_c(1.0, max_dim / preview_width);

	uint32* bits = (uint32*)preview_bitmap->Bits();
	int32 bpr = preview_bitmap->BytesPerRow() / 4;

	// Here we clear the bitmap.
	union color_conversion color;

	color.bytes[0] = 0xFF;
	color.bytes[1] = 0xFF;
	color.bytes[2] = 0xFF;
	color.bytes[3] = 0x00;

	BitmapUtilities::ClearBitmap(preview_bitmap, color.word);

	if (brush_bmap != NULL) {
		brush_bmap->Lock();
		uint32* brush_bits = (uint32*)brush_bmap->Bits();
		uint32 brush_bpr = brush_bmap->BytesPerRow() / 4;
		brush_bmap->Unlock();

		int32 draw_width = brush_bpr / scale;
		int32 draw_height = brush_bmap->Bounds().Height() / scale;

		int32 top = (int32)((bmap_height - draw_height) / 2.0);
		int32 left = (int32)((bmap_width - draw_width) / 2.0);

		// Here we draw the brush to the bitmap.
		for (int32 y = 0; y < preview_height; ++y) {
			if (ceil(y * scale) > brush_bmap->Bounds().Height())
				break;
			for (int32 x = 0; x < preview_width; ++x) {
				if (ceil(x * scale) > brush_bpr)
					break;

				union color_conversion color;
				color.word = *(brush_bits + (int32)ceil(x * scale) + ((int32)ceil(y * scale) * brush_bpr));
				color.bytes[0] = 255 - color.bytes[0];
				color.bytes[1] = 255 - color.bytes[1];
				color.bytes[2] = 255 - color.bytes[2];

				color.bytes[3] = 0xFF;
				*(bits + (top + y) * bpr + left + x) = color.word;
			}
		}
	}

	// Add the brush size to the bottom of the preview
	BFont font;
	font.SetSize(bmap_width / 6);
	BString brushSizeString;
	float brushSize = max_c(actual_width, actual_height);
	brushSizeString.SetToFormat("%d", (int32)brushSize);

	int32 strWidth = font.StringWidth("000");
	font_height strHeight;
	font.GetHeight(&strHeight);
	int32 strX = 0;
	int32 strY = (int32)(bmap_height - 1);
	BRect fontBGRect(strX - 1, strY - strHeight.ascent + 2,
		strX + strWidth + 2, bmap_height);

	preview_bitmap->Lock();
	BView* numView = new BView(preview_bitmap->Bounds(), "", B_FOLLOW_NONE, B_WILL_DRAW);
	preview_bitmap->AddChild(numView);
	rgb_color highColor = numView->HighColor();
	numView->StrokeRect(preview_bitmap->Bounds(), B_SOLID_HIGH);
	numView->StrokeRect(fontBGRect.OffsetByCopy(1.0, -1.0), B_SOLID_HIGH);
	numView->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	numView->FillRect(fontBGRect, B_SOLID_HIGH);
	numView->MovePenTo(BPoint(strX, strY));
	numView->SetHighColor(highColor);
	numView->SetFont(&font);
	numView->DrawString(brushSizeString);

	numView->Sync();
	preview_bitmap->Unlock();

	return preview_width / max_dim;
}


void
Brush::print_brush(uint32** b)
{
	printf("Brush:\n");
	for (int32 y = 0; y < height_; y++) {
		for (int32 x = 0; x < width_; x++)
			printf("%" B_PRIu32 " ", b[y][x]);

		printf("\n");
	}
}


void
Brush::draw(BBitmap* buffer, BPoint point, Selection* selection)
{
	BRect bitmap_bounds = buffer->Bounds();

	int32 left_bound = (int32)bitmap_bounds.left;
	int32 right_bound = (int32)bitmap_bounds.right;
	int32 top_bound = (int32)bitmap_bounds.top;
	int32 bottom_bound = (int32)bitmap_bounds.bottom;

	span* spans = brush_span;
	int32 px = (int32)point.x;
	int32 py = (int32)point.y;

	if (brush_bmap == NULL)
		return;

	uint32* brush_bits = (uint32*)brush_bmap->Bits();
	uint32 brush_bpr = brush_bmap->BytesPerRow() / 4;
	uint32* bits = (uint32*)buffer->Bits();
	uint32 bpr = buffer->BytesPerRow() / 4;
	uint32* target_bits;
	while ((spans != NULL) && (spans->row + py <= bottom_bound)) {
		int32 left = max_c(px + spans->span_start, left_bound);
		int32 right = min_c(px + spans->span_end, right_bound);
		int32 y = spans->row;
		if (y + py >= top_bound) {
			// This works even if there are many spans in one row.
			target_bits = bits + (y + py) * bpr + left;
			for (int32 x = left; x <= right; ++x) {
				if (selection->IsEmpty() || selection->ContainsPoint(x, y + py)) {
					float sel_alpha = 1.0;
					if (selection->IsEmpty() == false && selection->ContainsPoint(x, y + py))
						sel_alpha = selection->Value(x, y + py) / 255.;

					union color_conversion brush_color, target_color, result;
					brush_color.word = *(brush_bits + (x - px) + y * brush_bpr);
					brush_color.bytes[0] = 0xFF;
					brush_color.bytes[1] = 0xFF;
					brush_color.bytes[2] = 0xFF;
					brush_color.bytes[3] *= sel_alpha;

					target_color.word = *target_bits;

					for (int i = 0; i < 4; ++i)
						result.bytes[i] = max_c(target_color.bytes[i], brush_color.bytes[i]);

					*target_bits = result.word;
				}
				target_bits++;
			}
		}
		spans = spans->next;
	}
}


BRect
Brush::draw_line(BBitmap* buffer, BPoint start, BPoint end, Selection* selection)
{
	int32 brush_width_per_2 = (int32)floor(this->Width() / 2);
	int32 brush_height_per_2 = (int32)floor(this->Height() / 2);
	BRect a_rect = MakeRectFromPoints(start, end);
	a_rect.InsetBy(-brush_width_per_2 - 1, -brush_height_per_2 - 1);

	// first check whether the line is longer in x direction than y
	bool increase_x = fabs(start.x - end.x) >= fabs(start.y - end.y);

	// check which direction the line is going
	float sign_x;
	float sign_y;
	int32 number_of_points;
	if ((end.x - start.x) != 0)
		sign_x = (end.x - start.x) / fabs(start.x - end.x);
	else
		sign_x = 0;

	if ((end.y - start.y) != 0)
		sign_y = (end.y - start.y) / fabs(start.y - end.y);
	else
		sign_y = 0;

	int32 new_x, new_y;
	BPoint last_point;

	if (increase_x) {
		float y_add = ((float)fabs(start.y - end.y)) / ((float)fabs(start.x - end.x));
		number_of_points = (int32)fabs(start.x - end.x);
		for (int32 i = 0; i < number_of_points; i++) {
			last_point = start;
			start.x += sign_x;
			start.y += sign_y * y_add;
			new_x = (int32)round(start.x);
			new_y = (int32)round(start.y);

			this->draw(
				buffer, BPoint(new_x - brush_width_per_2, new_y - brush_height_per_2), selection);
		}
	} else {
		float x_add = ((float)fabs(start.x - end.x)) / ((float)fabs(start.y - end.y));
		number_of_points = (int32)fabs(start.y - end.y);
		for (int32 i = 0; i < number_of_points; i++) {
			last_point = start;
			start.y += sign_y;
			start.x += sign_x * x_add;
			new_x = (int32)round(start.x);
			new_y = (int32)round(start.y);

			this->draw(
				buffer, BPoint(new_x - brush_width_per_2, new_y - brush_height_per_2), selection);
		}
	}

	return a_rect;
}


int
Brush::GetShapes(BPolygon** poly_shapes)
{
	for (int i = 0; i < num_shapes; ++i)
		poly_shapes[i] = shapes[i]->GetBPolygon();

	return num_shapes;
}
