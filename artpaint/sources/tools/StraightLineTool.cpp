/*

	Filename:	StraightLineTool.cpp
	Contents:	StraigLineTool-class definitions
	Author:		Heikki Suhonen

*/


#include <CheckBox.h>
#include <math.h>
#include <stdio.h>

#include "StraightLineTool.h"
#include "HSPolygon.h"
#include "Selection.h"
#include "StringServer.h"
#include "Cursors.h"

#define PI M_PI


StraightLineTool::StraightLineTool()
	: DrawingTool(StringServer::ReturnString(STRAIGHT_LINE_TOOL_NAME_STRING),STRAIGHT_LINE_TOOL)
{
	options = SIZE_OPTION | ANTI_ALIASING_LEVEL_OPTION | MODE_OPTION;
	number_of_options = 3;

	SetOption(SIZE_OPTION,1);
	SetOption(ANTI_ALIASING_LEVEL_OPTION,B_CONTROL_OFF);
	SetOption(MODE_OPTION,B_CONTROL_ON);
}


StraightLineTool::~StraightLineTool()
{

}


ToolScript* StraightLineTool::UseTool(ImageView *view,uint32 buttons,BPoint point,BPoint view_point)
{
	// In this function we calculate the line as a polygon.
	// We make the polygon by first making a horizontal rectangle
	// of appropriate size and the rotating around the starting-point
	// by proper angle.


	// Wait for the last_updated_region to become empty
	while (last_updated_rect.IsValid() == TRUE)
		snooze(50 * 1000);

	BWindow *window = view->Window();
	drawing_mode old_mode;
	BBitmap *bitmap = view->ReturnImage()->ReturnActiveBitmap();
//	BView *bitmap_view = view->getBufferView();

	Selection *selection = view->GetSelection();

	if (window != NULL) {
		ToolScript *the_script = new ToolScript(type,settings,((PaintApplication*)be_app)->GetColor(TRUE));

		BitmapDrawer *drawer = new BitmapDrawer(bitmap);

		BPoint original_point,original_view_point,prev_view_point;
		BRect bitmap_rect,old_rect,new_rect;
		HSPolygon *view_polygon = NULL;
		BPoint point_list[4];

		window->Lock();
		old_mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_INVERT);
		window->Unlock();
		original_point = point;
		rgb_color c = ((PaintApplication*)be_app)->GetColor(TRUE);

		prev_view_point = original_view_point = view_point;
		bitmap_rect = BRect(point.x,point.y-floor(((float)GetCurrentValue(SIZE_OPTION)-1.0)/2.0),point.x,point.y+ceil(((float)GetCurrentValue(SIZE_OPTION)-1.0)/2.0));
		old_rect = new_rect = view->convertBitmapRectToView(bitmap_rect);
		point_list[0] = new_rect.LeftTop();
		point_list[1] = new_rect.RightTop();
		point_list[2] = new_rect.RightBottom();
		point_list[3] = new_rect.LeftBottom();

		window->Lock();
		if ((GetCurrentValue(SIZE_OPTION) > 2) && (settings.mode == B_CONTROL_OFF)) {
			view_polygon = new HSPolygon(point_list,4);
			BPolygon *bpoly = view_polygon->GetBPolygon();
			view->StrokePolygon(bpoly);
			delete bpoly;
		}
		else
			view->StrokeLine(original_view_point,view_point);
		window->Unlock();

		float angle = 0;
		while (buttons) {
			window->Lock();
			view->getCoords(&point,&buttons,&view_point);
			if (modifiers() & B_LEFT_CONTROL_KEY) {
				// Make the new point be so that the angle is a multiple of 45°.
				float x_diff,y_diff;
				x_diff = fabs(original_point.x-point.x);
				y_diff = fabs(original_point.y-point.y);

				if (x_diff < y_diff) {
					if (x_diff < y_diff/2)
						x_diff = 0;
					else
						x_diff = y_diff;
				}
				else {
					if (y_diff < x_diff/2)
						y_diff = 0;
					else
						y_diff = x_diff;
				}

				float signed_x_diff = (point.x-original_point.x);
				float signed_y_diff = (point.y-original_point.y);

				if (signed_x_diff != 0)
					point.x = original_point.x + x_diff * signed_x_diff/fabs(signed_x_diff);

				if (signed_y_diff != 0)
					point.y = original_point.y + y_diff * signed_y_diff/fabs(signed_y_diff);



				x_diff = fabs(original_view_point.x-view_point.x);
				y_diff = fabs(original_view_point.y-view_point.y);

				if (x_diff < y_diff) {
					if (x_diff < y_diff/2)
						x_diff = 0;
					else
						x_diff = y_diff;
				}
				else {
					if (y_diff < x_diff/2)
						y_diff = 0;
					else
						y_diff = x_diff;
				}


				signed_x_diff = (view_point.x-original_view_point.x);
				signed_y_diff = (view_point.y-original_view_point.y);
				if (signed_x_diff != 0)
					view_point.x = original_view_point.x + x_diff * signed_x_diff/fabs(signed_x_diff);

				if (signed_y_diff != 0)
					view_point.y = original_view_point.y + y_diff * signed_y_diff/fabs(signed_y_diff);

			}

			bitmap_rect = BRect(original_point.x,original_point.y-floor(((float)GetCurrentValue(SIZE_OPTION)-1.0)/2.0),original_point.x+sqrt(pow(original_point.x-point.x,2)+pow(original_point.y-point.y,2)),original_point.y+ceil(((float)GetCurrentValue(SIZE_OPTION)-1.0)/2.0));
			new_rect = view->convertBitmapRectToView(bitmap_rect);
			if (old_rect != new_rect) {
				if ((GetCurrentValue(SIZE_OPTION) > 2) && (settings.mode == B_CONTROL_OFF)) {
					BPolygon *bpoly = view_polygon->GetBPolygon();
					view->StrokePolygon(bpoly);
					delete view_polygon;
					delete bpoly;
					point_list[0] = new_rect.LeftTop();
					point_list[1] = new_rect.RightTop();
					point_list[2] = new_rect.RightBottom();
					point_list[3] = new_rect.LeftBottom();
					view_polygon = new HSPolygon(point_list,4);
					angle = atan2((view_point.y-original_view_point.y),(view_point.x-original_view_point.x))*180/PI;
					view_polygon->Rotate(original_view_point,angle);
					bpoly = view_polygon->GetBPolygon();
					view->StrokePolygon(bpoly);
					delete bpoly;
				}
				else {
					view->StrokeLine(original_view_point,prev_view_point);
					view->StrokeLine(original_view_point,view_point);
					angle = atan2((view_point.y-original_view_point.y),(view_point.x-original_view_point.x))*180/PI;
					prev_view_point = view_point;
				}
				old_rect = new_rect;
			}
			window->Unlock();
			snooze(20 * 1000);
		}
		int32 size = GetCurrentValue(SIZE_OPTION);
		bool draw_line = true;
		new_rect = old_rect;
		if (settings.mode == B_CONTROL_ON) { // Adjust the width of the line.
			bool continue_adjusting_width = true;
			BPoint p1 = original_point;
			BPoint width_point;
			view_polygon = new HSPolygon(NULL,0);
			BRect orig_rect = bitmap_rect;
			orig_rect.bottom = orig_rect.top = original_point.y;

			size = 0;

			while (continue_adjusting_width) {
				if (is_clicks_data_valid) {
					continue_adjusting_width = false;
					is_clicks_data_valid = false;
				}
				else if (is_keys_data_valid) {
					if (last_key_event_bytes[0] == B_ESCAPE) {
						continue_adjusting_width = false;
						draw_line = false;
					}
					is_keys_data_valid = false;
				}
				else {
					if (view->LockLooper()) {
						new_rect = orig_rect;
						new_rect.bottom += size/2;
						new_rect.top -= size/2;
						new_rect = view->convertBitmapRectToView(new_rect);
						if (new_rect != old_rect) {
							if (size > 0) {
								BPolygon *bpoly = view_polygon->GetBPolygon();
								view->StrokePolygon(bpoly);
								delete view_polygon;
								delete bpoly;
								point_list[0] = new_rect.LeftTop();
								point_list[1] = new_rect.RightTop();
								point_list[2] = new_rect.RightBottom();
								point_list[3] = new_rect.LeftBottom();
								view_polygon = new HSPolygon(point_list,4);
								view_polygon->Rotate(original_view_point,angle);
								bpoly = view_polygon->GetBPolygon();
								view->StrokePolygon(bpoly);
								delete bpoly;
							}
							old_rect = new_rect;
						}
						view->getCoords(&width_point,&buttons);
						view->UnlockLooper();
						width_point = width_point-p1;
						BPoint spare = width_point;
						width_point.x = cos(-angle/180*PI)*spare.x - sin(-angle/180*PI)*spare.y;
						width_point.y = sin(-angle/180*PI)*spare.x + cos(-angle/180*PI)*spare.y;
						size = (int32)(2*fabs(width_point.y));
					}
					snooze(20 * 1000);
				}
			}
		}
		delete view_polygon;

		bool anti_alias;
		if (GetCurrentValue(ANTI_ALIASING_LEVEL_OPTION) == B_CONTROL_OFF)
			anti_alias = FALSE;
		else
			anti_alias = TRUE;

		if (size > 1)
			drawer->DrawLine(original_point,point,RGBColorToBGRA(c),size,anti_alias,selection);
		else
			drawer->DrawHairLine(original_point,point,RGBColorToBGRA(c),anti_alias,selection);

		BRect updated_rect = make_rect_from_points(original_point,point);

		// This extension might actually be too little.
		updated_rect.left -= size/2;
		updated_rect.top -= size/2;
		updated_rect.right += size/2;
		updated_rect.bottom += size/2;

		last_updated_rect = updated_rect;
		window->Lock();
		view->SetDrawingMode(old_mode);
		view->UpdateImage(updated_rect);
		view->Sync();
		window->Unlock();

		delete drawer;

		the_script->AddPoint(original_point);
		the_script->AddPoint(point);
		return the_script;
	}
	else {
		return NULL;
	}
}

int32 StraightLineTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_NO_ERROR;
}

BView* StraightLineTool::makeConfigView()
{
	StraightLineToolConfigView *target_view = new StraightLineToolConfigView(BRect(0,0,150,0),this);

	return target_view;
}


const char* StraightLineTool::ReturnHelpString(bool is_in_use)
{
	if (!is_in_use)
		return StringServer::ReturnString(STRAIGHT_LINE_TOOL_READY_STRING);
	else
		return StringServer::ReturnString(STRAIGHT_LINE_TOOL_IN_USE_STRING);
}

const void* StraightLineTool::ReturnToolCursor()
{
	return HS_LINE_CURSOR;
}


StraightLineToolConfigView::StraightLineToolConfigView(BRect rect, DrawingTool *t)
	: DrawingToolConfigView(rect,t)
{
	BMessage *message;

	BRect controller_frame = BRect(EXTRA_EDGE,EXTRA_EDGE,150+EXTRA_EDGE,EXTRA_EDGE);

	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",SIZE_OPTION);
	message->AddInt32("value",tool->GetCurrentValue(SIZE_OPTION));
	size_slider = new ControlSliderBox(controller_frame,"size",StringServer::ReturnString(SIZE_STRING),"1",message,1,100);
	AddChild(size_slider);

	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",ANTI_ALIASING_LEVEL_OPTION);
	message->AddInt32("value",0x00000000);
	controller_frame = size_slider->Frame();
	controller_frame.OffsetBy(0,controller_frame.Height()+EXTRA_EDGE);
	anti_aliasing_checkbox = new BCheckBox(controller_frame,"enable anti-aliasing box",StringServer::ReturnString(ENABLE_ANTI_ALIASING_STRING),message);
	AddChild(anti_aliasing_checkbox);
	anti_aliasing_checkbox->ResizeToPreferred();
	if (tool->GetCurrentValue(ANTI_ALIASING_LEVEL_OPTION) != B_CONTROL_OFF) {
		anti_aliasing_checkbox->SetValue(B_CONTROL_ON);
	}


	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",MODE_OPTION);
	message->AddInt32("value",0x00000000);
	controller_frame = anti_aliasing_checkbox->Frame();
	controller_frame.OffsetBy(0,controller_frame.Height()+EXTRA_EDGE);
	width_adjusting_checkbox = new BCheckBox(controller_frame,"adjustable_width_box",StringServer::ReturnString(ADJUSTABLE_WIDTH_STRING),message);
	AddChild(width_adjusting_checkbox);
	width_adjusting_checkbox->ResizeToPreferred();
	if (tool->GetCurrentValue(MODE_OPTION) != B_CONTROL_OFF) {
		width_adjusting_checkbox->SetValue(B_CONTROL_ON);
	}

	ResizeTo(max_c(width_adjusting_checkbox->Frame().right,size_slider->Frame().right)+EXTRA_EDGE,width_adjusting_checkbox->Frame().bottom + EXTRA_EDGE);
}


void StraightLineToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	size_slider->SetTarget(new BMessenger(this));
	anti_aliasing_checkbox->SetTarget(BMessenger(this));
	width_adjusting_checkbox->SetTarget(BMessenger(this));
}

