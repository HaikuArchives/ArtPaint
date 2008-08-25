/* 

	Filename:	AddOnTemplate.cpp
	Contents:	A template for the ArtPaint add-ons.	
	Author:		Heikki Suhonen
	
*/

#include "AddOnTypes.h"
#include "MotionBlur.h"
#include "DirectionControl.h"

extern "C" ViewManipulator* start_manipulator(BView*,manipulator_data&);

#pragma export on
bool creates_gui = TRUE;		// If this is true, we should inherit from GUIManipilator.
char name[255] = "MotionBlur…";
int32 add_on_api_version = ADD_ON_API_VERSION;
add_on_types add_on_type = BLUR_FILTER_ADD_ON;


ViewManipulator* start_manipulator(BView *target_view,manipulator_data &data)
{
	// Here create a view-manipulator. The class should inherit
	// from the ViewManipulator base-class. It will be deleted
	// in the application program.
	return new MotionBlurManipulator(target_view,data);	
}

#pragma export off



MotionBlurManipulator::MotionBlurManipulator(BView *target,manipulator_data &data)
		: GUIManipulator(target)
{
	the_data = data;	// Especially important data is the image_updater-function. 
	node = data.node;
	
	if (node->ReadAttr("blur_amount",B_INT32_TYPE,0,&blur_amount,sizeof(int32)) != sizeof(int32))
		blur_amount = 5;
	if (node->ReadAttr("direction",B_INT32_TYPE,0,&direction,sizeof(int32)) != sizeof(int32))
		direction = 0;
	if (node->ReadAttr("blur_alpha",B_INT32_TYPE,0,&blur_alpha,sizeof(int32)) != sizeof(int32))
		blur_alpha = B_CONTROL_OFF;

		
	blur_window = new MotionBlurWindow(target,this,blur_amount,direction,blur_alpha);	
	blur_window->Show();
	target_bitmap = data.current_layer;

	status_view = NULL;
	
	// Make a copy of target_bitmap. It should be a little bit larger than the target_bitmap.
	original_bitmap = ExtendAndCopyBitmap(target_bitmap,MAX_BLUR_AMOUNT);
	deltas = new blur_delta[MAX_BLUR_AMOUNT];
	preview_enabled = TRUE;
	CalculateBlur(this);
}


MotionBlurManipulator::~MotionBlurManipulator()
{
	if (status == TRUE) {
		node->WriteAttr("blur_amount",B_INT32_TYPE,0,&blur_amount,sizeof(int32));
		node->WriteAttr("blur_alpha",B_INT32_TYPE,0,&blur_alpha,sizeof(int32));
		node->WriteAttr("direction",B_INT32_TYPE,0,&direction,sizeof(int32));		
	}

	delete node;
}


BBitmap* MotionBlurManipulator::ManipulateBitmap(BBitmap *original, BView *progress_view, float prog_step)
{
	// We may create another bitmap and return it instead of original, but we may
	// also do the manipulation on the original and return it. We Should send messages
	// to progress_view that contain B_UPDATE_STATUS_BAR as their 'what'. The sum of
	// message deltas should equal 100*prog_step.
	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta",0.0);

	if ((target_bitmap != original) || (!preview_enabled)) { 	
		target_bitmap = original;
		CopySmallBitmapToExtended(original,original_bitmap,MAX_BLUR_AMOUNT);
		status_view = progress_view;
		progress_step = prog_step;
		CalculateBlur(this);
	}
	else {
		// Swap the contents of buffers
		uint32 *orig_bits = (uint32*)original_bitmap->Bits();
		uint32 *target_bits = (uint32*)target_bitmap->Bits();
		int32 orig_bpr = original_bitmap->BytesPerRow()/4;
		orig_bits += MAX_BLUR_AMOUNT*orig_bpr;
		uint32 spare_bits;
		
		BRect bounds = target_bitmap->Bounds();
		int32 width = bounds.Width();
		int32 height = bounds.Height();

		for (int32 y=0;y<=height;y++) {
			orig_bits+=MAX_BLUR_AMOUNT;
			for (int32 x=0;x<=width;x++) {	
				spare_bits = *target_bits;
				*target_bits++ = *orig_bits;
				*orig_bits++ = spare_bits;
			}
			orig_bits+=MAX_BLUR_AMOUNT;
		}		
	}
		
//	progress_message.ReplaceFloat("delta",(100.0*prog_step)/(float)height*10.0);
//	progress_view->Window()->PostMessage(&progress_message,progress_view);
	return original;
}


void MotionBlurManipulator::FinishManipulation(bool s)
{
	uint32 *orig_bits = (uint32*)original_bitmap->Bits();
	uint32 *target_bits = (uint32*)target_bitmap->Bits();
	int32 orig_bpr = original_bitmap->BytesPerRow()/4;
	orig_bits += MAX_BLUR_AMOUNT*orig_bpr;
	uint32 spare_bits;
	status = s;
		
	BRect bounds = target_bitmap->Bounds();
	int32 width = bounds.Width();
	int32 height = bounds.Height();

	if (!preview_enabled) {		
		for (int32 y=0;y<=height;y++) {
			orig_bits+=MAX_BLUR_AMOUNT;
			for (int32 x=0;x<=width;x++) {	
				*target_bits++ = *orig_bits++;
			}
			orig_bits+=MAX_BLUR_AMOUNT;
		}
	}
	else {
		// Swap the contents of buffers
		for (int32 y=0;y<=height;y++) {
			orig_bits+=MAX_BLUR_AMOUNT;
			for (int32 x=0;x<=width;x++) {	
				spare_bits = *target_bits;
				*target_bits++ = *orig_bits;
				*orig_bits++ = spare_bits;
			}
			orig_bits+=MAX_BLUR_AMOUNT;
		}	
	}
	// Also take note that the MotionBlur-window is now gone
	blur_window = NULL;
}


void MotionBlurManipulator::ChangeValue(int32 amount)
{
	if (blur_amount != amount) {
		blur_amount = amount;
		thread_id blur_thread = spawn_thread(MotionBlurManipulator::CalculateBlur,"MotionBlur main thread",B_NORMAL_PRIORITY,this);
		resume_thread(blur_thread);

	}
}

void MotionBlurManipulator::ChangeTransparency(int32 new_value)
{
	if (blur_alpha != new_value) {
		blur_alpha = new_value;
		thread_id blur_thread = spawn_thread(MotionBlurManipulator::CalculateBlur,"MotionBlur main thread",B_NORMAL_PRIORITY,this);
		resume_thread(blur_thread);
	}
}


void MotionBlurManipulator::ChangeDirection(int32 dir)
{
	if (direction != dir) {
		direction = dir;
		thread_id blur_thread = spawn_thread(MotionBlurManipulator::CalculateBlur,"MotionBlur main thread",B_NORMAL_PRIORITY,this);
		resume_thread(blur_thread);	
	}		
}


BBitmap* MotionBlurManipulator::ExtendAndCopyBitmap(BBitmap *source, int32 extend_amount)
{
	BBitmap *extended_bitmap;
	BRect original_bounds = source->Bounds();
	original_bounds.SetRightBottom(original_bounds.RightBottom()+BPoint(2*extend_amount,2*extend_amount));
	
	extended_bitmap = new BBitmap(original_bounds,B_RGB_32_BIT);
	
	CopySmallBitmapToExtended(source,extended_bitmap,extend_amount);
	return extended_bitmap;
}

void MotionBlurManipulator::CopySmallBitmapToExtended(BBitmap *small,BBitmap *extended,int32 extend_amount)
{
	uint32 *new_bits = (uint32*)extended->Bits();	
	uint32 *source_bits = (uint32*)small->Bits();
	int32 new_bpr = extended->BytesPerRow()/4;
	int32 source_bpr = small->BytesPerRow()/4;
	int32 source_height = small->Bounds().Height();
	
	// First duplicate the first row for MAX_MotionBlur_AMOUNT times
	for (int32 y=0;y<extend_amount;y++) {
		// Duplicate the first column MAX_MotionBlur_AMOUNT times.
		for (int32 x=0;x<extend_amount;x++) {
			*new_bits++ = *source_bits;
		}
		// Duplicate the actual row
		for (int32 x=0;x<source_bpr;x++) {
			*new_bits++ = *source_bits++;	
		}
		source_bits--;
		// Duplicate the last column MAX_MotionBlur_AMOUNT times.
		for (int32 x=0;x<extend_amount;x++) {
			*new_bits++ = *source_bits;
		}
		source_bits -= (source_bpr - 1);
	}		

	// Then copy the rows and extend the first and last columns	
	for (int32 y=0;y<source_height;y++) {
		// Duplicate the first column MAX_MotionBlur_AMOUNT times.
		for (int32 x=0;x<extend_amount;x++) {
			*new_bits++ = *source_bits;
		}
		// Duplicate the actual row
		for (int32 x=0;x<source_bpr;x++) {
			*new_bits++ = *source_bits++;	
		}
		source_bits--;
		// Duplicate the last column MAX_MotionBlur_AMOUNT times.
		for (int32 x=0;x<extend_amount;x++) {
			*new_bits++ = *source_bits;
		}
		source_bits++;
	}		
	source_bits -= source_bpr;
	
	// Finally duplicate the last row for MAX_MotionBlur_AMOUNT times
	for (int32 y=0;y<extend_amount;y++) {
		// Duplicate the first column MAX_MotionBlur_AMOUNT times.
		for (int32 x=0;x<extend_amount;x++) {
			*new_bits++ = *source_bits;
		}
		// Duplicate the actual row
		for (int32 x=0;x<source_bpr;x++) {
			*new_bits++ = *source_bits++;	
		}
		source_bits--;
		// Duplicate the last column MAX_MotionBlur_AMOUNT times.
		for (int32 x=0;x<extend_amount;x++) {
			*new_bits++ = *source_bits;
		}
		source_bits -= (source_bpr - 1);
	}		
}

int32 MotionBlurManipulator::CalculateBlur(void *data)
{
	((MotionBlurManipulator*)data)->calculate_blur();

	return B_OK;
}


void MotionBlurManipulator::calculate_blur()
{
	// Here start a thread for each processor or just one if height is less than processor count.
	system_info s_info;
	get_system_info(&s_info);
	
	int32 row_count = target_bitmap->Bounds().Height() + 1;
	int32 amount_of_blur = blur_amount;	// Take this here so that each thread has the same value.
	int32 blur_also_alpha = blur_alpha;
	uint32 *target_bits = (uint32*)target_bitmap->Bits();
	uint32 *source_bits = (uint32*)original_bitmap->Bits();
	int32 target_bpr = target_bitmap->BytesPerRow()/4;
	int32 source_bpr = original_bitmap->BytesPerRow()/4;
	
	thread_data *t_data;

	CalculateDeltas();
	
	if ((status_view == NULL) && (blur_window != NULL)) {
		status_view = ((MotionBlurWindow*)blur_window)->DisplayProgressBar();
		progress_step = 1.0/s_info.cpu_count;
	}
	else {
		progress_step = progress_step/s_info.cpu_count;
	}	

	if (row_count < s_info.cpu_count) {
		// Here we start just one thread that calculates the MotionBlur.
		t_data = new thread_data();		// The thread-function should then delete this data
		
		t_data->target_bits = target_bits;
		t_data->source_bits = source_bits;
		t_data->target_bpr = target_bpr;
		t_data->source_bpr = source_bpr;
		
		t_data->start_y = 0;
		t_data->end_y = row_count-1;
		t_data->the_manipulator = this;
		
		t_data->blur_amount = amount_of_blur;
		t_data->blur_alpha = blur_also_alpha;	
		thread_id MotionBlur_thread = spawn_thread(MotionBlurManipulator::threadEntry,"MotionBlur thread",B_NORMAL_PRIORITY,t_data);
		int32 return_value;
		wait_for_thread(MotionBlur_thread,&return_value);
	}
	else {
		// Here we start info.cpu_count threads to calculate the MotionBlur.
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
			t_data->blur_amount = amount_of_blur;
			t_data->blur_alpha = blur_also_alpha;	
			thread_array[p] = spawn_thread(MotionBlurManipulator::threadEntry,"MotionBlur thread",B_NORMAL_PRIORITY,t_data);			
			resume_thread(thread_array[p]);
		}
		for (int32 p=0;p<s_info.cpu_count;p++) {
			int32 return_value;
			wait_for_thread(thread_array[p],&return_value);
		}
		delete[] thread_array;
	}
//	delete deltas;
	status_view = NULL;
	
	if (blur_window != NULL) {
		((MotionBlurWindow*)blur_window)->DisplayConfirmButtons();
	}	
	
	// Then update the view.
	target_view->Window()->Lock();
	the_data.image_updater(target_view,BRect(0,0,-1,-1),FALSE);
	target_view->Window()->Unlock();
}


void MotionBlurManipulator::CalculateDeltas()
{
	// This uses a DDA-algorithm to calculate the pixels to be modified
	// We could also calculate pixel coverage and put coefficients
	// according to that.
	BPoint point_array[2];
	point_array[0] = BPoint(0,0);
	point_array[1] = BPoint(0,blur_amount-1);
	HSPolygon *line = new HSPolygon(point_array,2);
	line->Rotate(BPoint(0,0),direction);
	BPoint *points = line->GetPointList();
	BPoint end_point = points[1];
	BPoint start_point = BPoint(0,0);
	
	// Round the endpoint so that it does not exceed the blur amount limits
	if (end_point.x > 0)
		end_point.x = floor(end_point.x);
	else
		end_point.x = ceil(end_point.x);
	
	if (end_point.y > 0)
		end_point.y = floor(end_point.y);
	else
		end_point.y = ceil(end_point.y);
	
	
	// use DDA-algorithm to calculate line between the two points		
	// first check whether the line is longer in x direction than y
	bool increase_x = fabs(start_point.x - end_point.x) >= fabs(start_point.y - end_point.y);
	// check which direction the line is going
	float sign_x;
	float sign_y;
	float x_factor = 0.9;	// This must be between 0 and 1.

	if ((end_point.x-start_point.x) != 0) {
		sign_x = (end_point.x-start_point.x)/fabs(end_point.x-start_point.x);
	}
	else {
		sign_x = 0;
	}
	if ((end_point.y-start_point.y) != 0) {
		sign_y = (end_point.y-start_point.y)/fabs(end_point.y-start_point.y);		
	}
	else {
		sign_y = 0;
	}
	if (increase_x) {
		int32 number_of_points = (int32)fabs(start_point.x - end_point.x) + 1;
		number_of_deltas = number_of_points;
		float a_coeff = (1.0*(1-x_factor))/(1.0-pow(x_factor,number_of_points));
		float y_add = ((float)fabs(start_point.y - end_point.y)) / ((float)fabs(start_point.x - end_point.x));
		for (int32 i=0;i<number_of_points;i++) {
			deltas[i].dx = start_point.x;
			deltas[i].dy = start_point.y;
			deltas[i].coeff = a_coeff * pow(x_factor,i);
			
			start_point.x += sign_x;
			start_point.y += sign_y * y_add;	
		}				
	}
	
	else {
		int32 number_of_points = (int32)fabs(start_point.y - end_point.y) + 1;
		float a_coeff = (1.0*(1-x_factor))/(1.0-pow(x_factor,number_of_points));
		number_of_deltas = number_of_points;
		float x_add = ((float)fabs(start_point.x - end_point.x)) / ((float)fabs(start_point.y - end_point.y));
		for (int32 i=0;i<number_of_points;i++) {
			deltas[i].dx = start_point.x;
			deltas[i].dy = start_point.y;
			deltas[i].coeff = a_coeff * pow(x_factor,i);
			
			start_point.y += sign_y;
			start_point.x += sign_x * x_add;	
		}				
	}	
}

int32 MotionBlurManipulator::threadEntry(void *data)
{
	thread_data *t_data = (thread_data*)data;
	if (t_data == NULL)
		return B_ERROR;
		
	MotionBlurManipulator *the_manipulator = cast_as(t_data->the_manipulator,MotionBlurManipulator);
	
	if (the_manipulator == NULL)
		return B_ERROR;
		
	the_manipulator->blur_func(t_data->target_bits,t_data->source_bits,t_data->target_bpr,t_data->source_bpr,t_data->start_y,t_data->end_y,t_data->blur_amount,t_data->blur_alpha);
	
	delete data;

	return B_OK;
}



int32 MotionBlurManipulator::blur_func(uint32 *target_bits,uint32 *source_bits,int32 target_bpr,int32 source_bpr,int32 start_y,int32 end_y,int32 amount_of_blur,int32 blur_also_alpha)
{
	// This function calculates the MotionBlur using a uniform kernel that has the same value in each cell.
	// The value is 1/(amount_of_blur*2.0+1)**2. It then loops for each pixel for y-value from
	// pixel.y-amount_of_blur to pixel.y+amount_of MotionBlur. Likewise for x-values.  	
	// This function leaves the alpha-channel untouched.
	source_bits += (start_y+MAX_BLUR_AMOUNT) * source_bpr;
	target_bits += start_y * target_bpr;

	float coeff = 1.0 / pow(((float)amount_of_blur*2.0 + 1.0),2);
	float red_value,green_value,blue_value,alpha_value;
	
	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta",0.0);

	BStopWatch *watch = new BStopWatch("MotionBlur Time");
//	int32 *dy_array = new int32[2*amount_of_blur+1];
//	for (int32 y=-amount_of_blur;y<=amount_of_blur;y++)
//		dy_array[y+amount_of_blur] = y*source_bpr;	
	if (blur_also_alpha == B_CONTROL_OFF) {
	 	for (int32 y=start_y;y<=end_y;y++) {
	 		source_bits += MAX_BLUR_AMOUNT;	
	 		for (int32 x=0;x<target_bpr;x++) {
				red_value = green_value = blue_value = 0.0;
				for (int32 i=0;i<number_of_deltas;i++) {
					red_value += ((*(source_bits  + deltas[i].dx+ deltas[i].dy*source_bpr)>>8) & 0xFF) * deltas[i].coeff;
					green_value += ((*(source_bits  +deltas[i].dx+ deltas[i].dy*source_bpr)>>16) & 0xFF) * deltas[i].coeff;
					blue_value += ((*(source_bits  +deltas[i].dx + deltas[i].dy*source_bpr)>>24) & 0xFF) * deltas[i].coeff;									
				}
	 			*target_bits++ = 	((int32)blue_value<<24) | 
	 								((int32)green_value<<16) |
	 								((int32)red_value<<8) |
	 								(*source_bits & 0xFF);
				source_bits++;
	 		}
	 		source_bits += MAX_BLUR_AMOUNT;
	 		// Send a progress-message if required
	 		if ((status_view != NULL) && (y%10 == 0)) {
	 			progress_message.ReplaceFloat("delta",(100.0*progress_step)/(float)(end_y-start_y)*10.0);
				status_view->Window()->PostMessage(&progress_message,status_view);
	 		}
	 	}
 	}
	else {
	 	for (int32 y=start_y;y<=end_y;y++) {
	 		source_bits += MAX_BLUR_AMOUNT;	
	 		for (int32 x=0;x<target_bpr;x++) {
				red_value = green_value = blue_value = alpha_value = 0.0;
				for (int32 i=0;i<number_of_deltas;i++) {
					alpha_value += ((*(source_bits  + deltas[i].dx+ deltas[i].dy*source_bpr)) & 0xFF) * deltas[i].coeff;
					red_value += ((*(source_bits  + deltas[i].dx+ deltas[i].dy*source_bpr)>>8) & 0xFF) * deltas[i].coeff;
					green_value += ((*(source_bits  +deltas[i].dx+ deltas[i].dy*source_bpr)>>16) & 0xFF) * deltas[i].coeff;
					blue_value += ((*(source_bits  +deltas[i].dx + deltas[i].dy*source_bpr)>>24) & 0xFF) * deltas[i].coeff;									
				}
	 			*target_bits++ = 	((int32)blue_value<<24) | 
	 								((int32)green_value<<16) |
	 								((int32)red_value<<8) |
	 								((int32)alpha_value);
				source_bits++;
	 		}
	 		source_bits += MAX_BLUR_AMOUNT;
	 		// Send a progress-message if required
	 		if ((status_view != NULL) && (y%10 == 0)) {
	 			progress_message.ReplaceFloat("delta",(100.0*progress_step)/(float)(end_y-start_y)*10.0);
				status_view->Window()->PostMessage(&progress_message,status_view);
	 		}
	 	}		
	}
	delete watch;
	return B_OK;
}


MotionBlurWindow::MotionBlurWindow(BView *target,MotionBlurManipulator *manipulator,int32 am, int32 dir, int32 alp)
	: BWindow(BRect(300,300,400,400),"MotionBlur",B_TITLED_WINDOW,B_NOT_RESIZABLE)
{
	target_view = target;
	the_manipulator = manipulator;
	BBox *container = new BBox(BRect(0,0,150,100));
	background = container;
	AddChild(container);
	ResizeTo(container->Bounds().Width(),container->Bounds().Height());

	// The message to slider must not be NULL, and it must contain an int32 named "value".
	// Also, the message is not copied to the control.
	BMessage *message = new BMessage(BLUR_AMOUNT_CHANGED);
	message->AddInt32("value",0);	
	ControlSliderBox *slider1 = new ControlSliderBox(BRect(4,4,146,44),"blur amount","Blur Amount","",message,1,MAX_BLUR_AMOUNT,B_PLAIN_BORDER,FALSE);
	container->AddChild(slider1); 
	slider1->setValue(am);
	BRect slider_frame = slider1->Frame();
	slider_frame.OffsetBy(0,slider_frame.Height()+4);
	
	BCheckBox *a_checkbox = new BCheckBox(slider_frame,"motion blur alpha","Blur Transparecy",new BMessage(BLUR_TRANSPARENCY_CHANGED));
	a_checkbox->ResizeToPreferred();
	a_checkbox->SetValue(alp);
	container->AddChild(a_checkbox);
	slider_frame = slider1->Frame();
	slider_frame.OffsetBy(0,a_checkbox->Frame().bottom - slider_frame.top+EXTRA_EDGE);
	
	DirectionControlBox *d_control = new DirectionControlBox(slider_frame,"motion blur direction","Blur Direction",new BMessage(BLUR_DIRECTION_CHANGED));
	d_control->setValue(dir);
	container->AddChild(d_control);
	slider_frame = d_control->Frame();
	slider_frame.OffsetBy(0,slider_frame.Height()+4);
	
	message = new BMessage(HS_OPERATION_FINISHED);
	message->AddBool("status",FALSE);
	cancel_button = new BButton(slider_frame,"motion blur cancel button","Cancel",message);
	container->AddChild(cancel_button);
	cancel_button->ResizeToPreferred();
	
	slider_frame = cancel_button->Frame();
	slider_frame.OffsetBy(slider_frame.Width()+4,0);

	message = new BMessage(HS_OPERATION_FINISHED);
	message->AddBool("status",TRUE);
	ok_button = new BButton(slider_frame,"motion blur accept button","OK",message);
	container->AddChild(ok_button);
	ok_button->ResizeToPreferred();


	preview_progress_bar = new BStatusBar(BRect(cancel_button->Frame().LeftTop(),ok_button->Frame().RightBottom()),"MotionBlur preview progress bar","Calculating Preview");
	ResizeTo(Bounds().Width(),ok_button->Frame().bottom + 4);
	container->ResizeTo(Bounds().Width(),ok_button->Frame().bottom + 4);
	container->AddChild(preview_progress_bar);
	preview_progress_bar->RemoveSelf();	
	preview_progress_bar->SetBarHeight(16 - (preview_progress_bar->Frame().bottom-ok_button->Frame().bottom));		

	status = FALSE;	
}


MotionBlurWindow::~MotionBlurWindow()
{
	BMessage *a_message = new BMessage(HS_OPERATION_FINISHED);
	a_message->AddBool("status",status);
	target_view->Window()->PostMessage(a_message,target_view);
	delete a_message;
}



void MotionBlurWindow::MessageReceived(BMessage *message)
{
	BLooper *source;
	message->FindPointer("source",&source);
	
	switch (message->what) {
		case BLUR_AMOUNT_CHANGED:
			int32 blur_amount = message->FindInt32("value");
			the_manipulator->ChangeValue(blur_amount);
			break;
		case BLUR_TRANSPARENCY_CHANGED:
			BCheckBox *check_box;
			check_box = cast_as(source,BCheckBox);
			if (check_box != NULL) {
				the_manipulator->ChangeTransparency(check_box->Value());
			}
			break;		
		case BLUR_DIRECTION_CHANGED:
			BControl *control;
			control = cast_as(source,BControl);
			if (control != NULL) {
				the_manipulator->ChangeDirection(control->Value());
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

BView* MotionBlurWindow::DisplayProgressBar()
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

void MotionBlurWindow::DisplayConfirmButtons()
{
	Lock();
	if (preview_progress_bar->Parent() == background) {
		preview_progress_bar->RemoveSelf();
		background->AddChild(ok_button);
		background->AddChild(cancel_button);
	}
	Unlock();
}