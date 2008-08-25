/* 

	Filename:	Bump.cpp
	Contents:	Bump manipulator functions.	
	Author:		Heikki Suhonen
	
*/

#include "AddOnTypes.h"
#include "Bump.h"
#include "PixelOperations.h"

extern "C" ViewManipulator* start_manipulator(BView*,manipulator_data&);

#pragma export on
bool creates_gui = TRUE;		// If this is true, we should inherit from GUIManipilator.
char name[255] = "Bump…";
int32 add_on_api_version = ADD_ON_API_VERSION;
add_on_types add_on_type = DISTORT_ADD_ON;


ViewManipulator* start_manipulator(BView *target_view,manipulator_data &data)
{
	// Here create a view-manipulator. The class should inherit
	// from the ViewManipulator base-class. It will be deleted
	// in the application program.
	return new BumpManipulator(target_view,data);	
}

#pragma export off



BumpManipulator::BumpManipulator(BView *target,manipulator_data &data)
		: GUIManipulator(target)
{
	the_data = data;	// Especially important data is the image_updater-function. 
	node = data.node;
	
	if (node->ReadAttr("bump_amount",B_INT32_TYPE,0,&bump_amount,sizeof(int32)) != sizeof(int32))
		bump_amount = INITIAL_BUMP_AMOUNT;
	if (node->ReadAttr("bump_length",B_INT32_TYPE,0,&bump_length,sizeof(int32)) != sizeof(int32))
		bump_length = INITIAL_BUMP_LENGTH;

	bump_window = new BumpWindow(target,this,bump_amount,bump_length);	
	bump_window->Show();
	target_bitmap = data.current_layer;

	status_view = NULL;
	
	// Make a copy of target_bitmap. It should be a little bit larger than the target_bitmap.
	original_bitmap = CopyBitmap(target_bitmap);
	preview_enabled = TRUE;
	
	// This assumes that the left and top are zeroes.
	center_x = target_bitmap->Bounds().right/2;
	center_y = target_bitmap->Bounds().bottom/2;
	preview_level = GOOD_PREVIEW;	
	sin_table = new float[720];
	
	for (int32 i=0;i<720;i++) 
		sin_table[i] = sin((float)i/720.0*2*PI);
			
	CalculateBump(this);	
}


BumpManipulator::~BumpManipulator()
{
	if (status == TRUE) {
		node->WriteAttr("bump_amount",B_INT32_TYPE,0,&bump_amount,sizeof(int32));
		node->WriteAttr("bump_length",B_INT32_TYPE,0,&bump_length,sizeof(int32));
	}
	delete[] sin_table;
	delete original_bitmap;
	delete node;
}


BBitmap* BumpManipulator::ManipulateBitmap(BBitmap *original, BView *progress_view, float prog_step)
{
	// We may create another bitmap and return it instead of original, but we may
	// also do the manipulation on the original and return it. We Should send messages
	// to progress_view that contain B_UPDATE_STATUS_BAR as their 'what'. The sum of
	// message deltas should equal 100*prog_step.
	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta",0.0);

	if (target_bitmap != original) {
		target_bitmap = original;
		delete original_bitmap;
		original_bitmap = CopyBitmap(original);
	}
	status_view = progress_view;
	progress_step = prog_step;
	preview_level = FULL_CALCULATION;
	CalculateBump(this);
	return original;
}


void BumpManipulator::FinishManipulation(bool s)
{
	uint32 *orig_bits = (uint32*)original_bitmap->Bits();
	uint32 *target_bits = (uint32*)target_bitmap->Bits();
	int32 orig_bpr = original_bitmap->BytesPerRow()/4;
	uint32 spare_bits;
	status = s;
	
	BRect bounds = target_bitmap->Bounds();
	int32 width = bounds.Width();
	int32 height = bounds.Height();

	for (int32 y=0;y<=height;y++) {
		for (int32 x=0;x<=width;x++) {	
			*target_bits++ = *orig_bits++;
		}
	}

	bump_window = NULL;
}
void BumpManipulator::MouseDown(BPoint point,uint32 buttons,uint32,GET_MOUSE mouse_function)
{
	BWindow *target_window = target_view->Window();
	
	// Make sure that the view is updated even with just a single click.	
	BPoint prev_point = point - BPoint(1,1);	
	BPoint view_point;
	BRect bitmap_bounds = original_bitmap->Bounds();
	BPoint old_point = point - BPoint(1,1);
	preview_level = QUICK_PREVIEW;
		
	if (target_window != NULL) {	
		while (buttons) {
			target_window->Lock();
			mouse_function(target_view,&point,&buttons,&view_point);			
			target_window->Unlock();
			if (point != prev_point) {
				center_x = point.x;
				center_y = point.y;				
				calculate_bump();
			}
			snooze(20.0 * 1000.0);
		}
	}
	center_x = point.x;
	center_y = point.y;

	preview_level = GOOD_PREVIEW;
	// Update the image
	thread_id bump_thread = spawn_thread(BumpManipulator::CalculateBump,"bump main thread",B_NORMAL_PRIORITY,this);
	resume_thread(bump_thread);
		
	target_window->PostMessage(HS_ACTION_FINISHED,target_view);
}


void BumpManipulator::ChangeValue(int32 amount,preview_levels level)
{
	if ((bump_amount != amount) || (preview_level != level)) {
		preview_level = level;
		bump_amount = amount;
		thread_id bump_thread = spawn_thread(BumpManipulator::CalculateBump,"bump main thread",B_NORMAL_PRIORITY,this);
		resume_thread(bump_thread);
	}
}

void BumpManipulator::ChangeBumpLength(int32 length,preview_levels level)
{
	if ((bump_length != length) || (preview_level != level)) {
		preview_level = level;
		bump_length = length;
		thread_id bump_thread = spawn_thread(BumpManipulator::CalculateBump,"bump main thread",B_NORMAL_PRIORITY,this);
		resume_thread(bump_thread);
	}	
}


int32 BumpManipulator::CalculateBump(void *data)
{
	((BumpManipulator*)data)->calculate_bump();

	return B_OK;
}


void BumpManipulator::calculate_bump()
{
	// Here start a thread for each processor or just one if height is less than processor count.
	system_info s_info;
	get_system_info(&s_info);

	if (bump_window != NULL) {
		((BumpWindow*)bump_window)->PreviewFinished(FALSE);			
	}
		
	int32 row_count = target_bitmap->Bounds().Height() + 1;
	int32 amount_of_bump = bump_amount;	// Take this here so that each thread has the same value.
	int32 length_of_bump = bump_length;
	uint32 *target_bits = (uint32*)target_bitmap->Bits();
	uint32 *source_bits = (uint32*)original_bitmap->Bits();
	int32 target_bpr = target_bitmap->BytesPerRow()/4;
	int32 source_bpr = original_bitmap->BytesPerRow()/4;
	float top = original_bitmap->Bounds().top;
	float bottom = original_bitmap->Bounds().bottom;
	
	thread_data *t_data;
	if ((status_view == NULL) && (bump_window != NULL)) {
		if (preview_level != QUICK_PREVIEW) {
			status_view = ((BumpWindow*)bump_window)->DisplayProgressBar();
			progress_step = 1.0/s_info.cpu_count;
		}
	}
	else {
		progress_step = progress_step/s_info.cpu_count;
	}	
	if (row_count < s_info.cpu_count) {
		// Here we start just one thread that calculates the blur.
		t_data = new thread_data();		// The thread-function should then delete this data
		
		t_data->target_bits = target_bits;
		t_data->source_bits = source_bits;
		t_data->target_bpr = target_bpr;
		t_data->source_bpr = source_bpr;
		
		t_data->start_y = 0;
		t_data->end_y = row_count-1;
		t_data->the_manipulator = this;
		
		t_data->bump_amount = amount_of_bump;
		t_data->bump_length = length_of_bump;
		t_data->center_x = center_x;
		t_data->center_y = center_y;		
		t_data->top = top;
		t_data->bottom = bottom;
		
		thread_id bump_thread = spawn_thread(BumpManipulator::threadEntry,"bump thread",B_NORMAL_PRIORITY,t_data);
		int32 return_value;
		wait_for_thread(bump_thread,&return_value);
	}
	else {
		// Here we start info.cpu_count threads to calculate the blur.
		thread_id *thread_array = new thread_id[s_info.cpu_count];
		for (int32 p=0;p<s_info.cpu_count;p++) {
			t_data = new thread_data();		// The thread-function should then delete this data
			
			t_data->target_bits = target_bits;
			t_data->source_bits = source_bits;
			t_data->target_bpr = target_bpr;
			t_data->source_bpr = source_bpr;
			
			t_data->start_y = p+p*((int32)((row_count-1)/s_info.cpu_count));
			if (p != s_info.cpu_count-1) {
				t_data->end_y = p+(p+1)*((int32)((row_count-1)/s_info.cpu_count));
			}			
			else
				t_data->end_y = row_count-1;

			t_data->the_manipulator = this;			
			t_data->bump_amount = amount_of_bump;
			t_data->bump_length = length_of_bump;
			t_data->center_x = center_x;
			t_data->center_y = center_y;		
			t_data->top = top;
			t_data->bottom = bottom;
			thread_array[p] = spawn_thread(BumpManipulator::threadEntry,"bump thread",B_NORMAL_PRIORITY,t_data);			
			resume_thread(thread_array[p]);
		}
		for (int32 p=0;p<s_info.cpu_count;p++) {
			int32 return_value;
			wait_for_thread(thread_array[p],&return_value);
		}
		delete[] thread_array;
	}
	status_view = NULL;
	
	if (bump_window != NULL ) {
		if (preview_level == GOOD_PREVIEW)
			((BumpWindow*)bump_window)->DisplayConfirmButtons();
	
		((BumpWindow*)bump_window)->PreviewFinished(TRUE);		
	}	
	
	// Then update the view.
	if (preview_level == QUICK_PREVIEW) {
		target_view->Window()->Lock();
		the_data.image_updater(target_view,BRect(0,0,-1,-1),TRUE);
		target_view->Window()->Unlock();
	}
	else {
		target_view->Window()->Lock();
		the_data.image_updater(target_view,BRect(0,0,-1,-1),FALSE);
		target_view->Window()->Unlock();
	}


}


int32 BumpManipulator::threadEntry(void *data)
{
	thread_data *t_data = (thread_data*)data;
	if (t_data == NULL)
		return B_ERROR;
		
	BumpManipulator *the_manipulator = cast_as(t_data->the_manipulator,BumpManipulator);
	
	if (the_manipulator == NULL)
		return B_ERROR;
		
	the_manipulator->bump_func(t_data->target_bits,t_data->source_bits,t_data->target_bpr,t_data->source_bpr,t_data->start_y,t_data->end_y,t_data->bump_amount,t_data->bump_length,t_data->dampening,t_data->center_x,t_data->center_y,t_data->top,t_data->bottom);
	
	delete data;

	return B_OK;
}



int32 BumpManipulator::bump_func(uint32 *target_bits,uint32 *source_bits,int32 target_bpr,int32 source_bpr,int32 start_y,int32 end_y,int32 amount_of_bump,int32 length_of_bump,int32 dampening_of_bump,float center_x, float center_y,float top_of_image,float bottom_of_image)
{
	// First move the bits to right starting positions.
	// In this manipulator do not move the source bits, because it will be
	// 'indexed' directly.
//	source_bits += start_y * source_bpr;
	target_bits += start_y * target_bpr;

	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta",0.0);

	BStopWatch *watch = new BStopWatch("Bump Time");
	
	float s = length_of_bump;
	float A = amount_of_bump;
//	float k = (float)(dampening_of_bump)/(float)MAX_BUMP_DAMPENING;
	float dx,dy;
	register float sqrt_x_plus_y;
	register float one_per_sqrt_x_plus_y;
	float cx,cy;
	float top,bottom;
	top = top_of_image;
	bottom = bottom_of_image;
	
	cx = floor(center_x);
	cy = floor(center_y);
	float R = sqrt(pow(max_c(abs(cx),target_bpr-abs(cx)),2)+pow(max_c(abs(cy),bottom_of_image-abs(cy)),2));
	

	float real_x,real_y;
	register float two_pi_per_s = 2*PI/s;
	register float two_360_per_s = 720.0/s;
	
	if (preview_level == FULL_CALCULATION) {
		uint32 p1,p2,p3,p4;
	 	for (float y=start_y;y<=end_y;y++) {
	 		for (float x=0;x<source_bpr;x++) {
				real_x = x-cx;
				real_y = y-cy;
				uint32 target_value = 0x00000000;
				dx = A*sin_table[abs((int32)(real_x*two_360_per_s)%720)];
				dy = A*sin_table[abs((int32)(real_y*two_360_per_s)%720)];
				float y_mix_upper = ceil(y+dy)-(y+dy);
				float x_mix_right = (x+dx)- floor(x+dx);
				if ((ceil(y+dy) <= bottom) && (ceil(y+dy)>=top)) {
					if ((floor(x+dx) >= 0) && (floor(x+dx) <source_bpr)) {
						p1 = *(source_bits + (int32)ceil(y+dy)*source_bpr + (int32)floor(x+dx));
					}						
					else
						p1 = 0xFFFFFF00;
											
					if ((ceil(x+dx) >= 0) && (ceil(x+dx) <source_bpr)) {
						p2 = *(source_bits + (int32)ceil(y+dy)*source_bpr + (int32)ceil(x+dx));
					}						
					else
						p2 = 0xFFFFFF00;
				}
				else {
					p1 = 0xFFFFFF00;
					p2 = 0xFFFFFF00;
				}
				if ((floor(y+dy) <= bottom) && (floor(y+dy)>=top)) {
					if ((floor(x+dx) >= 0) && (floor(x+dx) <source_bpr)) {
						p3 = *(source_bits + (int32)floor(y+dy)*source_bpr + (int32)floor(x+dx));
					}						
					else
						p3 = 0xFFFFFF00;
											
					if ((ceil(x+dx) >= 0) && (ceil(x+dx) <source_bpr)) {
						p4 = *(source_bits + (int32)floor(y+dy)*source_bpr + (int32)ceil(x+dx));
					}						
					else
						p4 = 0xFFFFFF00;
				}
				else {
					p3 = 0xFFFFFF00;
					p4 = 0xFFFFFF00;
				}

				*target_bits++ = combine_4_pixels(p1,p2,p3,p4,(1-y_mix_upper)*(1-x_mix_right),(1-y_mix_upper)*(x_mix_right),(y_mix_upper)*(1-x_mix_right),(y_mix_upper)*(x_mix_right));

			}
	 		// Send a progress-message if required
	 		if ((status_view != NULL) && ((int32)y%10 == 0)) {
	 			progress_message.ReplaceFloat("delta",(100.0*progress_step)/(float)(end_y-start_y)*10.0);
				status_view->Window()->PostMessage(&progress_message,status_view);
	 		}
	 	}
	}
	else if (preview_level == GOOD_PREVIEW) {
	 	for (float y=start_y;y<=end_y;y++) {
	 		for (float x=0;x<source_bpr;x++) {
				real_x = x-cx;
				real_y = y-cy;
				dx = A*sin_table[abs((int32)(real_x*two_360_per_s)%720)];
				dy = A*sin_table[abs((int32)(real_y*two_360_per_s)%720)];

				// This if is quite slow.
				if (((y+dy) <= bottom) && ((y+dy)>=top) && ((x+dx) >= 0) && ((x+dx)<target_bpr)) {
					*(target_bits) = *(source_bits + (int32)(x+dx) + (int32)(y+dy)*source_bpr);
				}
				else {
					*target_bits = 0xFFFFFF00;
				}
				target_bits++;
			}
	 		// Send a progress-message if required
	 		if ((status_view != NULL) && ((int32)y%10 == 0)) {
	 			progress_message.ReplaceFloat("delta",(100.0*progress_step)/(float)(end_y-start_y)*10.0);
				status_view->Window()->PostMessage(&progress_message,status_view);
	 		}
	 	}		
	}
	else {
		int32 width = source_bpr/2*2;
	 	for (float y=start_y;y<end_y;y+=2) {
	 		for (float x=0;x<width;x+=2) {
				real_x = x-cx;
				real_y = y-cy;
					
				dx = A*sin_table[abs((int32)(real_x*two_360_per_s)%720)];
				dy = A*sin_table[abs((int32)(real_y*two_360_per_s)%720)];
				// This if is quite slow.
				if (((y+dy) < bottom) && ((y+dy)>=top) && ((x+dx) >= 0) && ((x+dx)<target_bpr-1)) {
					uint32 new_value = *(source_bits + (int32)(x+dx) + (int32)(y+dy)*source_bpr);
					*(target_bits) = new_value;
					*(target_bits+1) = new_value;
					*(target_bits+target_bpr) = new_value;
					*(target_bits+target_bpr+1) = new_value;
					
				}
				else {
					*target_bits = 0xFFFFFF00;
					*(target_bits+1) = 0xFFFFFF00;
					*(target_bits+target_bpr) = 0xFFFFFF00;
					*(target_bits+target_bpr+1) = 0xFFFFFF00;
				}
				target_bits+=2;
			}
			if ((source_bpr % 2) == 0) {
				target_bits += target_bpr;
			}
			else
				target_bits += target_bpr + 1;
	 	}
	 	// Do the last row if necessary.
	 	if ((end_y-start_y) % 2 == 0) {
			float y= end_y;
	 		for (float x=0;x<width;x+=2) {
				real_x = x-cx;
				real_y = y-cy;
					
				dx = A*sin_table[abs((int32)(real_x*two_360_per_s)%720)];
				dy = A*sin_table[abs((int32)(real_y*two_360_per_s)%720)];
				// This if is quite slow.
				if (((y+dy) < bottom) && ((y+dy)>=top) && ((x+dx) >= 0) && ((x+dx)<target_bpr-1)) {
					uint32 new_value = *(source_bits + (int32)(x+dx) + (int32)(y+dy)*source_bpr);
					*(target_bits) = new_value;
					*(target_bits+1) = new_value;					
				}
				else {
					*target_bits = 0xFFFFFF00;
					*(target_bits+1) = 0xFFFFFF00;
				}
				target_bits+=2;
			}	 		
	 	}		
	}
	delete watch;
	return B_OK;
}

BumpWindow::BumpWindow(BView *target,BumpManipulator *manipulator,int32 am,int32 len)
	: BWindow(BRect(300,300,400,400),"Bump",B_TITLED_WINDOW,B_NOT_RESIZABLE)
{
	target_view = target;
	the_manipulator = manipulator;
	BBox *container = new BBox(BRect(0,0,200,100));
	background = container;
	AddChild(container);
	ResizeTo(container->Bounds().Width(),container->Bounds().Height());

	// The message to slider must not be NULL, and it must contain an int32 named "value".
	// Also, the message is not copied to the control.
	BMessage *message = new BMessage(BUMP_AMOUNT_CHANGED);
	message->AddInt32("value",0);	
	ControlSliderBox *slider1 = new ControlSliderBox(BRect(4,4,196,44),"bump amount","Bump Amount","",message,1,MAX_BUMP_AMOUNT,B_PLAIN_BORDER,TRUE);
	container->AddChild(slider1); 
	slider1->setValue(am);
	BRect slider_frame = slider1->Frame();
	slider_frame.OffsetBy(0,slider_frame.Height()+4);
	
	message = new BMessage(BUMP_LENGTH_CHANGED);
	message->AddInt32("value",0);	
	slider1 = new ControlSliderBox(slider_frame,"bump length","Bump Length","",message,1,MAX_BUMP_LENGTH,B_PLAIN_BORDER,TRUE);
	container->AddChild(slider1); 
	slider1->setValue(len);
	slider_frame = slider1->Frame();
	slider_frame.OffsetBy(0,slider_frame.Height()+4);	
		
	message = new BMessage(HS_OPERATION_FINISHED);
	message->AddBool("status",FALSE);
	cancel_button = new BButton(slider_frame,"bump cancel button","Cancel",message);
	container->AddChild(cancel_button);
	cancel_button->ResizeToPreferred();
	
	slider_frame = cancel_button->Frame();
	slider_frame.OffsetBy(slider_frame.Width()+4,0);

	message = new BMessage(HS_OPERATION_FINISHED);
	message->AddBool("status",TRUE);
	ok_button = new BButton(slider_frame,"bump accept button","OK",message);
	container->AddChild(ok_button);
	ok_button->ResizeToPreferred();


	preview_progress_bar = new BStatusBar(BRect(cancel_button->Frame().LeftTop(),ok_button->Frame().RightBottom()),"blur preview progress bar","Calculating Preview");
	ResizeTo(Bounds().Width(),ok_button->Frame().bottom + 4);
	container->ResizeTo(Bounds().Width(),ok_button->Frame().bottom + 4);
	container->AddChild(preview_progress_bar);
	preview_progress_bar->RemoveSelf();	
	preview_progress_bar->SetBarHeight(16 - (preview_progress_bar->Frame().bottom-ok_button->Frame().bottom));		

	status = FALSE;	
	preview_finished = FALSE;
}


BumpWindow::~BumpWindow()
{
	BMessage *a_message = new BMessage(HS_OPERATION_FINISHED);
	a_message->AddBool("status",status);
	target_view->Window()->PostMessage(a_message,target_view);
	delete a_message;
}



void BumpWindow::MessageReceived(BMessage *message)
{
	BLooper *source;
	message->FindPointer("source",&source);
	bool final;
	message->FindBool("final",&final);
	switch (message->what) {
		case BUMP_AMOUNT_CHANGED:
			int32 bump_amount = message->FindInt32("value");
			if (final==FALSE) {				
				if (preview_finished == TRUE) {
					the_manipulator->ChangeValue(bump_amount,QUICK_PREVIEW);
				}
			}
			else {
				the_manipulator->ChangeValue(bump_amount,GOOD_PREVIEW);
			}
			break;
		case BUMP_LENGTH_CHANGED:
			int32 bump_length = message->FindInt32("value");
			if (final==FALSE) {				
				if (preview_finished == TRUE) {					
					the_manipulator->ChangeBumpLength(bump_length,QUICK_PREVIEW);
				}
			}
			else {
				the_manipulator->ChangeBumpLength(bump_length,GOOD_PREVIEW);		
			}
			break;			
		case HS_OPERATION_FINISHED:
			message->FindBool("status",&status);
			Quit();
			break;
		default:
			inherited::MessageReceived(message);
			break;			
	}	
}

BView* BumpWindow::DisplayProgressBar()
{
	Lock();
	if (preview_progress_bar->Parent() == NULL) {
		ok_button->RemoveSelf();
		cancel_button->RemoveSelf();
		background->AddChild(preview_progress_bar);
	}
	preview_progress_bar->Reset("Calculating Preview");
	Unlock();

	return preview_progress_bar;
}

void BumpWindow::DisplayConfirmButtons()
{
	Lock();
	if (preview_progress_bar->Parent() == background) {
		preview_progress_bar->RemoveSelf();
		background->AddChild(ok_button);
		background->AddChild(cancel_button);
	}
	Unlock();
}