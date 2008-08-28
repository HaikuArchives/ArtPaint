/* 

	Filename:	RectangleTool.cpp
	Contents:	RectangleTool definitions.	
	Author:		Heikki Suhonen
	
*/

#include <Box.h>
#include <CheckBox.h>
#include <RadioButton.h>


#include "EllipseTool.h"
#include "Selection.h"
#include "StringServer.h"
#include "Cursors.h"


EllipseTool::EllipseTool()
	: DrawingTool(StringServer::ReturnString(ELLIPSE_TOOL_NAME_STRING),ELLIPSE_TOOL)
{
	options = FILL_ENABLED_OPTION | SIZE_OPTION | SHAPE_OPTION;
	number_of_options = 3;
	
	SetOption(FILL_ENABLED_OPTION,B_CONTROL_OFF);
	SetOption(SIZE_OPTION,1);	
	SetOption(SHAPE_OPTION,HS_CENTER_TO_CORNER);
}


EllipseTool::~EllipseTool()
{

}


ToolScript* EllipseTool::UseTool(ImageView *view,uint32 buttons,BPoint point,BPoint)
{
	// Wait for the last_updated_region to become empty
	while (last_updated_rect.IsValid() == TRUE)
		snooze(50 * 1000);

	BWindow *window = view->Window();
	drawing_mode old_mode;
	BBitmap *bitmap = view->ReturnImage()->ReturnActiveBitmap();

	ToolScript *the_script = new ToolScript(type,settings,((PaintApplication*)be_app)->GetColor(TRUE));
	Selection *selection = view->GetSelection();	
	
	if (window != NULL) {		
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
		view->StrokeEllipse(new_rect);
		window->Unlock();
			
		while (buttons) {
			window->Lock();
			if (old_rect != new_rect) {
				view->StrokeEllipse(old_rect);
				view->StrokeEllipse(new_rect);
				old_rect = new_rect;
			}
			view->getCoords(&point,&buttons);
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
			snooze(20 * 1000);
		}
		BitmapDrawer *drawer = new BitmapDrawer(bitmap);
		bool use_fill = (GetCurrentValue(FILL_ENABLED_OPTION) == B_CONTROL_ON);
		bool use_anti_aliasing = FALSE;
		drawer->DrawEllipse(bitmap_rect,RGBColorToBGRA(c),use_fill,use_anti_aliasing,selection);
		delete drawer;
		
		the_script->AddPoint(bitmap_rect.LeftTop());
		the_script->AddPoint(bitmap_rect.RightTop());
		the_script->AddPoint(bitmap_rect.RightBottom());
		the_script->AddPoint(bitmap_rect.LeftBottom());
		
		bitmap_rect.InsetBy(-1,-1);
		window->Lock();
		view->SetDrawingMode(old_mode);
		view->UpdateImage(bitmap_rect);
		view->Sync();
		window->Unlock();
		last_updated_rect = bitmap_rect;
	}		

	return the_script;
}


int32 EllipseTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_NO_ERROR;
}

BView* EllipseTool::makeConfigView()
{
	EllipseToolConfigView *target_view = new EllipseToolConfigView(BRect(0,0,150,0),this);

	return target_view;
}


const char* EllipseTool::ReturnHelpString(bool is_in_use)
{
	if (!is_in_use)
		return StringServer::ReturnString(ELLIPSE_TOOL_READY_STRING);
	else
		return StringServer::ReturnString(ELLIPSE_TOOL_IN_USE_STRING);
}

const void* EllipseTool::ReturnToolCursor()
{
	return HS_ELLIPSE_CURSOR;
}


EllipseToolConfigView::EllipseToolConfigView(BRect rect,DrawingTool *t)
	: DrawingToolConfigView(rect,t)
{
	BRect controller_frame = BRect(EXTRA_EDGE,EXTRA_EDGE,150+EXTRA_EDGE,EXTRA_EDGE);
	
	BBox *container = new BBox(controller_frame,"enable ellipse fill-box");
	AddChild(container);

	BMessage *control_message;	
	// Add a check-box for enabling the fill.
	control_message = new BMessage(OPTION_CHANGED);
	control_message->AddInt32("option",FILL_ENABLED_OPTION);
	control_message->AddInt32("value",0x00000000);
	fill_checkbox = new BCheckBox(BRect(EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE),"enable fill box",StringServer::ReturnString(FILL_ELLIPSE_STRING),control_message);	
	container->AddChild(fill_checkbox);
	fill_checkbox->ResizeToPreferred();
	if (tool->GetCurrentValue(FILL_ENABLED_OPTION) != B_CONTROL_OFF) {
		fill_checkbox->SetValue(B_CONTROL_ON);
	}	

	container->ResizeBy(0,fill_checkbox->Bounds().Height()+2*EXTRA_EDGE);

	controller_frame.OffsetBy(0,container->Bounds().Height()+EXTRA_EDGE);
	container = new BBox(controller_frame,"ellipse mode select buttons");
	AddChild(container);
	
	control_message = new BMessage(OPTION_CHANGED);
	control_message->AddInt32("option",SHAPE_OPTION);
	control_message->AddInt32("value",HS_CORNER_TO_CORNER);
	
	radio_button_1 = new BRadioButton(BRect(EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE),"a radio button",StringServer::ReturnString(CORNER_TO_CORNER_STRING),control_message);
	radio_button_1->ResizeToPreferred();
	container->AddChild(radio_button_1);
	if (tool->GetCurrentValue(SHAPE_OPTION) == HS_CORNER_TO_CORNER)
		radio_button_1->SetValue(B_CONTROL_ON);
	
	control_message = new BMessage(OPTION_CHANGED);
	control_message->AddInt32("option",SHAPE_OPTION);
	control_message->AddInt32("value",HS_CENTER_TO_CORNER);
	radio_button_2 = new BRadioButton(BRect(EXTRA_EDGE,radio_button_1->Frame().bottom,EXTRA_EDGE,radio_button_1->Frame().bottom)," a radio button",StringServer::ReturnString(CENTER_TO_CORNER_STRING),control_message);
	radio_button_2->ResizeToPreferred();		
	container->AddChild(radio_button_2);
	if (tool->GetCurrentValue(SHAPE_OPTION) == HS_CENTER_TO_CORNER)
		radio_button_2->SetValue(B_CONTROL_ON);
	container->ResizeBy(0,radio_button_2->Frame().bottom+EXTRA_EDGE);
		
	// Finally resize the target-view to proper size.
	ResizeTo(container->Bounds().Width()+2*EXTRA_EDGE,container->Frame().bottom + EXTRA_EDGE);	
}


void EllipseToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();
	
	fill_checkbox->SetTarget(BMessenger(this));
	radio_button_1->SetTarget(BMessenger(this));
	radio_button_2->SetTarget(BMessenger(this));
}
