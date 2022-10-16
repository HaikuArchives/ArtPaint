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

#include "HSPolygon.h"
#include "ImageView.h"
#include "Patterns.h"


#include <Debug.h>
#include <Window.h>


#include <new>
#include <string.h>


Selection::Selection(BRect imageBounds)
	: selection_data(NULL),
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
		delete [] original_selections;
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
Selection::SetSelectionData(const SelectionData *data)
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
		selection_view = new BView(image_bounds, "", B_FOLLOW_NONE,
			B_WILL_DRAW);
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

		drawer_thread = spawn_thread(&thread_entry_func, "selection drawer",
			B_NORMAL_PRIORITY, (void*)this);
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
		delete [] original_selections;
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
	} else {
		poly->ChangeDirection(HS_POLYGON_CLOCKWISE);
	}

	selection_data->AddSelection(poly);

	if (selection_map == NULL) {
		selection_map = new BBitmap(image_bounds, B_GRAY8, true);
		selection_view = new BView(image_bounds, "", B_FOLLOW_NONE,
			B_WILL_DRAW);
		selection_map->AddChild(selection_view);
		selection_bits = (uint8*)selection_map->Bits();
		selection_bpr = selection_map->BytesPerRow();

		if (selection_map->Lock()) {
			rgb_color low = selection_view->LowColor();
			rgb_color high = selection_view->HighColor();

			selection_view->SetLowColor(255, 255, 255, 255);
			selection_view->SetHighColor(0, 0, 0, 0);

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
		selection_view->SetHighColor(0, 0, 0, 0);

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
		delete [] original_selections;
		original_selections = NULL;
	}

	selection_bounds = BRect();

	if (selection_map == NULL) {
		selection_map = new BBitmap(image_bounds, B_GRAY8, true);
		selection_view = new BView(image_bounds, "", B_FOLLOW_NONE,
			B_WILL_DRAW);
		selection_map->AddChild(selection_view);
		selection_bits = (uint8*)selection_map->Bits();
		selection_bpr = selection_map->BytesPerRow();

		if (selection_map->Lock()) {
			rgb_color low = selection_view->LowColor();
			rgb_color high = selection_view->HighColor();

			selection_view->SetLowColor(255, 255, 255, 255);
			selection_view->SetHighColor(0, 0, 0, 0);

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
	int32 height
		= min_c(image_bounds.IntegerHeight(), bitmap->Bounds().IntegerHeight());
	for (int32 y = 0; y <= height; ++y) {
		for (int32 x = 0; x < width; ++x) {
			uint8* ptr = selection_bits + x + y * selection_bpr;
			if (add_to_selection) {
				*ptr = *ptr | *(new_bits + x + y * new_bpr);
			} else {
				*ptr = *ptr & ~(*(new_bits + x + y * new_bpr));
			}
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
		delete [] original_selections;
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
					if (x<width) {
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

	// This is almost like dilation. First we invert the bitmap, then dilate
	// and then invert again.

	selection_bounds = BRect();

	if (selection_map) {
		int8* old_bits = (int8*)selection_map->Bits();
		int32 bits_length = selection_map->BitsLength();
		for (int32 i = 0; i < bits_length; ++i) {
			*old_bits = ~(*old_bits);
			old_bits++;
		}
		old_bits = (int8*)selection_map->Bits();

		BBitmap* new_map = new BBitmap(selection_map->Bounds(), B_GRAY8);
		int32 new_bpr = new_map->BytesPerRow();
		int8* new_bits = (int8*)new_map->Bits();

		memcpy(new_bits, old_bits, bits_length);

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
					if (x<width) {
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

		memcpy(old_bits, new_bits, bits_length);
		delete new_map;

		for (int32 i = 0; i < bits_length; ++i) {
			*old_bits = ~(*old_bits);
			old_bits++;
		}

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
	acquire_sem(selection_mutex);	// This might actually not be useful.
	if (original_selections == NULL) {
		original_selections = new HSPolygon*[selection_data->SelectionCount()];
		for (int32 i = 0; i < selection_data->SelectionCount(); i++) {
			original_selections[i] =
				new HSPolygon(selection_data->ReturnSelectionAt(i));
		}
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
		for (int32 i = 0; i < selection_data->SelectionCount(); i++) {
			delete original_selections[i];
		}
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
Selection::Recalculate()
{
	if (needs_recalculating) {
		if (original_selections) {
			for (int32 i = 0; i < selection_data->SelectionCount(); ++i)
				delete original_selections[i];
			delete [] original_selections;
			original_selections = NULL;
		}

		if (selection_map && selection_map->Lock()) {
			// First clear the selection
			selection_view->FillRect(selection_map->Bounds(), B_SOLID_HIGH);
			selection_view->Sync();

			for (int32 i = 0; i < selection_data->SelectionCount(); ++i) {
				if (HSPolygon* p = selection_data->ReturnSelectionAt(i)) {
					BPolygon* bPoly = p->GetBPolygon();
					if (p->GetDirection() == HS_POLYGON_CLOCKWISE) {
						// selection
						selection_view->FillPolygon(bPoly, B_SOLID_LOW);
						selection_view->StrokePolygon(bPoly, true, B_SOLID_LOW);
					} else if (p->GetDirection() ==
						HS_POLYGON_COUNTERCLOCKWISE) {
						// a de-selection
						selection_view->FillPolygon(bPoly, B_SOLID_HIGH);
						selection_view->StrokePolygon(bPoly, true,
							B_SOLID_HIGH);
					}
					delete bPoly;
				}
			}
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
			selection_view = new BView(image_bounds, "selection view",
				B_FOLLOW_NONE, B_WILL_DRAW);
			selection_map->AddChild(selection_view);
			selection_bits = (uint8*)selection_map->Bits();
			selection_bpr = selection_map->BytesPerRow();

			if (selection_map->Lock()) {
				selection_view->FillRect(image_bounds, B_SOLID_HIGH);
				selection_map->Unlock();
			}

			int32 height = min_c(previous_map->Bounds().IntegerHeight(),
				selection_map->Bounds().IntegerHeight());
			int32 width = min_c(selection_bpr,
				uint32(previous_map->BytesPerRow()));

			int32 previous_bpr = previous_map->BytesPerRow();
			uint8* previous_bits = (uint8*)previous_map->Bits();

			for (int32 y = 0; y <= height; y++) {
				for (int32 x = 0; x < width; x++) {
					*(selection_bits + x + y * selection_bpr) =
						*(previous_bits + x + y * previous_bpr);
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
		for (int32 i = 0; i < selection_data->SelectionCount(); i++) {
			delete original_selections[i];
		}
		delete[] original_selections;
		original_selections = NULL;
	}

	if (!IsEmpty()) {
		uint8* bits = (uint8*)selection_map->Bits();
		int32 bits_length = selection_map->BitsLength();
		for (int32 i = 0; i < bits_length; i++) {
			*bits = ~(*bits);
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
	} else {
		selection_bounds = image_bounds;
	}
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


int32 Selection::thread_func()
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

	if (IsEmpty() == TRUE) {
		continue_drawing = FALSE;
	}

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
	// This function uses the selection_map to make
	// a new set of polygons that make up the selection.
	// The selection_map is searched for the edges of selections
	// and then those edges are traced to form the polygons.
	// This version makes the polygons by connecting centers
	// of edge pixels.
	BRect bounds = GetBoundingRect();
	int32 leftb = (int32)bounds.left;
	int32 rightb = (int32)bounds.right;
	int32 topb = (int32)bounds.top;
	int32 bottomb = (int32)bounds.bottom;

	BPoint neighbors[4] = {
		BPoint(-1,  0),
		BPoint( 0, -1),
		BPoint( 1,  0),
		BPoint( 0,  1)
	};

	int8 direction;
	bool wall;

	PointContainer* included_points = new PointContainer();

	selection_data->EmptySelectionData();

	for (int32 y = topb; y <= bottomb; y++) {
		for (int32 x = leftb; x <= rightb; x++) {
			// We start a new polygon if point at x,y is at the edge and
			// has not been included in previous polygons.
			direction = -1;
			wall = false;
			if (ContainsPoint(x, y) &&
				included_points->HasPoint(x, y) == false) {
				// If this point is at the edge we start making a polygon.
				for (int i = 0; i < 4; ++i) {
					if (!ContainsPoint(x + neighbors[i].x,
							y + neighbors[i].y)) {
						direction = (i + 1) % 4;
						wall = true;
						i = 4;
					}
				}

				if ((direction != -1) && (wall == true)) {
					int32 max_point_count = 32;
					BPoint* point_list = new BPoint[max_point_count];
					int32 point_count = 0;

					BPoint starting_point(x, y);
					int32 turns = 0;
					included_points->InsertPoint(int32(starting_point.x),
						int32(starting_point.y));
					BPoint next_point;
					next_point = starting_point;
					point_list[point_count++] = starting_point;

					// This if structure decides what is the next point.
					int32 new_x = (int32)next_point.x;
					int32 new_y = (int32)next_point.y;

					int8 left = (direction + 3) % 4;
					int8 right = (direction + 1) % 4;
					int8 down = (direction + 2) % 4;
					if (ContainsPoint(new_x + neighbors[left].x,
						new_y + neighbors[left].y)) {
						--turns;
						direction = left;
					} else if (!ContainsPoint(new_x + neighbors[direction].x,
						new_y + neighbors[direction].y)) {
						if (ContainsPoint(new_x + neighbors[right].x,
							new_y + neighbors[right].y)) {
							++turns;
							direction = right;
						} else if (ContainsPoint(new_x + neighbors[down].x,
							new_y + neighbors[down].y)) {
							turns += 2;
							direction = down;
						} else
							direction = -1;
					}

					if (direction >= 0) {
						new_x += neighbors[direction].x;
						new_y += neighbors[direction].y;
					}

					next_point = BPoint(new_x, new_y);
					while (next_point != starting_point && direction != -1) {
						included_points->InsertPoint(int32(next_point.x),
							int32(next_point.y));
						point_list[point_count++] = next_point;
						if (point_count == max_point_count) {
							max_point_count *= 2;
							BPoint* new_points = new BPoint[max_point_count];
							for (int32 i = 0; i < point_count; i++)
								new_points[i] = point_list[i];

							delete[] point_list;
							point_list = new_points;
						}

						// This if structure decides what is the next point.
						int32 new_x = (int32)next_point.x;
						int32 new_y = (int32)next_point.y;

						int8 left = (direction + 3) % 4;
						int8 right = (direction + 1) % 4;
						int8 down = (direction + 2) % 4;
						if (ContainsPoint(new_x + neighbors[left].x,
							new_y + neighbors[left].y)) {
							--turns;
							direction = left;
						} else if (!ContainsPoint(
							new_x + neighbors[direction].x,
							new_y + neighbors[direction].y)) {
							if (ContainsPoint(new_x + neighbors[right].x,
								new_y + neighbors[right].y)) {
								++turns;
								direction = right;
							} else if (ContainsPoint(new_x + neighbors[down].x,
								new_y + neighbors[down].y)) {
								turns += 2;
								direction = down;
							} else
								direction = -1;
						}

						if (direction >= 0) {
							new_x += neighbors[direction].x;
							new_y += neighbors[direction].y;
						}

						next_point = BPoint(new_x, new_y);
					}

					HSPolygon* new_polygon;
					if (turns > 0)
						new_polygon = new HSPolygon(point_list, point_count,
							HS_POLYGON_CLOCKWISE);
					else
						new_polygon = new HSPolygon(point_list, point_count,
							HS_POLYGON_COUNTERCLOCKWISE);

					delete[] point_list;
					selection_data->AddSelection(new_polygon);
				}
			}
		}
	}

	delete included_points;

	if (selection_data->SelectionCount() == 0) {
		Clear();
	} else if (selection_data->SelectionCount() == 1) {
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


//---------------

// primes 1021,2311
PointContainer::PointContainer()
	: hash_table_size(2311)
{
	hash_table = new BPoint*[hash_table_size];
	list_length_table = new int32[hash_table_size];
	for (int32 i = 0; i < hash_table_size; i++) {
		hash_table[i] = NULL;
		list_length_table[i] = 0;
	}
}


PointContainer::~PointContainer()
{
	int32 slots = 0;
	int32 hits = 0;
	for (int32 i = 0; i < hash_table_size; i++) {
		if (list_length_table[i] > 0) {
			slots++;
			hits += list_length_table[i];
		}
	}

	for (int32 i = 0; i < hash_table_size; i++) {
		delete[] hash_table[i];
		hash_table[i] = NULL;
	}
	delete[] hash_table;
	delete[] list_length_table;
}


void
PointContainer::InsertPoint(int32 x, int32 y)
{
	int32 key = hash_value(x, y);

	if (list_length_table[key] == 0) {
		hash_table[key] = new BPoint[1];
		list_length_table[key] = 1;
		hash_table[key][0] = BPoint(x, y);
	} else {
		BPoint* new_array = new BPoint[list_length_table[key] + 1];

		for (int32 i = 0; i < list_length_table[key]; i++) {
			new_array[i] = hash_table[key][i];
		}

		delete[] hash_table[key];
		hash_table[key] = new_array;
		hash_table[key][list_length_table[key]] = BPoint(x, y);
		list_length_table[key] += 1;
	}
}


bool
PointContainer::HasPoint(int32 x, int32 y)
{
	int32 key = hash_value(x, y);
	bool has = FALSE;
	BPoint point(x, y);
	for (int32 i = 0; i < list_length_table[key] && has == FALSE; i++) {
		if (hash_table[key][i] == point)
			has = TRUE;
	}

	return has;
}


int32
PointContainer::hash_value(int32 x, int32 y)
{
	int32 value;
	value = (x + (y << 8)) % hash_table_size;
	return value;
}


// --------------------------
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

	for (int32 i = 0; i < number_of_selections; i++) {
		selections[i] = new HSPolygon(original->selections[i]);
	}
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
			if (i<number_of_selections) {
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
