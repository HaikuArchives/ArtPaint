/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "SimpleTool.h"
#include "StringServer.h"
#include "Cursors.h"

// this class is for simple test tool
SimpleTool::SimpleTool()
		: LineTool(StringServer::ReturnString(FREE_LINE_TOOL_NAME_STRING),FREE_LINE_TOOL)
{
	// here we initialize the settings

	// tell which options we have
	options = SIZE_OPTION;
	number_of_options = 1;

//	// set the max and min and current values
//	size_max = 100;
//	size_min = 1;
	SetOption(SIZE_OPTION,1);

//	pressure_max = 100;
//	pressure_min = 0;
}


SimpleTool::~SimpleTool()
{
	// free whatever storage this class allocated
}


ToolScript* SimpleTool::UseTool(ImageView *view,uint32 buttons,BPoint point,BPoint)
{
	// here we first get the necessary data from view
	// and then start drawing while mousebutton is held down

	// Wait for the last_updated_region to become empty
	while (last_updated_rect.IsValid() == TRUE)
		snooze(50 * 1000);

	coordinate_queue = new CoordinateQueue();
	image_view = view;
	thread_id coordinate_reader = spawn_thread(CoordinateReader,"read coordinates",B_NORMAL_PRIORITY,this);
	resume_thread(coordinate_reader);
	reading_coordinates = TRUE;
	ToolScript *the_script = new ToolScript(type,settings,((PaintApplication*)be_app)->GetColor(TRUE));

	BBitmap* buffer = view->ReturnImage()->ReturnActiveBitmap();
	Selection *selection = view->GetSelection();
//	BView *buffer_view = view->getBufferView();
	BitmapDrawer *drawer = new BitmapDrawer(buffer);
	rgb_color new_color;
	uint32 new_color_bgra;
//	int32 original_mouse_speed;
//	get_mouse_speed(&original_mouse_speed);

	if (buffer == NULL) {
		delete the_script;
		return NULL;
	}
	BPoint prev_point;

	prev_point = point;
	BRect updated_rect;
	status_t status_of_read;
	int diameter = settings.size;
	if ((diameter%2) == 0)
		diameter++;

	new_color = ((PaintApplication*)be_app)->GetColor(TRUE);
	new_color_bgra = RGBColorToBGRA(new_color);
	if (diameter != 1)
		drawer->DrawCircle(prev_point,diameter/2,new_color_bgra,TRUE,TRUE,selection);
	else
		drawer->DrawHairLine(prev_point,point,new_color_bgra,(bool)FALSE,selection);

	// This makes sure that the view is updated even if just one point is drawn
	updated_rect.left = min_c(point.x-diameter/2,prev_point.x-diameter/2);
	updated_rect.top = min_c(point.y-diameter/2,prev_point.y-diameter/2);
	updated_rect.right = max_c(point.x+diameter/2,prev_point.x+diameter/2);
	updated_rect.bottom = max_c(point.y+diameter/2,prev_point.y+diameter/2);

	// We should do the composite picture and re-draw the window in
	// a separate thread.
	view->ReturnImage()->Render(updated_rect);
	view->Window()->Lock();
	// We have to use Draw, because Invalidate causes flickering by erasing
	// the area before calling Draw.
	view->Draw(view->convertBitmapRectToView(updated_rect));
	view->Window()->Unlock();

	last_updated_rect = updated_rect;
	the_script->AddPoint(point);
	while (((status_of_read = coordinate_queue->Get(point)) == B_OK) || (reading_coordinates == TRUE)) {
		if ( (status_of_read == B_OK) && (prev_point != point) ) {
			the_script->AddPoint(point);
//			if (modifiers() & B_LEFT_CONTROL_KEY) {
//				set_mouse_speed(0);
//			}
//			else {
//				set_mouse_speed(original_mouse_speed);
//			}
			// first set the color
			new_color = ((PaintApplication*)be_app)->GetColor(TRUE);
			new_color_bgra = RGBColorToBGRA(new_color);
			diameter = settings.size;
			if ((diameter%2) == 0)
				diameter++;


			if (diameter != 1) {
				drawer->DrawCircle(point,diameter/2,new_color_bgra,true,false,selection);
				drawer->DrawLine(prev_point,point,new_color_bgra,diameter,false,selection);
			}
			else
				drawer->DrawHairLine(prev_point,point,new_color_bgra,(bool)false,selection);

			updated_rect.left = min_c(point.x-diameter/2-1,prev_point.x-diameter/2-1);
			updated_rect.top = min_c(point.y-diameter/2-1,prev_point.y-diameter/2-1);
			updated_rect.right = max_c(point.x+diameter/2+1,prev_point.x+diameter/2+1);
			updated_rect.bottom = max_c(point.y+diameter/2+1,prev_point.y+diameter/2+1);

			last_updated_rect = last_updated_rect | updated_rect;

			// We should do the composite picture and re-draw the window in
			// a separate thread.
			view->Window()->Lock();
			view->UpdateImage(updated_rect);
			view->Sync();
			view->Window()->Unlock();
			prev_point = point;
		}
		else
			snooze(20 * 1000);
	}


	delete drawer;
	delete coordinate_queue;
	return the_script;
}

int32 SimpleTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_NO_ERROR;
}

BView* SimpleTool::makeConfigView()
{
	SimpleToolConfigView *target_view = new SimpleToolConfigView(BRect(0,0,150,0),this);

	return target_view;
}


const char* SimpleTool::ReturnHelpString(bool is_in_use)
{
	if (!is_in_use)
		return StringServer::ReturnString(FREE_LINE_TOOL_READY_STRING);
	else
		return StringServer::ReturnString(FREE_LINE_TOOL_IN_USE_STRING);
}

const void* SimpleTool::ReturnToolCursor()
{
	return HS_FREE_LINE_CURSOR;
}


int32 SimpleTool::CoordinateReader(void *data)
{
	SimpleTool *this_pointer = (SimpleTool*)data;
	return this_pointer->read_coordinates();
}


int32 SimpleTool::read_coordinates()
{
	reading_coordinates = TRUE;
	uint32 buttons;
	BPoint point,prev_point;
	BPoint view_point;
	image_view->Window()->Lock();
	image_view->getCoords(&point,&buttons,&view_point);
	image_view->MovePenTo(view_point);
	image_view->Window()->Unlock();
	prev_point = point + BPoint(1,1);

	while (buttons) {
		image_view->Window()->Lock();
		if (point != prev_point) {
			coordinate_queue->Put(point);
			image_view->StrokeLine(view_point);
			prev_point = point;
		}
		image_view->getCoords(&point,&buttons,&view_point);
		image_view->Window()->Unlock();
		snooze(20 * 1000);
	}

	reading_coordinates = FALSE;
	return B_OK;
}






SimpleToolConfigView::SimpleToolConfigView(BRect rect, DrawingTool *t)
	: DrawingToolConfigView(rect,t)
{
	BMessage *message;

	BRect controller_frame = BRect(EXTRA_EDGE,EXTRA_EDGE,150+2*EXTRA_EDGE,EXTRA_EDGE);

	// First add the controller for size.
	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",SIZE_OPTION);
	message->AddInt32("value",tool->GetCurrentValue(SIZE_OPTION));
	size_slider = new ControlSliderBox(controller_frame,"size",StringServer::ReturnString(SIZE_STRING),"1",message,1,100);
	AddChild(size_slider);
	// Remember to offset the controller_frame
	controller_frame.OffsetBy(0,size_slider->Bounds().Height() + EXTRA_EDGE);


	ResizeTo(controller_frame.Width()+2*EXTRA_EDGE,size_slider->Frame().bottom + EXTRA_EDGE);
}



void SimpleToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();
	size_slider->SetTarget(new BMessenger(this));
}
