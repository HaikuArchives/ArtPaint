/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "SelectorTool.h"

#include "BitmapDrawer.h"
#include "BitmapUtilities.h"
#include "Cursors.h"
#include "HSPolygon.h"
#include "Image.h"
#include "IntelligentPathFinder.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "Patterns.h"
#include "PixelOperations.h"
#include "Selection.h"
#include "ToolScript.h"
#include "UtilityClasses.h"


#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>
#include <SeparatorView.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


using ArtPaint::Interface::NumberSliderControl;


SelectorTool::SelectorTool()
	: DrawingTool(B_TRANSLATE("Selection tool"),
		SELECTOR_TOOL)
	, ToolEventAdapter()
{
	// the value for mode will be either B_OP_ADD or B_OP_SUBTRACT
	fOptions = MODE_OPTION | SHAPE_OPTION | TOLERANCE_OPTION;

	SetOption(MODE_OPTION, B_OP_ADD);
	SetOption(SHAPE_OPTION, HS_RECTANGLE);
	SetOption(TOLERANCE_OPTION, 10);
}


SelectorTool::~SelectorTool()
{
}


ToolScript*
SelectorTool::UseTool(ImageView *view, uint32 buttons, BPoint point,
	BPoint view_point)
{
	// these are used when drawing the preview to the view
	BPoint original_point = point;
	BPoint view_original_point = view_point;

	Selection *selection = view->GetSelection();

	BWindow *window = view->Window();
	if (window == NULL)
		return NULL;

	ToolScript* the_script = new ToolScript(Type(), fToolSettings,
		((PaintApplication*)be_app)->Color(true));

	window->Lock();
	view->SetHighColor(255,255,255,255);
	view->SetLowColor(0,0,0,255);
	view->SetPenSize(1);
	view->SetDrawingMode(B_OP_COPY);
	view->MovePenTo(view_point);
	BBitmap* buffer = view->ReturnImage()->ReturnActiveBitmap();
	window->Unlock();

	if (fToolSettings.shape == HS_INTELLIGENT_SCISSORS) {
		if (buffer->Bounds().Contains(point) == FALSE) {
			return NULL;
		}

		IntelligentPathFinder *path_finder =
			new IntelligentPathFinder(view->ReturnImage()->ReturnActiveBitmap());
		HSPolygon *the_polygon = new HSPolygon(&point,1);
		BPolygon *active_view_polygon = NULL;

		path_finder->SetSeedPoint(int32(point.x), int32(point.y));
		bool finished = false;
		BPoint prev_point = point;
		BPoint seed_point = point;
		if (view->LockLooper()) {
			view->SetDrawingMode(B_OP_INVERT);
			view->SetPenSize(2.0);
			view->UnlockLooper();
		}
		BRect old_rect;
		BRect old_bitmap_rect;

		while (!finished) {
			// Add support for different zoom-levels. Currently it does not support
			// changing the zoom-level while the tool is working.
			BRect view_rect;
			BRect bitmap_rect = view->ReturnImage()->ReturnActiveBitmap()->Bounds();
			if (is_clicks_data_valid) {
				is_clicks_data_valid = false;
				if (last_click_clicks >= 2) {
					finished = true;
					delete active_view_polygon;
					active_view_polygon = NULL;
				}
				else if ((point != seed_point) && bitmap_rect.Contains(point)) {
					// Here we should add a section from seed_point to point
					// (or last_click_bitmap_location) to the_polygon, and
					// respective changes to view_polygon too. If the route
					// between these two points is not yet calculated we should
					// wait until it becomes calculated by the dynamic
					// programming. Then the distance-map should be cleared
					// and new round of dynamic programming should be started
					// with last_click_bitmap_location as a seed_point.
					int32 num_points;
					BPoint *point_list = path_finder->ReturnPath(int32(point.x),
						int32(point.y), &num_points);
					while (point_list == NULL) {
						point_list = path_finder->ReturnPath(int32(point.x),
							int32(point.y), &num_points);
						snooze(50 * 1000);
					}
					the_polygon->AddPoints(point_list,num_points,true);
					if (view->LockLooper()) {
						view->SetDrawingMode(B_OP_COPY);
						BPolygon *bpoly = the_polygon->GetBPolygon();
						bitmap_rect = bpoly->Frame();
						view_rect = view->convertBitmapRectToView(bitmap_rect);
						bpoly->MapTo(bitmap_rect,view_rect);
						view->SetHighColor(255,255,0,255);
						view->SetLowColor(0,0,0,255);
						view->StrokePolygon(bpoly,false,HS_ANIMATED_STRIPES_1);
						view->SetDrawingMode(B_OP_INVERT);
						view->UnlockLooper();
						delete bpoly;
					}
					delete[] point_list;

					path_finder->SetSeedPoint(int32(point.x), int32(point.y));
					seed_point = point;
					delete active_view_polygon;
					active_view_polygon = NULL;
				}
			}
			else {
				// We should draw the active segment to the view if necessary
				// (i.e. point has changed).
				if (point != prev_point) {
					if (view->LockLooper()) {
						if (active_view_polygon != NULL) {
							bitmap_rect = active_view_polygon->Frame();
							view_rect = view->convertBitmapRectToView(bitmap_rect);
							view->Draw(view_rect);
							delete active_view_polygon;
						}
						int32 num_points;
						BPoint *point_list = path_finder->ReturnPath(int32(point.x),
							int32(point.y), &num_points);
						if (point_list != NULL) {
							active_view_polygon = new BPolygon(point_list,num_points);
							bitmap_rect = active_view_polygon->Frame();
							view_rect = view->convertBitmapRectToView(bitmap_rect);
							active_view_polygon->MapTo(bitmap_rect,view_rect);
						}
						else {
							point_list = new BPoint[3];
							point_list[0] = seed_point;
							point_list[1] = point;
							point_list[2] = point;
							active_view_polygon = new BPolygon(point_list,3);
							bitmap_rect = active_view_polygon->Frame();
							view_rect = view->convertBitmapRectToView(bitmap_rect);
							active_view_polygon->MapTo(bitmap_rect,view_rect);
						}
						delete[] point_list;
						view->SetDrawingMode(B_OP_OVER);
						view->DrawBitmap(buffer, old_bitmap_rect, old_rect);
						view->Draw(old_rect);
						view->SetDrawingMode(B_OP_INVERT);
						view->StrokePolygon(active_view_polygon,false); //, B_SOLID_LOW);
						old_bitmap_rect = bitmap_rect.InsetByCopy(-1, -1);
						old_rect = view->convertBitmapRectToView(old_bitmap_rect);
						view->SetDrawingMode(B_OP_COPY);
						BPolygon *bpoly = the_polygon->GetBPolygon();
						bitmap_rect = bpoly->Frame();
						view_rect = view->convertBitmapRectToView(bitmap_rect);
						bpoly->MapTo(bitmap_rect,view_rect);
						view->SetHighColor(255,255,0,255);
						view->SetLowColor(0,0,0,255);
						view->StrokePolygon(bpoly,false,HS_ANIMATED_STRIPES_1);
						view->UnlockLooper();
					}
					prev_point = point;
				}
			}
			if (view->LockLooper()) {
				view->getCoords(&point,&buttons,&view_point);
				view->UnlockLooper();
			}
			snooze(50 * 1000);
		}

		if (view->LockLooper()) {
			view->SetPenSize(1);
			view->SetDrawingMode(B_OP_COPY);
			view->UnlockLooper();
		}
		delete path_finder;
		selection->AddSelection(the_polygon,fToolSettings.mode == B_OP_ADD);
	}
	else if (fToolSettings.shape == HS_FREE_LINE) {
		int32 turn = 0;
		int32 size = 100;
		BPoint *point_list = new BPoint[size];
		int32 next_index = 0;

		while (buttons) {
			point_list[next_index++] = point;
			the_script->AddPoint(point);
			if (next_index == size) {
				BPoint *new_list = new BPoint[2*size];
				for (int32 i=0;i<next_index;i++) {
					new_list[i] = point_list[i];
				}
				delete[] point_list;
				point_list = new_list;
				size = 2*size;
			}

			view->Window()->Lock();
			if (turn == 0)
				view->StrokeLine(view_point,B_SOLID_HIGH);
			else
				view->StrokeLine(view_point,B_SOLID_LOW);
			view->getCoords(&point,&buttons,&view_point);
			view->Window()->Unlock();

			snooze(20 * 1000);
			turn = (turn + 1) % 2;
		}
		HSPolygon *poly = new HSPolygon(point_list,next_index,HS_POLYGON_CLOCKWISE);
		delete[] point_list;
		selection->AddSelection(poly,fToolSettings.mode == B_OP_ADD);
	}
	else if (fToolSettings.shape == HS_RECTANGLE) {
		BRect old_rect, new_rect;
		old_rect = new_rect = BRect(point,point);
		float left,top,right,bottom;
		view->Window()->Lock();
		drawing_mode old_mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_INVERT);
		view->StrokeRect(view->convertBitmapRectToView(new_rect), B_SOLID_LOW);
		view->Window()->Unlock();

		the_script->AddPoint(point);

		while (buttons) {
			if (old_rect != new_rect) {
				view->Window()->Lock();
				view->Draw(view->convertBitmapRectToView(old_rect));
				view->StrokeRect(view->convertBitmapRectToView(new_rect), B_SOLID_LOW );
				view->Window()->Unlock();

				old_rect = new_rect;
			}
			view->Window()->Lock();
			view->getCoords(&point,&buttons,&view_point);
			view->Window()->Unlock();

			left = min_c(original_point.x,point.x);
			top = min_c(original_point.y,point.y);
			right = max_c(original_point.x,point.x);
			bottom = max_c(original_point.y,point.y);
			new_rect = BRect(BPoint(left,top),BPoint(right,bottom));
			snooze(20 * 1000);
		}
		the_script->AddPoint(point);
		view->Window()->Lock();
		view->Draw(view->convertBitmapRectToView(old_rect));
		view->SetDrawingMode(old_mode);
		view->Window()->Unlock();

		BPoint point_list[4];
		point_list[0] = old_rect.LeftTop();
		point_list[1] = old_rect.RightTop();
		point_list[2] = old_rect.RightBottom();
		point_list[3] = old_rect.LeftBottom();
		HSPolygon *poly = new HSPolygon(point_list,4,HS_POLYGON_CLOCKWISE);
		selection->AddSelection(poly,fToolSettings.mode == B_OP_ADD);
	}
	else if (fToolSettings.shape == HS_MAGIC_WAND) {
		BBitmap *original_bitmap = view->ReturnImage()->ReturnActiveBitmap();
		BBitmap *selection_map;
		// We use a fill-tool to select the area:
		BitmapDrawer *drawer = new BitmapDrawer(original_bitmap);
		selection_map = MakeFloodBinaryMap(drawer, 0,
			int32(original_bitmap->Bounds().right), 0,
			int32(original_bitmap->Bounds().bottom),
			drawer->GetPixel(original_point), original_point);
		selection->AddSelection(selection_map,fToolSettings.mode == B_OP_ADD);
		delete drawer;
		delete selection_map;
	} else if (fToolSettings.shape == HS_CIRCLE) {
		BRect old_rect, new_rect;
		old_rect = new_rect = BRect(point,point);
		float left,top,right,bottom;
		view->Window()->Lock();
		drawing_mode old_mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_INVERT);
		view->StrokeEllipse(view->convertBitmapRectToView(new_rect), B_SOLID_LOW);
		view->Window()->Unlock();

		the_script->AddPoint(point);

		while (buttons) {
			if (old_rect != new_rect) {
				view->Window()->Lock();
				view->Draw(view->convertBitmapRectToView(old_rect));
				view->StrokeEllipse(view->convertBitmapRectToView(new_rect), B_SOLID_LOW);
				view->Window()->Unlock();

				old_rect = new_rect;
			}
			view->Window()->Lock();
			view->getCoords(&point,&buttons,&view_point);
			view->Window()->Unlock();

			left = min_c(original_point.x,point.x);
			top = min_c(original_point.y,point.y);
			right = max_c(original_point.x,point.x);
			bottom = max_c(original_point.y,point.y);
			new_rect = BRect(BPoint(left,top),BPoint(right,bottom));
			snooze(20 * 1000);
		}
		the_script->AddPoint(point);
		view->Window()->Lock();
		view->Draw(view->convertBitmapRectToView(old_rect));
		view->SetDrawingMode(old_mode);
		view->Window()->Unlock();

		BBitmap* selection_map;
		BBitmap* draw_map = new BBitmap(view->ReturnImage()->ReturnActiveBitmap()->Bounds(), B_RGBA32);
		// We use a fill-tool to select the area:
		BitmapDrawer *drawer = new BitmapDrawer(draw_map);
		drawer->DrawEllipse(new_rect, 0xFFFFFFFF, TRUE, FALSE);
		selection_map = BitmapUtilities::ConvertColorSpace(draw_map, B_GRAY8);
		selection->AddSelection(selection_map, fToolSettings.mode == B_OP_ADD);
		delete drawer;
		delete selection_map;
	}

	view->Window()->Lock();
	view->Invalidate();
	view->Window()->Unlock();

	return the_script;
}


BView*
SelectorTool::ConfigView()
{
	return new SelectorToolConfigView(this);
}


const void*
SelectorTool::ToolCursor() const
{
	return HS_SELECTOR_CURSOR;
}


const char*
SelectorTool::HelpString(bool isInUse) const
{
	return (isInUse
		? B_TRANSLATE("Making a selection.")
		: B_TRANSLATE("Selection tool"));
}


void
SelectorTool::CheckUpperSpans(BPoint span_start, BitmapDrawer* drawer,
	PointStack &stack, int32 min_x, int32 max_x, uint32 old_color,
	int32 tolerance, BBitmap* binary_fill_map)
{
	// First get the vital data.
	int32 x,start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;
	bool inside_upper_span = false;

	uint32 binary_bpr = binary_fill_map->BytesPerRow();
	uchar *binary_bits = (uchar*)binary_fill_map->Bits();

	// Then go from start towards the left side of the bitmap.
	while ( (x >= min_x)
		&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance))
		&& ((*(binary_bits + y*binary_bpr + (x/*/8*/))&(0x01/* << (7-x%8)*/)) == 0x00) ) {
		*(binary_bits + y*binary_bpr + (x/*/8*/))/* |= (0x01 << (7 - x%8))*/ = 0xff;
		if ( (inside_upper_span == false)
			&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
			stack.Push(BPoint(x,y-1));
			inside_upper_span = true;
		}
		else if ( (inside_upper_span == true)
			&& (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1), old_color,tolerance)) ) {
			inside_upper_span = false;
		}
		x--;
	}

	// Then go from start_x+1 towards the right side of the bitmap.
	// We might already be inside a lower span
	inside_upper_span = (compare_2_pixels_with_variance(drawer->GetPixel(start_x,y-1),
		old_color, tolerance));
	x = start_x + 1;
	while ((x <= max_x)
		&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y), old_color,tolerance))
		&& ((*(binary_bits + y*binary_bpr + (x/*/8*/))&(0x01/* << (7-x%8)*/)) == 0x00)  ) {
		*(binary_bits + y*binary_bpr + (x/*/8*/))/* |= (0x01 << (7 - x%8))*/ = 0xff;
		if ( (inside_upper_span == false)
			&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
			stack.Push(BPoint(x,y-1));
			inside_upper_span = true;
		}
		else if ( (inside_upper_span == true)
			&& (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
			inside_upper_span = false;
		}
		x++;
	}
}


void
SelectorTool::CheckBothSpans(BPoint span_start, BitmapDrawer* drawer,
	PointStack &stack, int32 min_x, int32 max_x, uint32 old_color,
	int32 tolerance, BBitmap* binary_fill_map)
{
	// First get the vital data.
	int32 x,start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;
	bool inside_lower_span = false;
	bool inside_upper_span = false;
		// This is the case that takes the variance into account. We must use a
		// binary bitmap to see what parts have already been filled.
		uint32 binary_bpr = binary_fill_map->BytesPerRow();
		uchar *binary_bits = (uchar*)binary_fill_map->Bits();

	// Then go from start towards the left side of the bitmap.
	while ( (x >= min_x)
		&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y), old_color,tolerance))
		&& ((*(binary_bits + y*binary_bpr + (x/*/8*/))&(0x01/* << (7-x%8)*/)) == 0x00)  ) {
		*(binary_bits + y*binary_bpr + (x/*/8*/))/* |= (0x01 << (7 - x%8))*/ = 0xff;

		if ( (inside_lower_span == false)
			&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			stack.Push(BPoint(x,y+1));
			inside_lower_span = true;
		}
		else if ( (inside_lower_span == true)
			&& (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			inside_lower_span = false;
		}

		if ( (inside_upper_span == false)
			&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
			stack.Push(BPoint(x,y-1));
			inside_upper_span = true;
		}
		else if ( (inside_upper_span == true)
			&& (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1), old_color,tolerance)) ) {
			inside_upper_span = false;
		}

		x--;
	}

	// Then go from start_x+1 towards the right side of the bitmap.
	// We might already be inside a lower span
	inside_lower_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y+1),
		old_color,tolerance) );
	inside_upper_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y-1),
		old_color,tolerance) );
	x = start_x + 1;
	while ( (x <= max_x)
		&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance))
		&& ((*(binary_bits + y*binary_bpr + (x/*/8*/))&(0x01/* << (7-x%8)*/)) == 0x00) ) {
		*(binary_bits + y*binary_bpr + (x/*/8*/)) /*|= (0x01 << (7 - x%8))*/ = 0xff;

		if ( (inside_lower_span == false)
			&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			stack.Push(BPoint(x,y+1));
			inside_lower_span = true;
		}
		else if ( (inside_lower_span == true)
			&& (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			inside_lower_span = false;
		}

		if ( (inside_upper_span == false)
			&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1), old_color,tolerance)) ) {
			stack.Push(BPoint(x,y-1));
			inside_upper_span = true;
		}
		else if ( (inside_upper_span == true)
			&& (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
			inside_upper_span = false;
		}

		x++;
	}
}


void
SelectorTool::FillSpan(BPoint span_start, BitmapDrawer* drawer, int32 min_x,
	int32 max_x, uint32 old_color, int32 tolerance, BBitmap* binary_fill_map)
{
	// First get the vital data.
	int32 x,start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;

	// This is the case that takes the variance into account. We must use a
	// binary bitmap to see what parts have already been filled.
	uint32 binary_bpr = binary_fill_map->BytesPerRow();
	uchar *binary_bits = (uchar*)binary_fill_map->Bits();

	// Then go from start towards the left side of the bitmap.
	while ( (x >= min_x)
		&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y), old_color,tolerance)) ) {
		*(binary_bits + y*binary_bpr + (x/*/8*/)) = 0xff;
			/**(binary_bits + y*binary_bpr + (x/8)) | (0x01 << (7 - x%8));*/
		x--;
	}

	// Then go from start_x+1 towards the right side of the bitmap.
	x = start_x + 1;
	while ( (x <= max_x)
		&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance)) ) {
		*(binary_bits + y*binary_bpr + (x/*/8*/)) = 0xff;
			/**(binary_bits + y*binary_bpr + (x/8)) | (0x01 << (7 - x%8));*/
		x++;
	}
}


BBitmap*
SelectorTool::MakeFloodBinaryMap(BitmapDrawer* drawer, int32 min_x, int32 max_x,
	int32 min_y, int32 max_y, uint32 old_color, BPoint start)
{
	// This function makes a binary bitmap of the image. It contains 255 where
	// the flood fill should fill and zeroes elsewhere.
	BBitmap* map = new BBitmap(BRect(min_x, min_y, max_x, max_y), B_GRAY8);
	memset(map->Bits(), 0x00, map->BitsLength());

	// Here fill the area using drawer's SetPixel and GetPixel.
	// The algorithm uses 4-connected version of flood-fill.
	// The SetPixel and GetPixel functions are versions that
	// do not check bounds so we have to be careful not to exceed
	// bitmap's bounds.
	uint32 tolerance = (uint32)((float)fToolSettings.tolerance / 100.0 * 255);

	PointStack stack;
	stack.Push(start);

	while (!stack.IsEmpty()) {
		BPoint span_start = stack.Pop();
		if ( (span_start.y == min_y) && (min_y != max_y) ) {
			// Only check the spans below this line
			CheckLowerSpans(span_start,drawer, stack,min_x,max_x,old_color,tolerance,map);
		}
		else if ( (span_start.y == max_y) && (min_y != max_y) ) {
			// Only check the spans above this line.
			CheckUpperSpans(span_start,drawer, stack,min_x,max_x,old_color,tolerance,map);
		}
		else if (min_y != max_y) {
			// Check the spans above and below this line.
			CheckBothSpans(span_start,drawer, stack,min_x,max_x,old_color,tolerance,map);
		}
		else {
			// The image is only one pixel high. Check the only span.
			FillSpan(span_start, drawer, min_x, max_x, old_color, tolerance,map);
		}
	}

	// Remember to NULL the attribute binary_fill_map
	return map;
}


void
SelectorTool::CheckLowerSpans(BPoint span_start, BitmapDrawer* drawer,
	PointStack& stack, int32 min_x, int32 max_x, uint32 old_color,
	int32 tolerance, BBitmap* binary_fill_map)
{
	// First get the vital data.
	int32 x,start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;
	bool inside_lower_span = false;

	// This is the case that takes the variance into account. We must use a
	// binary bitmap to see what parts have already been filled.
	uint32 binary_bpr = binary_fill_map->BytesPerRow();
	uchar *binary_bits = (uchar*)binary_fill_map->Bits();
	// Then go from start towards the left side of the bitmap.
	while ( (x >= min_x)
		&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance))
		&& ((*(binary_bits + y*binary_bpr + (x/*/8*/))&(0x01/* << (7-x%8)*/)) == 0x00) ) {
		*(binary_bits + y*binary_bpr + (x/*/8*/))/* |= (0x01 << (7 - x%8))*/ = 0xff;
		if ( (inside_lower_span == false)
			&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			stack.Push(BPoint(x,y+1));
			inside_lower_span = true;
		}
		else if ( (inside_lower_span == true)
			&& (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			inside_lower_span = false;
		}
		x--;
	}

	// Then go from start_x+1 towards the right side of the bitmap.
	// We might already be inside a lower span
	inside_lower_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y+1),
		old_color,tolerance) );
	x = start_x + 1;
	while ( (x <= max_x)
		&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance))
		&& ((*(binary_bits + y*binary_bpr + (x/*/8*/))&(0x01/* << (7-x%8)*/)) == 0x00)  ) {
		*(binary_bits + y*binary_bpr + (x/*/8*/))/* |= (0x01 << (7 - x%8))*/ = 0xff;
		if ( (inside_lower_span == false)
			&& (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			stack.Push(BPoint(x,y+1));
			inside_lower_span = true;
		}
		else if ( (inside_lower_span == true)
			&& (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			inside_lower_span = false;
		}
		x++;
	}
}


// #pragma mark -- SelectorToolConfigView


SelectorToolConfigView::SelectorToolConfigView(DrawingTool* tool)
	:
	DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", MODE_OPTION);
		message->AddInt32("value", B_OP_ADD);
		fAddArea = new BRadioButton(B_TRANSLATE("Add area"),
			new BMessage(*message));

		message->ReplaceInt32("value", B_OP_SUBTRACT);
		fSubstractArea =
			new BRadioButton(B_TRANSLATE("Subtract area"),
				message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SHAPE_OPTION);
		message->AddInt32("value", HS_FREE_LINE);
		fFreeLine = new BRadioButton(B_TRANSLATE("Freehand"),
			new BMessage(*message));

		message->ReplaceInt32("value", HS_RECTANGLE);
		fRectangle =
			new BRadioButton(B_TRANSLATE("Rectangle"),
				new BMessage(*message));

		message->ReplaceInt32("value", HS_CIRCLE);
		fEllipse =
			new BRadioButton(B_TRANSLATE("Ellipse"),
				new BMessage(*message));

		message->ReplaceInt32("value", HS_INTELLIGENT_SCISSORS);
		fScissors =  new
			BRadioButton(B_TRANSLATE("Intelligent scissors"),
				new BMessage(*message));

		message->ReplaceInt32("value", HS_MAGIC_WAND);
		fMagicWand =
			new BRadioButton(B_TRANSLATE("Magic wand"),
				new BMessage(*message));

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", TOLERANCE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(TOLERANCE_OPTION));
		fTolerance =
			new NumberSliderControl(B_TRANSLATE("Tolerance:"),
				"10", message, 0, 100, false);

		BGridLayout* toleranceLayout = LayoutSliderGrid(fTolerance);

		layout->AddView(BGroupLayoutBuilder(B_VERTICAL, kWidgetSpacing)
			.Add(SeparatorView(B_TRANSLATE("Behavior")))
			.AddGroup(B_VERTICAL, kWidgetSpacing)
					.Add(fAddArea)
					.Add(fSubstractArea)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.AddStrut(kWidgetSpacing)
			.Add(SeparatorView(B_TRANSLATE("Mode")))
			.AddGroup(B_VERTICAL, kWidgetSpacing)
				.Add(fFreeLine)
				.Add(fRectangle)
				.Add(fEllipse)
				.Add(fScissors)
				.Add(fMagicWand)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.AddStrut(kWidgetSpacing)
			.Add(SeparatorView(B_TRANSLATE("Wand tolerance")))
			.Add(toleranceLayout)
			.TopView()
		);

		if (tool->GetCurrentValue(MODE_OPTION) == B_OP_ADD)
			fAddArea->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(MODE_OPTION) == B_OP_SUBTRACT)
			fSubstractArea->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(SHAPE_OPTION) == HS_FREE_LINE)
			fFreeLine->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(SHAPE_OPTION) == HS_RECTANGLE)
			fRectangle->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(SHAPE_OPTION) == HS_CIRCLE)
			fEllipse->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(SHAPE_OPTION) == HS_MAGIC_WAND)
			fMagicWand->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(SHAPE_OPTION) == HS_INTELLIGENT_SCISSORS)
			fScissors->SetValue(B_CONTROL_ON);
	}
}


void
SelectorToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fAddArea->SetTarget(this);
	fSubstractArea->SetTarget(this);
	fFreeLine->SetTarget(this);
	fRectangle->SetTarget(this);
	fEllipse->SetTarget(this);
	fMagicWand->SetTarget(this);
	fScissors->SetTarget(this);
	fTolerance->SetTarget(this);
}

