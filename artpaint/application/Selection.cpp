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

#include "Selection.h"

#include "BitmapUtilities.h"
#include "HSPolygon.h"
#include "ImageView.h"
#include "Patterns.h"
#include "UtilityClasses.h"


#include <Debug.h>
#include <Window.h>


#include <new>
#include <stdio.h>
#include <string.h>


Selection::Selection(BRect imageBounds)
	:
	selection_data(NULL),
	original_selections(NULL),
	selection_map(NULL),
	selection_view(NULL),
	selection_bits(NULL),
	selection_bpr(0),
	selection_bounds(imageBounds),
	image_bounds(imageBounds),
	image_view(NULL),
	needs_recalculating(false),
	view_magnifying_scale(0),
	animation_offset(0),
	drawer_thread(-1),
	continue_drawing(false),
	selection_mutex(-1)
{
	selection_data = new SelectionData();
	selection_mutex = create_sem(1, "selection_mutex");
}


Selection::~Selection()
{
	if (original_selections) {
		for (int32 i = 0; i < selection_data->SelectionCount(); ++i)
			delete original_selections[i];
		delete[] original_selections;
		original_selections = NULL;
	}

	delete selection_data;

	if (continue_drawing) {
		continue_drawing = false;
		status_t value;
		wait_for_thread(drawer_thread, &value);
	}

	delete_sem(selection_mutex);
}


void
Selection::SetSelectionData(const SelectionData* data)
{
	acquire_sem(selection_mutex);

	if (!(*selection_data == *data)) {
		delete selection_data;
		selection_data = new SelectionData(data);
	}

	if (selection_data->SelectionCount() == 0)
		Clear();
	else if (selection_map == NULL) {
		selection_map = new BBitmap(image_bounds, B_GRAY8, true);
		selection_view = new BView(image_bounds, "", B_FOLLOW_NONE, B_WILL_DRAW);
		selection_map->AddChild(selection_view);
		selection_bits = (uint8*)selection_map->Bits();
		selection_bpr = selection_map->BytesPerRow();

		if (selection_map->Lock()) {
			selection_view->FillRect(image_bounds, B_SOLID_HIGH);
			selection_view->StrokeRect(image_bounds, B_SOLID_HIGH);
			selection_map->Unlock();
		}
	}

	needs_recalculating = true;
	Recalculate();

	release_sem(selection_mutex);

	StartDrawing(image_view, view_magnifying_scale);
}


void
Selection::StartDrawing(BView* view, float mag_scale)
{
	if (continue_drawing == false) {
		int32 value;
		wait_for_thread(drawer_thread, &value);
		continue_drawing = true;

		image_view = view;
		view_magnifying_scale = mag_scale;

		drawer_thread
			= spawn_thread(&thread_entry_func, "selection drawer", B_NORMAL_PRIORITY, (void*)this);
		resume_thread(drawer_thread);
	}
}


void
Selection::StopDrawing()
{
	if (drawer_thread > 0)
		kill_thread(drawer_thread);

	continue_drawing = false;
}


void
Selection::AddSelection(HSPolygon* poly, bool add_to_selection)
{
	acquire_sem(selection_mutex);

	if (original_selections) {
		for (int32 i = 0; i < selection_data->SelectionCount(); ++i)
			delete original_selections[i];
		delete[] original_selections;
		original_selections = NULL;
	}

	selection_bounds = BRect();
	HSPolygon* bound_poly = NULL;

	// Selections are HS_POLYGON_CLOCKWISE and de-selections
	// HS_POLYGON_COUNTERCLOCKWISE
	if (!add_to_selection) {
		poly->ChangeDirection(HS_POLYGON_COUNTERCLOCKWISE);
		if (IsEmpty()) {
			BPoint points[4];
			points[0] = image_bounds.LeftTop();
			points[1] = image_bounds.RightTop();
			points[2] = image_bounds.RightBottom();
			points[3] = image_bounds.LeftBottom();
			bound_poly = new (std::nothrow) HSPolygon(points, 4);
			if (bound_poly) {
				bound_poly->ChangeDirection(HS_POLYGON_CLOCKWISE);
				selection_data->AddSelection(bound_poly);
			}
		}
	} else
		poly->ChangeDirection(HS_POLYGON_CLOCKWISE);

	selection_data->AddSelection(poly);

	if (selection_map == NULL) {
		selection_map = new BBitmap(image_bounds, B_GRAY8, true);
		selection_view = new BView(image_bounds, "", B_FOLLOW_NONE, B_WILL_DRAW);
		selection_map->AddChild(selection_view);
		selection_bits = (uint8*)selection_map->Bits();
		selection_bpr = selection_map->BytesPerRow();

		if (selection_map->Lock()) {
			rgb_color low = selection_view->LowColor();
			rgb_color high = selection_view->HighColor();

			selection_view->SetLowColor(255, 255, 255, 255);
			selection_view->SetHighColor(0, 0, 0, 255);

			selection_view->FillRect(image_bounds, B_SOLID_HIGH);
			selection_view->StrokeRect(image_bounds, B_SOLID_HIGH);

			selection_view->SetLowColor(low);
			selection_view->SetHighColor(high);
			selection_map->Unlock();
		}
	}

	if (selection_map->Lock()) {
		rgb_color low = selection_view->LowColor();
		rgb_color high = selection_view->HighColor();

		selection_view->SetLowColor(255, 255, 255, 255);
		selection_view->SetHighColor(0, 0, 0, 255);

		selection_view->DrawBitmap(selection_map);
		BPolygon* p = poly->GetBPolygon();
		if (!add_to_selection) {
			if (bound_poly) {
				BPolygon* polygon = bound_poly->GetBPolygon();
				selection_view->FillPolygon(polygon, B_SOLID_LOW);
				selection_view->StrokePolygon(polygon, true, B_SOLID_LOW);
				delete polygon;
			}
			selection_view->FillPolygon(p, B_SOLID_HIGH);
			selection_view->StrokePolygon(p, true, B_SOLID_HIGH);
		} else {
			selection_view->FillPolygon(p, B_SOLID_LOW);
			selection_view->StrokePolygon(p, true, B_SOLID_LOW);
		}

		selection_view->Sync();
		selection_view->SetLowColor(low);
		selection_view->SetHighColor(high);
		selection_map->Unlock();
		delete p;
	}

	SimplifySelection();

	release_sem(selection_mutex);
}


void
Selection::AddSelection(BBitmap* bitmap, bool add_to_selection)
{
	if (!bitmap)
		return;

	acquire_sem(selection_mutex);

	if (original_selections) {
		for (int32 i = 0; i < selection_data->SelectionCount(); ++i)
			delete original_selections[i];
		delete[] original_selections;
		original_selections = NULL;
	}

	selection_bounds = BRect();

	if (selection_map == NULL) {
		selection_map = new BBitmap(image_bounds, B_GRAY8, true);
		selection_view = new BView(image_bounds, "", B_FOLLOW_NONE, B_WILL_DRAW);
		selection_map->AddChild(selection_view);
		selection_bits = (uint8*)selection_map->Bits();
		selection_bpr = selection_map->BytesPerRow();

		if (selection_map->Lock()) {
			rgb_color low = selection_view->LowColor();
			rgb_color high = selection_view->HighColor();

			selection_view->SetLowColor(255, 255, 255, 255);
			selection_view->SetHighColor(0, 0, 0, 255);

			selection_view->FillRect(image_bounds, B_SOLID_HIGH);
			selection_view->StrokeRect(image_bounds, B_SOLID_HIGH);

			if (!add_to_selection) {
				selection_view->FillRect(selection_map->Bounds(), B_SOLID_LOW);
				selection_view->StrokeRect(selection_map->Bounds(), B_SOLID_LOW);
			}
			selection_view->Sync();
			selection_view->SetHighColor(high);
			selection_view->SetLowColor(low);
			selection_map->Unlock();
		}
	}

	uint32 new_bpr = bitmap->BytesPerRow();
	uint8* new_bits = (uint8*)bitmap->Bits();

	int32 width = min_c(selection_bpr, new_bpr);
	int32 height = min_c(image_bounds.IntegerHeight(), bitmap->Bounds().IntegerHeight());
	for (int32 y = 0; y <= height; ++y) {
		for (int32 x = 0; x < width; ++x) {
			uint8* ptr = selection_bits + x + y * selection_bpr;
			if (add_to_selection)
				*ptr = *ptr | *(new_bits + x + y * new_bpr);
			else
				*ptr = *ptr & ~(*(new_bits + x + y * new_bpr));
		}
	}

	SimplifySelection();

	release_sem(selection_mutex);
}


void
Selection::ReplaceSelection(BBitmap* bitmap)
{
	if (!bitmap)
		return;

	acquire_sem(selection_mutex);

	if (original_selections) {
		for (int32 i = 0; i < selection_data->SelectionCount(); ++i)
			delete original_selections[i];
		delete [] original_selections;
		original_selections = NULL;
	}

	selection_bounds = BRect();

	delete selection_map;
	selection_map = new BBitmap(image_bounds, B_GRAY8, true);
	selection_view = new BView(image_bounds, "", B_FOLLOW_NONE,
		B_WILL_DRAW);
	selection_map->AddChild(selection_view);
	selection_bits = (uint8*)selection_map->Bits();
	selection_bpr = selection_map->BytesPerRow();

	uint32 new_bpr = bitmap->BytesPerRow();
	uint8* new_bits = (uint8*)bitmap->Bits();

	int32 width = selection_bpr;
	int32 height = image_bounds.IntegerHeight();
	for (int32 y = 0; y <= height; ++y) {
		for (int32 x = 0; x < width; ++x) {
			uint8* ptr = selection_bits + x + y * selection_bpr;
			if (x > new_bpr || y > bitmap->Bounds().IntegerHeight())
				*ptr = 0;
			else
				*ptr = *(new_bits + x + y * new_bpr);
		}
	}

	SimplifySelection();

	release_sem(selection_mutex);
}


void
Selection::SelectAll()
{
	BPoint* corners = new BPoint[4];

	corners[0] = image_bounds.LeftTop();
	corners[1] = image_bounds.RightTop();
	corners[2] = image_bounds.RightBottom();
	corners[3] = image_bounds.LeftBottom();

	HSPolygon* bound_poly = new (std::nothrow) HSPolygon(corners, 4);
	bound_poly->ChangeDirection(HS_POLYGON_CLOCKWISE);
	AddSelection(bound_poly, true);
}


void
Selection::Clear()
{
	if (original_selections) {
		for (int32 i = 0; i < selection_data->SelectionCount(); ++i)
			delete original_selections[i];
		delete[] original_selections;
		original_selections = NULL;
	}

	if (!IsEmpty()) {
		delete selection_map;
		selection_map = NULL;
		selection_bits = NULL;
		selection_bpr = 0;
		selection_bounds = image_bounds;
		selection_data->EmptySelectionData();
	}

	selection_bounds = image_bounds;
}


void
Selection::Dilate()
{
	acquire_sem(selection_mutex);

	/*
		This uses two bitmaps to dilate the selection. Dilation is done with
		the following pattern:

		0	0	0
		0	X	0
		0	0	0

		The pattern is moved over the original selection and whenever the x is
		inside the selection, all 0s and X will be marked selected in the new
		selection. Otherwise x will be marked not selected. First and last
		row/column must be handled as a special case
	*/
	selection_bounds = BRect();

	if (selection_map != NULL) {
		BBitmap* new_map = new BBitmap(selection_map->Bounds(), B_GRAY8);
		int32 new_bpr = new_map->BytesPerRow();
		int8* new_bits = (int8*)new_map->Bits();

		memcpy(new_bits, selection_map->Bits(), selection_map->BitsLength());

		int32 width = selection_map->Bounds().IntegerWidth();
		int32 height = selection_map->Bounds().IntegerHeight();
		for (int32 y = 0; y <= height; ++y) {
			for (int32 x = 0; x <= width; ++x) {
				if (ContainsPoint(x, y)) {
					if (x > 0) {
						*(new_bits + y * new_bpr + (x - 1)) = 0xff;
						if (y > 0) {
							*(new_bits + (y - 1) * new_bpr + (x - 1)) = 0xff;
							*(new_bits + (y - 1) * new_bpr + x) = 0xff;
						}
						if (y < height) {
							*(new_bits + (y + 1) * new_bpr + (x - 1)) = 0xff;
							*(new_bits + (y + 1) * new_bpr + x) = 0xff;
						}
					}
					if (x < width) {
						*(new_bits + y * new_bpr + (x + 1)) = 0xff;
						if (y > 0) {
							*(new_bits + (y - 1) * new_bpr + (x + 1)) = 0xff;
							*(new_bits + (y - 1) * new_bpr + x) = 0xff;
						}
						if (y < height) {
							*(new_bits + (y + 1) * new_bpr + (x + 1)) = 0xff;
							*(new_bits + (y + 1) * new_bpr + x) = 0xff;
						}
					}
				}
			}
		}

		memcpy(selection_map->Bits(), new_bits, selection_map->BitsLength());
		delete new_map;

		SimplifySelection();
	}

	release_sem(selection_mutex);
}


void
Selection::Erode()
{
	acquire_sem(selection_mutex);

	/*
		This uses two bitmaps to erode the selection. Erosion is done with
		the following pattern:

		1	1	1
		1	X	1
		1	1	1

		All 1s must be contained in the selection for every X, otherwise X is
		not included in the new selection. The image edges are always eroded
		because they cannot satisfy this condition.
	*/
	selection_bounds = GetBoundingRect();

	if (selection_map != NULL) {
		BBitmap* new_map = new BBitmap(selection_map->Bounds(), B_GRAY8);
		int32 new_bpr = new_map->BytesPerRow();
		int8* new_bits = (int8*)new_map->Bits();

		memcpy(new_bits, selection_map->Bits(), selection_map->BitsLength());

		int32 width = selection_map->Bounds().IntegerWidth();
		int32 height = selection_map->Bounds().IntegerHeight();
		int32 left = selection_bounds.left;
		int32 right = selection_bounds.right;
		int32 top = selection_bounds.top;
		int32 bottom = selection_bounds.bottom;

		for (int32 y = top; y <= bottom; ++y) {
			for (int32 x = left; x <= right; ++x) {
				if (ContainsPoint(x, y)) {
					// edge pixels get eroded
					if (x == 0 || y == 0 ||
						x == width || y == height)
						*(new_bits + y * new_bpr + x) = 0x00;
					else {
						bool all = true;
						for (int32 yy = y - 1; yy <= y + 1; ++yy) {
							for (int32 xx = x - 1; xx <= x + 1; ++xx) {
								if (ContainsPoint(xx, yy) == false) {
									all = false;
									xx = x + 1;
									yy = y + 1;
								}
							}
						}

						if (all == false)
							*(new_bits + y * new_bpr + x) = 0x00;
					}
				}
			}
		}

		memcpy(selection_map->Bits(), new_bits, selection_map->BitsLength());
		delete new_map;

		SimplifySelection();
	}

	release_sem(selection_mutex);
}


void
Selection::Draw()
{
	acquire_sem(selection_mutex);

	BRect in_rect;
	BRect out_rect;
	int32 ao = animation_offset;

	for (int32 i = 0; i < selection_data->SelectionCount(); i++) {
		HSPolygon* poly = selection_data->ReturnSelectionAt(i);
		BPolygon* p = poly->GetBPolygon();
		out_rect = in_rect = p->Frame();
		out_rect.left *= view_magnifying_scale;
		out_rect.top *= view_magnifying_scale;
		out_rect.right = (out_rect.right + 1) * view_magnifying_scale - 1;
		out_rect.bottom = (out_rect.bottom + 1) * view_magnifying_scale - 1;
		p->MapTo(in_rect, out_rect);
		if ((ao % 8) == 0)
			image_view->StrokePolygon(p, TRUE, HS_ANIMATED_STRIPES_1);
		else if ((ao % 8) == 1)
			image_view->StrokePolygon(p, TRUE, HS_ANIMATED_STRIPES_2);
		else if ((ao % 8) == 2)
			image_view->StrokePolygon(p, TRUE, HS_ANIMATED_STRIPES_3);
		else if ((ao % 8) == 3)
			image_view->StrokePolygon(p, TRUE, HS_ANIMATED_STRIPES_4);
		else if ((ao % 8) == 4)
			image_view->StrokePolygon(p, TRUE, HS_ANIMATED_STRIPES_5);
		else if ((ao % 8) == 5)
			image_view->StrokePolygon(p, TRUE, HS_ANIMATED_STRIPES_6);
		else if ((ao % 8) == 6)
			image_view->StrokePolygon(p, TRUE, HS_ANIMATED_STRIPES_7);
		else if ((ao % 8) == 7)
			image_view->StrokePolygon(p, TRUE, HS_ANIMATED_STRIPES_8);

		delete p;
	}
	release_sem(selection_mutex);
}


void
Selection::RotateTo(BPoint pivot, float angle)
{
	acquire_sem(selection_mutex); // This might actually not be useful.
	if (original_selections == NULL) {
		original_selections = new HSPolygon*[selection_data->SelectionCount()];
		for (int32 i = 0; i < selection_data->SelectionCount(); i++)
			original_selections[i] = new HSPolygon(selection_data->ReturnSelectionAt(i));
	}

	int32 item_count = selection_data->SelectionCount();
	selection_data->EmptySelectionData();

	for (int32 i = 0; i < item_count; i++) {
		HSPolygon* p = new HSPolygon(original_selections[i]);
		p->Rotate(pivot, angle);
		selection_data->AddSelection(p);
	}

	needs_recalculating = TRUE;
	release_sem(selection_mutex);
}


void
Selection::Translate(int32 dx, int32 dy)
{
	acquire_sem(selection_mutex);
	if (original_selections != NULL) {
		for (int32 i = 0; i < selection_data->SelectionCount(); i++)
			delete original_selections[i];

		delete[] original_selections;
		original_selections = NULL;
	}

	for (int32 i = 0; i < selection_data->SelectionCount(); i++) {
		HSPolygon* p = selection_data->ReturnSelectionAt(i);
		p->TranslateBy(dx, dy);
	}

	needs_recalculating = TRUE;
	release_sem(selection_mutex);
}


void
Selection::ScaleBy(BPoint origin, float dx, float dy)
{
	acquire_sem(selection_mutex);
	if (original_selections != NULL) {
		for (int32 i = 0; i < selection_data->SelectionCount(); i++)
			delete original_selections[i];

		delete[] original_selections;
		original_selections = NULL;
	}

	for (int32 i = 0; i < selection_data->SelectionCount(); i++) {
		HSPolygon* p = selection_data->ReturnSelectionAt(i);
		p->ScaleBy(origin, dx, dy);
	}

	needs_recalculating = TRUE;
	release_sem(selection_mutex);
}


void
Selection::ScaleTo(BPoint origin, float x_scale, float y_scale)
{
	acquire_sem(selection_mutex);
	if (original_selections != NULL) {
		for (int32 i = 0; i < selection_data->SelectionCount(); i++)
			delete original_selections[i];

		delete[] original_selections;
		original_selections = NULL;
	}

	BRect bounds = GetBoundingRect();

	float curr_x_scale = bounds.Width();
	float curr_y_scale = bounds.Height();

	float dx = x_scale / curr_x_scale;
	float dy = y_scale / curr_y_scale;

	for (int32 i = 0; i < selection_data->SelectionCount(); i++) {
		HSPolygon* p = selection_data->ReturnSelectionAt(i);
		p->ScaleBy(origin, dx, dy);
	}

	selection_bounds.right = selection_bounds.left + x_scale;
	selection_bounds.bottom = selection_bounds.top + y_scale;
	needs_recalculating = TRUE;
	release_sem(selection_mutex);
}


void
Selection::FlipHorizontally()
{
	acquire_sem(selection_mutex);
	if (original_selections != NULL) {
		for (int32 i = 0; i < selection_data->SelectionCount(); i++)
			delete original_selections[i];

		delete[] original_selections;
		original_selections = NULL;
	}

	BRect bounds = GetBoundingRect();
	float x_axis = (bounds.left + bounds.right) / 2.;
	for (int32 i = 0; i < selection_data->SelectionCount(); i++) {
		HSPolygon* p = selection_data->ReturnSelectionAt(i);
		p->FlipX(x_axis);
	}

	needs_recalculating = TRUE;
	release_sem(selection_mutex);
}


void
Selection::FlipVertically()
{
	acquire_sem(selection_mutex);
	if (original_selections != NULL) {
		for (int32 i = 0; i < selection_data->SelectionCount(); i++)
			delete original_selections[i];

		delete[] original_selections;
		original_selections = NULL;
	}

	BRect bounds = GetBoundingRect();
	float y_axis = (bounds.top + bounds.bottom) / 2.;
	for (int32 i = 0; i < selection_data->SelectionCount(); i++) {
		HSPolygon* p = selection_data->ReturnSelectionAt(i);
		p->FlipY(y_axis);
	}

	needs_recalculating = TRUE;
	release_sem(selection_mutex);
}


void
Selection::Recalculate()
{
	if (needs_recalculating) {
		if (original_selections) {
			for (int32 i = 0; i < selection_data->SelectionCount(); ++i)
				delete original_selections[i];
			delete[] original_selections;
			original_selections = NULL;
		}

		if (selection_map && selection_map->Lock()) {
			// First clear the selection
			rgb_color low = selection_view->LowColor();
			rgb_color high = selection_view->HighColor();

			selection_view->SetLowColor(255, 255, 255, 255);
			selection_view->SetHighColor(0, 0, 0, 255);

			selection_view->FillRect(selection_map->Bounds(), B_SOLID_HIGH);
			selection_view->StrokeRect(selection_map->Bounds(), B_SOLID_HIGH);
			selection_view->Sync();

			for (int32 i = 0; i < selection_data->SelectionCount(); ++i) {
				if (HSPolygon* p = selection_data->ReturnSelectionAt(i)) {
					BPolygon* bPoly = p->GetBPolygon();
					if (p->GetDirection() == HS_POLYGON_CLOCKWISE) {
						// selection
						selection_view->FillPolygon(bPoly, B_SOLID_LOW);
						selection_view->StrokePolygon(bPoly, true, B_SOLID_LOW);
					} else if (p->GetDirection() == HS_POLYGON_COUNTERCLOCKWISE) {
						// a de-selection
						selection_view->FillPolygon(bPoly, B_SOLID_HIGH);
						selection_view->StrokePolygon(bPoly, true, B_SOLID_HIGH);
					}
					delete bPoly;
				}
			}
			selection_view->SetHighColor(high);
			selection_view->SetLowColor(low);
			selection_view->Sync();
			selection_map->Unlock();
		}

		needs_recalculating = false;
		selection_bounds = BRect(0, 0, -1, -1);
	}
}


void
Selection::ImageSizeChanged(BRect rect)
{
	acquire_sem(selection_mutex);
	if (rect != image_bounds) {
		image_bounds = rect;
		if (selection_map != NULL) {
			BBitmap* previous_map = selection_map;
			selection_map = new BBitmap(image_bounds, B_GRAY8, true);
			selection_view = new BView(image_bounds, "selection view", B_FOLLOW_NONE, B_WILL_DRAW);
			selection_map->AddChild(selection_view);
			selection_bits = (uint8*)selection_map->Bits();
			selection_bpr = selection_map->BytesPerRow();

			if (selection_map->Lock()) {
				selection_view->FillRect(image_bounds, B_SOLID_HIGH);
				selection_map->Unlock();
			}

			int32 height = min_c(
				previous_map->Bounds().IntegerHeight(), selection_map->Bounds().IntegerHeight());
			int32 width = min_c(
				selection_bpr, uint32(previous_map->BytesPerRow()));

			int32 previous_bpr = previous_map->BytesPerRow();
			uint8* previous_bits = (uint8*)previous_map->Bits();

			for (int32 y = 0; y <= height; y++) {
				for (int32 x = 0; x < width; x++) {
					*(selection_bits + x + y * selection_bpr)
						= *(previous_bits + x + y * previous_bpr);
				}
			}
			delete previous_map;

			SimplifySelection();
		} else
			selection_bounds = image_bounds;
	}
	release_sem(selection_mutex);
}


BRect
Selection::GetBoundingRect()
{
	if (!selection_bounds.IsValid())
		calculateBoundingRect();

	return selection_bounds;
}


void
Selection::Invert()
{
	acquire_sem(selection_mutex);
	if (original_selections != NULL) {
		for (int32 i = 0; i < selection_data->SelectionCount(); i++)
			delete original_selections[i];

		delete[] original_selections;
		original_selections = NULL;
	}

	if (!IsEmpty()) {
		uint8* bits = (uint8*)selection_map->Bits();
		int32 bits_length = selection_map->BitsLength();
		for (int32 i = 0; i < bits_length; i++) {
			union color_conversion pixel;
			pixel.word = *bits;
			for (int i = 0; i < 4; ++i)
				pixel.bytes[i] = 255 - pixel.bytes[i];

			*bits = pixel.word;
			++bits;
		}
		selection_bounds = BRect(0, 0, -1, -1);
		SimplifySelection();
	}
	release_sem(selection_mutex);
}


bool
Selection::IsEmpty()
{
	return selection_map == NULL;
}


void
Selection::calculateBoundingRect()
{
	if (selection_map) {
		BRect selection(1000000, 1000000, -1000000, -1000000);
		int32 width = selection_map->Bounds().IntegerWidth();
		int32 height = selection_map->Bounds().IntegerHeight();
		for (int32 y = 0; y <= height; y++) {
			for (int32 x = 0; x <= width; x++) {
				if (ContainsPoint(x, y)) {
					selection.left = min_c(selection.left, x);
					selection.top = min_c(selection.top, y);
					selection.right = max_c(selection.right, x);
					selection.bottom = max_c(selection.bottom, y);
				}
			}
		}
		selection_bounds = selection & image_bounds;
	} else
		selection_bounds = image_bounds;
}


void
Selection::deSelect()
{
	Clear();
}


int32
Selection::thread_entry_func(void* data)
{
	return ((Selection*)data)->thread_func();
}


int32
Selection::thread_func()
{
	BWindow* window = image_view->Window();
	while (!window && continue_drawing)
		window = image_view->Window();

	while ((continue_drawing == TRUE) && (IsEmpty() == FALSE)) {
		if (window->LockWithTimeout(0) == B_OK) {
			Draw();
			window->Unlock();
		}
		snooze(500 * 1000);
		animation_offset = animation_offset + 1;
	}

	if (IsEmpty() == TRUE)
		continue_drawing = FALSE;

	return B_OK;
}


void
Selection::ChangeMagnifyingScale(float mag_scale)
{
	view_magnifying_scale = mag_scale;
}


void
Selection::SimplifySelection()
{
	selection_data->EmptySelectionData();

	BRect bounds = GetBoundingRect();

	BList polygons;
	BitmapUtilities::RasterToPolygonsMoore(selection_map, bounds, &polygons);

	for (uint32 i = 0; i < polygons.CountItems(); ++i) {
		HSPolygon* new_polygon = (HSPolygon*)polygons.ItemAt(i);
		selection_data->AddSelection(new_polygon);
	}

	if (selection_data->SelectionCount() == 0)
		Clear();
	else if (selection_data->SelectionCount() == 1) {
		// Count the included and excluded points points
		int32 included = 0;
		int32 excluded = 0;
		int32 width = image_bounds.IntegerWidth();
		int32 height = image_bounds.IntegerHeight();
		for (int32 y = 0; y <= height; ++y) {
			for (int32 x = 0; x <= width; ++x) {
				if (ContainsPoint(x, y))
					included++;
				else
					excluded++;
			}
		}

		if (included == 0 && excluded == 0)
			Clear();
	}
}


SelectionData::SelectionData()
{
	selections_array_size = 4;
	number_of_selections = 0;
	selections = new HSPolygon*[selections_array_size];
	for (int32 i = 0; i < selections_array_size; i++)
		selections[i] = NULL;
}


SelectionData::SelectionData(const SelectionData* original)
{
	selections_array_size = original->selections_array_size;
	number_of_selections = original->number_of_selections;
	selections = new HSPolygon*[selections_array_size];

	for (int32 i = 0; i < selections_array_size; i++)
		selections[i] = NULL;

	for (int32 i = 0; i < number_of_selections; i++)
		selections[i] = new HSPolygon(original->selections[i]);
}


SelectionData::~SelectionData()
{
	EmptySelectionData();
	delete[] selections;
}


void
SelectionData::AddSelection(HSPolygon* poly)
{
	if (number_of_selections == selections_array_size) {
		selections_array_size *= 2;
		HSPolygon** new_array = new HSPolygon*[selections_array_size];
		for (int32 i = 0; i < selections_array_size; i++) {
			if (i < number_of_selections) {
				new_array[i] = selections[i];
				selections[i] = NULL;
			} else
				new_array[i] = NULL;
		}
		delete[] selections;
		selections = new_array;
	}

	selections[number_of_selections] = poly;
	number_of_selections++;
}


void
SelectionData::EmptySelectionData()
{
	for (int32 i = 0; i < number_of_selections; i++) {
		delete selections[i];
		selections[i] = NULL;
	}

	number_of_selections = 0;
}


bool
SelectionData::operator==(const SelectionData& data)
{
	bool similar = TRUE;
	similar = (number_of_selections == data.number_of_selections);

	for (int32 i = 0; i < number_of_selections && similar; i++)
		similar = (*selections[i] == *(data.selections[i]));

	return similar;
}
