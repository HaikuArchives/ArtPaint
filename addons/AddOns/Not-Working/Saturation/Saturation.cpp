/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <StatusBar.h>
#include <Slider.h>
#include <string.h>
#include <Window.h>

#include "AddOns.h"
#include "ManipulatorInformer.h"
#include "Saturation.h"
#include "Selection.h"

#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = "Saturation…";
	char menu_help_string[255] = "Starts adjusting the image saturation.";
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = COLOR_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new SaturationManipulator(bm);
}



SaturationManipulator::SaturationManipulator(BBitmap *bm)
		: WindowGUIManipulator()
{
	preview_bitmap = NULL;
	config_view = NULL;
	copy_of_the_preview_bitmap = NULL;
	luminance_image = NULL;

	previous_settings.saturation = settings.saturation + 1;

	SetPreviewBitmap(bm);
}


SaturationManipulator::~SaturationManipulator()
{
	delete copy_of_the_preview_bitmap;
	delete config_view;
	delete luminance_image;
}


BBitmap* SaturationManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	SaturationManipulatorSettings *new_settings = dynamic_cast<SaturationManipulatorSettings*>(set);

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

	delete luminance_image;
	luminance_image = new BBitmap(source_bitmap->Bounds(),B_CMAP8);
	CalculateLuminanceImage(source_bitmap);

	start_threads();

	return target_bitmap;
}


void SaturationManipulator::CalculateLuminanceImage(BBitmap *bitmap)
{
	uint8 *luminance_bits = (uint8*)luminance_image->Bits();
	uint32 luminance_bpr = luminance_image->BytesPerRow();

	uint32 *bits = (uint32*)bitmap->Bits();
	uint32 bpr = bitmap->BytesPerRow()/4;

	int32 width = bitmap->Bounds().Width();
	int32 height = bitmap->Bounds().Height();

	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	for (int32 y=0;y<=height;y++) {
		int32 y_times_bpr = y*bpr;
		int32 y_times_luminance_bpr = y*luminance_bpr;
		for (int32 x=0;x<=width;x++) {
			color.word = *(bits + x + y_times_bpr);
			uint8 luminance;
			luminance = min_c(255,max_c(0,0.144 * color.bytes[0] + 0.587 * color.bytes[1] + 0.299 * color.bytes[2]));
			*(luminance_bits + x + y_times_luminance_bpr) = luminance;
		}
	}
}

int32 SaturationManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
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


void SaturationManipulator::start_threads()
{
	system_info info;
	get_system_info(&info);
	number_of_threads = info.cpu_count;

	thread_id *threads = new thread_id[number_of_threads];

	for (int32 i=0;i<number_of_threads;i++) {
		threads[i] = spawn_thread(thread_entry,"saturation_thread",B_NORMAL_PRIORITY,this);
		resume_thread(threads[i]);
		send_data(threads[i],i,NULL,0);
	}

	for (int32 i=0;i<number_of_threads;i++) {
		int32 return_value;
		wait_for_thread(threads[i],&return_value);
	}

	delete[] threads;
}

int32 SaturationManipulator::thread_entry(void *data)
{
	int32 thread_number;
	thread_number = receive_data(NULL,NULL,0);

	SaturationManipulator *this_pointer = (SaturationManipulator*)data;

	return this_pointer->thread_function(thread_number);
}


int32 SaturationManipulator::thread_function(int32 thread_number)
{
	// This function interpolates the image with a degenerate version,
	// which in this case is the luminance image. The luminance image
	// is in bitmap luminance_image, which is in B_CMAP8 color-space.

	int32 step = current_resolution;
	uint32 saturation = settings.saturation;

	BWindow *progress_bar_window = NULL;
	if (progress_bar != NULL)
		progress_bar_window = progress_bar->Window();


	uint32 *source_bits = (uint32*)source_bitmap->Bits();
	uint32 *target_bits = (uint32*)target_bitmap->Bits();
	int32 source_bpr = source_bitmap->BytesPerRow()/4;
	int32 target_bpr = target_bitmap->BytesPerRow()/4;
	uint8 *luminance_bits = (uint8*)luminance_image->Bits();
	uint32 luminance_bpr = luminance_image->BytesPerRow();

	// This union must be used to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	float coeff = current_settings.saturation / 100.0;
	float one_minus_coeff = 1.0 - coeff;

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
		uint32 sum;
		saturation *= 3;
		for (int32 y=top;y<=bottom;y+=step) {
			int32 y_times_source_bpr = y*source_bpr;
			int32 y_times_target_bpr = y*target_bpr;
			int32 y_times_luminance_bpr = y*luminance_bpr;
			uint8 luminance;
			for (int32 x=left;x<=right;x+=step) {
				color.word = *(source_bits + x + y_times_source_bpr);
				luminance = *(luminance_bits + x + y_times_luminance_bpr);
				color.bytes[0] = max_c(0,min_c(255,color.bytes[0] * coeff + luminance*one_minus_coeff));
				color.bytes[1] = max_c(0,min_c(255,color.bytes[1] * coeff + luminance*one_minus_coeff));
				color.bytes[2] = max_c(0,min_c(255,color.bytes[2] * coeff + luminance*one_minus_coeff));
				*(target_bits + x + y_times_target_bpr) = color.word;
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
		for (int32 y=top;y<=bottom;y+=step) {
			int32 y_times_source_bpr = y*source_bpr;
			int32 y_times_target_bpr = y*target_bpr;
			int32 y_times_luminance_bpr = y*luminance_bpr;
			uint8 luminance;
			for (int32 x=left;x<=right;x+=step) {
				if (current_selection->ContainsPoint(x,y)) {
					color.word = *(source_bits + x + y_times_source_bpr);
					luminance = *(luminance_bits + x + y_times_luminance_bpr);
					color.bytes[0] = max_c(0,min_c(255,color.bytes[0] * coeff + luminance*one_minus_coeff));
					color.bytes[1] = max_c(0,min_c(255,color.bytes[1] * coeff + luminance*one_minus_coeff));
					color.bytes[2] = max_c(0,min_c(255,color.bytes[2] * coeff + luminance*one_minus_coeff));
					*(target_bits + x + y_times_target_bpr) = color.word;
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


void SaturationManipulator::MouseDown(BPoint point,uint32,BView*,bool first_click)
{
	// This function does nothing in SaturationManipulator.
}


void SaturationManipulator::SetPreviewBitmap(BBitmap *bm)
{
	if (preview_bitmap != bm) {
		delete copy_of_the_preview_bitmap;
		delete luminance_image;

		if (bm != NULL) {
			preview_bitmap = bm;
			copy_of_the_preview_bitmap = DuplicateBitmap(bm,0);
			luminance_image = new BBitmap(preview_bitmap->Bounds(),B_CMAP8);
			CalculateLuminanceImage(preview_bitmap);
		}
		else {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
			luminance_image = NULL;
		}
	}

	if (preview_bitmap != NULL) {
		system_info info;
		get_system_info(&info);
		double speed = info.cpu_count * 2000; // TODO: used to be info.cpu_clock_speed but was removed

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
}


void SaturationManipulator::Reset(Selection*)
{
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with a loop.
		uint32 *source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target,source,bits_length);
	}
}

BView* SaturationManipulator::MakeConfigurationView(const BMessenger& target)
{
	if (config_view == NULL) {
		config_view = new SaturationManipulatorView(this,target);
		config_view->ChangeSettings(&settings);
	}

	return config_view;
}


ManipulatorSettings* SaturationManipulator::ReturnSettings()
{
	return new SaturationManipulatorSettings(settings);
}

void SaturationManipulator::ChangeSettings(ManipulatorSettings *s)
{
	SaturationManipulatorSettings *new_settings;
	new_settings = dynamic_cast<SaturationManipulatorSettings*>(s);

	if (new_settings != NULL) {
		settings = *new_settings;
	}
}

char* SaturationManipulator::ReturnName()
{
	return "Saturation";
}

char* SaturationManipulator::ReturnHelpString()
{
	return "Use the slider to set the image saturation.";
}




// -------------------------------------
SaturationManipulatorView::SaturationManipulatorView(SaturationManipulator *manip,
		const BMessenger& t)
	: WindowGUIManipulatorView()
{
	target = t;
	manipulator = manip;
	started_adjusting = FALSE;

	saturation_slider = new BSlider(BRect(0,0,200,0), "saturation_slider",
		"Saturation", new BMessage(SATURATION_ADJUSTING_FINISHED), 0, 255,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	saturation_slider->SetModificationMessage(new BMessage(SATURATION_ADJUSTED));
	saturation_slider->SetLimitLabels("Low","High");
	saturation_slider->ResizeToPreferred();
	saturation_slider->MoveTo(4,4);
	AddChild(saturation_slider);

	ResizeTo(saturation_slider->Bounds().Width()+8,saturation_slider->Bounds().Height()+8);
}

SaturationManipulatorView::~SaturationManipulatorView()
{
}

void SaturationManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();
	saturation_slider->SetTarget(BMessenger(this));
}

void SaturationManipulatorView::AllAttached()
{
	saturation_slider->SetValue(settings.saturation);
}

void SaturationManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case SATURATION_ADJUSTED:
			settings.saturation = saturation_slider->Value();
			manipulator->ChangeSettings(&settings);
			if (!started_adjusting) {
				target.SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
				started_adjusting = TRUE;
			}
			break;

		case SATURATION_ADJUSTING_FINISHED:
			started_adjusting = FALSE;
			settings.saturation = saturation_slider->Value();
			manipulator->ChangeSettings(&settings);
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}

void SaturationManipulatorView::ChangeSettings(ManipulatorSettings *set)
{
	SaturationManipulatorSettings *new_settings = dynamic_cast<SaturationManipulatorSettings*>(set);

	if (set != NULL) {
		settings = *new_settings;

		BWindow *window = Window();
		if (window != NULL) {
			window->Lock();
			saturation_slider->SetValue(settings.saturation);
			window->Unlock();
		}
	}
}
