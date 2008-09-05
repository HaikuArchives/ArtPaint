/* 

	Filename:	Selection.cpp
	Contents:	Selection-class definitions	
	Author:		Heikki Suhonen
	
*/

#include <string.h>

#include "Selection.h"
#include "Patterns.h"
#include "HSPolygon.h"
#include "ImageView.h"
#include <Window.h>


//#define DEBUG 1
#include <Debug.h>

Selection::Selection(BRect iBounds)
{
	// iBounds is the dimensions of the image.
	number_of_rows = iBounds.IntegerHeight() + 1;
	pixels_in_a_row = iBounds.IntegerWidth() + 1;
	image_bounds = iBounds;
	image_view = NULL;	

	view_magnifying_scale = 0;

	selection_map = NULL;
	selection_bits = NULL;
	selection_bpr = 0;
	
	// At this point the bounding rectangle of the selection is the same as the 
	// whole images bounding rectangle.
	selection_bounds = image_bounds;	

	animation_offset = 0;

//	selections = new BList();
	selection_data = new SelectionData();
	original_selections = NULL;
	selection_iterator = NULL;
	drawer_thread = -1;
	continue_drawing = FALSE;

	needs_recalculating = FALSE;
	
	selection_mutex = create_sem(1,"selection_mutex");		
//	test_window = new BWindow(iBounds,"win",B_TITLED_WINDOW,NULL);
//	test_view = new BView(iBounds,"view",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
//	test_window->AddChild(test_view);
//	test_window->Show();
}


Selection::Selection(const Selection *sel)
{
	number_of_rows = sel->number_of_rows;
	pixels_in_a_row = sel->pixels_in_a_row;
	needs_recalculating = sel->needs_recalculating;
	
	selection_data = new SelectionData(sel->selection_data);
		
	if (sel->original_selections != NULL) {
		original_selections = new HSPolygon*[selection_data->SelectionCount()];
		for (int32 i=0;i<sel->selection_data->SelectionCount();i++) {
			original_selections[i] = new HSPolygon(sel->original_selections[i]);
		}		
	}
	else
		original_selections = NULL;

	if (sel->selection_map != NULL) {		
		sel->selection_map->Lock();
		selection_map = new BBitmap(sel->selection_map->Bounds(),sel->selection_map->ColorSpace(),TRUE);
		selection_map->Lock();
		selection_view = new BView(sel->selection_view->Bounds(),"selection view",B_FOLLOW_NONE,B_WILL_DRAW);
		selection_map->AddChild(selection_view);
		selection_bits = (uint8*)selection_map->Bits();
		
		selection_bpr = selection_map->BytesPerRow();
		for (int32 i=0;i<selection_map->BitsLength();i++)
			*(selection_bits + i) = *(sel->selection_bits + i);
		sel->selection_map->Unlock();
		selection_map->Unlock();
	}	
	else {
 		selection_view = NULL;
 		selection_map = NULL;
	}

	image_bounds = sel->image_bounds;
	selection_bounds = sel->selection_bounds;
	needs_recalculating = sel->needs_recalculating;
	image_view = NULL;
	selection_iterator = NULL;
	view_magnifying_scale = sel->view_magnifying_scale;
	drawer_thread = -1;
	continue_drawing = FALSE;
	selection_mutex = create_sem(1,"selection_mutex");		
}

Selection::~Selection()
{
	if (original_selections != NULL) {
		for (int32 i=0;i<selection_data->SelectionCount();i++) {
			delete original_selections[i];
		}
		delete[] original_selections;
		original_selections = NULL;
	}
	
	delete selection_data;		

	if (continue_drawing == TRUE) {
		continue_drawing = FALSE;
		status_t value;	
		wait_for_thread(drawer_thread,&value);
	}

	delete_sem(selection_mutex);
}



void Selection::SetSelectionData(const SelectionData *data)
{
	acquire_sem(selection_mutex);
	if (!(*selection_data == *data)) {
		delete selection_data;
		selection_data = new SelectionData(data);	
	}
	if (selection_data->SelectionCount() == 0) {
		Clear();
	}
	else if (selection_map == NULL) {
		selection_map = new BBitmap(image_bounds,B_GRAY1,TRUE);
		selection_view = new BView(image_bounds,"selection view",B_FOLLOW_NONE,B_WILL_DRAW);
		selection_map->AddChild(selection_view);
		selection_bits = (uint8*)selection_map->Bits();
		selection_bpr = selection_map->BytesPerRow();		
	}
	needs_recalculating = TRUE;
	Recalculate();
	release_sem(selection_mutex);

	// Restart drawing. If the drawing is already going on this does nothing.
	StartDrawing(image_view,view_magnifying_scale);
}

void Selection::StartDrawing(BView *view, float mag_scale)
{
	if (continue_drawing == FALSE) {
		int32 value;
		wait_for_thread(drawer_thread,&value);
		continue_drawing = TRUE;
		
		image_view = view;
		view_magnifying_scale = mag_scale;
		
		drawer_thread = spawn_thread(&thread_entry_func,"selection drawer",B_NORMAL_PRIORITY,(void*)this);
		resume_thread(drawer_thread);	
	}
}


void Selection::AddSelection(HSPolygon *poly,bool add_to_selection)
{
	// Selections are HS_POLYGON_CLOCKWISE and de-selections HS_POLYGON_COUNTERCLOCKWISE
	acquire_sem(selection_mutex);
	if (original_selections != NULL) {
		for (int32 i=0;i<selection_data->SelectionCount();i++) {
			delete original_selections[i];
		}
		delete[] original_selections;
		original_selections = NULL;
	}

	selection_bounds = BRect(0,0,-1,-1);
	HSPolygon *bound_poly = NULL;
		
	if (add_to_selection) {
		poly->ChangeDirection(HS_POLYGON_CLOCKWISE);
	}
	else {
		poly->ChangeDirection(HS_POLYGON_COUNTERCLOCKWISE);
		if (IsEmpty() == TRUE) {
			BPoint points[4];
			points[0] = image_bounds.LeftTop();
			points[1] = image_bounds.RightTop();
			points[2] = image_bounds.RightBottom();
			points[3] = image_bounds.LeftBottom();
			bound_poly = new HSPolygon(points,4);
			bound_poly->ChangeDirection(HS_POLYGON_CLOCKWISE);
//			bound_poly->SetMaximumInterPointDistance(MAXIMUM_INTER_POINT_DISTANCE);
			selection_data->AddSelection(bound_poly);
		}
	}

	selection_data->AddSelection(poly);	

	if (selection_map == NULL) {
		selection_map = new BBitmap(image_bounds,B_GRAY1,TRUE);
		selection_view = new BView(image_bounds,"selection view",B_FOLLOW_NONE,B_WILL_DRAW);
		selection_map->AddChild(selection_view);
		selection_bits = (uint8*)selection_map->Bits();
		selection_bpr = selection_map->BytesPerRow();
	}
	
	BPolygon *p = poly->GetBPolygon();
	selection_map->Lock();
	if (add_to_selection)
		selection_view->FillPolygon(p,B_SOLID_HIGH);
	else {
		if (bound_poly != NULL)
			selection_view->FillPolygon(bound_poly->GetBPolygon(),B_SOLID_HIGH);
			
		selection_view->FillPolygon(p,B_SOLID_LOW);
	}
				
	selection_view->Sync();
	selection_map->Unlock();	
	delete p;


	SimplifySelection();	
	release_sem(selection_mutex);
}



void Selection::AddSelection(BBitmap *bitmap,bool add_to_selection)
{
	acquire_sem(selection_mutex);
	if (bitmap != NULL) {
		selection_bounds = BRect(0,0,-1,-1);
		if (original_selections != NULL) {
			for (int32 i=0;i<selection_data->SelectionCount();i++) {
				delete original_selections[i];
			}
			delete[] original_selections;
			original_selections = NULL;
		}
		
		if (selection_map == NULL) {
			selection_map = new BBitmap(image_bounds,B_GRAY1,TRUE);
			selection_view = new BView(image_bounds,"selection view",B_FOLLOW_NONE,B_WILL_DRAW);
			selection_map->AddChild(selection_view);
			selection_bits = (uint8*)selection_map->Bits();
			selection_bpr = selection_map->BytesPerRow();
			
			if (add_to_selection == FALSE) {
				selection_map->Lock();
				selection_view->FillRect(selection_map->Bounds(),B_SOLID_HIGH);
				selection_view->Sync();
				selection_map->Unlock();
			}
		}
		uint8 *old_bits = (uint8*)selection_map->Bits();
		uint8 *new_bits = (uint8*)bitmap->Bits();
		
		uint32 old_bpr = selection_map->BytesPerRow();
		uint32 new_bpr = bitmap->BytesPerRow();
		int32 width = min_c(old_bpr,new_bpr);
		int32 height = min_c(selection_map->Bounds().IntegerHeight(),bitmap->Bounds().IntegerHeight());
		for (int32 y=0;y<=height;y++) {
			for (int32 x=0;x<width;x++) {
				if (add_to_selection == TRUE) {
					*(old_bits + x + y*old_bpr) = *(old_bits + x + y*old_bpr) | *(new_bits + x + y*new_bpr);
				}
				else {
					*(old_bits + x + y*old_bpr) = *(old_bits + x + y*old_bpr) & ~(*(new_bits + x + y*new_bpr));
				}
			}
		}
		SimplifySelection();
	}
	release_sem(selection_mutex);
}

void Selection::Clear()
{
	if (original_selections != NULL) {
		for (int32 i=0;i<selection_data->SelectionCount();i++) {
			delete original_selections[i];
		}
		delete[] original_selections;
		original_selections = NULL;
	}

	if (IsEmpty() == FALSE) {
		delete selection_map;
		selection_map = NULL;		
		selection_bits = NULL;
		selection_bpr = 0;
		selection_bounds = image_bounds;

		selection_data->EmptySelectionData();
	}

	selection_bounds = image_bounds;
}



void Selection::Dilatate()
{
	/*
		This uses two bitmaps to dilatate the selection. Dilatation is done with the following
		pattern:
					0	0	0
					0	X	0
					0	0	0
		
		The pattern is moved over the original selection and whenever the x is inside the selection,
		all 0s and X will be marked selected in the new selection. Otherwise x will be marked not selected.
		First and last row/column must be handled as a special case				
				
	*/
	acquire_sem(selection_mutex);
	selection_bounds = BRect(0,0,-1,-1);
	
	if (selection_map != NULL) {
		BBitmap *new_map = new BBitmap(selection_map->Bounds(),B_GRAY1);
		int32 width = selection_map->Bounds().IntegerWidth();
		int32 height = selection_map->Bounds().IntegerHeight();	
		int8 *new_bits = (int8*)new_map->Bits();
		int8 *old_bits = (int8*)selection_map->Bits();
		int32 bits_length = selection_map->BitsLength();
		int32 new_bpr = new_map->BytesPerRow();
		memcpy(new_bits,old_bits,bits_length);
		
		for (int32 y=0;y<=height;y++) {
			for (int32 x=0;x<=width;x++) {
				if (ContainsPoint(x,y)) {
					if (x>0) {
						*(new_bits + y*new_bpr + (x-1)/8) |= (0x80 >> ((x-1)%8));
						if (y > 0) {
							*(new_bits + (y-1)*new_bpr + (x-1)/8) |= (0x80 >> ((x-1)%8));							
							*(new_bits + (y-1)*new_bpr + x/8) |= (0x80 >> (x%8));							
						}
						if (y < height) {
							*(new_bits + (y+1)*new_bpr + (x-1)/8) |= (0x80 >> ((x-1)%8));							
							*(new_bits + (y+1)*new_bpr + x/8) |= (0x80 >> (x%8));													
						}
					}
					if (x<width) {
						*(new_bits + y*new_bpr + (x+1)/8) |= (0x80 >> ((x+1)%8));
						if (y > 0) {
							*(new_bits + (y-1)*new_bpr + (x+1)/8) |= (0x80 >> ((x+1)%8));							
							*(new_bits + (y-1)*new_bpr + x/8) |= (0x80 >> (x%8));							
						}
						if (y < height) {
							*(new_bits + (y+1)*new_bpr + (x+1)/8) |= (0x80 >> ((x+1)%8));							
							*(new_bits + (y+1)*new_bpr + x/8) |= (0x80 >> (x%8));													
						}					
					}					
				}			
			}	
		}	

		memcpy(old_bits,new_bits,bits_length);
		delete new_map;
			
		SimplifySelection();
	}
	release_sem(selection_mutex);
}


void Selection::Erode()
{
	// This is almost like dilatation. First we invert the bitmap, then dilatate and then invert again.
	
	acquire_sem(selection_mutex);
	selection_bounds = BRect(0,0,-1,-1);
	if (selection_map != NULL) {
		BBitmap *new_map = new BBitmap(selection_map->Bounds(),B_GRAY1);
		int32 width = selection_map->Bounds().IntegerWidth();
		int32 height = selection_map->Bounds().IntegerHeight();	
		int8 *new_bits = (int8*)new_map->Bits();
		int8 *old_bits = (int8*)selection_map->Bits();
		int32 bits_length = selection_map->BitsLength();
		int32 new_bpr = new_map->BytesPerRow();
		for (int32 i=0;i<bits_length;i++) {
			*old_bits = ~(*old_bits);
			old_bits++;
		}
		old_bits = (int8*)selection_map->Bits();
		
		memcpy(new_bits,old_bits,bits_length);
		
		for (int32 y=0;y<=height;y++) {
			for (int32 x=0;x<=width;x++) {
				if (ContainsPoint(x,y)) {
					if (x>0) {
						*(new_bits + y*new_bpr + (x-1)/8) |= (0x80 >> ((x-1)%8));
						if (y > 0) {
							*(new_bits + (y-1)*new_bpr + (x-1)/8) |= (0x80 >> ((x-1)%8));							
							*(new_bits + (y-1)*new_bpr + x/8) |= (0x80 >> (x%8));							
						}
						if (y < height) {
							*(new_bits + (y+1)*new_bpr + (x-1)/8) |= (0x80 >> ((x-1)%8));							
							*(new_bits + (y+1)*new_bpr + x/8) |= (0x80 >> (x%8));													
						}
					}
					if (x<width) {
						*(new_bits + y*new_bpr + (x+1)/8) |= (0x80 >> ((x+1)%8));
						if (y > 0) {
							*(new_bits + (y-1)*new_bpr + (x+1)/8) |= (0x80 >> ((x+1)%8));							
							*(new_bits + (y-1)*new_bpr + x/8) |= (0x80 >> (x%8));							
						}
						if (y < height) {
							*(new_bits + (y+1)*new_bpr + (x+1)/8) |= (0x80 >> ((x+1)%8));							
							*(new_bits + (y+1)*new_bpr + x/8) |= (0x80 >> (x%8));													
						}					
					}					
				}			
			}	
		}	

		memcpy(old_bits,new_bits,bits_length);
		delete new_map;
		for (int32 i=0;i<bits_length;i++) {
			*old_bits = ~(*old_bits);
			old_bits++;
		}
			
		SimplifySelection();
	}
	release_sem(selection_mutex);
}


void Selection::Draw()
{
	acquire_sem(selection_mutex);
	BRect in_rect;
	BRect out_rect;
	int32 ao = animation_offset;
	
	for (int32 i=0;i<selection_data->SelectionCount();i++) {
		HSPolygon *poly = selection_data->ReturnSelectionAt(i);
		BPolygon *p = poly->GetBPolygon();
		out_rect = in_rect = p->Frame();
		out_rect.left *= view_magnifying_scale;
		out_rect.top *= view_magnifying_scale;
		out_rect.right = (out_rect.right + 1)*view_magnifying_scale - 1;
		out_rect.bottom = (out_rect.bottom + 1)*view_magnifying_scale - 1;
		p->MapTo(in_rect,out_rect);
		if ((ao % 8) == 0)
			image_view->StrokePolygon(p,TRUE,HS_ANIMATED_STRIPES_1);
		else if ((ao % 8) == 1)
			image_view->StrokePolygon(p,TRUE,HS_ANIMATED_STRIPES_2);
		else if ((ao % 8) == 2)
			image_view->StrokePolygon(p,TRUE,HS_ANIMATED_STRIPES_3);
		else if ((ao % 8) == 3)
			image_view->StrokePolygon(p,TRUE,HS_ANIMATED_STRIPES_4);
		else if ((ao % 8) == 4)
			image_view->StrokePolygon(p,TRUE,HS_ANIMATED_STRIPES_5);
		else if ((ao % 8) == 5)
			image_view->StrokePolygon(p,TRUE,HS_ANIMATED_STRIPES_6);
		else if ((ao % 8) == 6)
			image_view->StrokePolygon(p,TRUE,HS_ANIMATED_STRIPES_7);
		else if ((ao % 8) == 7)
			image_view->StrokePolygon(p,TRUE,HS_ANIMATED_STRIPES_8);
		
		delete p;			
	}
	release_sem(selection_mutex);
}


void Selection::RotateTo(BPoint pivot, float angle)
{
	acquire_sem(selection_mutex);	// This might actually not be useful.
	if (original_selections == NULL) {
		original_selections = new HSPolygon*[selection_data->SelectionCount()];
		for (int32 i=0;i<selection_data->SelectionCount();i++) {
			original_selections[i] = new HSPolygon(selection_data->ReturnSelectionAt(i));		
		}
	}

	int32 item_count = selection_data->SelectionCount();
	selection_data->EmptySelectionData();

	for (int32 i=0;i<item_count;i++) {
		HSPolygon *p = new HSPolygon(original_selections[i]);
		p->Rotate(pivot,angle);
		selection_data->AddSelection(p);
	}

	needs_recalculating = TRUE;
	release_sem(selection_mutex);
}


void Selection::Translate(int32 dx, int32 dy)
{
	acquire_sem(selection_mutex);
	if (original_selections != NULL) {
		for (int32 i=0;i<selection_data->SelectionCount();i++) {
			delete original_selections[i];
		}
		delete[] original_selections;
		original_selections = NULL;
	}
	
	for (int32 i=0;i<selection_data->SelectionCount();i++) {
		HSPolygon *p = selection_data->ReturnSelectionAt(i);
		p->TranslateBy(dx,dy);
	}

	needs_recalculating = TRUE;
	release_sem(selection_mutex);
}

void Selection::Recalculate()
{
	if (needs_recalculating == TRUE) {
		if (original_selections != NULL) {
			for (int32 i=0;i<selection_data->SelectionCount();i++) {
				delete original_selections[i];
			}
			delete[] original_selections;
			original_selections = NULL;
		}
	
		if (selection_map != NULL) {
			// First clear the selection
			for (int32 i=0;i<selection_map->BitsLength();i++) {
				selection_bits[i] = 0x00;
			}

			selection_map->Lock();
			for (int32 i=0;i<selection_data->SelectionCount();i++) {
				HSPolygon *p = selection_data->ReturnSelectionAt(i);
				if (p->GetDirection() == HS_POLYGON_CLOCKWISE) {	// A selection
					selection_view->FillPolygon(p->GetBPolygon(),B_SOLID_HIGH);	
				}
				else if (p->GetDirection() == HS_POLYGON_COUNTERCLOCKWISE) {	// A de-selection
					selection_view->FillPolygon(p->GetBPolygon(),B_SOLID_LOW);				
				}
			}
			selection_view->Sync();
			selection_map->Unlock();
		}

//		SimplifySelection();	
		selection_bounds = BRect(0,0,-1,-1);
		needs_recalculating = FALSE;
	}
}



void Selection::ImageSizeChanged(BRect rect)
{
	acquire_sem(selection_mutex);
	if (rect != image_bounds) {
		image_bounds = rect;
		if (selection_map != NULL) {
			BBitmap *previous_map = selection_map;
			selection_map = new BBitmap(image_bounds,B_GRAY1,TRUE);
			selection_view = new BView(image_bounds,"selection view",B_FOLLOW_NONE,B_WILL_DRAW);
			selection_map->AddChild(selection_view);
			selection_bits = (uint8*)selection_map->Bits();
			selection_bpr = selection_map->BytesPerRow();

			int32 height = min_c(previous_map->Bounds().IntegerHeight(),selection_map->Bounds().IntegerHeight());
			int32 width = min_c(selection_bpr,previous_map->BytesPerRow());
			
			int32 previous_bpr = previous_map->BytesPerRow();
			uint8 *previous_bits = (uint8*)previous_map->Bits();
			
			for (int32 y=0;y<=height;y++) {
				for (int32 x=0;x<width;x++) {
					*(selection_bits + x + y*selection_bpr) = *(previous_bits + x + y*previous_bpr);
				}
			}
			
			delete previous_map;


			SimplifySelection();
		}
		else {
			selection_bounds = image_bounds;
		}
	}
	release_sem(selection_mutex);
}

BRect Selection::GetBoundingRect()
{
	if (selection_bounds.IsValid() != TRUE)
		calculateBoundingRect();
		
	return selection_bounds;
}


void Selection::Invert()
{
	acquire_sem(selection_mutex);
	if (original_selections != NULL) {
		for (int32 i=0;i<selection_data->SelectionCount();i++) {
			delete original_selections[i];
		}
		delete[] original_selections;
		original_selections = NULL;
	}

	if (!IsEmpty()) {
		uint8 *bits = (uint8*)selection_map->Bits();
		int32 bits_length = selection_map->BitsLength();
		for (int32 i=0;i<bits_length;i++) {
			*bits = ~(*bits);
			++bits;
		}
		selection_bounds = BRect(0,0,-1,-1);
		SimplifySelection();
	}
	release_sem(selection_mutex);
}


bool Selection::IsEmpty()
{
	return (selection_map == NULL);
}


void Selection::calculateBoundingRect()
{
	if (selection_map == NULL)
		selection_bounds = image_bounds;
		
	else {
		int32 bpr = selection_map->BytesPerRow();
		uint8 *bits = (uint8*)selection_map->Bits();
		int32 bytes = selection_map->BitsLength();

		selection_bounds = BRect(1000000,1000000,-1000000,-1000000);		

		int32 x_base;
		int32 y = 0;
		
		for (int32 i=0;i<bytes;i++) {
			x_base = (i % bpr)*8;
			y = i / bpr;
			if (*bits != 0x00) {
				selection_bounds.top = min_c(selection_bounds.top,y); 
				selection_bounds.bottom = max_c(selection_bounds.bottom,y); 
				for (int32 offset = 7;offset>=0;offset--) {
					if ( (*bits >> offset) & 0x01 ) {
						selection_bounds.left = min_c(selection_bounds.left,x_base+(7-offset));
						selection_bounds.right = max_c(selection_bounds.right,x_base+(7-offset));						
					}
				}

			}
			bits++;				
		}
		selection_bounds = selection_bounds & image_bounds;			
	}	
}


void Selection::deSelect()
{
	Clear();
}


int32 Selection::thread_entry_func(void *data)
{
	return ((Selection*)data)->thread_func();
}


int32 Selection::thread_func()
{
	BWindow *window = image_view->Window();
	while (!window && continue_drawing)
		window = image_view->Window();

	while ((continue_drawing == TRUE) && (IsEmpty() == FALSE)) {
		if (window->LockWithTimeout(0) == B_OK) {
			Draw();
			window->Unlock();
		}
		snooze(500*1000);
		animation_offset = animation_offset + 1;
	}
	
	if (IsEmpty() == TRUE) {
		continue_drawing = FALSE;
	}
	
	return B_NO_ERROR;
}

void Selection::ChangeMagnifyingScale(float mag_scale)
{
	view_magnifying_scale = mag_scale;
}






void Selection::SimplifySelection()
{
	// This function uses the selection_map to make
	// a new set of polygons that make up the selection.
	// The selection_map is searched for the edges of selections
	// and then those edges are traced to form the polygons.
	// This version makes the polygons by connecting centers
	// of edge pixels.
	BRect bounds = GetBoundingRect();
	int32 left = (int32)bounds.left;
	int32 right = (int32)bounds.right;
	int32 top = (int32)bounds.top;
	int32 bottom = (int32)bounds.bottom;
	
	enum {
		NONE,
		LEFT,
		UP,
		RIGHT,
		DOWN
	} direction,wall;

	PointContainer *included_points = new PointContainer();

	selection_data->EmptySelectionData();
				
	for (int32 y=top;y<=bottom;y++) {
		for (int32 x=left;x<=right;x++) {			
			// We start a new polygon if point at x,y is at the edge and 
			// has not been included in previous polygons.
			if (ContainsPoint(x,y) && (included_points->HasPoint(x,y) == false)) {
				// If this point is at the edge we start making a polygon.
				if (!ContainsPoint(BPoint(x-1,y))) {
					direction = UP;
					wall = LEFT;
				}
				else if (!ContainsPoint(BPoint(x,y-1))) {
					direction = RIGHT;
					wall = UP;
				}
				else if (!ContainsPoint(BPoint(x+1,y))){
					direction = DOWN;
					wall = RIGHT;
				}
				else if (!ContainsPoint(BPoint(x,y+1))) {
					direction = LEFT;
					wall = DOWN;
				}
				else {
					direction = NONE;
					wall = NONE;
				}
				if ((direction != NONE) && (wall != NONE)) {
					int32 max_point_count = 32;
					BPoint *point_list = new BPoint[max_point_count];
					int32 point_count = 0;
				
					BPoint starting_point(x,y);
					int32 turns = 0;
					included_points->InsertPoint(starting_point.x,starting_point.y);
					BPoint next_point;
					next_point = starting_point;
					point_list[point_count++] = starting_point;
										
					// This if structure decides what is the next point.
					int32 new_x = (int32)next_point.x;
					int32 new_y = (int32)next_point.y;
					bool contains_left = ContainsPoint(BPoint(new_x-1,new_y));
					bool contains_up = ContainsPoint(BPoint(new_x,new_y-1));
					bool contains_right = ContainsPoint(BPoint(new_x+1,new_y));
					bool contains_down = ContainsPoint(BPoint(new_x,new_y+1));
					if (direction == UP) {
						if (contains_left) {
							turns--;
							direction = LEFT;
						}
						else if (!contains_up) {
							if (contains_right) {
								turns++;
								direction = RIGHT;
							}
							else if (contains_down) {
								turns += 2;
								direction = DOWN;
							}
							else
								direction = NONE;
						}
					}
					else if (direction == RIGHT) {
						if (contains_up) {
							turns--;
							direction = UP;
						}
						else if (!contains_right) {
							if (contains_down) {
								turns++;
								direction = DOWN;
							}
							else if (contains_left) {
								turns += 2;
								direction = LEFT;
							}
							else
								direction = NONE;
						}
					}
					else if (direction == DOWN) {
						if (contains_right) {
							turns--;
							direction = RIGHT;
						}
						else if (!contains_down) {
							if (contains_left) {
								turns++;
								direction = LEFT;
							}
							else if (contains_up) {
								turns += 2;
								direction = UP;
							}
							else direction = NONE;
						}
					}	
					else if (direction == LEFT) {
						if (contains_down) {
							turns--;
							direction = DOWN;
						}
						else if (!contains_left) {
							if (contains_up) {
								turns++;
								direction = UP;
							}
							else if (contains_right) {
								turns += 2;
								direction = RIGHT;
							}
							else
								direction = NONE;
						}
					}
					
					if (direction == UP)
						new_y = new_y - 1;
					else if (direction == RIGHT)
						new_x = new_x + 1;
					else if (direction == DOWN)
						new_y = new_y + 1;
					else if (direction == LEFT)
						new_x = new_x - 1;
					
					
					next_point = BPoint(new_x,new_y);
					while ((next_point != starting_point) && (direction != NONE)) {
						included_points->InsertPoint(next_point.x,next_point.y);
						point_list[point_count++] = next_point;
						if (point_count == max_point_count) {
							max_point_count *= 2;
							BPoint *new_points = new BPoint[max_point_count];
							for (int32 i=0;i<point_count;i++)
								new_points[i] = point_list[i];
							
							delete[] point_list;
							point_list = new_points;
						}					

						// This if structure decides what is the next point.
						int32 new_x = (int32)next_point.x;
						int32 new_y = (int32)next_point.y;
						bool contains_left = ContainsPoint(BPoint(new_x-1,new_y));
						bool contains_up = ContainsPoint(BPoint(new_x,new_y-1));
						bool contains_right = ContainsPoint(BPoint(new_x+1,new_y));
						bool contains_down = ContainsPoint(BPoint(new_x,new_y+1));
						if (direction == UP) {
							if (contains_left) {
								turns--;
								direction = LEFT;
							}
							else if (!contains_up) {
								if (contains_right) {
									turns++;
									direction = RIGHT;
								}
								else if (contains_down) {
									turns += 2;
									direction = DOWN;
								}
								else
									direction = NONE;
							}
						}
						else if (direction == RIGHT) {
							if (contains_up) {
								turns--;
								direction = UP;
							}
							else if (!contains_right) {
								if (contains_down) {
									turns++;
									direction = DOWN;
								}
								else if (contains_left) {
									turns += 2;
									direction = LEFT;
								}
								else
									direction = NONE;
							}
						}
						else if (direction == DOWN) {
							if (contains_right) {
								turns--;
								direction = RIGHT;
							}
							else if (!contains_down) {
								if (contains_left) {
									turns++;
									direction = LEFT;
								}
								else if (contains_up) {
									turns += 2;
									direction = UP;
								}
								else direction = NONE;
							}
						}	
						else if (direction == LEFT) {
							if (contains_down) {
								turns--;
								direction = DOWN;
							}
							else if (!contains_left) {
								if (contains_up) {
									turns++;
									direction = UP;
								}
								else if (contains_right) {
									turns += 2;
									direction = RIGHT;
								}
								else
									direction = NONE;
							}
						}
						
						if (direction == UP)
							new_y = new_y - 1;
						else if (direction == RIGHT)
							new_x = new_x + 1;
						else if (direction == DOWN)
							new_y = new_y + 1;
						else if (direction == LEFT)
							new_x = new_x - 1;						
					
						next_point = BPoint(new_x,new_y);
					}					
									
					HSPolygon *new_polygon;
					if (turns > 0)
						new_polygon = new HSPolygon(point_list,point_count,HS_POLYGON_CLOCKWISE);
					else
						new_polygon = new HSPolygon(point_list,point_count,HS_POLYGON_COUNTERCLOCKWISE);
						
					delete[] point_list;
					selection_data->AddSelection(new_polygon);
				}							
			}			
		}
	}


	delete included_points;

	if (selection_data->SelectionCount() == 0) {
		Clear();
	}
	else if (selection_data->SelectionCount() == 1) {
		// Count the included and excluded points points
		int32 included=0;
		int32 excluded=0;
		int32 width = image_bounds.IntegerWidth();
		int32 height = image_bounds.IntegerHeight();
		for (int32 y=0;y<=height;y++) {
			for (int32 x=0;x<=width;x++) {
				if (ContainsPoint(x,y))
					included++;
				else
					excluded++;
			}
		}
		if ((included == 0) || (excluded == 0))
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
	for (int32 i=0;i<hash_table_size;i++) {
		hash_table[i] = NULL;
		list_length_table[i] = 0;
	}
}


PointContainer::~PointContainer()
{
	int32 slots = 0;
	int32 hits = 0;
	for (int32 i=0;i<hash_table_size;i++) {
		if (list_length_table[i] > 0) {
			slots++;
			hits += list_length_table[i];
		}	
	}

	for (int32 i=0;i<hash_table_size;i++) {
		delete[] hash_table[i];
		hash_table[i] = NULL;
	}
	delete[] hash_table;
	delete[] list_length_table;
}


void PointContainer::InsertPoint(int32 x, int32 y)
{
	int32 key = hash_value(x,y);

	if (list_length_table[key] == 0) {
		hash_table[key] = new BPoint[1];
		list_length_table[key] = 1;
		hash_table[key][0] = BPoint(x,y);	
	}
	else {
		BPoint *new_array = new BPoint[list_length_table[key] + 1];
		
		for (int32 i=0;i<list_length_table[key];i++) {
			new_array[i] = hash_table[key][i];
		}
		
		delete[] hash_table[key];
		hash_table[key] = new_array;	
		hash_table[key][list_length_table[key]] = BPoint(x,y);
		list_length_table[key] += 1;
	}
}



bool PointContainer::HasPoint(int32 x,int32 y)
{
	int32 key = hash_value(x,y);
	bool has = FALSE;
	BPoint point(x,y);
	for (int32 i=0;(i<list_length_table[key]) && (has == FALSE);i++) {
		if (hash_table[key][i] == point)
			has = TRUE;
	}

	return has;
}



int32 PointContainer::hash_value(int32 x, int32 y)
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
	for (int32 i=0;i<selections_array_size;i++)
		selections[i] = NULL;
}


SelectionData::SelectionData(const SelectionData *original)
{
	selections_array_size = original->selections_array_size;
	number_of_selections = original->number_of_selections;
	selections = new HSPolygon*[selections_array_size];

	for (int32 i=0;i<selections_array_size;i++)
		selections[i] = NULL;

	for (int32 i=0;i<number_of_selections;i++) {
		selections[i] = new HSPolygon(original->selections[i]);
	}
}


SelectionData::~SelectionData()
{
	EmptySelectionData();
	delete[] selections;
}


void SelectionData::AddSelection(HSPolygon *poly)
{
	if (number_of_selections == selections_array_size) {
		selections_array_size *= 2;
		HSPolygon **new_array = new HSPolygon*[selections_array_size];
		for (int32 i=0;i<selections_array_size;i++) {
			if (i<number_of_selections) {
				new_array[i] = selections[i];
				selections[i] = NULL;
			}
			else {
				new_array[i] = NULL;
			}
		}
		delete[] selections;
		selections = new_array;
	}
	
	selections[number_of_selections] = poly;
	number_of_selections++;
}


void SelectionData::EmptySelectionData()
{
	for (int32 i=0;i<number_of_selections;i++) {
		delete selections[i];
		selections[i] = NULL;
	}

	number_of_selections = 0;
}


bool SelectionData::operator==(const SelectionData &data)
{
	bool similar = TRUE;
	similar = (number_of_selections == data.number_of_selections);
	
	for (int32 i=0;(i<number_of_selections) && similar;i++) {
		similar = (*selections[i] == *(data.selections[i]));
	}
	
	return similar;
}