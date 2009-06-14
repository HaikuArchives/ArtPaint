/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <RadioButton.h>

#include "Cursors.h"
#include "EraserTool.h"
#include "StringServer.h"

// this class is for simple test tool
EraserTool::EraserTool()
		: LineTool(StringServer::ReturnString(ERASER_TOOL_NAME_STRING),ERASER_TOOL)
{
	options = SIZE_OPTION | MODE_OPTION;
	number_of_options = 2;

	SetOption(SIZE_OPTION,1);
	SetOption(MODE_OPTION,HS_ERASE_TO_BACKGROUND_MODE);
}


EraserTool::~EraserTool()
{
	// free whatever storage this class allocated
}


ToolScript* EraserTool::UseTool(ImageView *view,uint32 buttons,BPoint point,BPoint)
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
	ToolScript *the_script = new ToolScript(type,settings,((PaintApplication*)be_app)->Color(TRUE));

	BBitmap* buffer = view->ReturnImage()->ReturnActiveBitmap();
	Selection *selection = view->GetSelection();
	BitmapDrawer *drawer = new BitmapDrawer(buffer);
	union {
		uint8 bytes[4];
		uint32 word;
	} background;
	background.bytes[0] = 0xFF;
	background.bytes[1] = 0xFF;
	background.bytes[2] = 0xFF;
	background.bytes[3] = 0x00;


	if (settings.mode == HS_ERASE_TO_BACKGROUND_MODE) {
		rgb_color c = ((PaintApplication*)be_app)->Color(false);
		background.bytes[0] = c.blue;
		background.bytes[1] = c.green;
		background.bytes[2] = c.red;
		background.bytes[3] = c.alpha;
	}

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

	if (diameter != 1)
		drawer->DrawCircle(prev_point,diameter/2,background.word,TRUE,TRUE,selection);
	else
		drawer->DrawHairLine(prev_point,point,background.word,(bool)FALSE,selection);

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
			diameter = settings.size;
			if ((diameter%2) == 0)
				diameter++;


			if (diameter != 1) {
				drawer->DrawCircle(point,diameter/2,background.word,TRUE,FALSE,selection);
				drawer->DrawLine(prev_point,point,background.word,diameter,FALSE,selection);
			}
			else
				drawer->DrawHairLine(prev_point,point,background.word,(bool)FALSE,selection);

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

//	set_mouse_speed(original_mouse_speed);
//	help_message = new BMessage(HS_REGULAR_HELP_MESSAGE);
//	help_message->AddString("message",HS_DRAW_MODE_HELP_MESSAGE);
//	view->Window()->PostMessage(help_message,view->Window());
//
//	delete help_message;

	delete drawer;
	delete coordinate_queue;
	return the_script;
}

int32 EraserTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_ERROR;
}

BView* EraserTool::makeConfigView()
{
	EraserToolConfigView *target_view = new EraserToolConfigView(BRect(0,0,150,0),this);

	return target_view;
}


int32 EraserTool::CoordinateReader(void *data)
{
	EraserTool *this_pointer = (EraserTool*)data;
	return this_pointer->read_coordinates();
}


const char* EraserTool::ReturnHelpString(bool is_in_use)
{
	if (!is_in_use)
		return StringServer::ReturnString(ERASER_TOOL_READY_STRING);
	else
		return StringServer::ReturnString(ERASER_TOOL_IN_USE_STRING);
}

const void* EraserTool::ReturnToolCursor()
{
	return HS_ERASER_CURSOR;
}


int32 EraserTool::read_coordinates()
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






EraserToolConfigView::EraserToolConfigView(BRect rect, DrawingTool *t)
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

	// Create a BBox for the radio-buttons.
	BBox *container = new BBox(controller_frame,"eraser_mode");
	container->SetLabel(StringServer::ReturnString(COLOR_STRING));
	AddChild(container);

	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",MODE_OPTION);
	message->AddInt32("value",HS_ERASE_TO_BACKGROUND_MODE);

	// Create the first radio-button.
	font_height fHeight;
	container->GetFontHeight(&fHeight);
	mode_button_1 = new BRadioButton(BRect(EXTRA_EDGE,EXTRA_EDGE+fHeight.descent+fHeight.ascent/2,EXTRA_EDGE,EXTRA_EDGE+fHeight.descent+fHeight.ascent/2),"a radio button",StringServer::ReturnString(BACKGROUND_STRING),new BMessage(*message));
	mode_button_1->ResizeToPreferred();
	container->AddChild(mode_button_1);
	if (tool->GetCurrentValue(MODE_OPTION) == HS_ERASE_TO_BACKGROUND_MODE)
		mode_button_1->SetValue(B_CONTROL_ON);

	// Create the second radio-button.
	message->ReplaceInt32("value",HS_ERASE_TO_TRANSPARENT_MODE);
	mode_button_2 = new BRadioButton(BRect(EXTRA_EDGE,mode_button_1->Frame().bottom,EXTRA_EDGE,mode_button_1->Frame().bottom)," a radio button",StringServer::ReturnString(TRANSPARENT_STRING),message);
	mode_button_2->ResizeToPreferred();
	container->AddChild(mode_button_2);
	if (tool->GetCurrentValue(MODE_OPTION) == HS_ERASE_TO_TRANSPARENT_MODE)
		mode_button_2->SetValue(B_CONTROL_ON);

	container->ResizeBy(0,mode_button_2->Frame().bottom+EXTRA_EDGE);

	ResizeTo(controller_frame.Width()+2*EXTRA_EDGE,container->Frame().bottom + EXTRA_EDGE);
}



void EraserToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();
	size_slider->SetTarget(new BMessenger(this));
	mode_button_1->SetTarget(BMessenger(this));
	mode_button_2->SetTarget(BMessenger(this));
}
