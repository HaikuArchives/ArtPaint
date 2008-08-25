/* 

	Filename:	AddOnTemplate.cpp
	Contents:	A template for the ArtPaint add-ons.	
	Author:		Heikki Suhonen
	
*/

#include "AddOnTypes.h"
#include "Impressionist.h"
#include "DirectionControl.h"
#include "BitmapDrawer.h"
#include <new.h>

extern "C" Manipulator* manipulator_creator(BBitmap*);

#pragma export on
char name[255] = "Impressionist…";
int32 add_on_api_version = ADD_ON_API_VERSION;
add_on_types add_on_type = EFFECT_FILTER_ADD_ON;


Manipulator* manipulator_creator(BBitmap *bm)
{
	return new ImpressionistManipulator(bm);	
}

#pragma export off



ImpressionistManipulator::ImpressionistManipulator(BBitmap *bm)
	: WindowGUIManipulator(), random_array_length(100)
{
	preview_bitmap = NULL;
	try {
		random_array = new int32[random_array_length];
	}
	catch (bad_alloc e) {
		throw e;
	}
	
	for (int32 i=0;i<random_array_length;i++) {
		random_array[i] = rand();
	}	

	small_preview_bitmap = new BBitmap(BRect(0,0,PREVIEW_WIDTH-1,PREVIEW_HEIGHT-1),B_RGB32);
	if (small_preview_bitmap->IsValid() == FALSE)
		throw bad_alloc();

	SetPreviewBitmap(bm);
}


ImpressionistManipulator::~ImpressionistManipulator()
{
	delete small_preview_bitmap;
}


BBitmap* ImpressionistManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
//	// We may create another bitmap and return it instead of original, but we may
//	// also do the manipulation on the original and return it. We Should send messages
//	// to progress_view that contain B_UPDATE_STATUS_BAR as their 'what'. The sum of
//	// message deltas should equal 100*prog_step.
//	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
//	progress_message.AddFloat("delta",0.0);
//
//	BStopWatch *watch = new BStopWatch("Impressionist");
//	if (target_bitmap == original) {
//		status_view = progress_view;
//		progress_step = prog_step;
//		CalculateEffect(this);
//	}
//	else {
//		delete original_bitmap;
//		original_bitmap = CopyBitmap(original);
//		status_view = progress_view;
//		progress_step = prog_step;
//		CalculateEffect(this);		
//	}
//	delete watch;		
////	progress_message.ReplaceFloat("delta",(100.0*prog_step)/(float)height*10.0);
////	progress_view->Window()->PostMessage(&progress_message,progress_view);
	return original;
}


void ImpressionistManipulator::MouseDown(BPoint point,uint32 buttons,BView*,bool)
{
//	BWindow *target_window = target_view->Window();
//	
//	// Make sure that the view is updated even with just a single click.	
//	BPoint prev_point = point - BPoint(1,1);	
//	BPoint view_point;
//	BRect bitmap_bounds = original_bitmap->Bounds();
//	
//	if (target_window != NULL) {	
//		while (buttons) {
//			target_window->Lock();
//			mouse_function(target_view,&point,&buttons,&view_point);			
//			target_window->Unlock();
//			if ((point != prev_point) && (bitmap_bounds.Contains(point) == TRUE)) {
//				preview_location = point;
//				((ImpressionistWindow*)impressionist_window)->UpdatePreview();			
//				prev_point = point;
//			}
//			snooze(20.0 * 1000.0);
//		}
//	}
//	
//	target_window->PostMessage(HS_ACTION_FINISHED,target_view);
}

void ImpressionistManipulator::SetPreviewBitmap(BBitmap *bm)
{
	preview_bitmap = bm;
	// Also recalculate the preview and inform the config-view about it
}

BView* ImpressionistManipulator::MakeConfigurationView(BMessenger*)
{
	config_view = new ImpressionistManipulatorView(BRect(0,0,0,0),this);
	config_view->ChangeSettings(&settings);
	
	return config_view;
}


void ImpressionistManipulator::ChangeSettings(ManipulatorSettings *s)
{
	ImpressionistManipulatorSettings *new_settings = cast_as(s,ImpressionistManipulatorSettings);
	
	if (new_settings != NULL) {
		previous_settings = settings;
		settings = *new_settings;
	}
}


ManipulatorSettings* ImpressionistManipulator::ReturnSettings()
{
	return new ImpressionistManipulatorSettings(&settings);
}


// ----------------

ImpressionistManipulatorView::ImpressionistManipulatorView(BRect rect,ImpressionistManipulator *manip)
	: WindowGUIManipulatorView(rect)
{
}



void ImpressionistManipulatorView::AttachedToWindow()
{

}


void ImpressionistManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}



void ImpressionistManipulatorView::ChangeSettings(ImpressionistManipulatorSettings *s)
{

}

//void ImpressionistManipulator::CalculatePreview(BBitmap *bitmap)
//{
//	BRect preview_area = bitmap->Bounds();
//	BRect orig_bounds = original_bitmap->Bounds();
//	preview_area.OffsetTo(preview_location.x-preview_area.Width()/2,preview_location.y-preview_area.Height()/2);
//	BitmapDrawer *preview_drawer = new BitmapDrawer(bitmap);
//	BitmapDrawer *original_drawer = new BitmapDrawer(original_bitmap);
//	float width = preview_area.Width();
//	float height = preview_area.Height();
//	
//	preview_area.left = max_c(preview_area.left,orig_bounds.left);
//	preview_area.right = preview_area.left + width;
//	preview_area.top = max_c(preview_area.top,orig_bounds.top);
//	preview_area.bottom = preview_area.top + height;
//
//	preview_area = preview_area & orig_bounds;
//	int32 left = preview_area.left;
//	int32 right = preview_area.right;
//	int32 start_y = preview_area.top;
//	int32 end_y = preview_area.bottom;
//			
//	// Here make a horizontal line and then rotate it to right direction.
////	HSPolygon *line;
////	BPoint line_ends[2];
////	line_ends[0] = BPoint(0,stroke_length/2);
////	line_ends[1] = BPoint(0,-stroke_length/2);
////	line = new HSPolygon(line_ends,2);
////	line->Rotate(BPoint(0,0),stroke_direction);
////	BPoint *ends = line->GetPointList();
////	BPoint start = ends[0];
////	BPoint end = ends[1];
////	start.PrintToStream();
////	end.PrintToStream();
////	delete line;
//	int32 x_offset = preview_area.left;
//	int32 y_offset = preview_area.top;
////	printf("x-offset: %d, y-offset: %d\n",x_offset,y_offset);	
//////	bitmap->Lock();
//////	BView *a_view = bitmap->ChildAt(0);	
////	uint32 original_color;
////	for (int32 y=preview_area.top;y<=preview_area.bottom;y+=2) {
////		for (int32 x=preview_area.left;x<=preview_area.right;x+=2) {
////			original_color = original_drawer->GetPixel(BPoint(x,y));		
//////			a_view->SetHighColor(BGRAColorToRGB(original_color));
//////			a_view->StrokeLine(BPoint(x-x_offset,y-y_offset)+start,BPoint(x-x_offset,y-y_offset)+end);
////			preview_drawer->DrawLine(BPoint(x-x_offset,y-y_offset)+start,BPoint(x-x_offset,y-y_offset)+end,original_color,stroke_width);
//////			printf("Point (%d,%d), color %0x\n",x,y,original_color);
////		}		
////	}
//	float length_of_stroke = stroke_length;
//	float width_of_stroke = stroke_width;
//	float direction_of_stroke = stroke_direction;
//	uint32 original_color;
//	float dx,dy;
//	for (int32 y=start_y;y<=end_y;y+=2) {
//		for (int32 x=left;x<=right;x+=2) {
//			float random_difference = rand() % 15;
//			if (rand() % 2 == 0)
//				random_difference = -random_difference;
//			dy = (random_difference+length_of_stroke)*cos((direction_of_stroke+random_difference)/180*PI);
//			dx = (random_difference+length_of_stroke)*sin((direction_of_stroke+random_difference)/180*PI);			
//			original_color = original_drawer->GetPixel(x,y);
//			preview_drawer->DrawLine(BPoint(x+dx-x_offset,y-dy-y_offset),BPoint(x-dx-x_offset,y+dy-y_offset),original_color,width_of_stroke,FALSE);
//		}		
//	}
//	
//
//	delete original_drawer;
//	delete preview_drawer;
//}


//int32 ImpressionistManipulator::CalculateEffect(void *data)
//{
//	((ImpressionistManipulator*)data)->calculate_effect();
//
//	return B_OK;
//}
//
//
//void ImpressionistManipulator::calculate_effect()
//{
//	// Here start a thread for each processor or just one if height is less than processor count.
//	system_info s_info;
//	get_system_info(&s_info);
//	
//	int32 row_count = target_bitmap->Bounds().Height() + 1;
//	uint32 *target_bits = (uint32*)target_bitmap->Bits();
//	uint32 *source_bits = (uint32*)original_bitmap->Bits();
//	int32 target_bpr = target_bitmap->BytesPerRow()/4;
//	int32 source_bpr = original_bitmap->BytesPerRow()/4;
//	
//	thread_data *t_data;
//	// We start only one thread that calculates the strokes. Another thread will be
//	// started to render the strokes. This is to avoid memory bus contention.
//	// The calculating thread should not even look at the bitmap, it should only
//	// calculate the endpoints and widths for strokes. The threads should use
//	// a bounded buffer for communication. The bounded buffer should be read and written
//	// in small packets that contain data for more than just one calculation to reduce the 
//	// contention for buffer access.
//	// Remember to add an option to the rendering thread, where strokes for the
//	// background-color would not be made.
//	data_queue = new DataQueue<data_item>(1000);
//	
//	calculating = TRUE;
//	
//	t_data = new thread_data();		// The thread-function should not delete this data
//		
//	t_data->target = target_bitmap;
//	t_data->source = original_bitmap;
//	
//	t_data->start_y = 0;
//	t_data->end_y = row_count-1;
//	t_data->left = target_bitmap->Bounds().left;
//	t_data->right = target_bitmap->Bounds().right;
//	t_data->the_manipulator = this;
//		
//	t_data->stroke_direction = stroke_direction;
//	t_data->stroke_width = stroke_width;
//	t_data->stroke_length = stroke_length;
//		
//	thread_id calculating_thread = spawn_thread(ImpressionistManipulator::CalculateEntry,"calculating thread",B_NORMAL_PRIORITY,t_data);
//	resume_thread(calculating_thread);
//	thread_id rendering_thread = spawn_thread(ImpressionistManipulator::RenderEntry,"rendering thread",B_NORMAL_PRIORITY,t_data);
//	resume_thread(rendering_thread);
//	
//	int32 return_value;
//	wait_for_thread(calculating_thread,&return_value);
//	wait_for_thread(rendering_thread,&return_value);
//
//	delete t_data;
//	delete data_queue;
//		
//	status_view = NULL;
//	// Then update the view.
//	target_view->Window()->Lock();
//	the_data.image_updater(target_view,BRect(0,0,-1,-1),FALSE);
//	target_view->Window()->Unlock();
//}
//
//int32 ImpressionistManipulator::CalculateEntry(void *data)
//{
//	thread_data *t_data = (thread_data*)data;
//	ImpressionistManipulator *the_manipulator = cast_as(t_data->the_manipulator,ImpressionistManipulator);
//	
//	if (the_manipulator == NULL)
//		return B_ERROR;
//		
//	the_manipulator->calculate_func(t_data->left,t_data->right,t_data->start_y,t_data->end_y,t_data->stroke_direction,t_data->stroke_length,t_data->stroke_width);
//	
//
//	return B_OK;
//}
//
//
//int32 ImpressionistManipulator::RenderEntry(void *data)
//{
//	thread_data *t_data = (thread_data*)data;
//	ImpressionistManipulator *the_manipulator = cast_as(t_data->the_manipulator,ImpressionistManipulator);
//	
//	if (the_manipulator == NULL)
//		return B_ERROR;
//		
//	the_manipulator->render_func(t_data->source,t_data->target);	
//
//	return B_OK;
//}
//
//int32 ImpressionistManipulator::calculate_func(int32 left,int32 right,int32 start_y,int32 end_y,float direction_of_stroke,float length_of_stroke,float width_of_stroke)
//{
//	float dx,dy;	// It is very important to have random variance in the direction and length.
//					// Also some variance in color is good.
//	const int32 block_size = 100;
//	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
//	progress_message.AddFloat("delta",0.0);
//	data_item item[block_size];	// Lets fill the buffer in block_size item blocks
//	int32 item_number=0;
//
//	// The longer and wider the stroke is, the less strokes we need to
//	// use.
//	int32 sparse = min_c(length_of_stroke,width_of_stroke);	
//	// We select x and y coordinates in random order
//	int32 number_of_pixels = (end_y-start_y+1)*(right-left+1)/pow(sparse,2);
//	int32 *offsets = new int32[number_of_pixels];
//	for (int32 i=0;i<number_of_pixels;i++)
//		offsets[i] = i;
//
//	int32 x,y;
//	int32 offset;
//	int32 last_offset = number_of_pixels;	
//	float ls = length_of_stroke/2;
//	if (use_constant_direction == TRUE) {
//		for (int32 i=0;i<number_of_pixels;i++) {
//			offset = rand() % last_offset;
//			x = left + (offsets[offset] % ((right-left)/sparse))*sparse;
//			y = start_y + (offsets[offset] % ((end_y - start_y)/sparse))*sparse;
//			offsets[offset] = offsets[last_offset];
//			last_offset--;
//			
//			float random_difference = (rand() % 17) / 8.0;
//			if (rand() % 2 == 0)
//				random_difference = -random_difference;
//			dy = (random_difference+ls)*cos((direction_of_stroke+random_difference)/180*PI);
//			dx = (random_difference+ls)*sin((direction_of_stroke+random_difference)/180*PI);			
//			item[item_number].x1 = x+dx;
//			item[item_number].x2 = x-dx;
//			item[item_number].y1 = y - dy;
//			item[item_number].y2 = y+dy;
//			item[item_number].cx = x;
//			item[item_number].cy=y;
//			item[item_number].width = width_of_stroke;
//			item_number++;
//			if (item_number == block_size) {			
//				data_queue->PutItems(item,item_number);
//				item_number = 0;
//			}
//			if (i%1000 == 0) {
//				progress_message.ReplaceFloat("delta",(100.0*progress_step)/(float)number_of_pixels*1000);
//				status_view->Window()->PostMessage(&progress_message,status_view);			
//			}
//		}
//		data_queue->PutItems(item,item_number);
//		item_number = 0;
//	}
//	else {
//		BBitmap *sobel;
//		// First calculate the intensity-image
//		sobel = CalculateIntensityMap(original_bitmap);
//			
//		// Then blur the intensity-image
//		sobel = CalculateBlur(sobel);	
//
//		// Make a sobel-operation on the blurred intensity-image
//		// to find the edges.
////		sobel = CalculateSobel(sobel);
//				
//		// Then use the sobel-image to clip strokes at edges. An edge is found
//		// on a path whenever magnitude decreases.
//		target_bitmap->Lock();
//		target_bitmap->ChildAt(0)->DrawBitmap(sobel);
//		target_bitmap->ChildAt(0)->Sync();
//		target_bitmap->Unlock();
//	}	
//	calculating = FALSE;
//	return B_OK;
//}
//
//
//int32 ImpressionistManipulator::render_func(BBitmap *source, BBitmap *target)
//{
//	BitmapDrawer *source_drawer = new BitmapDrawer(source);
//	BitmapDrawer *target_drawer = new BitmapDrawer(target);
//
//	uint32 original_color;
//	int32 items_got = 0;
//	data_item item[20]; 	// Lets read the items in blocks of 20.
//	int32 items_used = 0;
//	items_got = data_queue->GetItems(item,20);
//	int32 failed = 0;
//	while ((calculating) || (items_got>0)) {
//		if (items_got > 0) {
//			original_color = source_drawer->GetPixel(item[items_used].cx,item[items_used].cy);
//			// Here we should add a little randomness to the color.
//			target_drawer->DrawLine(BPoint(item[items_used].x1,item[items_used].y1),BPoint(item[items_used].x2,item[items_used].y2),original_color,item[items_used].width,FALSE);		
//			items_used++;
//		}
//		else {
//		//	snooze(1 * 1000);	snoozing here makes the thread execute very lazily
//			failed++;
//		}
//		if ((items_used == (items_got-1)) || (items_got == 0) ) {
//			items_got = data_queue->GetItems(item,20);	
//			items_used = 0;
//		}
//	}	
//	printf("Rendering thread failed to get data %d times\n",failed);
//	return B_OK;
//}
//
//
//BBitmap* ImpressionistManipulator::CalculateIntensityMap(BBitmap *orig)
//{
//	BBitmap *intens = new BBitmap(orig->Bounds(),B_COLOR_8_BIT);
//	uchar *intens_bits = (uchar*)intens->Bits();
//	uint32 *orig_bits = (uint32*)orig->Bits();
//	uint32 intens_bpr = intens->BytesPerRow();
//	uint32 width = orig->Bounds().Width();
//	uint32 height = orig->Bounds().Height();
//	
//	for (int32 y=0;y<=height;y++) {
//		for (int32 x=0;x<=width;x++) {
//			*intens_bits++ = (((*orig_bits) >> 24) & 0xFF) * 0.114 +
//							 (((*orig_bits) >> 16) & 0xFF) * 0.587 +
//							 (((*orig_bits) >> 8) & 0xFF) * 0.299;
//			orig_bits++;
//		}
//		intens_bits += (intens_bpr - width - 1); // pad to full 32-bits
//	}
//
//	return intens;
//}
//
//
//BBitmap* ImpressionistManipulator::CalculateBlur(BBitmap *orig)
//{
//	const blur_size = 11;
//	BRect frame = orig->Bounds();
//	int32 width = frame.Width();
//	int32 height = frame.Height();
//	
//	frame.InsetBy(-ceil(blur_size/2.9),-ceil(blur_size/2.0));
//	BBitmap *spare = new BBitmap(frame,B_COLOR_8_BIT);
//	uchar *spare_bits = (uchar*)spare->Bits();
//	uchar *orig_bits = (uchar*)orig->Bits();
//	uint32 spare_bpr = spare->BytesPerRow();
//	uint32 orig_bpr = orig->BytesPerRow();
//	
//	// copy the rows
//	spare_bits += (int32)ceil(blur_size/2.0)*spare_bpr;		
//	for (int32 y=0;y<=height;y++) {
//		spare_bits += (int32)ceil(blur_size/2.0);		
//		for (int32 x=0;x<=width;x++) {
//			*spare_bits++ = *orig_bits++;			
//		}
//		// pad to end of rows
//		orig_bits += (orig_bpr - width - 1);
//		spare_bits += (spare_bpr - width -blur_size/2 - 1);
//	}	
//
//	// pad rows
//	for (int32 y=0;y<ceil(blur_size/2.0);y++) {
//		spare_bits = (uchar*)spare->Bits()+(int32)ceil(blur_size/2.0) + y*spare_bpr;
//		orig_bits = (uchar*)orig->Bits();
//		for (int32 x=0;x<=width;x++) 
//			*spare_bits++ = *orig_bits++;
//	}	
//
//	for (int32 y=0;y<ceil(blur_size/2.0);y++) {
//		spare_bits = (uchar*)spare->Bits()+(int32)ceil(blur_size/2.0) + (int32)(height + ceil(blur_size/2.0)+y)*spare_bpr;
//		orig_bits = (uchar*)orig->Bits()+height*orig_bpr;
//		for (int32 x=0;x<=width;x++) 
//			*spare_bits++ = *orig_bits++;
//	}	
//
//	// pad columns
//	spare_bits = (uchar*)spare->Bits();
//	for (int32 y=0;y<=height + 2*ceil(blur_size/2.0);y++) {
//		spare_bits += spare_bpr;
//		for (int32 x=0;x<ceil(blur_size/2.0);x++) {
//			*(spare_bits + x) = *(spare_bits + (int32)ceil(blur_size/2.0));
//			*(spare_bits + width + 2*(int32)ceil(blur_size/2.0)-x) = *(spare_bits + (int32)ceil(blur_size/2.0)+width);
//		}
//	}
//
//
//	// Finally do the blur. Use the separability...
//	orig_bits = (uchar*)orig->Bits();
//	spare_bits = (uchar*)spare->Bits() + (int32)ceil(blur_size/2)*spare_bpr + (int32)ceil(blur_size/2);
//	int32 blur_size_per_2 = (blur_size - 1)/2;
//	for (int32 y=0;y<=height;y++) {
//		for (int32 x=0;x<width;x++) {
//			uchar new_value = 0;
//			for (int i=0;i<blur_size;i++) {
//				//new_value += *(spare_bits - blur_size_per_2 + i) * (1.0/pow((i-blur_size_per_2),2));   
//				// for now do just a uniform blur instead of a gaussian
//				new_value += *(spare_bits - blur_size_per_2+i) * (1.0/(float)blur_size);
//			}
//			*orig_bits++ = new_value;
//			spare_bits++;
//		}
//		// pad to end of rows
//		orig_bits = (uchar*)orig->Bits() + (y+1)*orig_bpr;
//		spare_bits = (uchar*)spare->Bits() + (y+1)*spare_bpr + blur_size_per_2;
//	}
//	
//
//	// Copy original to spare
//	
//			
//	return orig;
//}
//
//
//BBitmap* ImpressionistManipulator::CalculateSobel(BBitmap *orig)
//{
//	
//}


//ImpressionistWindow::ImpressionistWindow(BView *target,ImpressionistManipulator *manipulator,float dir,float len,float wid)
//	: BWindow(BRect(300,300,400,400),"Impressionist",B_TITLED_WINDOW,B_NOT_RESIZABLE)
//{
//	target_view = target;
//	the_manipulator = manipulator;
//	BBox *container = new BBox(BRect(0,0,150,100));
//	background = container;
//	AddChild(container);
//	ResizeTo(container->Bounds().Width(),container->Bounds().Height());
//
//	// The message to slider must not be NULL, and it must contain an int32 named "value".
//	// Also, the message is not copied to the control.
//	BMessage *message = new BMessage(STROKE_LENGTH_CHANGED);
//	message->AddInt32("value",0);	
//	ControlSliderBox *slider1 = new ControlSliderBox(BRect(4,4,146,44),"stroke length","Stroke Length","",message,MIN_STROKE_LENGTH,MAX_STROKE_LENGTH,B_PLAIN_BORDER,FALSE);
//	container->AddChild(slider1); 
//	slider1->setValue(len);
//	BRect slider_frame = slider1->Frame();
//	slider_frame.OffsetBy(0,slider_frame.Height()+4);
//
//	message = new BMessage(STROKE_WIDTH_CHANGED);
//	message->AddInt32("value",2);	
//	slider1 = new ControlSliderBox(slider_frame,"stroke width","Stroke Width","",message,MIN_STROKE_WIDTH,MAX_STROKE_WIDTH,B_PLAIN_BORDER,FALSE);
//	container->AddChild(slider1); 
//	slider1->setValue(wid);
//	slider_frame = slider1->Frame();
//	slider_frame.OffsetBy(0,slider_frame.Height()+4);	
////	BCheckBox *a_checkbox = new BCheckBox(slider_frame,"motion blur alpha","Blur Transparecy",new BMessage(BLUR_TRANSPARENCY_CHANGED));
////	a_checkbox->ResizeToPreferred();
////	container->AddChild(a_checkbox);
////	slider_frame = slider1->Frame();
////	slider_frame.OffsetBy(0,a_checkbox->Frame().bottom - slider_frame.top+EXTRA_EDGE);
//	
//
//	DirectionControlBox *d_control = new DirectionControlBox(slider_frame,"stroke direction","Stroke Direction",new BMessage(STROKE_DIRECTION_CHANGED));
//	d_control->setValue(dir);
//	container->AddChild(d_control);
//	slider_frame = d_control->Frame();
//	slider_frame.OffsetBy(0,slider_frame.Height()+4);
//	//39, 39
//	preview_bitmap = new BBitmap(BRect(0,0,63,63),B_RGB_32_BIT);
//	preview_view = new BitmapViewBox(preview_bitmap,slider_frame,"Preview");
//	container->AddChild(preview_view);
//	slider_frame = preview_view->Frame();
//	slider_frame.OffsetBy(0,slider_frame.Height()+4);
//
//	message = new BMessage(HS_OPERATION_FINISHED);
//	message->AddBool("status",FALSE);
//	cancel_button = new BButton(slider_frame,"impressionist cancel button","Cancel",message);
//	container->AddChild(cancel_button);
//	cancel_button->ResizeToPreferred();
//	
//	slider_frame = cancel_button->Frame();
//	slider_frame.OffsetBy(slider_frame.Width()+4,0);
//
//	message = new BMessage(HS_OPERATION_FINISHED);
//	message->AddBool("status",TRUE);
//	ok_button = new BButton(slider_frame,"impressionist accept button","OK",message);
//	container->AddChild(ok_button);
//	ok_button->ResizeToPreferred();
//
//	container->ResizeTo(container->Frame().Width(),slider_frame.bottom+4);
//	ResizeTo(container->Frame().Width(),container->Frame().Height());
//	status = FALSE;	
//}
//
//
//ImpressionistWindow::~ImpressionistWindow()
//{
//	BMessage *a_message = new BMessage(HS_OPERATION_FINISHED);
//	a_message->AddBool("status",status);
//	target_view->Window()->PostMessage(a_message,target_view);
//	delete a_message;
//}
//
//
//
//void ImpressionistWindow::MessageReceived(BMessage *message)
//{
//	BLooper *source;
//	message->FindPointer("source",&source);
//	
//	switch (message->what) {
//		case STROKE_LENGTH_CHANGED:
//			int32 blur_amount = message->FindInt32("value");
//			the_manipulator->ChangeValue(blur_amount);
//			the_manipulator->CalculatePreview(preview_bitmap);
//			preview_view->UpdateBitmap();
//			break;
//		case STROKE_DIRECTION_CHANGED:
//			BControl *control;
//			control = cast_as(source,BControl);
//			if (control != NULL) {
//				the_manipulator->ChangeDirection(control->Value());
//				the_manipulator->CalculatePreview(preview_bitmap);
//				preview_view->UpdateBitmap();
//			}
//			break;
//		case STROKE_WIDTH_CHANGED:
//			int32 width= message->FindInt32("value");
//			the_manipulator->ChangeWidth(width);
//			the_manipulator->CalculatePreview(preview_bitmap);
//			preview_view->UpdateBitmap();
//			break;
//		case HS_OPERATION_FINISHED:
//			message->FindBool("status",&status);
//			Quit();
//			break;
//		default:
//			inherited::MessageReceived(message);
//			break;			
//	}	
//}
//
//
//void ImpressionistWindow::UpdatePreview()
//{
//	the_manipulator->CalculatePreview(preview_bitmap);
//	Lock();
//	preview_view->UpdateBitmap();
//	Unlock();	
//}