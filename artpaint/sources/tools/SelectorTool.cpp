/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Box.h>
#include <RadioButton.h>
#include <stdio.h>

#include "Cursors.h"
#include "SelectorTool.h"
#include "Selection.h"
#include "HSPolygon.h"
#include "PixelOperations.h"
#include "StringServer.h"
#include "IntelligentPathFinder.h"
#include "Patterns.h"

SelectorTool::SelectorTool()
		:	DrawingTool(StringServer::ReturnString(SELECTOR_TOOL_NAME_STRING),SELECTOR_TOOL),
			ToolEventAdapter()
{
	options = MODE_OPTION | SHAPE_OPTION | TOLERANCE_OPTION;	// the value for mode will be either B_OP_ADD or B_OP_SUBTRACT

	SetOption(MODE_OPTION,B_OP_ADD);
	SetOption(SHAPE_OPTION,HS_RECTANGLE);
	SetOption(TOLERANCE_OPTION,10);
}

SelectorTool::~SelectorTool()
{
}

ToolScript* SelectorTool::UseTool(ImageView *view,uint32 buttons,BPoint point,BPoint view_point)
{
	BPoint original_point;
	// these are used when drawing the preview to the view
	BPoint view_original_point;

	original_point = point;
	view_original_point = view_point;

	Selection *selection = view->GetSelection();

	BWindow *window = view->Window();
	if (window == NULL)
		return NULL;

	ToolScript *the_script = new ToolScript(type,settings,((PaintApplication*)be_app)->GetColor(TRUE));

	window->Lock();
	view->SetHighColor(255,255,255,255);
	view->SetLowColor(0,0,0,255);
	view->SetPenSize(1);
	view->SetDrawingMode(B_OP_COPY);
	view->MovePenTo(view_point);
	window->Unlock();

	if (settings.shape == HS_INTELLIGENT_SCISSORS) {
		IntelligentPathFinder *path_finder = new IntelligentPathFinder(view->ReturnImage()->ReturnActiveBitmap());
		HSPolygon *the_polygon = new HSPolygon(&point,1);
		BPolygon *active_view_polygon = NULL;

		path_finder->SetSeedPoint(int32(point.x), int32(point.y));
		bool finished = FALSE;
		BPoint prev_point = point;
		BPoint seed_point = point;
		if (view->LockLooper()) {
			view->SetDrawingMode(B_OP_INVERT);
			view->SetPenSize(2.0);
			view->UnlockLooper();
		}
		while (!finished) {
			// Add support for different zoom-levels. Currently it does not support
			// changing the zoom-level while the tool is working.
			BRect view_rect;
			BRect bitmap_rect = view->ReturnImage()->ReturnActiveBitmap()->Bounds();
			if (is_clicks_data_valid) {
				is_clicks_data_valid = FALSE;
				if (last_click_clicks >= 2) {
					finished = TRUE;
					delete active_view_polygon;
					active_view_polygon = NULL;
				}
				else if ((point != seed_point) && bitmap_rect.Contains(point)) {
					// Here we should add a section from seed_point to point (or last_click_bitmap_location)
					// to the_polygon, and respective changes to view_polygon too. If the route between
					// these two points is not yet calculated we should wait until it becomes
					// calculated by the dynamic programming. Then the distance-map should be cleared
					// and new round of dynamic programming should be started with last_click_bitmap_location
					// as a seed_point.
					int32 num_points;
					BPoint *point_list = path_finder->ReturnPath(int32(point.x),
						int32(point.y), &num_points);
					while (point_list == NULL) {
						point_list = path_finder->ReturnPath(int32(point.x),
							int32(point.y), &num_points);
						snooze(50 * 1000);
					}
					the_polygon->AddPoints(point_list,num_points,TRUE);
					if (view->LockLooper()) {
						view->SetDrawingMode(B_OP_COPY);
						BPolygon *bpoly = the_polygon->GetBPolygon();
						bitmap_rect = bpoly->Frame();
						view_rect = view->convertBitmapRectToView(bitmap_rect);
						bpoly->MapTo(bitmap_rect,view_rect);
						view->SetHighColor(255,255,0,255);
						view->SetLowColor(0,0,0,255);
						view->StrokePolygon(bpoly,FALSE,HS_ANIMATED_STRIPES_1);
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
				// We should draw the active segment to the view if necessary (i.e. point has changed).
				if (point != prev_point) {
					if (view->LockLooper()) {
						if (active_view_polygon != NULL) {
							view->StrokePolygon(active_view_polygon,FALSE);
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
						view->StrokePolygon(active_view_polygon,FALSE);
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
		selection->AddSelection(the_polygon,settings.mode == B_OP_ADD);
	}
	else if (settings.shape == HS_FREE_LINE) {
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
		selection->AddSelection(poly,settings.mode == B_OP_ADD);
	}
	else if (settings.shape == HS_RECTANGLE) {
		BRect old_rect, new_rect;
		old_rect = new_rect = BRect(point,point);
		float left,top,right,bottom;
		view->Window()->Lock();
		drawing_mode old_mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_INVERT);
		view->StrokeRect(view->convertBitmapRectToView(new_rect));
		view->Window()->Unlock();

		the_script->AddPoint(point);

		while (buttons) {
			if (old_rect != new_rect) {
				view->Window()->Lock();
				view->StrokeRect(view->convertBitmapRectToView(old_rect));
				view->StrokeRect(view->convertBitmapRectToView(new_rect));
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
		view->StrokeRect(view->convertBitmapRectToView(old_rect));
		view->SetDrawingMode(old_mode);
		view->Window()->Unlock();

		BPoint point_list[4];
		point_list[0] = old_rect.LeftTop();
		point_list[1] = old_rect.RightTop();
		point_list[2] = old_rect.RightBottom();
		point_list[3] = old_rect.LeftBottom();
		HSPolygon *poly = new HSPolygon(point_list,4,HS_POLYGON_CLOCKWISE);
		selection->AddSelection(poly,settings.mode == B_OP_ADD);
	}
	else if (settings.shape == HS_MAGIC_WAND) {
		BBitmap *original_bitmap = view->ReturnImage()->ReturnActiveBitmap();
		BBitmap *selection_map;
		// We use a fill-tool to select the area:
		BitmapDrawer *drawer = new BitmapDrawer(original_bitmap);
		selection_map = MakeFloodBinaryMap(drawer, 0,
			int32(original_bitmap->Bounds().right), 0,
			int32(original_bitmap->Bounds().bottom),
			drawer->GetPixel(original_point), original_point);
		selection->AddSelection(selection_map,settings.mode == B_OP_ADD);
		delete drawer;
		delete selection_map;
	}

	view->Window()->Lock();
	view->Invalidate();
	view->Window()->Unlock();

	return the_script;

}


BView* SelectorTool::makeConfigView()
{
	SelectorToolConfigView *target_view = new SelectorToolConfigView(BRect(0,0,150,0),this);
	return target_view;
}

const char* SelectorTool::ReturnHelpString(bool is_in_use)
{
	if (!is_in_use)
		return StringServer::ReturnString(SELECTOR_TOOL_READY_STRING);
	else
		return StringServer::ReturnString(SELECTOR_TOOL_IN_USE_STRING);
}


const void* SelectorTool::ReturnToolCursor()
{
	return HS_SELECTOR_CURSOR;
}


BBitmap* SelectorTool::MakeFloodBinaryMap(BitmapDrawer *drawer,int32 min_x,int32 max_x,int32 min_y,int32 max_y,uint32 old_color, BPoint start)
{
	// This function makes a binary bitmap of the image. It contains ones where the
	// flood fill should fill and zeroes elsewhere.
	BBitmap *binary_map;
	binary_map = new BBitmap(BRect(min_x,min_y,max_x,max_y),B_MONOCHROME_1_BIT);
	uchar *binary_bits = (uchar*)binary_map->Bits();
	int32 binary_bitslength = binary_map->BitsLength();

	// Clear the binary map.
	for (int32 i=0;i<binary_bitslength;i++)
		*binary_bits++ = 0x00;

	binary_bits = (uchar*)binary_map->Bits();

	PointStack *stack = new PointStack(10000);
	stack->Push(start);

	// Here fill the area using drawer's SetPixel and GetPixel.
	// The algorithm uses 4-connected version of flood-fill.
	// The SetPixel and GetPixel functions are versions that
	// do not check bounds so we have to be careful not to exceed
	// bitmap's bounds.
	uint32 tolerance = (uint32)((float)settings.tolerance/100.0 * 255);
	while (stack->IsEmpty() == FALSE) {
		BPoint span_start = stack->Pop();
		if ( (span_start.y == min_y) && (min_y != max_y) ) {
			// Only check the spans below this line
			CheckLowerSpans(span_start,drawer,*stack,min_x,max_x,old_color,tolerance,binary_map);
		}
		else if ( (span_start.y == max_y) && (min_y != max_y) ) {
			// Only check the spans above this line.
			CheckUpperSpans(span_start,drawer,*stack,min_x,max_x,old_color,tolerance,binary_map);
		}
		else if (min_y != max_y) {
			// Check the spans above and below this line.
			CheckBothSpans(span_start,drawer,*stack,min_x,max_x,old_color,tolerance,binary_map);
		}
		else {
			// The image is only one pixel high. Check the only span.
			FillSpan(span_start,drawer,min_x,max_x,old_color,tolerance,binary_map);
		}
	}
	delete stack;

	// Remember to NULL the attribute binary_fill_map
	return binary_map;
}



void SelectorTool::CheckLowerSpans(BPoint span_start,BitmapDrawer *drawer,PointStack &stack,int32 min_x,int32 max_x,uint32 old_color,int32 tolerance,BBitmap *binary_fill_map)
{
	// First get the vital data.
	int32 x,start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;
	bool inside_lower_span = FALSE;

	// This is the case that takes the variance into account. We must use a
	// binary bitmap to see what parts have already been filled.
	uint32 binary_bpr = binary_fill_map->BytesPerRow();
	uchar *binary_bits = (uchar*)binary_fill_map->Bits();
	// Then go from start towards the left side of the bitmap.
	while ( (x >= min_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance)) && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00) ) {
		*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
		if ( (inside_lower_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			stack.Push(BPoint(x,y+1));
			inside_lower_span = TRUE;
		}
		else if ( (inside_lower_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			inside_lower_span = FALSE;
		}
		x--;
	}

	// Then go from start_x+1 towards the right side of the bitmap.
	// We might already be inside a lower span
	inside_lower_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y+1),old_color,tolerance) );
	x = start_x + 1;
	while ( (x <= max_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance)) && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00)  ) {
		*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
		if ( (inside_lower_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			stack.Push(BPoint(x,y+1));
			inside_lower_span = TRUE;
		}
		else if ( (inside_lower_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			inside_lower_span = FALSE;
		}
		x++;
	}
}

void SelectorTool::CheckUpperSpans(BPoint span_start,BitmapDrawer *drawer,PointStack &stack,int32 min_x,int32 max_x,uint32 old_color,int32 tolerance,BBitmap *binary_fill_map)
{
	// First get the vital data.
	int32 x,start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;
	bool inside_upper_span = FALSE;

	uint32 binary_bpr = binary_fill_map->BytesPerRow();
	uchar *binary_bits = (uchar*)binary_fill_map->Bits();

	// Then go from start towards the left side of the bitmap.
	while ( (x >= min_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance))  && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00) ) {
		*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
		if ( (inside_upper_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
			stack.Push(BPoint(x,y-1));
			inside_upper_span = TRUE;
		}
		else if ( (inside_upper_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1), old_color,tolerance)) ) {
			inside_upper_span = FALSE;
		}
		x--;
	}

	// Then go from start_x+1 towards the right side of the bitmap.
	// We might already be inside a lower span
	inside_upper_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y-1) , old_color,tolerance) );
	x = start_x + 1;
	while ( (x <= max_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y), old_color,tolerance)) && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00)  ) {
		*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
		if ( (inside_upper_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
			stack.Push(BPoint(x,y-1));
			inside_upper_span = TRUE;
		}
		else if ( (inside_upper_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
			inside_upper_span = FALSE;
		}
		x++;
	}
}

void SelectorTool::CheckBothSpans(BPoint span_start,BitmapDrawer *drawer,PointStack &stack,int32 min_x,int32 max_x,uint32 old_color,int32 tolerance,BBitmap *binary_fill_map)
{
	// First get the vital data.
	int32 x,start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;
	bool inside_lower_span = FALSE;
	bool inside_upper_span = FALSE;
		// This is the case that takes the variance into account. We must use a
		// binary bitmap to see what parts have already been filled.
		uint32 binary_bpr = binary_fill_map->BytesPerRow();
		uchar *binary_bits = (uchar*)binary_fill_map->Bits();

	// Then go from start towards the left side of the bitmap.
	while ( (x >= min_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y), old_color,tolerance)) && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00)  ) {
		*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));

		if ( (inside_lower_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			stack.Push(BPoint(x,y+1));
			inside_lower_span = TRUE;
		}
		else if ( (inside_lower_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			inside_lower_span = FALSE;
		}

		if ( (inside_upper_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
			stack.Push(BPoint(x,y-1));
			inside_upper_span = TRUE;
		}
		else if ( (inside_upper_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1), old_color,tolerance)) ) {
			inside_upper_span = FALSE;
		}

		x--;
	}

	// Then go from start_x+1 towards the right side of the bitmap.
	// We might already be inside a lower span
	inside_lower_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y+1),old_color,tolerance) );
	inside_upper_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y-1), old_color,tolerance) );
	x = start_x + 1;
	while ( (x <= max_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance))  && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00) ) {
		*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));

		if ( (inside_lower_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			stack.Push(BPoint(x,y+1));
			inside_lower_span = TRUE;
		}
		else if ( (inside_lower_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
			inside_lower_span = FALSE;
		}

		if ( (inside_upper_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1), old_color,tolerance)) ) {
			stack.Push(BPoint(x,y-1));
			inside_upper_span = TRUE;
		}
		else if ( (inside_upper_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
			inside_upper_span = FALSE;
		}

		x++;
	}
}

void SelectorTool::FillSpan(BPoint span_start,BitmapDrawer *drawer,int32 min_x, int32 max_x, uint32 old_color,int32 tolerance,BBitmap *binary_fill_map)
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
	while ( (x >= min_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y), old_color,tolerance)) ) {
		*(binary_bits + y*binary_bpr + (x/8)) = *(binary_bits + y*binary_bpr + (x/8)) | (0x01 << (7 - x%8));
		x--;
	}

	// Then go from start_x+1 towards the right side of the bitmap.
	x = start_x + 1;
	while ( (x <= max_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance)) ) {
		*(binary_bits + y*binary_bpr + (x/8)) = *(binary_bits + y*binary_bpr + (x/8)) | (0x01 << (7 - x%8));
		x++;
	}
}


SelectorToolConfigView::SelectorToolConfigView(BRect rect,DrawingTool *t)
	: DrawingToolConfigView(rect,t)
{
	BMessage *message;

	BBox *container;

	// This rect will be used as a frame-rect when creating controllers.
	// Each controller will automatically resize to proper vertical size.
	// When adding a new controller we must move this rect's offset by
	// previous controller's height + EXTRA_EDGE
	BRect controller_frame = BRect(EXTRA_EDGE,EXTRA_EDGE,150+EXTRA_EDGE,EXTRA_EDGE);

	// Add two radio-buttons for defining the mode of selector.
	// Add a menu for defining the shape of the selector.

	// Create a BBox for the radio-buttons.
	container = new BBox(controller_frame,"selector_type");
	container->SetLabel(StringServer::ReturnString(MODE_STRING));
	AddChild(container);

	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",MODE_OPTION);
	message->AddInt32("value",B_OP_ADD);

	// Create the first radio-button.
	font_height fHeight;
	container->GetFontHeight(&fHeight);
	mode_button_1 = new BRadioButton(BRect(EXTRA_EDGE,EXTRA_EDGE+fHeight.descent+fHeight.ascent/2,EXTRA_EDGE,EXTRA_EDGE+fHeight.descent+fHeight.ascent/2),"a radio button",StringServer::ReturnString(ADD_AREA_STRING),new BMessage(*message));
	mode_button_1->ResizeToPreferred();
	container->AddChild(mode_button_1);
	if (tool->GetCurrentValue(MODE_OPTION) == B_OP_ADD)
		mode_button_1->SetValue(B_CONTROL_ON);

	// Create the second radio-button.
	message->ReplaceInt32("value",B_OP_SUBTRACT);
	mode_button_2 = new BRadioButton(BRect(EXTRA_EDGE,mode_button_1->Frame().bottom,EXTRA_EDGE,mode_button_1->Frame().bottom)," a radio button",StringServer::ReturnString(SUBTRACT_AREA_STRING),message);
	mode_button_2->ResizeToPreferred();
	container->AddChild(mode_button_2);
	if (tool->GetCurrentValue(MODE_OPTION) == B_OP_SUBTRACT)
		mode_button_2->SetValue(B_CONTROL_ON);

	container->ResizeBy(0,mode_button_2->Frame().bottom+EXTRA_EDGE);
	controller_frame.OffsetBy(0,container->Frame().Height() + EXTRA_EDGE);


	// Create a box for the shape buttons.
	container = new BBox(controller_frame,StringServer::ReturnString(SHAPE_STRING));
	container->SetLabel(StringServer::ReturnString(SHAPE_STRING));
	AddChild(container);

	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",SHAPE_OPTION);
	message->AddInt32("value",HS_FREE_LINE);
	shape_button_1 = new BRadioButton(BRect(EXTRA_EDGE,EXTRA_EDGE+fHeight.descent+fHeight.ascent/2,EXTRA_EDGE,EXTRA_EDGE+fHeight.descent+fHeight.ascent/2),"a radio button",StringServer::ReturnString(FREE_LINE_STRING),new BMessage(*message));
	shape_button_1->ResizeToPreferred();
	container->AddChild(shape_button_1);
	if (tool->GetCurrentValue(SHAPE_OPTION) == HS_FREE_LINE)
		shape_button_1->SetValue(B_CONTROL_ON);

	message->ReplaceInt32("value",HS_RECTANGLE);
	shape_button_2 = new BRadioButton(BRect(EXTRA_EDGE,shape_button_1->Frame().bottom,EXTRA_EDGE,shape_button_1->Frame().bottom),"a radio button",StringServer::ReturnString(RECTANGLE_STRING),new BMessage(*message));
	shape_button_2->ResizeToPreferred();
	container->AddChild(shape_button_2);
	if (tool->GetCurrentValue(SHAPE_OPTION) == HS_RECTANGLE)
		shape_button_2->SetValue(B_CONTROL_ON);


	message->ReplaceInt32("value",HS_MAGIC_WAND);
	shape_button_3 = new BRadioButton(BRect(EXTRA_EDGE,shape_button_2->Frame().bottom,EXTRA_EDGE,shape_button_2->Frame().bottom),"a radio button",StringServer::ReturnString(MAGIC_WAND_STRING),new BMessage(*message));
	shape_button_3->ResizeToPreferred();
	container->AddChild(shape_button_3);
	if (tool->GetCurrentValue(SHAPE_OPTION) == HS_MAGIC_WAND)
		shape_button_3->SetValue(B_CONTROL_ON);

	message->ReplaceInt32("value",HS_INTELLIGENT_SCISSORS);
	shape_button_4 = new BRadioButton(BRect(EXTRA_EDGE,shape_button_3->Frame().bottom,EXTRA_EDGE,shape_button_3->Frame().bottom),"a radio button",StringServer::ReturnString(INTELLIGENT_SCISSORS_STRING),new BMessage(*message));
	shape_button_4->ResizeToPreferred();
	container->AddChild(shape_button_4);
	if (tool->GetCurrentValue(SHAPE_OPTION) == HS_INTELLIGENT_SCISSORS)
		shape_button_4->SetValue(B_CONTROL_ON);


	container->ResizeBy(0,shape_button_4->Frame().bottom+EXTRA_EDGE);


	controller_frame.OffsetBy(0,container->Frame().Height() + EXTRA_EDGE);


	// Create a box for the tolerance control.
	container = new BBox(controller_frame,"Wand Tolerance");
	container->SetLabel(StringServer::ReturnString(WAND_TOLERANCE_STRING));
	AddChild(container);

	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",TOLERANCE_OPTION);
	message->AddInt32("value",tool->GetCurrentValue(TOLERANCE_OPTION));
	tolerance_slider = new ControlSliderBox(BRect(EXTRA_EDGE,EXTRA_EDGE+fHeight.descent+fHeight.ascent/2,controller_frame.Width()-EXTRA_EDGE,EXTRA_EDGE+fHeight.descent+fHeight.ascent/2),"tolerance_slider",StringServer::ReturnString(TOLERANCE_STRING),"10",message,0,100);
	container->AddChild(tolerance_slider);
	container->ResizeBy(0,tolerance_slider->Frame().bottom+EXTRA_EDGE);

	ResizeTo(container->Frame().right+EXTRA_EDGE,container->Frame().bottom + EXTRA_EDGE);
}


void SelectorToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	mode_button_1->SetTarget(BMessenger(this));
	mode_button_2->SetTarget(BMessenger(this));
	shape_button_1->SetTarget(BMessenger(this));
	shape_button_2->SetTarget(BMessenger(this));
	shape_button_3->SetTarget(BMessenger(this));
	shape_button_4->SetTarget(BMessenger(this));
	tolerance_slider->SetTarget(new BMessenger(this));
}
