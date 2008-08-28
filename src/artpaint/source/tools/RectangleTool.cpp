/* 

	Filename:	RectangleTool.cpp
	Contents:	RectangleTool definitions.	
	Author:		Heikki Suhonen
	
*/

#include <Box.h>
#include <CheckBox.h>
#include <math.h>
#include <RadioButton.h>
#include <stdio.h>

#include "RectangleTool.h"
#include "HSPolygon.h"
#include "Selection.h"
#include "StringServer.h"
#include "Cursors.h"

#define	PI	M_PI


RectangleTool::RectangleTool()
	: DrawingTool(StringServer::ReturnString(RECTANGLE_TOOL_NAME_STRING),RECTANGLE_TOOL)
{
	options = FILL_ENABLED_OPTION | SIZE_OPTION | SHAPE_OPTION | ROTATION_ENABLED_OPTION | ANTI_ALIASING_LEVEL_OPTION;
	number_of_options = 5;
	
	SetOption(FILL_ENABLED_OPTION,B_CONTROL_OFF);
	SetOption(SIZE_OPTION,1);	
	SetOption(SHAPE_OPTION,HS_CORNER_TO_CORNER);
	SetOption(ROTATION_ENABLED_OPTION,B_CONTROL_OFF);
	SetOption(ANTI_ALIASING_LEVEL_OPTION,B_CONTROL_OFF);
}


RectangleTool::~RectangleTool()
{

}


ToolScript* RectangleTool::UseTool(ImageView *view,uint32 buttons,BPoint point,BPoint view_point)
{
	// Wait for the last_updated_region to become empty
	while (last_updated_rect.IsValid() == TRUE)
		snooze(50 * 1000);

	BWindow *window = view->Window();
	drawing_mode old_mode;
	BBitmap *bitmap = view->ReturnImage()->ReturnActiveBitmap();
//	BView *bitmap_view = view->getBufferView();

	ToolScript *the_script = new ToolScript(type,settings,((PaintApplication*)be_app)->GetColor(TRUE));
	Selection *selection = view->GetSelection();
	
	bool draw_rectangle = TRUE;
				
	if (window != NULL) {
		BitmapDrawer *drawer = new BitmapDrawer(bitmap);
		
		BPoint original_point;
		BRect bitmap_rect,old_rect,new_rect;
		window->Lock();
		old_mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_INVERT);
		window->Unlock();
		rgb_color c=((PaintApplication*)be_app)->GetColor(TRUE);
		original_point = point;
		bitmap_rect = BRect(point,point);
		old_rect = new_rect = view->convertBitmapRectToView(bitmap_rect);
		window->Lock();
		view->StrokeRect(new_rect);
		window->Unlock();
		
		while (buttons) {
			window->Lock();
			view->getCoords(&point,&buttons,&view_point);
			window->Unlock();
			bitmap_rect = make_rect_from_points(original_point,point);
			if (modifiers() & B_LEFT_CONTROL_KEY) {
				// Make the rectangle square.
				float max_distance = max_c(bitmap_rect.Height(),bitmap_rect.Width());
				if (original_point.x == bitmap_rect.left)
					bitmap_rect.right = bitmap_rect.left + max_distance;
				else
					bitmap_rect.left = bitmap_rect.right - max_distance;

				if (original_point.y == bitmap_rect.top)
					bitmap_rect.bottom = bitmap_rect.top + max_distance;
				else 
					bitmap_rect.top = bitmap_rect.bottom - max_distance;				
			}			
			if (GetCurrentValue(SHAPE_OPTION) == HS_CENTER_TO_CORNER) {
				// Make the the rectangle original corner be at the center of
				// new rectangle.
				float y_distance = bitmap_rect.Height();
				float x_distance = bitmap_rect.Width();
				
				if (bitmap_rect.left == original_point.x)
					bitmap_rect.left = bitmap_rect.left - x_distance;
				else
					bitmap_rect.right = bitmap_rect.right + x_distance;
				
				if (bitmap_rect.top == original_point.y)
					bitmap_rect.top = bitmap_rect.top - y_distance;
				else
					bitmap_rect.bottom = bitmap_rect.bottom + y_distance;
				
			}
			new_rect = view->convertBitmapRectToView(bitmap_rect);
						
			if (view->LockLooper() == true) {
				if (old_rect != new_rect) {
					view->StrokeRect(old_rect);
					view->StrokeRect(new_rect);
					old_rect = new_rect;
				}
				view->UnlockLooper();
			}
			snooze(20 * 1000);
		}

		HSPolygon *poly;
		BPoint *corners = new BPoint[4];
		corners[0] = bitmap_rect.LeftTop();
		corners[1] = bitmap_rect.RightTop();
		corners[2] = bitmap_rect.RightBottom();
		corners[3] = bitmap_rect.LeftBottom();
		poly = new HSPolygon(corners,4);

		if (GetCurrentValue(ROTATION_ENABLED_OPTION) == B_CONTROL_ON) {
			HSPolygon *view_poly;			
			corners[0] = new_rect.LeftTop();
			corners[1] = new_rect.RightTop();
			corners[2] = new_rect.RightBottom();
			corners[3] = new_rect.LeftBottom();
			view_poly = new HSPolygon(corners,4);
			
			BPoint centroid = bitmap_rect.LeftTop() + bitmap_rect.RightTop() + bitmap_rect.RightBottom() + bitmap_rect.LeftBottom();
			centroid.x /= 4;
			centroid.y /= 4;
			float new_angle,prev_angle;
			prev_angle = new_angle =0;
			bool continue_rotating = TRUE;
			float original_angle;
			if (centroid.x != point.x)
				original_angle = atan((centroid.y-point.y)/(centroid.x-point.x)) * 180 /PI;
			else {
				original_angle = 90;
			}
			while (continue_rotating) {
				if (is_clicks_data_valid) {
					continue_rotating = FALSE;
					is_clicks_data_valid = FALSE;
				}
				else if (is_keys_data_valid) {
					if (last_key_event_bytes[0] == B_ESCAPE) {
						continue_rotating = FALSE;
						draw_rectangle = FALSE;
					}
					is_keys_data_valid = FALSE;
				}
				else {
					// Here we should rotate the polygon
					window->Lock();
					if (new_angle != prev_angle) {
						BPolygon *draw_poly = view_poly->GetBPolygon();
						view->StrokePolygon(draw_poly);
						delete draw_poly;
						
//						poly->RotateAboutCenter(new_angle - prev_angle);
						view_poly->RotateAboutCenter(new_angle - prev_angle);
			
						draw_poly = view_poly->GetBPolygon();
						view->StrokePolygon(draw_poly);
						delete draw_poly;
						prev_angle = new_angle;
					}	
					view->getCoords(&point,&buttons);
					window->Unlock();
					if (centroid.x != point.x) {
						new_angle = atan((centroid.y-point.y)/(centroid.x-point.x)) * 180 /PI;
					}
					else
						new_angle = 90;

					new_angle -= original_angle;

					if (modifiers() & B_LEFT_CONTROL_KEY) {
						int32 divider = (int32)new_angle/5;
						float remainder = new_angle - divider*5;
						if (remainder >= 2.5)
							new_angle = (divider+1)*5;
						else
							new_angle = divider*5;
					}
					snooze(20 * 1000);
				
				}
			}
			poly->RotateAboutCenter(prev_angle);
				
			delete view_poly;
		}

		bitmap->Lock();
		bool fill = (GetCurrentValue(FILL_ENABLED_OPTION) == B_CONTROL_ON);
		bool anti_a = (GetCurrentValue(ANTI_ALIASING_LEVEL_OPTION) == B_CONTROL_ON);
		if (draw_rectangle == TRUE) {
			if (poly->GetPointCount() == 4) {
				drawer->DrawRectanglePolygon(poly->GetPointList(),RGBColorToBGRA(c),fill,anti_a,selection);
			}
			else { 
				drawer->DrawRectanglePolygon(corners,RGBColorToBGRA(c),fill,anti_a,selection);
			}
		}
		bitmap->Unlock();
		last_updated_rect = poly->BoundingBox();
		last_updated_rect.InsetBy(-1,-1);
		if (poly->GetPointCount() == 4) {
			BPoint *points = poly->GetPointList();
			for (int32 i=0;i<4;i++)
				the_script->AddPoint(points[i]);
		}
		else {
			for (int32 i=0;i<4;i++)
				the_script->AddPoint(corners[i]);
		}
		delete poly;
		delete[] corners;

		window->Lock();
		view->SetDrawingMode(old_mode);
		view->UpdateImage(last_updated_rect);
		view->Sync();
		window->Unlock();

		delete drawer;
		
	}		

	return the_script;
}


int32 RectangleTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_NO_ERROR;
}

BView* RectangleTool::makeConfigView()
{
	RectangleToolConfigView *target_view = new RectangleToolConfigView(BRect(0,0,150,0),this);

	return target_view;
}

const char* RectangleTool::ReturnHelpString(bool is_in_use)
{
	if (!is_in_use)
		return StringServer::ReturnString(RECTANGLE_TOOL_READY_STRING);
	else
		return StringServer::ReturnString(RECTANGLE_TOOL_IN_USE_STRING);
}


const void* RectangleTool::ReturnToolCursor()
{
	return HS_RECTANGLE_CURSOR;
}


RectangleToolConfigView::RectangleToolConfigView(BRect rect,DrawingTool *t)
	: DrawingToolConfigView(rect,t)
{
	BRect controller_frame = BRect(EXTRA_EDGE,EXTRA_EDGE,150+EXTRA_EDGE,EXTRA_EDGE);	
	BBox *container = new BBox(controller_frame,"enable rectangle fill-box");
	AddChild(container);

	BMessage *control_message;	

	// Add a check-box for enabling the fill.
	control_message = new BMessage(OPTION_CHANGED);
	control_message->AddInt32("option",FILL_ENABLED_OPTION);
	control_message->AddInt32("value",0x00000000);
	fill_checkbox = new BCheckBox(BRect(EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE),"enable fill box",StringServer::ReturnString(FILL_RECTANGLE_STRING),control_message);	
	container->AddChild(fill_checkbox);
	fill_checkbox->ResizeToPreferred();
	if (tool->GetCurrentValue(FILL_ENABLED_OPTION) != B_CONTROL_OFF) {
		fill_checkbox->SetValue(B_CONTROL_ON);
	}	

	container->ResizeBy(0,fill_checkbox->Bounds().Height()+2*EXTRA_EDGE);

	controller_frame.OffsetBy(0,container->Bounds().Height()+EXTRA_EDGE);
	container = new BBox(controller_frame,"ellipse mode select buttons");
	AddChild(container);
	
	// Create the first radio-button.			
	control_message = new BMessage(OPTION_CHANGED);
	control_message->AddInt32("option",SHAPE_OPTION);
	control_message->AddInt32("value",HS_CORNER_TO_CORNER);
	
	radio_button_1 = new BRadioButton(BRect(EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE),"a radio button",StringServer::ReturnString(CORNER_TO_CORNER_STRING),control_message);
	radio_button_1->ResizeToPreferred();
	container->AddChild(radio_button_1);
	if (tool->GetCurrentValue(SHAPE_OPTION) == HS_CORNER_TO_CORNER)
		radio_button_1->SetValue(B_CONTROL_ON);
	
	// Create the second radio-button.		
	control_message = new BMessage(OPTION_CHANGED);
	control_message->AddInt32("option",SHAPE_OPTION);
	control_message->AddInt32("value",HS_CENTER_TO_CORNER);
	radio_button_2 = new BRadioButton(BRect(EXTRA_EDGE,radio_button_1->Frame().bottom,EXTRA_EDGE,radio_button_1->Frame().bottom)," a radio button",StringServer::ReturnString(CENTER_TO_CORNER_STRING),control_message);
	radio_button_2->ResizeToPreferred();		
	container->AddChild(radio_button_2);
	if (tool->GetCurrentValue(SHAPE_OPTION) == HS_CENTER_TO_CORNER)
		radio_button_2->SetValue(B_CONTROL_ON);

	container->ResizeBy(0,radio_button_2->Frame().bottom+EXTRA_EDGE);

	// Create a check-box for enabling the rotation.
	controller_frame.OffsetBy(0,container->Bounds().Height()+EXTRA_EDGE);
	container = new BBox(controller_frame,"ellipse mode select buttons");
	AddChild(container);

	control_message = new BMessage(OPTION_CHANGED);
	control_message->AddInt32("option",ROTATION_ENABLED_OPTION);
	control_message->AddInt32("value",0x00000000);
	rotation_checkbox = new BCheckBox(BRect(EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE),"enable rotation box",StringServer::ReturnString(ENABLE_ROTATION_STRING),control_message);	
	container->AddChild(rotation_checkbox);
	rotation_checkbox->ResizeToPreferred();
	if (tool->GetCurrentValue(ROTATION_ENABLED_OPTION) != B_CONTROL_OFF) {
		rotation_checkbox->SetValue(B_CONTROL_ON);
	}	
	// Create a check-box for enabling anti-aliasing.
	control_message = new BMessage(OPTION_CHANGED);
	control_message->AddInt32("option",ANTI_ALIASING_LEVEL_OPTION);
	control_message->AddInt32("value",0x00000000);
	controller_frame = rotation_checkbox->Frame();
	controller_frame.OffsetBy(0,controller_frame.Height());
	anti_alias_checkbox = new BCheckBox(controller_frame,"enable anti-aliasing box",StringServer::ReturnString(ENABLE_ANTI_ALIASING_STRING),control_message);	
	container->AddChild(anti_alias_checkbox);
	anti_alias_checkbox->ResizeToPreferred();
	if (tool->GetCurrentValue(ANTI_ALIASING_LEVEL_OPTION) != B_CONTROL_OFF) {
		anti_alias_checkbox->SetValue(B_CONTROL_ON);
	}	

	container->ResizeBy(0,anti_alias_checkbox->Frame().bottom+EXTRA_EDGE);

	controller_frame = container->Frame();
	controller_frame.OffsetBy(0,controller_frame.Height()+EXTRA_EDGE);
	
	// Finally resize the target-view to proper size.
	ResizeTo(container->Bounds().Width()+2*EXTRA_EDGE,container->Frame().bottom + EXTRA_EDGE);	
}


void RectangleToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();
	
	fill_checkbox->SetTarget(BMessenger(this));
	radio_button_1->SetTarget(BMessenger(this));
	radio_button_2->SetTarget(BMessenger(this));
	rotation_checkbox->SetTarget(BMessenger(this));
	anti_alias_checkbox->SetTarget(BMessenger(this));
}
