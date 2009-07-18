/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "FreeLineTool.h"

#include "BitmapDrawer.h"
#include "Controls.h"
#include "Cursors.h"
#include "CoordinateQueue.h"
#include "Image.h"
#include "PaintApplication.h"
#include "StringServer.h"
#include "UtilityClasses.h"


#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Window.h>


FreeLineTool::FreeLineTool()
	: LineTool(StringServer::ReturnString(FREE_LINE_TOOL_NAME_STRING),
		FREE_LINE_TOOL)
{
	options = SIZE_OPTION;
	number_of_options = 1;

	SetOption(SIZE_OPTION, 1);
}


FreeLineTool::~FreeLineTool()
{
}


ToolScript*
FreeLineTool::UseTool(ImageView* view, uint32 buttons, BPoint point, BPoint)
{
	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	coordinate_queue = new CoordinateQueue();
	image_view = view;
	thread_id coordinate_reader = spawn_thread(CoordinateReader,
		"read coordinates", B_NORMAL_PRIORITY, this);
	resume_thread(coordinate_reader);
	reading_coordinates = true;
	ToolScript *the_script = new ToolScript(Type(), settings,
		((PaintApplication*)be_app)->Color(true));

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

	new_color = ((PaintApplication*)be_app)->Color(true);
	new_color_bgra = RGBColorToBGRA(new_color);
	if (diameter != 1)
		drawer->DrawCircle(prev_point,diameter/2,new_color_bgra,true, true,selection);
	else
		drawer->DrawHairLine(prev_point,point,new_color_bgra, false, selection);

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

	SetLastUpdatedRect(updated_rect);
	the_script->AddPoint(point);
	while (((status_of_read = coordinate_queue->Get(point)) == B_OK)
		|| (reading_coordinates == true)) {
		if ( (status_of_read == B_OK) && (prev_point != point) ) {
			the_script->AddPoint(point);
//			if (modifiers() & B_LEFT_CONTROL_KEY) {
//				set_mouse_speed(0);
//			}
//			else {
//				set_mouse_speed(original_mouse_speed);
//			}
			// first set the color
			new_color = ((PaintApplication*)be_app)->Color(true);
			new_color_bgra = RGBColorToBGRA(new_color);
			diameter = settings.size;
			if ((diameter%2) == 0)
				diameter++;


			if (diameter != 1) {
				drawer->DrawCircle(point,diameter/2,new_color_bgra,true,false,selection);
				drawer->DrawLine(prev_point,point,new_color_bgra,diameter,false,selection);
			}
			else
				drawer->DrawHairLine(prev_point,point,new_color_bgra,false,selection);

			updated_rect.left = min_c(point.x-diameter/2-1,prev_point.x-diameter/2-1);
			updated_rect.top = min_c(point.y-diameter/2-1,prev_point.y-diameter/2-1);
			updated_rect.right = max_c(point.x+diameter/2+1,prev_point.x+diameter/2+1);
			updated_rect.bottom = max_c(point.y+diameter/2+1,prev_point.y+diameter/2+1);

			SetLastUpdatedRect(LastUpdatedRect() | updated_rect);

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


int32
FreeLineTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_OK;
}


BView*
FreeLineTool::makeConfigView()
{
	return (new FreeLineToolConfigView(this));
}


const void*
FreeLineTool::ToolCursor() const
{
	return HS_FREE_LINE_CURSOR;
}


const char*
FreeLineTool::HelpString(bool isInUse) const
{
	return StringServer::ReturnString(isInUse ? FREE_LINE_TOOL_IN_USE_STRING
		: FREE_LINE_TOOL_READY_STRING);
}


int32
FreeLineTool::CoordinateReader(void *data)
{
	FreeLineTool *this_pointer = (FreeLineTool*)data;
	return this_pointer->read_coordinates();
}


int32
FreeLineTool::read_coordinates()
{
	reading_coordinates = true;
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

	reading_coordinates = false;
	return B_OK;
}


// #pragma mark -- FreeLineToolConfigView


FreeLineToolConfigView::FreeLineToolConfigView(DrawingTool* newTool)
	: DrawingToolConfigView(newTool)
{
	BMessage* message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option", SIZE_OPTION);
	message->AddInt32("value", tool->GetCurrentValue(SIZE_OPTION));

	size_slider = new ControlSliderBox("size",
		StringServer::ReturnString(SIZE_STRING), "1", message, 1, 100);

	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 5.0).Add(size_slider));
}



void
FreeLineToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();
	size_slider->SetTarget(new BMessenger(this));
}
