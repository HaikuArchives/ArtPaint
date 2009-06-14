/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>

#include "Cursors.h"
#include "TransparencyTool.h"
#include "StringServer.h"

TransparencyTool::TransparencyTool()
	: DrawingTool(StringServer::ReturnString(TRANSPARENCY_TOOL_NAME_STRING),TRANSPARENCY_TOOL)
{
	// The pressure option controls the speed of transparency change.
	options = SIZE_OPTION | PRESSURE_OPTION;
	number_of_options = 3;

	SetOption(SIZE_OPTION,1);
	SetOption(PRESSURE_OPTION,1);
}


TransparencyTool::~TransparencyTool()
{

}


ToolScript* TransparencyTool::UseTool(ImageView *view,uint32 buttons,BPoint point,BPoint)
{
	// Wait for the last_updated_region to become empty
	while (last_updated_rect.IsValid() == TRUE)
		snooze(50 * 1000);

	BWindow *window = view->Window();
	BBitmap *bitmap = view->ReturnImage()->ReturnActiveBitmap();

	ToolScript *the_script = new ToolScript(type,settings,((PaintApplication*)be_app)->Color(TRUE));

	BRect bounds = bitmap->Bounds();
	uint32 *bits_origin = (uint32*)bitmap->Bits();
	int32 bpr = bitmap->BytesPerRow()/4;

	BRect rc;

	// for the quick calculation of square-roots
	float sqrt_table[5500];
	for (int32 i=0;i<5500;i++)
		sqrt_table[i] = sqrt(i);

	float half_size = settings.size/2;
	rc = BRect(floor(point.x-half_size),floor(point.y-half_size),ceil(point.x+half_size),ceil(point.y+half_size));
	rc = rc & bounds;
	last_updated_rect = rc;

	union {
		uint8 bytes[4];
		uint32 word;
	} color;
	while (buttons) {
		the_script->AddPoint(point);
		uint32 transparency_value = ((PaintApplication*)be_app)->Color(TRUE).alpha;
		int32 x_dist,y_sqr;

		int32 width = rc.IntegerWidth();
		int32 height = rc.IntegerHeight();
		for (int32 y=0;y<height+1;y++) {
			y_sqr = (int32)(point.y - rc.top - y);
			y_sqr *= y_sqr;
			int32 real_y = (int32)(rc.top + y);
			int32 real_x;
			for (int32 x=0;x<width+1;x++) {
				x_dist = (int32)(point.x-rc.left-x);
				real_x = (int32)(rc.left+x);
				if (sqrt_table[x_dist*x_dist + y_sqr] <= half_size) {
//					for (int32 i=0;i<(float)GetCurrentValue(PRESSURE_OPTION)/4.0;i++) {
						color.word = *(bits_origin + real_y*bpr + real_x);
						if (color.bytes[3] < transparency_value) {
							color.bytes[3] = (uint8)min_c(color.bytes[3] + GetCurrentValue(PRESSURE_OPTION)/4.0,transparency_value);
							*(bits_origin + real_y*bpr + real_x) = color.word;
						}
						else if (color.bytes[3] > transparency_value) {
							color.bytes[3] = (uint8)max_c(color.bytes[3] - GetCurrentValue(PRESSURE_OPTION)/4.0,transparency_value);
							*(bits_origin + real_y*bpr + real_x) = color.word;
						}
//					}
				}
			}
		}

		if (rc.IsValid()) {
			window->Lock();
			view->UpdateImage(rc);
			view->Sync();
			view->getCoords(&point,&buttons);
			window->Unlock();
			half_size = settings.size/2;
			rc = BRect(floor(point.x-half_size),floor(point.y-half_size),ceil(point.x+half_size),ceil(point.y+half_size));
			rc = rc & bounds;
			last_updated_rect = last_updated_rect | rc;
			//snooze(20.0 * 1000.0);
		}
		else {
			window->Lock();
			view->getCoords(&point,&buttons);
			window->Unlock();
			half_size = settings.size/2;
			rc = BRect(floor(point.x-half_size),floor(point.y-half_size),ceil(point.x+half_size),ceil(point.y+half_size));
			rc = rc & bounds;
			last_updated_rect = last_updated_rect | rc;
			snooze(20 * 1000);
		}
	}

	view->ReturnImage()->Render(rc);
	window->Lock();
	view->Draw(view->convertBitmapRectToView(rc));
	last_updated_rect = last_updated_rect | rc;	// ???
	window->Unlock();

	return the_script;
}

int32 TransparencyTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_NO_ERROR;
}

BView* TransparencyTool::makeConfigView()
{
	TransparencyToolConfigView *target_view = new TransparencyToolConfigView(BRect(0,0,150,0),this);
	return target_view;
}


const char* TransparencyTool::ReturnHelpString(bool is_in_use)
{
	if (!is_in_use)
		return StringServer::ReturnString(TRANSPARENCY_TOOL_READY_STRING);
	else
		return StringServer::ReturnString(TRANSPARENCY_TOOL_IN_USE_STRING);
}

const void* TransparencyTool::ReturnToolCursor()
{
	return HS_TRANSPARENCY_CURSOR;
}


TransparencyToolConfigView::TransparencyToolConfigView(BRect rect,DrawingTool *t)
	: DrawingToolConfigView(rect,t)
{
	BMessage *message;

	BRect controller_frame = BRect(EXTRA_EDGE,EXTRA_EDGE,150+EXTRA_EDGE,EXTRA_EDGE);

	// First add the controller for size.
	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",SIZE_OPTION);
	message->AddInt32("value",tool->GetCurrentValue(SIZE_OPTION));
	size_slider = new ControlSliderBox(controller_frame,"size",StringServer::ReturnString(SIZE_STRING),"1",message,1,100);
	AddChild(size_slider);

	// Then add the controller for speed.
	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",PRESSURE_OPTION);
	message->AddInt32("value",tool->GetCurrentValue(PRESSURE_OPTION));
	controller_frame = size_slider->Frame();
	controller_frame.OffsetBy(0,controller_frame.Height()+EXTRA_EDGE);
	speed_slider = new ControlSliderBox(controller_frame,"speed",StringServer::ReturnString(SPEED_STRING),"1",message,1,100);
	AddChild(speed_slider);

	float divider = max_c(size_slider->Divider(),speed_slider->Divider());

	size_slider->SetDivider(divider);
	speed_slider->SetDivider(divider);

//	// Then add the controller for target transparency.
//	message = new BMessage(OPTION_CHANGED);
//	message->AddInt32("option",TRANSPARENCY_OPTION);
//	message->AddInt32("value",tool->GetCurrentValue(TRANSPARENCY_OPTION));
//	controller_frame = size_slider->Frame();
//	controller_frame.OffsetBy(0,controller_frame.Height()+EXTRA_EDGE);
//	transparency_slider = new ControlSliderBox(controller_frame,"transparency","","100",message,0,100);
//	AddChild(speed_slider);

	ResizeTo(speed_slider->Frame().right+EXTRA_EDGE,speed_slider->Frame().bottom + EXTRA_EDGE);
}



void TransparencyToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	size_slider->SetTarget(new BMessenger(this));
	speed_slider->SetTarget(new BMessenger(this));
}
