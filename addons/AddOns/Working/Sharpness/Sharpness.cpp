/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <LayoutBuilder.h>
#include <Slider.h>
#include <StatusBar.h>
#include <StopWatch.h>
#include <string.h>
#include <Window.h>

#include "AddOns.h"
#include "ManipulatorInformer.h"
#include "Sharpness.h"
#include "ImageProcessingLibrary.h"
#include "Selection.h"
#include "SysInfoBeOS.h"

#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = "Sharpness…";
	char menu_help_string[255] = "Adjust the sharpness.";
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = COLOR_ADD_ON;
#ifdef __cplusplus
}
#endif

Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new SharpnessManipulator(bm);
}



SharpnessManipulator::SharpnessManipulator(BBitmap *bm)
		: WindowGUIManipulator()
{
	preview_bitmap = NULL;
	config_view = NULL;
	copy_of_the_preview_bitmap = NULL;
	blurred_image = NULL;

	previous_settings.sharpness = settings.sharpness + 1;

	system_info info;
	get_system_info(&info);
	processor_count = info.cpu_count;

	ipLibrary = new ImageProcessingLibrary();

	SetPreviewBitmap(bm);
}


SharpnessManipulator::~SharpnessManipulator()
{
	delete copy_of_the_preview_bitmap;
	delete config_view;
	delete blurred_image;

	delete ipLibrary;
}


BBitmap* SharpnessManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	SharpnessManipulatorSettings *new_settings = dynamic_cast<SharpnessManipulatorSettings*>(set);

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

	delete blurred_image;
	blurred_image = DuplicateBitmap(source_bitmap,0);
	ipLibrary->gaussian_blur(blurred_image,BLUR_AMOUNT);
//	CalculateLuminanceImage(source_bitmap);

	start_threads();

	return target_bitmap;
}


int32 SharpnessManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
{
	/*
		With floating-point arithmetic this function takes about 0.12s on
		a 2*350PII with image size of 400 * 540. With fixed point the time is
		about 0.035s.
	*/

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


void SharpnessManipulator::start_threads()
{
	thread_id *threads = new thread_id[processor_count];

	for (int32 i=0;i<processor_count;i++) {
		threads[i] = spawn_thread(thread_entry,"sharpness_thread",B_NORMAL_PRIORITY,this);
		resume_thread(threads[i]);
		send_data(threads[i],i,NULL,0);
	}

	for (int32 i=0;i<processor_count;i++) {
		int32 return_value;
		wait_for_thread(threads[i],&return_value);
	}

	delete[] threads;
}

int32 SharpnessManipulator::thread_entry(void *data)
{
	int32 thread_number;
	thread_number = receive_data(NULL,NULL,0);

	SharpnessManipulator *this_pointer = (SharpnessManipulator*)data;

	return this_pointer->thread_function(thread_number);
}


int32 SharpnessManipulator::thread_function(int32 thread_number)
{
	// This function interpolates the image with a degenerate version,
	// which in this case is the luminance image. The luminance image
	// is in bitmap blurred_image, which is in B_CMAP8 color-space.

	int32 step = current_resolution;
	float sharpness_coeff = current_settings.sharpness / 100.0;

	BWindow *progress_bar_window = NULL;
	if (progress_bar != NULL)
		progress_bar_window = progress_bar->Window();


	uint32 *source_bits = (uint32*)source_bitmap->Bits();
	uint32 *target_bits = (uint32*)target_bitmap->Bits();
	int32 source_bpr = source_bitmap->BytesPerRow()/4;
	int32 target_bpr = target_bitmap->BytesPerRow()/4;
	uint32 *blurred_bits = (uint32*)blurred_image->Bits();
	uint32 blurred_bpr = blurred_image->BytesPerRow()/4;

	// This union must be used to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} color,b_color;

	float one_minus_coeff = 1.0 - sharpness_coeff;

	int32 coeff_fixed = 32768 * sharpness_coeff;
	int32 one_minus_coeff_fixed = 32768 * one_minus_coeff;

	if (current_selection->IsEmpty()) {
		// Here handle the whole image.
		int32 left = target_bitmap->Bounds().left;
		int32 right = target_bitmap->Bounds().right;
		int32 top = target_bitmap->Bounds().top;
		int32 bottom = target_bitmap->Bounds().bottom;

		float height = bottom - top;
		top = height/processor_count*thread_number;
		top = ceil(top/(float)step);
		top *= step;
		bottom = min_c(bottom,top + (height+1)/processor_count);
		int32 update_interval = 10;
		float update_amount = 100.0/(bottom-top)*update_interval/(float)processor_count;
		float missed_update = 0;

		// Loop through all pixels in original.
		uint32 sum;
		for (int32 y=top;y<=bottom;y+=step) {
			int32 y_times_source_bpr = y*source_bpr;
			int32 y_times_target_bpr = y*target_bpr;
			int32 y_times_blurred_bpr = y*blurred_bpr;
			for (int32 x=left;x<=right;x+=step) {
				color.word = *(source_bits + x + y_times_source_bpr);
				b_color.word = *(blurred_bits + x + y_times_blurred_bpr);
				color.bytes[0] = max_c(0,min_c(255,((color.bytes[0] * coeff_fixed)>>15) + ((b_color.bytes[0] * one_minus_coeff_fixed)>>15)));
				color.bytes[1] = max_c(0,min_c(255,((color.bytes[1] * coeff_fixed)>>15) + ((b_color.bytes[1] * one_minus_coeff_fixed)>>15)));
				color.bytes[2] = max_c(0,min_c(255,((color.bytes[2] * coeff_fixed)>>15) + ((b_color.bytes[2] * one_minus_coeff_fixed)>>15)));
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
		top += height/processor_count*thread_number;
		top *= step;
		top /= step;

		bottom = min_c(bottom,top + (height+1)/processor_count);

		int32 update_interval = 10;
		float update_amount = 100.0/(bottom-top)*update_interval/(float)processor_count;

		// Loop through all pixels in original.
		for (int32 y=top;y<=bottom;y+=step) {
			int32 y_times_source_bpr = y*source_bpr;
			int32 y_times_target_bpr = y*target_bpr;
			int32 y_times_blurred_bpr = y*blurred_bpr;
			for (int32 x=left;x<=right;x+=step) {
				if (current_selection->ContainsPoint(x,y)) {
					color.word = *(source_bits + x + y_times_source_bpr);
					b_color.word = *(blurred_bits + x + y_times_blurred_bpr);
					color.bytes[0] = max_c(0,min_c(255,((color.bytes[0] * coeff_fixed)>>15) + ((b_color.bytes[0] * one_minus_coeff_fixed)>>15)));
					color.bytes[1] = max_c(0,min_c(255,((color.bytes[1] * coeff_fixed)>>15) + ((b_color.bytes[1] * one_minus_coeff_fixed)>>15)));
					color.bytes[2] = max_c(0,min_c(255,((color.bytes[2] * coeff_fixed)>>15) + ((b_color.bytes[2] * one_minus_coeff_fixed)>>15)));
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


void SharpnessManipulator::MouseDown(BPoint point,uint32,BView*,bool first_click)
{
	// This function does nothing in SharpnessManipulator.
}


void SharpnessManipulator::SetPreviewBitmap(BBitmap *bm)
{
	if (preview_bitmap != bm) {
		delete copy_of_the_preview_bitmap;
		delete blurred_image;

		if (bm != NULL) {
			preview_bitmap = bm;
			copy_of_the_preview_bitmap = DuplicateBitmap(bm,0);
			blurred_image = DuplicateBitmap(preview_bitmap,0);
			ipLibrary->gaussian_blur(blurred_image,settings.blur_size,processor_count);
		}
		else {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
			blurred_image = NULL;
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
}


void SharpnessManipulator::Reset(Selection*)
{
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with a loop.
		uint32 *source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target,source,bits_length);
	}
}

BView* SharpnessManipulator::MakeConfigurationView(const BMessenger& target)
{
	if (config_view == NULL) {
		config_view = new SharpnessManipulatorView(this, target);
		config_view->ChangeSettings(&settings);
	}

	return config_view;
}


ManipulatorSettings* SharpnessManipulator::ReturnSettings()
{
	return new SharpnessManipulatorSettings(settings);
}

void SharpnessManipulator::ChangeSettings(ManipulatorSettings *s)
{
	SharpnessManipulatorSettings *new_settings;
	new_settings = dynamic_cast<SharpnessManipulatorSettings*>(s);
	if (new_settings != NULL) {
		if (new_settings->blur_size != settings.blur_size) {
			int32 *d_bits = (int32*)blurred_image->Bits();
			int32 *s_bits = (int32*)copy_of_the_preview_bitmap->Bits();
			int32 bits_length = blurred_image->BitsLength();

			memcpy(d_bits,s_bits,bits_length);
			ipLibrary->gaussian_blur(blurred_image,new_settings->blur_size,processor_count);
		}

		settings = *new_settings;
	}
}

char* SharpnessManipulator::ReturnName()
{
	return "Sharpness";
}

char* SharpnessManipulator::ReturnHelpString()
{
	return "Use the slider to set the image sharpness.";
}




// -------------------------------------
SharpnessManipulatorView::SharpnessManipulatorView(SharpnessManipulator *manip,
		const BMessenger& t)
	: WindowGUIManipulatorView()
{
	target = t;
	manipulator = manip;
	started_adjusting = FALSE;

	sharpness_slider = new BSlider("sharpness_slider",
		"Sharpness:", new BMessage(SHARPNESS_ADJUSTING_FINISHED), 0, 255,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	sharpness_slider->SetModificationMessage(new BMessage(SHARPNESS_ADJUSTED));
	sharpness_slider->SetLimitLabels("Blurred","Sharp");
	sharpness_slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	sharpness_slider->SetHashMarkCount(11);

	blur_size_slider = new BSlider("blur_size_slider",
		"Effect strength:", new BMessage(BLUR_ADJUSTING_FINISHED), 1, 50,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	blur_size_slider->SetLimitLabels("Low","High");
	blur_size_slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	blur_size_slider->SetHashMarkCount(11);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_ITEM_SPACING)
		.Add(sharpness_slider)
		.Add(blur_size_slider)
		.SetInsets(B_USE_SMALL_INSETS)
		.End();
}

SharpnessManipulatorView::~SharpnessManipulatorView()
{
}

void SharpnessManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();
	sharpness_slider->SetTarget(BMessenger(this));
	blur_size_slider->SetTarget(BMessenger(this));
}

void SharpnessManipulatorView::AllAttached()
{
	sharpness_slider->SetValue(settings.sharpness);
	blur_size_slider->SetValue(settings.blur_size);
}

void SharpnessManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case SHARPNESS_ADJUSTED:
			settings.sharpness = sharpness_slider->Value();
			manipulator->ChangeSettings(&settings);
			if (!started_adjusting) {
				target.SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
				started_adjusting = TRUE;
			}
			break;

		case SHARPNESS_ADJUSTING_FINISHED:
			started_adjusting = FALSE;
			settings.sharpness = sharpness_slider->Value();
			manipulator->ChangeSettings(&settings);
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case BLUR_ADJUSTING_FINISHED:
			settings.blur_size = blur_size_slider->Value();
			manipulator->ChangeSettings(&settings);
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}

void SharpnessManipulatorView::ChangeSettings(ManipulatorSettings *set)
{
	SharpnessManipulatorSettings *new_settings = dynamic_cast<SharpnessManipulatorSettings*>(set);

	if (set != NULL) {
		settings = *new_settings;

		BWindow *window = Window();
		if (window != NULL) {
			window->Lock();
			sharpness_slider->SetValue(settings.sharpness);
			blur_size_slider->SetValue(settings.blur_size);
			window->Unlock();
		}
	}
}
