/* 

	Filename:	AddOnTemplate.cpp
	Contents:	A template for the ArtPaint add-ons.	
	Author:		Heikki Suhonen
	
*/

#include "AddOnTypes.h"
#include "Impressionist.h"
#include "DirectionControl.h"
#include "BitmapDrawer.h"

extern "C" ViewManipulator* start_manipulator(BView*,manipulator_data&);

#pragma export on
bool creates_gui = TRUE;		// If this is true, we should inherit from GUIManipilator.
char name[255] = "Impressionist…";
int32 add_on_api_version = ADD_ON_API_VERSION;
add_on_types add_on_type = EFFECT_FILTER_ADD_ON;


ViewManipulator* start_manipulator(BView *target_view,manipulator_data &data)
{
	// Here create a view-manipulator. The class should inherit
	// from the ViewManipulator base-class. It will be deleted
	// in the application program.
	return new ImpressionistManipulator(target_view,data);	
}

#pragma export off



ImpressionistManipulator::ImpressionistManipulator(BView *target,manipulator_data &data)
		: GUIManipulator(target)
{
	the_data = data;	// Especially important data is the image_updater-function. 
	node = data.node;
	
	if (node->ReadAttr("stroke_direction",B_FLOAT_TYPE,0,&stroke_direction,sizeof(float)) != sizeof(float))
		stroke_direction = 0;
	if (node->ReadAttr("stroke_length",B_FLOAT_TYPE,0,&stroke_length,sizeof(float)) != sizeof(float))
		stroke_length = MIN_STROKE_LENGTH;
	if (node->ReadAttr("stroke_width",B_FLOAT_TYPE,0,&stroke_width,sizeof(float)) != sizeof(float))
		stroke_width = MIN_STROKE_WIDTH;
	
		
	preview_location = BPoint(0,0);
	target_bitmap = data.current_layer;

	status_view = NULL;
	
	// Make a copy of target_bitmap.
	original_bitmap = CopyBitmap(target_bitmap);
	impressionist_window = new ImpressionistWindow(target,this,stroke_direction,stroke_length,stroke_width);	
	impressionist_window->Show();
	((ImpressionistWindow*)impressionist_window)->UpdatePreview();
}


ImpressionistManipulator::~ImpressionistManipulator()
{
	if (status == TRUE) {
		node->WriteAttr("stroke_direction",B_FLOAT_TYPE,0,&stroke_direction,sizeof(float));
		node->WriteAttr("stroke_width",B_FLOAT_TYPE,0,&stroke_width,sizeof(float));
		node->WriteAttr("stroke_length",B_FLOAT_TYPE,0,&stroke_length,sizeof(float));			
	}
	
	delete node;
}


BBitmap* ImpressionistManipulator::ManipulateBitmap(BBitmap *original, BView *progress_view, float prog_step)
{
	// We may create another bitmap and return it instead of original, but we may
	// also do the manipulation on the original and return it. We Should send messages
	// to progress_view that contain B_UPDATE_STATUS_BAR as their 'what'. The sum of
	// message deltas should equal 100*prog_step.
	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta",0.0);

	BStopWatch *watch = new BStopWatch("Impressionist");
	if (target_bitmap == original) {
		status_view = progress_view;
		progress_step = prog_step;
		CalculateEffect(this);
	}
	else {
		delete original_bitmap;
		original_bitmap = CopyBitmap(original);
		status_view = progress_view;
		progress_step = prog_step;
		CalculateEffect(this);		
	}
	delete watch;		
//	progress_message.ReplaceFloat("delta",(100.0*prog_step)/(float)height*10.0);
//	progress_view->Window()->PostMessage(&progress_message,progress_view);
	return original;
}


void ImpressionistManipulator::FinishManipulation(bool s)
{
	uint32 *orig_bits = (uint32*)original_bitmap->Bits();
	uint32 *target_bits = (uint32*)target_bitmap->Bits();
	int32 orig_bpr = original_bitmap->BytesPerRow()/4;

	status = s;
		
	BRect bounds = target_bitmap->Bounds();
	int32 width = bounds.Width();
	int32 height = bounds.Height();

	for (int32 y=0;y<=height;y++) {
		for (int32 x=0;x<=width;x++) {	
			*target_bits++ = *orig_bits++;
		}
	}
	
	// Also take note that the Impressionist-window is now gone
	impressionist_window = NULL;
}

void ImpressionistManipulator::MouseDown(BPoint point,uint32 buttons,uint32,GET_MOUSE mouse_function)
{
	BWindow *target_window = target_view->Window();
	
	// Make sure that the view is updated even with just a single click.	
	BPoint prev_point = point - BPoint(1,1);	
	BPoint view_point;
	BRect bitmap_bounds = original_bitmap->Bounds();
	
	if (target_window != NULL) {	
		while (buttons) {
			target_window->Lock();
			mouse_function(target_view,&point,&buttons,&view_point);			
			target_window->Unlock();
			if ((point != prev_point) && (bitmap_bounds.Contains(point) == TRUE)) {
				preview_location = point;
				((ImpressionistWindow*)impressionist_window)->UpdatePreview();			
				prev_point = point;
			}
			snooze(20.0 * 1000.0);
		}
	}
	
	target_window->PostMessage(HS_ACTION_FINISHED,target_view);
}

void ImpressionistManipulator::ChangeValue(int32 length)
{
	stroke_length = length;
}

void ImpressionistManipulator::ChangeWidth(int32 width)
{
	stroke_width = width;
}

void ImpressionistManipulator::ChangeDirection(int32 dir)
{
	stroke_direction = dir;
}

void ImpressionistManipulator::CalculatePreview(BBitmap *bitmap)
{
	BRect preview_area = bitmap->Bounds();
	BRect orig_bounds = original_bitmap->Bounds();
	preview_area.OffsetTo(preview_location.x-preview_area.Width()/2,preview_location.y-preview_area.Height()/2);
	BitmapDrawer *preview_drawer = new BitmapDrawer(bitmap);
	BitmapDrawer *original_drawer = new BitmapDrawer(original_bitmap);
	float width = preview_area.Width();
	float height = preview_area.Height();
	
	preview_area.left = max_c(preview_area.left,orig_bounds.left);
	preview_area.right = preview_area.left + width;
	preview_area.top = max_c(preview_area.top,orig_bounds.top);
	preview_area.bottom = preview_area.top + height;

	preview_area = preview_area & orig_bounds;
	int32 left = preview_area.left;
	int32 right = preview_area.right;
	int32 start_y = preview_area.top;
	int32 end_y = preview_area.bottom;
			
	// Here make a horizontal line and then rotate it to right direction.
//	HSPolygon *line;
//	BPoint line_ends[2];
//	line_ends[0] = BPoint(0,stroke_length/2);
//	line_ends[1] = BPoint(0,-stroke_length/2);
//	line = new HSPolygon(line_ends,2);
//	line->Rotate(BPoint(0,0),stroke_direction);
//	BPoint *ends = line->GetPointList();
//	BPoint start = ends[0];
//	BPoint end = ends[1];
//	start.PrintToStream();
//	end.PrintToStream();
//	delete line;
	int32 x_offset = preview_area.left;
	int32 y_offset = preview_area.top;
//	printf("x-offset: %d, y-offset: %d\n",x_offset,y_offset);	
////	bitmap->Lock();
////	BView *a_view = bitmap->ChildAt(0);	
//	uint32 original_color;
//	for (int32 y=preview_area.top;y<=preview_area.bottom;y+=2) {
//		for (int32 x=preview_area.left;x<=preview_area.right;x+=2) {
//			original_color = original_drawer->GetPixel(BPoint(x,y));		
////			a_view->SetHighColor(BGRAColorToRGB(original_color));
////			a_view->StrokeLine(BPoint(x-x_offset,y-y_offset)+start,BPoint(x-x_offset,y-y_offset)+end);
//			preview_drawer->DrawLine(BPoint(x-x_offset,y-y_offset)+start,BPoint(x-x_offset,y-y_offset)+end,original_color,stroke_width);
////			printf("Point (%d,%d), color %0x\n",x,y,original_color);
//		}		
//	}
	float length_of_stroke = stroke_length;
	float width_of_stroke = stroke_width;
	float direction_of_stroke = stroke_direction;
	uint32 original_color;
	float dx,dy;
	for (int32 y=start_y;y<=end_y;y+=2) {
		for (int32 x=left;x<=right;x+=2) {
			float random_difference = rand() % 15;
			if (rand() % 2 == 0)
				random_difference = -random_difference;
			dy = (random_difference+length_of_stroke)*cos((direction_of_stroke+random_difference)/180*PI);
			dx = (random_difference+length_of_stroke)*sin((direction_of_stroke+random_difference)/180*PI);			
			original_color = original_drawer->GetPixel(x,y);
			preview_drawer->DrawLine(BPoint(x+dx-x_offset,y-dy-y_offset),BPoint(x-dx-x_offset,y+dy-y_offset),original_color,width_of_stroke,FALSE);
		}		
	}
	

	delete original_drawer;
	delete preview_drawer;
}


int32 ImpressionistManipulator::CalculateEffect(void *data)
{
	((ImpressionistManipulator*)data)->calculate_effect();

	return B_OK;
}


void ImpressionistManipulator::calculate_effect()
{
	// Here start a thread for each processor or just one if height is less than processor count.
	system_info s_info;
	get_system_info(&s_info);
	
	int32 row_count = target_bitmap->Bounds().Height() + 1;
	uint32 *target_bits = (uint32*)target_bitmap->Bits();
	uint32 *source_bits = (uint32*)original_bitmap->Bits();
	int32 target_bpr = target_bitmap->BytesPerRow()/4;
	int32 source_bpr = original_bitmap->BytesPerRow()/4;
	
	thread_data *t_data;
	// We start only one thread that calculates the strokes. Another thread will be
	// started to render the strokes. This is to avoid memory bus contention.
	// The calculating thread should not even look at the bitmap, it should only
	// calculate the endpoints and widths for strokes. The threads should use
	// a bounded buffer for communication. The bounded buffer should be read and written
	// in small packets that contain data for more than just one calculation to reduce the 
	// contention for buffer access.
	// Remember to add an option to the rendering thread, where strokes for the
	// background-color would not be made.
	data_queue = new DataQueue<data_item>(1000);
	
	calculating = TRUE;
	
	t_data = new thread_data();		// The thread-function should not delete this data
		
	t_data->target = target_bitmap;
	t_data->source = original_bitmap;
	
	t_data->start_y = 0;
	t_data->end_y = row_count-1;
	t_data->left = target_bitmap->Bounds().left;
	t_data->right = target_bitmap->Bounds().right;
	t_data->the_manipulator = this;
		
	t_data->stroke_direction = stroke_direction;
	t_data->stroke_width = stroke_width;
	t_data->stroke_length = stroke_length;
		
	thread_id calculating_thread = spawn_thread(ImpressionistManipulator::CalculateEntry,"calculating thread",B_NORMAL_PRIORITY,t_data);
	resume_thread(calculating_thread);
	thread_id rendering_thread = spawn_thread(ImpressionistManipulator::RenderEntry,"rendering thread",B_NORMAL_PRIORITY,t_data);
	resume_thread(rendering_thread);
	
	int32 return_value;
	wait_for_thread(calculating_thread,&return_value);
	wait_for_thread(rendering_thread,&return_value);

	delete t_data;
	delete data_queue;
		
	status_view = NULL;
	// Then update the view.
	target_view->Window()->Lock();
	the_data.image_updater(target_view,BRect(0,0,-1,-1),FALSE);
	target_view->Window()->Unlock();
}

int32 ImpressionistManipulator::CalculateEntry(void *data)
{
	thread_data *t_data = (thread_data*)data;
	ImpressionistManipulator *the_manipulator = cast_as(t_data->the_manipulator,ImpressionistManipulator);
	
	if (the_manipulator == NULL)
		return B_ERROR;
		
	the_manipulator->calculate_func(t_data->left,t_data->right,t_data->start_y,t_data->end_y,t_data->stroke_direction,t_data->stroke_length,t_data->stroke_width);
	

	return B_OK;
}


int32 ImpressionistManipulator::RenderEntry(void *data)
{
	thread_data *t_data = (thread_data*)data;
	ImpressionistManipulator *the_manipulator = cast_as(t_data->the_manipulator,ImpressionistManipulator);
	
	if (the_manipulator == NULL)
		return B_ERROR;
		
	the_manipulator->render_func(t_data->source,t_data->target);	

	return B_OK;
}

int32 ImpressionistManipulator::calculate_func(int32 left,int32 right,int32 start_y,int32 end_y,float direction_of_stroke,float length_of_stroke,float width_of_stroke)
{
	float dx,dy;	// It is very important to have random variance in the direction and length.
					// Also some variance in color is good.
	const int32 block_size = 100;
	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta",0.0);
	data_item item[block_size];	// Lets fill the buffer in block_size item blocks
	int32 item_number=0;
	
	if (TRUE) {
		for (int32 y=start_y;y<=end_y;y+=2) {
			for (int32 x=left;x<=right;x+=2) {
				float random_difference = rand() % 15;
				if (rand() % 2 == 0)
					random_difference = -random_difference;
				dy = (random_difference+length_of_stroke)*cos((direction_of_stroke+random_difference)/180*PI);
				dx = (random_difference+length_of_stroke)*sin((direction_of_stroke+random_difference)/180*PI);			
				item[item_number].x1 = x+dx;
				item[item_number].x2 = x-dx;
				item[item_number].y1 = y - dy;
				item[item_number].y2 = y+dy;
				item[item_number].cx = x;
				item[item_number].cy=y;
				item[item_number].width = width_of_stroke;
				item_number++;
				if (item_number == block_size) {			
					data_queue->PutItems(item,item_number);
					item_number = 0;
				}
			}		
			if (y%10 == 0) {
				progress_message.ReplaceFloat("delta",(100.0*progress_step)/(float)(end_y-start_y)*10.0);
				status_view->Window()->PostMessage(&progress_message,status_view);			
			}
		}
		data_queue->PutItems(item,item_number);
		item_number = 0;
	}
	
	calculating = FALSE;
	return B_OK;
}


int32 ImpressionistManipulator::render_func(BBitmap *source, BBitmap *target)
{
	BitmapDrawer *source_drawer = new BitmapDrawer(source);
	BitmapDrawer *target_drawer = new BitmapDrawer(target);

	uint32 original_color;
	int32 items_got = 0;
	data_item item[20]; 	// Lets read the items in blocks of 20.
	int32 items_used = 0;
	items_got = data_queue->GetItems(item,20);
	int32 failed = 0;
	while ((calculating) || (items_got>0)) {
		if (items_got > 0) {
			original_color = source_drawer->GetPixel(item[items_used].cx,item[items_used].cy);
			target_drawer->DrawLine(BPoint(item[items_used].x1,item[items_used].y1),BPoint(item[items_used].x2,item[items_used].y2),original_color,item[items_used].width,FALSE);		
			items_used++;
		}
		else {
		//	snooze(1 * 1000);	snoozing here makes the thread execute very lazily
			failed++;
		}
		if ((items_used == (items_got-1)) || (items_got == 0) ) {
			items_got = data_queue->GetItems(item,20);	
			items_used = 0;
		}
	}	
	printf("Rendering thread failed to get dat %d times\n",failed);
	return B_OK;
}

ImpressionistWindow::ImpressionistWindow(BView *target,ImpressionistManipulator *manipulator,float dir,float len,float wid)
	: BWindow(BRect(300,300,400,400),"Impressionist",B_TITLED_WINDOW,B_NOT_RESIZABLE)
{
	target_view = target;
	the_manipulator = manipulator;
	BBox *container = new BBox(BRect(0,0,150,100));
	background = container;
	AddChild(container);
	ResizeTo(container->Bounds().Width(),container->Bounds().Height());

	// The message to slider must not be NULL, and it must contain an int32 named "value".
	// Also, the message is not copied to the control.
	BMessage *message = new BMessage(STROKE_LENGTH_CHANGED);
	message->AddInt32("value",0);	
	ControlSliderBox *slider1 = new ControlSliderBox(BRect(4,4,146,44),"stroke length","Stroke Length","",message,MIN_STROKE_LENGTH,MAX_STROKE_LENGTH,B_PLAIN_BORDER,FALSE);
	container->AddChild(slider1); 
	slider1->setValue(len);
	BRect slider_frame = slider1->Frame();
	slider_frame.OffsetBy(0,slider_frame.Height()+4);

	message = new BMessage(STROKE_WIDTH_CHANGED);
	message->AddInt32("value",2);	
	slider1 = new ControlSliderBox(slider_frame,"stroke width","Stroke Width","",message,MIN_STROKE_WIDTH,MAX_STROKE_WIDTH,B_PLAIN_BORDER,FALSE);
	container->AddChild(slider1); 
	slider1->setValue(wid);
	slider_frame = slider1->Frame();
	slider_frame.OffsetBy(0,slider_frame.Height()+4);	
//	BCheckBox *a_checkbox = new BCheckBox(slider_frame,"motion blur alpha","Blur Transparecy",new BMessage(BLUR_TRANSPARENCY_CHANGED));
//	a_checkbox->ResizeToPreferred();
//	container->AddChild(a_checkbox);
//	slider_frame = slider1->Frame();
//	slider_frame.OffsetBy(0,a_checkbox->Frame().bottom - slider_frame.top+EXTRA_EDGE);
	

	DirectionControlBox *d_control = new DirectionControlBox(slider_frame,"stroke direction","Stroke Direction",new BMessage(STROKE_DIRECTION_CHANGED));
	d_control->setValue(dir);
	container->AddChild(d_control);
	slider_frame = d_control->Frame();
	slider_frame.OffsetBy(0,slider_frame.Height()+4);
	//39, 39
	preview_bitmap = new BBitmap(BRect(0,0,63,63),B_RGB_32_BIT);
	preview_view = new BitmapViewBox(preview_bitmap,slider_frame,"Preview");
	container->AddChild(preview_view);
	slider_frame = preview_view->Frame();
	slider_frame.OffsetBy(0,slider_frame.Height()+4);

	message = new BMessage(HS_OPERATION_FINISHED);
	message->AddBool("status",FALSE);
	cancel_button = new BButton(slider_frame,"impressionist cancel button","Cancel",message);
	container->AddChild(cancel_button);
	cancel_button->ResizeToPreferred();
	
	slider_frame = cancel_button->Frame();
	slider_frame.OffsetBy(slider_frame.Width()+4,0);

	message = new BMessage(HS_OPERATION_FINISHED);
	message->AddBool("status",TRUE);
	ok_button = new BButton(slider_frame,"impressionist accept button","OK",message);
	container->AddChild(ok_button);
	ok_button->ResizeToPreferred();

	container->ResizeTo(container->Frame().Width(),slider_frame.bottom+4);
	ResizeTo(container->Frame().Width(),container->Frame().Height());
	status = FALSE;	
}


ImpressionistWindow::~ImpressionistWindow()
{
	BMessage *a_message = new BMessage(HS_OPERATION_FINISHED);
	a_message->AddBool("status",status);
	target_view->Window()->PostMessage(a_message,target_view);
	delete a_message;
}



void ImpressionistWindow::MessageReceived(BMessage *message)
{
	BLooper *source;
	message->FindPointer("source",&source);
	
	switch (message->what) {
		case STROKE_LENGTH_CHANGED:
			int32 blur_amount = message->FindInt32("value");
			the_manipulator->ChangeValue(blur_amount);
			the_manipulator->CalculatePreview(preview_bitmap);
			preview_view->UpdateBitmap();
			break;
		case STROKE_DIRECTION_CHANGED:
			BControl *control;
			control = cast_as(source,BControl);
			if (control != NULL) {
				the_manipulator->ChangeDirection(control->Value());
				the_manipulator->CalculatePreview(preview_bitmap);
				preview_view->UpdateBitmap();
			}
			break;
		case STROKE_WIDTH_CHANGED:
			int32 width= message->FindInt32("value");
			the_manipulator->ChangeWidth(width);
			the_manipulator->CalculatePreview(preview_bitmap);
			preview_view->UpdateBitmap();
			break;
		case HS_OPERATION_FINISHED:
			message->FindBool("status",&status);
			Quit();
			break;
		default:
			BWindow::MessageReceived(message);
			break;			
	}	
}


void ImpressionistWindow::UpdatePreview()
{
	the_manipulator->CalculatePreview(preview_bitmap);
	Lock();
	preview_view->UpdateBitmap();
	Unlock();	
}