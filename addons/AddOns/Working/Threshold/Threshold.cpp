/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */


/*
	This file shows how to create an add-on that creates a GUI.
	This also shows how the ManipulatorInformer-class is used.
*/

#include <Bitmap.h>
#include <Node.h>
#include <StatusBar.h>
#include <stdio.h>
#include <Window.h>

#include "AddOns.h"
#include "ManipulatorInformer.h"
#include "Selection.h"
#include "Threshold.h"
#include "ThresholdView.h"
#include "SysInfoBeOS.h"


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = "Threshold…";
	char menu_help_string[255] = "Starts thresholding the image.";
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = COLOR_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	return new ThresholdManipulator(bm,i);
}



ThresholdManipulator::ThresholdManipulator(BBitmap *bm,ManipulatorInformer *i)
		: WindowGUIManipulator()
{
	preview_bitmap = NULL;
	config_view = NULL;
	copy_of_the_preview_bitmap = NULL;

	previous_settings.threshold = settings.threshold + 1;
	settings.mode = HISTOGRAM_MODE_INTENSITY;

	SetPreviewBitmap(bm);

	light_color = i->GetForegroundColor();
	dark_color = i->GetBackgroundColor();

	delete i;	// This is not needed anymore
}


ThresholdManipulator::~ThresholdManipulator()
{
	delete copy_of_the_preview_bitmap;
	delete config_view;
}


BBitmap* ThresholdManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	ThresholdManipulatorSettings *new_settings = dynamic_cast<ThresholdManipulatorSettings*>(set);

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	if (original == preview_bitmap) {
		if ((*new_settings == previous_settings) && (last_calculated_resolution <= 1))
			return original;

		source_bitmap = copy_of_the_preview_bitmap;
		target_bitmap = original;
	}
	else {
		source_bitmap = original;
		target_bitmap = new BBitmap(original->Bounds(),B_RGB32,FALSE);
	}


	current_resolution = 1;
	current_selection = selection;
	current_settings = *new_settings;
	progress_bar = status_bar;

	start_threads();

	return target_bitmap;
}

int32 ThresholdManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
{
	progress_bar = NULL;
	current_selection = selection;
	if (settings == previous_settings ) {
		if ((last_calculated_resolution != highest_available_quality) && (last_calculated_resolution > 0))
			last_calculated_resolution = max_c(highest_available_quality,floor(last_calculated_resolution/2.0));
		else
			last_calculated_resolution = 0;
	}
	else
		last_calculated_resolution = lowest_available_quality;

	if (full_quality) {
		last_calculated_resolution = min_c(1,last_calculated_resolution);
	}
	previous_settings = settings;

	if (last_calculated_resolution > 0) {
		current_resolution = last_calculated_resolution;
		updated_region->Set(preview_bitmap->Bounds());

		target_bitmap = preview_bitmap;
		source_bitmap = copy_of_the_preview_bitmap;
		current_settings = settings;

		start_threads();
	}

	return last_calculated_resolution;
}


void ThresholdManipulator::start_threads()
{
	system_info info;
	get_system_info(&info);
	number_of_threads = info.cpu_count;

	thread_id *threads = new thread_id[number_of_threads];

	for (int32 i=0;i<number_of_threads;i++) {
		threads[i] = spawn_thread(thread_entry,"threshold_thread",B_NORMAL_PRIORITY,this);
		resume_thread(threads[i]);
		send_data(threads[i],i,NULL,0);
	}

	for (int32 i=0;i<number_of_threads;i++) {
		int32 return_value;
		wait_for_thread(threads[i],&return_value);
	}

	delete[] threads;
}

int32 ThresholdManipulator::thread_entry(void *data)
{
	int32 thread_number;
	thread_number = receive_data(NULL,NULL,0);

	ThresholdManipulator *this_pointer = (ThresholdManipulator*)data;

	return this_pointer->thread_function(thread_number);
}


int32 ThresholdManipulator::thread_function(int32 thread_number)
{
	int32 step = current_resolution;
	uint32 threshold = settings.threshold;
	int32 mode = settings.mode;

	BWindow *progress_bar_window = NULL;
	if (progress_bar != NULL)
		progress_bar_window = progress_bar->Window();


	uint32 *source_bits = (uint32*)source_bitmap->Bits();
	uint32 *target_bits = (uint32*)target_bitmap->Bits();
	int32 source_bpr = source_bitmap->BytesPerRow()/4;
	int32 target_bpr = target_bitmap->BytesPerRow()/4;

	// This union must be used to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} color,dark,light;

	dark.bytes[0] = dark_color.blue;
	dark.bytes[1] = dark_color.green;
	dark.bytes[2] = dark_color.red;
	dark.bytes[3] = dark_color.alpha;

	light.bytes[0] = light_color.blue;
	light.bytes[1] = light_color.green;
	light.bytes[2] = light_color.red;
	light.bytes[3] = light_color.alpha;

	if (current_selection->IsEmpty()) {
		// Here handle the whole image.
		int32 left = target_bitmap->Bounds().left;
		int32 right = target_bitmap->Bounds().right;
		int32 top = target_bitmap->Bounds().top;
		int32 bottom = target_bitmap->Bounds().bottom;

		float height = bottom - top;
		top = height/number_of_threads*thread_number;
		top = ceil(top/(float)step);
		top *= step;
		bottom = min_c(bottom,top + (height+1)/number_of_threads);
		int32 update_interval = 10;
		float update_amount = 100.0/(bottom-top)*update_interval/(float)number_of_threads;
		float missed_update = 0;

		// Loop through all pixels in original.
		uint32 value;

		for (int32 y=top;y<=bottom;y+=step) {
			int32 y_times_source_bpr = y*source_bpr;
			int32 y_times_target_bpr = y*target_bpr;
			for (int32 x=left;x<=right;x+=step) {
				color.word = *(source_bits + x + y_times_source_bpr);

				if (mode == HISTOGRAM_MODE_INTENSITY)
					value = (0.114*color.bytes[0] + 0.587*color.bytes[1] + 0.299*color.bytes[2]);
				else if (mode == HISTOGRAM_MODE_RED)
					value = color.bytes[2];
				else if (mode == HISTOGRAM_MODE_GREEN)
					value = color.bytes[1];
				else if (mode == HISTOGRAM_MODE_BLUE)
					value = color.bytes[0];

				*(target_bits + x + y_times_target_bpr) = ((value < threshold) ? dark.word : light.word);
			}


			// Update the status-bar
			if ( ((y % update_interval) == 0) && (progress_bar_window != NULL) && (progress_bar_window->LockWithTimeout(0) == B_OK) ) {
				progress_bar->Update(update_amount+missed_update);
				progress_bar_window->Unlock();
				missed_update = 0;
			}
			else if ((y % update_interval) == 0) {
				missed_update += update_amount;
			}
		}
	}
	else {
		// Here handle only those pixels for which selection->ContainsPoint(x,y) is true.
		BRect rect = current_selection->GetBoundingRect();

		int32 left = rect.left;
		int32 right = rect.right;
		int32 top = rect.top;
		int32 bottom = rect.bottom;

		float height = bottom - top;
		top += height/number_of_threads*thread_number;
		top *= step;
		top /= step;

		bottom = min_c(bottom,top + (height+1)/number_of_threads);

		int32 update_interval = 10;
		float update_amount = 100.0/(bottom-top)*update_interval/(float)number_of_threads;

		// Loop through all pixels in original.
		uint32 value;

		for (int32 y=top;y<=bottom;y+=step) {
			int32 y_times_source_bpr = y*source_bpr;
			int32 y_times_target_bpr = y*target_bpr;
			for (int32 x=left;x<=right;x+=step) {
				if (current_selection->ContainsPoint(x,y)) {	// Adjust only the pixels inside the selection.
					color.word = *(source_bits + x + y_times_source_bpr);

					if (mode == HISTOGRAM_MODE_INTENSITY)
						value = (0.114*color.bytes[0] + 0.587*color.bytes[1] + 0.299*color.bytes[2]);
					else if (mode == HISTOGRAM_MODE_RED)
						value = color.bytes[2];
					else if (mode == HISTOGRAM_MODE_GREEN)
						value = color.bytes[1];
					else if (mode == HISTOGRAM_MODE_BLUE)
						value = color.bytes[0];

					*(target_bits + x + y_times_target_bpr) = ((value < threshold) ? dark.word : light.word);
				}
			}

			// Update the status-bar
			if ( ((y % update_interval) == 0) && (progress_bar_window != NULL) && (progress_bar_window->LockWithTimeout(0) == B_OK) ) {
				progress_bar->Update(update_amount);
				progress_bar_window->Unlock();
			}
		}
	}

	return B_OK;
}


void ThresholdManipulator::MouseDown(BPoint point,uint32,BView*,bool first_click)
{
	// This function does nothing in ThresholdManipulator.
}


void ThresholdManipulator::SetPreviewBitmap(BBitmap *bm)
{
	if (preview_bitmap != bm) {
		delete copy_of_the_preview_bitmap;
		if (bm != NULL) {
			preview_bitmap = bm;
			copy_of_the_preview_bitmap = DuplicateBitmap(bm,0);
		}
		else {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
		}
	}

	if (preview_bitmap != NULL) {
		BeOS_system_info info;
		get_BeOS_system_info(&info);
		double speed = info.cpu_count * info.cpu_clock_speed;

		// Let's select a resolution that can handle all the pixels at least
		// 10 times in a second while assuming that one pixel calculation takes
		// about 50 CPU cycles.
		speed = speed / (10*50);
		BRect bounds = preview_bitmap->Bounds();
		float num_pixels = (bounds.Width()+1) * (bounds.Height() + 1);
		lowest_available_quality = 1;
		while ((num_pixels/lowest_available_quality/lowest_available_quality) > speed)
			lowest_available_quality *= 2;

		lowest_available_quality = min_c(lowest_available_quality,16);
		highest_available_quality = max_c(lowest_available_quality/2,1);
	}
	else {
		lowest_available_quality = 1;
		highest_available_quality = 1;
	}
	last_calculated_resolution = lowest_available_quality;

	if (config_view != NULL)
		config_view->SetBitmap(copy_of_the_preview_bitmap);
}


void ThresholdManipulator::Reset(Selection*)
{
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with a loop.
		uint32 *source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target,source,bits_length);
	}
}

BView* ThresholdManipulator::MakeConfigurationView(const BMessenger& target)
{
	if (config_view == NULL) {
		config_view = new ThresholdManipulatorView(this,target);
		config_view->ChangeSettings(&settings);
		config_view->SetBitmap(copy_of_the_preview_bitmap);
	}

	return config_view;
}


ManipulatorSettings* ThresholdManipulator::ReturnSettings()
{
	return new ThresholdManipulatorSettings(settings);
}

void ThresholdManipulator::ChangeSettings(ManipulatorSettings *s)
{
	ThresholdManipulatorSettings *new_settings;
	new_settings = dynamic_cast<ThresholdManipulatorSettings*>(s);
	if (new_settings != NULL) {
		settings = *new_settings;
	}
}

char* ThresholdManipulator::ReturnName()
{
	return "Threshold";
}

char* ThresholdManipulator::ReturnHelpString()
{
	return "Use the slider to set the thresholding value.";
}


status_t ThresholdManipulator::WriteSettings(BNode *node)
{
	ssize_t written = node->WriteAttr("threshold", B_INT32_TYPE, 0,
		&(settings.threshold), sizeof(int32));

	if (written == ssize_t(sizeof(int32)))
		return B_OK;
	return B_ERROR;
}

status_t ThresholdManipulator::ReadSettings(BNode *node)
{
	ssize_t read = node->ReadAttr("threshold", B_INT32_TYPE, 0,
		&(settings.threshold), sizeof(int32));

	if (read == ssize_t(sizeof(int32)))
		return B_OK;
	return B_ERROR;
}


// -------------------------------------
ThresholdManipulatorView::ThresholdManipulatorView(ThresholdManipulator *manip,
		const BMessenger& t)
	: WindowGUIManipulatorView()
{
	target = t;
	manipulator = manip;
	started_adjusting = FALSE;

	threshold_control = new ThresholdView(BRect(0,0,0,0),new BMessage(THRESHOLD_ADJUSTING_FINISHED));
	threshold_control->MoveTo(4,4);
	AddChild(threshold_control);

	ResizeTo(threshold_control->Bounds().Width()+8,threshold_control->Bounds().Height()+8);
}


ThresholdManipulatorView::~ThresholdManipulatorView()
{
}


void ThresholdManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();
	threshold_control->SetTarget(BMessenger(this));
}


void ThresholdManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case THRESHOLD_ADJUSTED:
			settings.threshold = threshold_control->Value();
			manipulator->ChangeSettings(&settings);
			if (!started_adjusting) {
				target.SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
				started_adjusting = TRUE;
			}
			break;

		case THRESHOLD_ADJUSTING_FINISHED: {
				started_adjusting = false;
				int32 threshold = 0;
				int32 mode = 0;
				message->FindInt32("threshold",&threshold);
				message->FindInt32("mode",&mode);

				settings.threshold = threshold;
				settings.mode = mode;
				manipulator->ChangeSettings(&settings);
				target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			}
			break;

		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}


void ThresholdManipulatorView::ChangeSettings(ManipulatorSettings *newSettings)
{
	ThresholdManipulatorSettings *s = dynamic_cast<ThresholdManipulatorSettings*>(newSettings);

	if (s != NULL) {
		settings.threshold = s->threshold;
		if (threshold_control != NULL) {
			if (threshold_control->LockLooper()) {
				threshold_control->SetValue(settings.threshold);
				threshold_control->UnlockLooper();
			}
			else {
				threshold_control->SetValue(settings.threshold);
			}
		}
	}
}

void ThresholdManipulatorView::SetBitmap(BBitmap *bitmap)
{
	threshold_control->SetBitmap(bitmap);
}
