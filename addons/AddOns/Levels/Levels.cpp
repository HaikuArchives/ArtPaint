/*
 * Copyright 2026, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuField.h>
#include <Node.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <StatusBar.h>
#include <Window.h>
#include <string.h>

#include <stdio.h>

#include "AddOns.h"
#include "ColorMultiSlider.h"
#include "Levels.h"
#include "ManipulatorInformer.h"
#include "MultiSlider.h"
#include "PixelOperations.h"
#include "PreviewView.h"
#include "Selection.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddOns_Levels"


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = B_TRANSLATE_MARK("Levels" B_UTF8_ELLIPSIS);
	char menu_help_string[255] = B_TRANSLATE_MARK("Adjusts the Levels.");
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = COLOR_ADD_ON;
#ifdef __cplusplus
}
#endif


using ArtPaint::Interface::FloatControl;
using ArtPaint::Interface::NumberControl;


Manipulator*
instantiate_add_on(BBitmap* bm, ManipulatorInformer* i)
{
	delete i;
	return new LevelsManipulator(bm);
}


LevelsManipulator::LevelsManipulator(BBitmap* bm)
	:
	WindowGUIManipulator(),
	selection(NULL)
{
	preview_bitmap = NULL;
	config_view = NULL;
	copy_of_the_preview_bitmap = NULL;

	for (int i = 0; i < 4; ++i) {
		previous_settings.inputMin[i] = settings.inputMin[i];
		previous_settings.inputMax[i] = settings.inputMax[i];
		previous_settings.gamma[i] = settings.gamma[i];
		previous_settings.outputMin[i] = settings.outputMin[i];
		previous_settings.outputMax[i] = settings.outputMax[i];
		original_settings.inputMin[i] = settings.inputMin[i];
		original_settings.inputMax[i] = settings.inputMax[i];
		original_settings.gamma[i] = settings.gamma[i];
		original_settings.outputMin[i] = settings.outputMin[i];
		original_settings.outputMax[i] = settings.outputMax[i];

	}

	SetPreviewBitmap(bm);
}


LevelsManipulator::~LevelsManipulator()
{
	delete copy_of_the_preview_bitmap;
	delete config_view;
}


BBitmap*
LevelsManipulator::ManipulateBitmap(
	ManipulatorSettings* set, BBitmap* original, BStatusBar* status_bar)
{
	LevelsManipulatorSettings* new_settings = dynamic_cast<LevelsManipulatorSettings*>(set);

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	if (original == preview_bitmap) {
		if ((*new_settings == previous_settings) && (last_calculated_resolution <= 1))
			return original;

		source_bitmap = copy_of_the_preview_bitmap;
		target_bitmap = original;
	} else {
		source_bitmap = original;
		target_bitmap = new BBitmap(original->Bounds(), B_RGB32, FALSE);
	}

	current_resolution = 1;
	current_settings = *new_settings;
	progress_bar = status_bar;

	uint32 inMin[4];
	uint32 inMax[4];
	float gamma[4];
	uint32 outMin[4];
	uint32 outMax[4];

	for (int i = 0; i < 4; ++i) {
		inMin[i] = current_settings.inputMin[i];
		inMax[i] = current_settings.inputMax[i];
		gamma[i] = 1. / current_settings.gamma[i];
		outMin[i] = current_settings.outputMin[i];
		outMax[i] = current_settings.outputMax[i];
	}

	union color_conversion color;

	for (float i = 0; i < 256; i++) {
		color.word = 0xffffffff;

		// apply R, G, B channels individually
		float inRangeR = inMax[1] - inMin[1];
		float inRangeG = inMax[2] - inMin[2];
		float inRangeB = inMax[3] - inMin[3];
		color.bytes[0] = min_c(255, max_c(0, (float)(i - inMin[3]) / (inRangeB) * 255.));
		color.bytes[1] = min_c(255, max_c(0, (float)(i - inMin[2]) / (inRangeG) * 255.));
		color.bytes[2] = min_c(255, max_c(0, (float)(i - inMin[1]) / (inRangeR) * 255.));

		for (int j = 1; j < 4; ++j) {
			if (gamma[j] != 1.0) {
				float gammaCorrected = pow((float)color.bytes[3 - j] / 255., gamma[j]);
				color.bytes[3 - j] = min_c(255, max_c(0, (255. * gammaCorrected)));
			}
		}

		float outRangeR = outMax[1] - outMin[1];
		float outRangeG = outMax[2] - outMin[2];
		float outRangeB = outMax[3] - outMin[3];

		if (outRangeR < 255)
			color.bytes[2] = (color.bytes[2] / 255.) * (outRangeR) + outMin[1];

		if (outRangeG < 255)
			color.bytes[1] = (color.bytes[1] / 255.) * (outRangeG) + outMin[2];

		if (outRangeB < 255)
			color.bytes[0] = (color.bytes[0] / 255.) * (outRangeB) + outMin[3];

		// then apply RGB channel afterwards
		float inRange = inMax[0] - inMin[0];
		float tmpR = (float)((float)color.bytes[2] - inMin[0]) / (inRange) * 255.;
		float tmpG = (float)((float)color.bytes[1] - inMin[0]) / (inRange) * 255.;
		float tmpB = (float)((float)color.bytes[0] - inMin[0]) / (inRange) * 255.;
		color.bytes[0] = min_c(255, max_c(0, tmpB));
		color.bytes[1] = min_c(255, max_c(0, tmpG));;
		color.bytes[2] = min_c(255, max_c(0, tmpR));;

		if (gamma[0] != 1.0) {
			float gammaCorrectedR = pow((float)color.bytes[2] / 255., gamma[0]);
			float gammaCorrectedG = pow((float)color.bytes[1] / 255., gamma[0]);
			float gammaCorrectedB = pow((float)color.bytes[0] / 255., gamma[0]);
			color.bytes[0] = min_c(255, max_c(0, (255. * gammaCorrectedB)));
			color.bytes[1] = min_c(255, max_c(0, (255. * gammaCorrectedG)));
			color.bytes[2] = min_c(255, max_c(0, (255. * gammaCorrectedR)));
		}

		float outRange = outMax[0] - outMin[0];
		if (outRange < 255) {
			color.bytes[0] = (color.bytes[0] / 255.) * (outRange) + outMin[0];
			color.bytes[1] = (color.bytes[1] / 255.) * (outRange) + outMin[0];
			color.bytes[2] = (color.bytes[2] / 255.) * (outRange) + outMin[0];
		}

		color_map[(int32)i].bytes[0] = min_c(255, max_c(color.bytes[0], 0));
		color_map[(int32)i].bytes[1] = min_c(255, max_c(color.bytes[1], 0));
		color_map[(int32)i].bytes[2] = min_c(255, max_c(color.bytes[2], 0));
	}

	start_threads();

	return target_bitmap;
}


int32
LevelsManipulator::PreviewBitmap(bool full_quality, BRegion* updated_region)
{
	progress_bar = NULL;
	if (settings == previous_settings) {
		if ((last_calculated_resolution != highest_available_quality)
			&& (last_calculated_resolution > 0)) {
			last_calculated_resolution
				= max_c(highest_available_quality, floor(last_calculated_resolution / 2.0));
		} else
			last_calculated_resolution = 0;
	} else
		last_calculated_resolution = lowest_available_quality;

	if (full_quality)
		last_calculated_resolution = min_c(1, last_calculated_resolution);

	previous_settings = settings;

	if (last_calculated_resolution > 0) {
		uint32 inMin[4];
		uint32 inMax[4];
		float gamma[4];
		uint32 outMin[4];
		uint32 outMax[4];

		for (int i = 0; i < 4; ++i) {
			inMin[i] = settings.inputMin[i];
			inMax[i] = settings.inputMax[i];
			gamma[i] = 1. / settings.gamma[i];
			outMin[i] = settings.outputMin[i];
			outMax[i] = settings.outputMax[i];
		}

		union color_conversion color;

		for (float i = 0; i < 256; i++) {
			color.word = 0xffffffff;

			// apply R, G, B channels individually
			float inRangeR = inMax[1] - inMin[1];
			float inRangeG = inMax[2] - inMin[2];
			float inRangeB = inMax[3] - inMin[3];
			color.bytes[0] = min_c(255, max_c(0, (float)(i - inMin[3]) / (inRangeB) * 255.));
			color.bytes[1] = min_c(255, max_c(0, (float)(i - inMin[2]) / (inRangeG) * 255.));
			color.bytes[2] = min_c(255, max_c(0, (float)(i - inMin[1]) / (inRangeR) * 255.));

			for (int j = 1; j < 4; ++j) {
				if (gamma[j] != 1.0) {
					float gammaCorrected = pow((float)color.bytes[3 - j] / 255., gamma[j]);
					color.bytes[3 - j] = min_c(255, max_c(0, (255. * gammaCorrected)));
				}
			}

			float outRangeR = outMax[1] - outMin[1];
			float outRangeG = outMax[2] - outMin[2];
			float outRangeB = outMax[3] - outMin[3];

			if (outRangeR < 255)
				color.bytes[2] = (color.bytes[2] / 255.) * (outRangeR) + outMin[1];

			if (outRangeG < 255)
				color.bytes[1] = (color.bytes[1] / 255.) * (outRangeG) + outMin[2];

			if (outRangeB < 255)
				color.bytes[0] = (color.bytes[0] / 255.) * (outRangeB) + outMin[3];

			// then apply RGB channel afterwards
			float inRange = inMax[0] - inMin[0];
			float tmpR = (float)((float)color.bytes[2] - inMin[0]) / (inRange) * 255.;
			float tmpG = (float)((float)color.bytes[1] - inMin[0]) / (inRange) * 255.;
			float tmpB = (float)((float)color.bytes[0] - inMin[0]) / (inRange) * 255.;
			color.bytes[0] = min_c(255, max_c(0, tmpB));
			color.bytes[1] = min_c(255, max_c(0, tmpG));;
			color.bytes[2] = min_c(255, max_c(0, tmpR));;

			if (gamma[0] != 1.0) {
				float gammaCorrectedR = pow((float)color.bytes[2] / 255., gamma[0]);
				float gammaCorrectedG = pow((float)color.bytes[1] / 255., gamma[0]);
				float gammaCorrectedB = pow((float)color.bytes[0] / 255., gamma[0]);
				color.bytes[0] = min_c(255, max_c(0, (255. * gammaCorrectedB)));
				color.bytes[1] = min_c(255, max_c(0, (255. * gammaCorrectedG)));
				color.bytes[2] = min_c(255, max_c(0, (255. * gammaCorrectedR)));
			}

			float outRange = outMax[0] - outMin[0];
			if (outRange < 255) {
				color.bytes[0] = (color.bytes[0] / 255.) * (outRange) + outMin[0];
				color.bytes[1] = (color.bytes[1] / 255.) * (outRange) + outMin[0];
				color.bytes[2] = (color.bytes[2] / 255.) * (outRange) + outMin[0];
			}

			color_map[(int32)i].bytes[0] = min_c(255, max_c(color.bytes[0], 0));
			color_map[(int32)i].bytes[1] = min_c(255, max_c(color.bytes[1], 0));
			color_map[(int32)i].bytes[2] = min_c(255, max_c(color.bytes[2], 0));
		}

		current_resolution = last_calculated_resolution;
		updated_region->Set(preview_bitmap->Bounds());

		target_bitmap = preview_bitmap;
		source_bitmap = copy_of_the_preview_bitmap;
		current_settings = settings;

		start_threads();
	}

	return last_calculated_resolution;
}


void
LevelsManipulator::start_threads()
{
	number_of_threads = GetSystemCpuCount();

	thread_id* threads = new thread_id[number_of_threads];

	for (int32 i = 0; i < number_of_threads; i++) {
		threads[i] = spawn_thread(thread_entry, "Levels_thread", B_NORMAL_PRIORITY, this);
		resume_thread(threads[i]);
		send_data(threads[i], i, NULL, 0);
	}

	for (int32 i = 0; i < number_of_threads; i++) {
		int32 return_value;
		wait_for_thread(threads[i], &return_value);
	}

	delete[] threads;
}


int32
LevelsManipulator::thread_entry(void* data)
{
	int32 thread_number;
	thread_number = receive_data(NULL, NULL, 0);

	LevelsManipulator* this_pointer = (LevelsManipulator*)data;

	return this_pointer->thread_function(thread_number);
}


int32
LevelsManipulator::thread_function(int32 thread_number)
{
	int32 step = current_resolution;

	BWindow* progress_bar_window = NULL;
	if (progress_bar != NULL)
		progress_bar_window = progress_bar->Window();

	uint32* source_bits = (uint32*)source_bitmap->Bits();
	uint32* target_bits = (uint32*)target_bitmap->Bits();
	int32 source_bpr = source_bitmap->BytesPerRow() / 4;
	int32 target_bpr = target_bitmap->BytesPerRow() / 4;

	int32 left = target_bitmap->Bounds().left;
	int32 right = target_bitmap->Bounds().right;
	int32 top = target_bitmap->Bounds().top;
	int32 bottom = target_bitmap->Bounds().bottom;

	float height = bottom - top;
	top = height / number_of_threads * thread_number;
	top = ceil(top / (float)step);
	top *= step;
	bottom = min_c(bottom, top + (height + 1) / number_of_threads);
	int32 update_interval = 10;
	float update_amount = 100.0 / (bottom - top) * update_interval / (float)number_of_threads;
	float missed_update = 0;

	// Loop through all pixels in original.
	uint32 value = 0;

	union color_conversion color;

	for (int32 y = top; y <= bottom; y += step) {
		int32 y_times_source_bpr = y * source_bpr;
		int32 y_times_target_bpr = y * target_bpr;
		for (int32 x = left; x <= right; x += step) {
			if (selection == NULL || selection->IsEmpty() == true
				|| selection->ContainsPoint(x, y)) {

				color.word = *(source_bits + x + y_times_source_bpr);

				color.bytes[0] = color_map[color.bytes[0]].bytes[0];
				color.bytes[1] = color_map[color.bytes[1]].bytes[1];
				color.bytes[2] = color_map[color.bytes[2]].bytes[2];

				*(target_bits + x + y_times_target_bpr)
					= color.word;
			}
		}

		// Update the status-bar
		if (((y % update_interval) == 0) && (progress_bar_window != NULL)
			&& (progress_bar_window->LockWithTimeout(0) == B_OK)) {
			progress_bar->Update(update_amount + missed_update);
			progress_bar_window->Unlock();
			missed_update = 0;
		} else if ((y % update_interval) == 0)
			missed_update += update_amount;
	}

	return B_OK;
}


void
LevelsManipulator::SetPreviewBitmap(BBitmap* bm)
{
	if (preview_bitmap != bm) {
		delete copy_of_the_preview_bitmap;
		if (bm != NULL) {
			preview_bitmap = bm;
			copy_of_the_preview_bitmap = DuplicateBitmap(bm, 0);
		} else {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
		}
	}

	if (preview_bitmap != NULL) {
		// Let's select a resolution that can handle all the pixels at least
		// 10 times in a second while assuming that one pixel calculation takes
		// about 50 CPU cycles.
		double speed = GetSystemClockSpeed() / (10 * 50);
		BRect bounds = preview_bitmap->Bounds();
		float num_pixels = (bounds.Width() + 1) * (bounds.Height() + 1);
		lowest_available_quality = 1;
		while ((num_pixels / lowest_available_quality / lowest_available_quality) > speed)
			lowest_available_quality *= 2;

		lowest_available_quality = min_c(lowest_available_quality, 16);
		highest_available_quality = max_c(lowest_available_quality / 2, 1);
	} else {
		lowest_available_quality = 1;
		highest_available_quality = 1;
	}
	last_calculated_resolution = lowest_available_quality;
}


void
LevelsManipulator::Reset()
{
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with a loop.
		uint32* source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32* target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target, source, bits_length);
	}
}


BView*
LevelsManipulator::MakeConfigurationView(const BMessenger& target)
{
	if (config_view == NULL) {
		config_view = new LevelsManipulatorView(this, target);
		config_view->ChangeSettings(&settings);
		config_view->SetBitmap(copy_of_the_preview_bitmap);
	}

	return config_view;
}


ManipulatorSettings*
LevelsManipulator::ReturnSettings()
{
	return new LevelsManipulatorSettings(settings);
}


void
LevelsManipulator::ChangeSettings(ManipulatorSettings* s)
{
	LevelsManipulatorSettings* new_settings;
	new_settings = dynamic_cast<LevelsManipulatorSettings*>(s);
	if (new_settings != NULL)
		settings = *new_settings;
}


void
LevelsManipulator::ResetSettings()
{
	settings = original_settings;
}


const char*
LevelsManipulator::ReturnName()
{
	return B_TRANSLATE("Levels");
}


const char*
LevelsManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Left-click for white point, right for black, SHIFT+left for gray");
}


void
LevelsManipulator::MouseDown(BPoint point, uint32 buttons, BView*, bool first_click) {
	if (copy_of_the_preview_bitmap != NULL) {
		uint32* source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 bpr = copy_of_the_preview_bitmap->BytesPerRow() / 4;

		copy_of_the_preview_bitmap->LockBits();
		union color_conversion color;
		BRect imgBounds = copy_of_the_preview_bitmap->Bounds();
		if (imgBounds.Contains(point)) {
			color.word = *(source_bits + (uint32)point.x + ((uint32)point.y * bpr));
			copy_of_the_preview_bitmap->UnlockBits();

			if (buttons & B_PRIMARY_MOUSE_BUTTON) {
				if (modifiers() & B_LEFT_SHIFT_KEY) {
					// gray point
					config_view->SetGrayPoint(color.word);
				} else {
					// white point
					config_view->SetWhitePoint(color.word);
				}
			} else {
				// black point
				config_view->SetBlackPoint(color.word);
			}
		}
	}
}


LevelsManipulatorView::LevelsManipulatorView(
	LevelsManipulator* manip, const BMessenger& t)
	:
	WindowGUIManipulatorView()
{
	target = t;
	manipulator = manip;
	started_adjusting = FALSE;

	histogramBitmap = NULL;

	mode = LEVELS_MODE_RGB;
	histogramRect = BRect(0, 0, 255, 100);
	histogramView = new PreviewView(histogramRect, 255, 100);
	histogramBitmap = histogramView->ReturnBitmap();
	histogramView->SetExplicitMaxSize(BSize(255, 100));
	histogramView->SetExplicitMinSize(BSize(255, 100));
	input_values_slider = new MultiSlider("Input_slider", B_TRANSLATE("Input Levels:"),
		new BMessage(INPUT_LEVELS_ADJUSTED), 0, 255, B_HORIZONTAL, B_TRIANGLE_THUMB);

	input_values_slider->SetModificationMessage(new BMessage(INPUT_LEVELS_ADJUSTING));

	inputMin = new NumberControl("", "", new BMessage(INPUT_MIN_ADJUSTED));
	inputMid = new NumberControl("", "", new BMessage(INPUT_MID_ADJUSTED));
	inputMax = new NumberControl("", "", new BMessage(INPUT_MAX_ADJUSTED));
	gamma	= new FloatControl("", "", new BMessage(GAMMA_ADJUSTED));
	inputMin->SetWidthInBytes(3);
	inputMax->SetWidthInBytes(3);
	inputMid->SetWidthInBytes(3);
	gamma->SetWidthInBytes(4);
	outputMin = new NumberControl("", "", new BMessage(OUTPUT_MIN_ADJUSTED));
	outputMax = new NumberControl("", "", new BMessage(OUTPUT_MAX_ADJUSTED));
	outputMin->SetWidthInBytes(3);
	outputMax->SetWidthInBytes(3);

	output_values_slider = new ColorMultiSlider("Output_slider", B_TRANSLATE("Output Levels:"),
		new BMessage(OUTPUT_LEVELS_ADJUSTED), 0, 255, B_HORIZONTAL, B_TRIANGLE_THUMB);

	rgb_color* color1 = new rgb_color({0, 0, 0, 255});
	fColorList.AddItem(color1);
	rgb_color* color3 = new rgb_color({255, 255, 255, 255});
	fColorList.AddItem(color3);

	output_values_slider->SetColors(&fColorList);
	output_values_slider->SetModificationMessage(new BMessage(OUTPUT_LEVELS_ADJUSTING));

	BMenu* a_menu = new BPopUpMenu("mode");
	a_menu->AddItem(
		new BMenuItem(B_TRANSLATE("RGB"), new BMessage(LEVELS_MODE_RGB)));
	a_menu->AddItem(new BMenuItem(B_TRANSLATE("Red"), new BMessage(LEVELS_MODE_RED)));
	a_menu->AddItem(new BMenuItem(B_TRANSLATE("Green"), new BMessage(LEVELS_MODE_GREEN)));
	a_menu->AddItem(new BMenuItem(B_TRANSLATE("Blue"), new BMessage(LEVELS_MODE_BLUE)));
	modeMenu = new BMenuField("modeMenu", B_TRANSLATE("Mode:"), a_menu);
	a_menu->ItemAt(0)->SetMarked(true);

	modeMenu->SetExplicitMinSize(BSize(256, B_SIZE_UNSET));

	autoLevelsButton = new BButton("Levels", new BMessage(AUTO_LEVELS_ADJUSTED));
	autoTonesButton = new BButton("Tones", new BMessage(AUTO_TONES_ADJUSTED));
	autoColorsButton = new BButton("Colors", new BMessage(AUTO_COLORS_ADJUSTED));

	resetButton = new BButton("Reset", new BMessage(LEVELS_RESET));

	BBox* autoBox = new BBox(B_FANCY_BORDER,
		BGroupLayoutBuilder(B_VERTICAL)
		.AddGroup(B_VERTICAL)
			.Add(autoLevelsButton)
			.Add(autoTonesButton)
			.Add(autoColorsButton)
			.End()
		.SetInsets(B_USE_SMALL_INSETS, B_USE_SMALL_INSETS,
				B_USE_SMALL_INSETS, B_USE_SMALL_INSETS)
		.TopView()
	);

	autoBox->SetLabel("Auto");

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.AddGroup(B_VERTICAL)
			.Add(modeMenu)
			.Add(histogramView)
			.Add(input_values_slider)
			.AddGroup(B_HORIZONTAL)
				.Add(inputMin->CreateTextViewLayoutItem())
				.AddGlue()
				.Add(inputMid->CreateTextViewLayoutItem())
				.Add(gamma->CreateTextViewLayoutItem())
				.AddGlue()
				.Add(inputMax->CreateTextViewLayoutItem())
				.End()
			.Add(output_values_slider)
			.AddGroup(B_HORIZONTAL)
				.Add(outputMin->CreateTextViewLayoutItem())
				.AddGlue()
				.AddGlue()
				.AddGlue()
				.AddGlue()
				.Add(outputMax->CreateTextViewLayoutItem())
				.End()
			.End()
		.AddGroup(B_VERTICAL)
			.Add(autoBox)
			.Add(resetButton)
			.AddGlue()
			.End()
		.End();

	for (int i = 0; i < 4; ++i)
		midRatios[i] = 0.5;
}


void
LevelsManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();
	input_values_slider->SetTarget(BMessenger(this));
	output_values_slider->SetTarget(BMessenger(this));
	modeMenu->Menu()->SetTargetForItems(BMessenger(this));
	inputMin->SetTarget(BMessenger(this));
	inputMid->SetTarget(BMessenger(this));
	inputMax->SetTarget(BMessenger(this));
	outputMin->SetTarget(BMessenger(this));
	outputMin->SetTarget(BMessenger(this));
	gamma->SetTarget(BMessenger(this));
	autoLevelsButton->SetTarget(BMessenger(this));
	autoTonesButton->SetTarget(BMessenger(this));
	autoColorsButton->SetTarget(BMessenger(this));
	resetButton->SetTarget(BMessenger(this));
}


void
LevelsManipulatorView::AllAttached()
{
	input_values_slider->AddThumb(128);
	input_values_slider->AddThumb(255);

	inputMin->SetValue((int32)*(input_values_slider->Positions().ItemAt(0)));
	inputMid->SetValue((int32)*(input_values_slider->Positions().ItemAt(1)));
	inputMax->SetValue((int32)*(input_values_slider->Positions().ItemAt(2)));

	output_values_slider->AddThumb(255);

	outputMin->SetValue((int32)*(output_values_slider->Positions().ItemAt(0)));
	outputMax->SetValue((int32)*(output_values_slider->Positions().ItemAt(1)));

	gamma->SetValue((float)1.0);
}

// Notes:
// adjusting R, G, or B channels changes RGB histogram

void
LevelsManipulatorView::MessageReceived(BMessage* message)
{
	switch (message->what) {

		case INPUT_LEVELS_ADJUSTED:
		case INPUT_LEVELS_ADJUSTING: {
			int32 oldMin = inputMin->Value();
			int32 oldMid = inputMid->Value();
			int32 oldMax = inputMax->Value();
			int32 newMin = *(input_values_slider->Positions().ItemAt(0));
			int32 newMid = *(input_values_slider->Positions().ItemAt(1));
			int32 newMax = *(input_values_slider->Positions().ItemAt(2));

			if (newMid == oldMid && (oldMin != newMin || oldMax != newMax)) {
				newMid = ((float)(newMax - newMin) * midRatios[mode]) + newMin;
				newMid = max_c(newMin, min_c(newMax, newMid));
				input_values_slider->SetValue(1, newMid);
			} else if (newMid != oldMid) {
				float midValue = (float)(newMid - newMin) / (float)(newMax - newMin);
				midRatios[mode] = midValue;

				if (midValue < 0.5) {
					midValue *= 2.;
					midValue = 1. + (9. * (1. - midValue));
					gamma->SetValue((float)min_c(midValue, 9.99));
				} else if (midValue > 0.5) {
					midValue *= 2.;
					midValue = 1. - (midValue - 1.);
					gamma->SetValue((float)max_c(midValue, 0.01));
				} else
					gamma->SetValue((float)1.0);

				settings.gamma[mode] = gamma->Value();
			}
			inputMin->SetValue((int32)newMin);
			inputMax->SetValue((int32)newMax);
			inputMid->SetValue((int32)newMid);

			settings.inputMin[mode] = inputMin->Value();
			settings.inputMax[mode] = inputMax->Value();

			manipulator->ChangeSettings(&settings);
			if (message->what == INPUT_LEVELS_ADJUSTED) {
				started_adjusting = false;
				target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			} else if (!started_adjusting) {
				target.SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
				started_adjusting = true;
			}

		}	break;
		case OUTPUT_LEVELS_ADJUSTED:
		case OUTPUT_LEVELS_ADJUSTING: {
			//output_values_slider->SetColorsByPosition(&fColorList);
			output_values_slider->Invalidate();

			outputMin->SetValue((int32)*(output_values_slider->Positions().ItemAt(0)));
			outputMax->SetValue((int32)*(output_values_slider->Positions().ItemAt(1)));

			settings.outputMin[mode] = outputMin->Value();
			settings.outputMax[mode] = outputMax->Value();

			manipulator->ChangeSettings(&settings);

			if (message->what == OUTPUT_LEVELS_ADJUSTED) {
				started_adjusting = false;
				target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			} else if (!started_adjusting) {
				target.SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
				started_adjusting = true;
			}
		}	break;
		case LEVELS_MODE_RGB:
		case LEVELS_MODE_RED:
		case LEVELS_MODE_GREEN:
		case LEVELS_MODE_BLUE: {
			mode = message->what;
			DrawHistogram();
			histogramView->Invalidate();
			input_values_slider->SetValue(0, settings.inputMin[mode]);
			input_values_slider->SetValue(2, settings.inputMax[mode]);
			inputMin->SetValue(settings.inputMin[mode]);
			inputMax->SetValue(settings.inputMax[mode]);
			float val = settings.gamma[mode];
			float midValue = val;
			if (val > 1.0) {
				midValue = 1 - ((val - 1) / 9.);
			} else if (val < 1.0) {
				midValue = (1 - val) + 1;
			}
			int32 curMin = inputMin->Value();
			int32 curMax = inputMax->Value();
			int32 range = curMax - curMin;
			midValue = midValue * floor(range / 2.) + curMin;
			input_values_slider->SetValue(1, midValue);
			output_values_slider->SetValue(0, settings.outputMin[mode]);
			output_values_slider->SetValue(1, settings.outputMax[mode]);
			inputMid->SetValue(midValue);
			gamma->SetValue(val);
			outputMin->SetValue(settings.outputMin[mode]);
			outputMax->SetValue(settings.outputMax[mode]);
			settings.mode = mode;
			manipulator->ChangeSettings(&settings);
			started_adjusting = false;
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		}	break;
		case INPUT_MIN_ADJUSTED: {
			int32 val = max_c(0, min_c(255, inputMin->Value()));
			input_values_slider->SetValue(0, val);
			settings.inputMin[mode] = val;
			int32 curMax = inputMax->Value();
			int32 newMid = ((float)(curMax - val) * midRatios[mode]) + val;
			newMid = max_c(val, min_c(curMax, newMid));
			input_values_slider->SetValue(1, newMid);
			inputMid->SetValue(newMid);
			manipulator->ChangeSettings(&settings);
			started_adjusting = false;
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		} 	break;
		case INPUT_MID_ADJUSTED: {
			int32 curMin = inputMin->Value();
			int32 curMax = inputMax->Value();
			int32 val = max_c(curMin, min_c(curMax, inputMid->Value()));
			input_values_slider->SetValue(1, val);
			if (val != inputMid->Value())
				inputMid->SetValue(val);

			float midValue = (float)(val - curMin) / (curMax - curMin);
			midRatios[mode] = midValue;
			if (midValue < 0.5) {
				midValue *= 2.;
				midValue = 1. + (9. * (1. - midValue));
				gamma->SetValue((float)min_c(midValue, 9.99));
			} else if (midValue > 0.5) {
				midValue *= 2.;
				midValue = 1. - (midValue - 1.);
				gamma->SetValue((float)max_c(midValue, 0.01));
			} else
				gamma->SetValue((float)1.0);
			settings.gamma[mode] = gamma->Value();

			manipulator->ChangeSettings(&settings);
			started_adjusting = false;
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		} 	break;
		case INPUT_MAX_ADJUSTED: {
			int32 val = max_c(0, min_c(255, inputMax->Value()));
			input_values_slider->SetValue(2, val);
			settings.inputMax[mode] = val;
			int32 curMin = inputMin->Value();
			int32 newMid = ((float)(val - curMin) * midRatios[mode]) + curMin;
			newMid = max_c(curMin, min_c(val, newMid));
			input_values_slider->SetValue(1, newMid);
			inputMid->SetValue(newMid);
			manipulator->ChangeSettings(&settings);
			started_adjusting = false;
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		} 	break;
		case OUTPUT_MIN_ADJUSTED: {
			int32 val = max_c(0, min_c(255, outputMin->Value()));
			output_values_slider->SetValue(0, val);
			settings.outputMin[mode] = val;
			manipulator->ChangeSettings(&settings);
			started_adjusting = false;
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		}	break;
		case OUTPUT_MAX_ADJUSTED: {
			int32 val = max_c(0, min_c(255, outputMax->Value()));
			output_values_slider->SetValue(1, val);
			settings.outputMax[mode] = val;
			manipulator->ChangeSettings(&settings);
			started_adjusting = false;
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		}	break;
		case GAMMA_ADJUSTED: {
			float val = max_c(0.01, min_c(9.99, gamma->Value()));
			settings.gamma[mode] = val;

			float midValue = val;
			if (val > 1.0) {
				midValue = 1 - ((val - 1) / 9.);
			} else if (val < 1.0) {
				midValue = (1 - val) + 1;
			}
			int32 curMin = inputMin->Value();
			int32 curMax = inputMax->Value();
			int32 range = curMax - curMin;
			midValue = midValue * (range / 2) + curMin;
			midRatios[mode] = (float)(midValue - curMin) / (float)(curMax - curMin);
			inputMid->SetValue(midValue);
			input_values_slider->SetValue(1, (int32)midValue);
			manipulator->ChangeSettings(&settings);
			started_adjusting = false;
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		}	break;
		case AUTO_LEVELS_ADJUSTED: {
			int32 newMin = min[LEVELS_MODE_RGB];
			int32 newMax = max[LEVELS_MODE_RGB];
			settings.inputMin[LEVELS_MODE_RGB] = newMin;
			settings.inputMax[LEVELS_MODE_RGB] = newMax;

			if (mode == LEVELS_MODE_RGB) {
				input_values_slider->SetValue(2, newMax);
				inputMax->SetValue(newMax);
				input_values_slider->SetValue(0, newMin);
				inputMin->SetValue(newMin);
				int32 newMid = ((float)(newMax - newMin) * midRatios[mode]) + newMin;
				newMid = max_c(newMin, min_c(newMax, newMid));
				input_values_slider->SetValue(1, newMid);
				inputMid->SetValue(newMid);
				gamma->SetValue(midRatios[mode]);
			}

			manipulator->ChangeSettings(&settings);
			started_adjusting = false;
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		}	break;
		case AUTO_TONES_ADJUSTED: {
			for (int i = 1; i < 4; ++i) {
				settings.inputMin[i] = min[i];
				settings.inputMax[i] = max[i];
			}

			if (mode != LEVELS_MODE_RGB) {
				int32 newMin = min[mode];
				int32 newMax = max[mode];

				input_values_slider->SetValue(2, newMax);
				inputMax->SetValue(newMax);
				input_values_slider->SetValue(0, newMin);
				inputMin->SetValue(newMin);
				int32 newMid = ((float)(newMax - newMin) * midRatios[mode]) + newMin;
				newMid = max_c(newMin, min_c(newMax, newMid));
				input_values_slider->SetValue(1, newMid);
				inputMid->SetValue(newMid);
				gamma->SetValue(midRatios[mode]);
			}

			manipulator->ChangeSettings(&settings);
			started_adjusting = false;
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		}	break;
		case AUTO_COLORS_ADJUSTED: {
			for (int i = 1; i < 4; ++i) {
				int32 curMin = min[i];
				int32 curMax = max[i];
				int32 range = curMax - curMin;

				int32 midValue = avg[i];

				float ratio = (float)(midValue - curMin) / (curMax - curMin);
				midRatios[i] = ratio;
				if (ratio < 0.5) {
					ratio *= 2.;
					ratio = 1. + (9. * (1. - ratio));
					settings.gamma[i] = min_c(ratio, 9.99);
				} else if (ratio > 0.5) {
					ratio *= 2.;
					ratio = 1. - (ratio - 1.);
					settings.gamma[i] = max_c(ratio, 0.01);
				} else
					settings.gamma[i] = 1.0;
			}

			for (int i = 1; i < 4; ++i) {
				settings.inputMin[i] = min[i];
				settings.inputMax[i] = max[i];
			}

			if (mode != LEVELS_MODE_RGB) {
				int32 newMin = min[mode];
				int32 newMax = max[mode];

				input_values_slider->SetValue(2, newMax);
				inputMax->SetValue(newMax);
				input_values_slider->SetValue(0, newMin);
				inputMin->SetValue(newMin);
				int32 newMid = ((float)(newMax - newMin) * midRatios[mode]) + newMin;
				newMid = max_c(newMin, min_c(newMax, newMid));
				input_values_slider->SetValue(1, newMid);
				inputMid->SetValue(newMid);
				gamma->SetValue(midRatios[mode]);
			}

			manipulator->ChangeSettings(&settings);
			started_adjusting = false;
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		}	break;
		case LEVELS_RESET:
		{
			manipulator->ResetSettings();
			ManipulatorSettings* oldset = manipulator->ReturnSettings();
			ChangeSettings(oldset);

			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		}	break;
		default:
			WindowGUIManipulatorView::MessageReceived(message);
	}
}


void
LevelsManipulatorView::ChangeSettings(ManipulatorSettings* set)
{
	LevelsManipulatorSettings* new_settings = dynamic_cast<LevelsManipulatorSettings*>(set);

	if (set != NULL) {
		settings = *new_settings;

		BWindow* window = Window();
		if (window != NULL) {
			window->Lock();
			//value_slider->SetValue(0, 0);//settings.brightness);
			MessageReceived(new BMessage(settings.mode));

			window->Unlock();
		}
	}
}


void
LevelsManipulatorView::SetBitmap(BBitmap* newBitmap)
{
	if (newBitmap != NULL) {
		analyzedBitmap = newBitmap;

		uint32* bits = (uint32*)analyzedBitmap->Bits();
		int32 bits_length = analyzedBitmap->BitsLength() / 4;
		union color_conversion c;

		for (int32 i = 0; i < 256; ++i) {
			histogram[LEVELS_MODE_RGB][i] = 0;
			histogram[LEVELS_MODE_BLUE][i] = 0;
			histogram[LEVELS_MODE_GREEN][i] = 0;
			histogram[LEVELS_MODE_RED][i] = 0;
		}

		for (int i = 0; i < 4; ++i) {
			min[i] = 255;
			max[i] = 0;
		}

		uint64 avgRGB = 0;
		uint64 avgR = 0;
		uint64 avgG = 0;
		uint64 avgB = 0;

		for (int32 i = 0; i < bits_length; ++i) {
			c.word = *bits++;
			int32 intensity = (int32)(0.299 * c.bytes[2] + 0.587 * c.bytes[1] + 0.114 * c.bytes[0]);
			histogram[LEVELS_MODE_RGB][intensity]++;
			histogram[LEVELS_MODE_BLUE][c.bytes[0]]++;
			histogram[LEVELS_MODE_GREEN][c.bytes[1]]++;
			histogram[LEVELS_MODE_RED][c.bytes[2]]++;

			if (min[LEVELS_MODE_RGB] > intensity)
				min[LEVELS_MODE_RGB] = intensity;
			if (min[LEVELS_MODE_BLUE] > c.bytes[0])
				min[LEVELS_MODE_BLUE] = c.bytes[0];
			if (min[LEVELS_MODE_GREEN] > c.bytes[1])
				min[LEVELS_MODE_GREEN] = c.bytes[1];
			if (min[LEVELS_MODE_RED] > c.bytes[2])
				min[LEVELS_MODE_RED] = c.bytes[2];

			if (max[LEVELS_MODE_RGB] < intensity)
				max[LEVELS_MODE_RGB] = intensity;
			if (max[LEVELS_MODE_BLUE] < c.bytes[0])
				max[LEVELS_MODE_BLUE] = c.bytes[0];
			if (max[LEVELS_MODE_GREEN] < c.bytes[1])
				max[LEVELS_MODE_GREEN] = c.bytes[1];
			if (max[LEVELS_MODE_RED] < c.bytes[2])
				max[LEVELS_MODE_RED] = c.bytes[2];

			avgRGB += intensity;
			avgB += c.bytes[0];
			avgG += c.bytes[1];
			avgR += c.bytes[2];
		}

		avg[LEVELS_MODE_RGB] = avgRGB / bits_length;
		avg[LEVELS_MODE_BLUE] = avgB / bits_length;
		avg[LEVELS_MODE_GREEN] = avgG / bits_length;
		avg[LEVELS_MODE_RED] = avgR / bits_length;

		alpha = (float)avg[LEVELS_MODE_GREEN] / (float)avg[LEVELS_MODE_RED];
		beta = (float)avg[LEVELS_MODE_GREEN] / (float)avg[LEVELS_MODE_BLUE];

		DrawHistogram();
	}
}


void
LevelsManipulatorView::DrawHistogram()
{
	if (histogramBitmap != NULL) {
		uint32* bits = (uint32*)histogramBitmap->Bits();
		int32 bpr = histogramBitmap->BytesPerRow() / 4;
		int32 bits_length = histogramBitmap->BitsLength() / 4;

		union color_conversion c;
		c.word = 0xFFFFFFFF;

		for (int32 i = 0; i < bits_length; i++)
			*bits++ = c.word;

		bits = (uint32*)histogramBitmap->Bits();

		int32 max = 0;
		for (int32 i = 0; i < 256; i++)
			max = (int32)max_c(max, histogram[mode][i]);

		if (max > 0) {
			int32 height = histogramBitmap->Bounds().IntegerHeight();

			c.word = 0x00000000;
			c.bytes[3] = 0xFF;
			for (int32 i = 0; i < 256; i++) {
				int32 top = height - (float)histogram[mode][i] / (float)max * height;
				for (int32 y = top; y <= height; y++)
					*(bits + y * bpr + i) = c.word;
			}
		}
	}
}


void
LevelsManipulatorView::SetBlackPoint(uint32 color)
{
	union color_conversion blackPoint;

	blackPoint.word = color;

	for (int i = 1; i < 4; ++i) {
		settings.inputMin[i] = blackPoint.bytes[3 - i];

		if (mode == i) {
			BWindow* window = Window();
			if (window != NULL) {
				window->Lock();
				inputMin->SetValue(settings.inputMin[i]);
				input_values_slider->SetValue(0, settings.inputMin[i]);
				window->Unlock();
			}
		}
	}

	manipulator->ChangeSettings(&settings);
	started_adjusting = false;
	target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);

}


void
LevelsManipulatorView::SetWhitePoint(uint32 color)
{
	union color_conversion whitePoint;

	whitePoint.word = color;

	for (int i = 1; i < 4; ++i) {
		settings.inputMax[i] = whitePoint.bytes[3 - i];

		if (mode == i) {
			BWindow* window = Window();
			if (window != NULL) {
				window->Lock();
				inputMax->SetValue(settings.inputMax[i]);
				input_values_slider->SetValue(2, settings.inputMax[i]);
				window->Unlock();
			}
		}
	}

	manipulator->ChangeSettings(&settings);
	started_adjusting = false;
	target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
}


void
LevelsManipulatorView::SetGrayPoint(uint32 color)
{
	union color_conversion grayPoint;

	grayPoint.word = color;

	for (int i = 1; i < 4; ++i) {
		int newMid = grayPoint.bytes[3 - i];

		int32 curMin = settings.inputMin[i];
		int32 curMax = settings.inputMax[i];
		int32 val = max_c(curMin, min_c(curMax, newMid));

		float midValue = (float)(val - curMin) / (curMax - curMin);
		midRatios[i] = midValue;
		if (midValue < 0.5) {
			midValue *= 2.;
			midValue = 1. + (9. * (1. - midValue));
		} else if (midValue > 0.5) {
			midValue *= 2.;
			midValue = 1. - (midValue - 1.);
		} else
			midValue = 1.0;

		settings.gamma[i] = midValue;

		if (mode == i) {
			BWindow* window = Window();
			if (window != NULL) {
				window->Lock();
				inputMid->SetValue(newMid);
				gamma->SetValue(midValue);
				input_values_slider->SetValue(1, newMid);
				window->Unlock();
			}
		}
	}

	manipulator->ChangeSettings(&settings);
	started_adjusting = false;
	target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
}
