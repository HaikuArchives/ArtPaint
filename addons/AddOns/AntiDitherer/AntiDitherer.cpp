/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ClassInfo.h>
#include <LayoutBuilder.h>
#include <Spinner.h>
#include <StatusBar.h>
#include <string.h>
#include <Window.h>

#include "AddOns.h"
#include "AntiDitherer.h"
#include "ManipulatorInformer.h"
#include "Selection.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddOns_AntiDither"


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = B_TRANSLATE_MARK("Anti-Dither" B_UTF8_ELLIPSIS);
	char menu_help_string[255] = B_TRANSLATE_MARK("Attempts to reverse the effects of dithering.");
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = COLOR_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new AntiDithererManipulator(bm);
}



AntiDithererManipulator::AntiDithererManipulator(BBitmap *bm)
		: WindowGUIManipulator(),
		selection(NULL)
{
	preview_bitmap = NULL;
	config_view = NULL;
	copy_of_the_preview_bitmap = NULL;

	SetPreviewBitmap(bm);
}


AntiDithererManipulator::~AntiDithererManipulator()
{
	delete copy_of_the_preview_bitmap;
	delete config_view;
}


BBitmap* AntiDithererManipulator::ManipulateBitmap(ManipulatorSettings* set,
	BBitmap* original,
	BStatusBar* status_bar)
{
	AntiDithererManipulatorSettings *new_settings = cast_as(set,AntiDithererManipulatorSettings);

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	if (original == preview_bitmap) {
		if (*new_settings == previous_settings)
			return original;

		source_bitmap = copy_of_the_preview_bitmap;
		target_bitmap = original;
	}
	else {
		source_bitmap = original;
		target_bitmap = new BBitmap(original->Bounds(),B_RGB32,FALSE);
	}


	settings = *new_settings;

	anti_dither();

	return target_bitmap;
}

int32 AntiDithererManipulator::PreviewBitmap(bool full_quality,
	BRegion* updated_region)
{
	updated_region->Set(preview_bitmap->Bounds());

	target_bitmap = preview_bitmap;
	source_bitmap = copy_of_the_preview_bitmap;

	anti_dither();

	return 1;
}


void AntiDithererManipulator::MouseDown(BPoint point,uint32,BView*,bool first_click)
{
	// This function does nothing in AntiDithererManipulator.
}


void AntiDithererManipulator::SetPreviewBitmap(BBitmap *bm)
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
}


void AntiDithererManipulator::Reset()
{
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with a loop.
		uint32 *source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target,source,bits_length);
	}
}


void
AntiDithererManipulator::anti_dither()
{
	uint32 *source_bits = (uint32*)source_bitmap->Bits();
	uint32 *target_bits = (uint32*)target_bitmap->Bits();

	int32 source_bpr = source_bitmap->BytesPerRow() / 4;
	int32 target_bpr = target_bitmap->BytesPerRow() / 4;

	BRect bounds = source_bitmap->Bounds();

	if (selection != NULL && selection->IsEmpty() == false)
		bounds = selection->GetBoundingRect();

	int32 width = bounds.IntegerWidth();
	int32 height = bounds.IntegerHeight();

	float bounds_left = bounds.left;
	float bounds_right = bounds.right;
	float bounds_top = bounds.top;
	float bounds_bottom = bounds.bottom;

	int32 block_size = settings.block_size;

	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	int32 step_size = 1;
	if (settings.reduce_resolution)
		step_size = block_size;

	for (int32 y = bounds_top; y <= bounds_bottom; y += step_size) {
		for (int32 x = bounds_left; x <= bounds_right; x += step_size) {
			int32 right = min_c(x + block_size - 1, bounds_right);
			int32 bottom = min_c(y + block_size - 1, bounds_bottom);

			float red = 0;
			float green = 0;
			float blue = 0;
			float alpha = 0;

			float divider = (right - x + 1) * (bottom - y + 1);

			for (int32 dy = y; dy <= bottom; dy++) {
				for (int32 dx = x; dx <= right; dx++) {
					color.word = *(source_bits + dy * source_bpr + dx);
					red += color.bytes[2];
					green += color.bytes[1];
					blue += color.bytes[0];
					alpha += color.bytes[3];
				}
			}
			red /= divider;
			green /= divider;
			blue /= divider;
			alpha /= divider;

			color.bytes[2] = red;
			color.bytes[1] = green;
			color.bytes[0] = blue;
			color.bytes[3] = alpha;

			if (settings.reduce_resolution) {
				for (int32 dy = y; dy <= bottom; dy++) {
					for (int32 dx = x; dx <= right; dx++) {
						if (selection == NULL || selection->IsEmpty() == true ||
							selection->ContainsPoint(dx, dy))
							*(target_bits + dy * target_bpr + dx) = color.word;
					}
				}
			} else if (selection == NULL || selection->IsEmpty() == true ||
				selection->ContainsPoint(x, y))
				*(target_bits + y * target_bpr + x) = color.word;
		}
	}
}


BView* AntiDithererManipulator::MakeConfigurationView(const BMessenger& target)
{
	if (config_view == NULL) {
		config_view = new AntiDithererManipulatorView(this,target);
		config_view->ChangeSettings(&settings);
	}

	return config_view;
}


ManipulatorSettings* AntiDithererManipulator::ReturnSettings()
{
	return new AntiDithererManipulatorSettings(settings);
}

void AntiDithererManipulator::ChangeSettings(ManipulatorSettings *s)
{
	AntiDithererManipulatorSettings *new_settings;
	new_settings = cast_as(s,AntiDithererManipulatorSettings);
	if (new_settings != NULL) {
		settings = *new_settings;
	}
}

const char* AntiDithererManipulator::ReturnName()
{
	return B_TRANSLATE("Anti-Dither");
}

const char* AntiDithererManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Attempts to reverse the effects of dithering.");
}




// -------------------------------------
AntiDithererManipulatorView::AntiDithererManipulatorView(AntiDithererManipulator *manip,
		const BMessenger& t)
	: WindowGUIManipulatorView()
{
	target = t;
	manipulator = manip;
	started_adjusting = FALSE;

	block_size_control = new BSpinner("blocksize", B_TRANSLATE("Block size:"),
		new BMessage(BLOCK_SIZE_ADJUSTED));
	block_size_control->SetMinValue(1);
	block_size_control->SetValue(1);

	reduce_resolution_box = new BCheckBox(
		"reduce_resolution", B_TRANSLATE("Reduce resolution"),
		new BMessage(REDUCE_RESOLUTION_ADJUSTED));
	reduce_resolution_box->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_ITEM_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(block_size_control)
			.AddGlue()
			.End()
		.Add(reduce_resolution_box)
		.SetInsets(B_USE_SMALL_INSETS)
		.End();
}

AntiDithererManipulatorView::~AntiDithererManipulatorView()
{
}

void AntiDithererManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();
	block_size_control->SetTarget(BMessenger(this));
	reduce_resolution_box->SetTarget(BMessenger(this));
}

void AntiDithererManipulatorView::AllAttached()
{
	block_size_control->SetValue(settings.block_size);
	reduce_resolution_box->SetValue(settings.reduce_resolution);
}

void AntiDithererManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case REDUCE_RESOLUTION_ADJUSTED:
			settings.reduce_resolution = reduce_resolution_box->Value();
			manipulator->ChangeSettings(&settings);
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;


		case BLOCK_SIZE_ADJUSTED:
			settings.block_size = block_size_control->Value();
			manipulator->ChangeSettings(&settings);
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}

void AntiDithererManipulatorView::ChangeSettings(ManipulatorSettings *set)
{
	AntiDithererManipulatorSettings *new_settings = cast_as(set,AntiDithererManipulatorSettings);

	if (set != NULL) {
		settings = *new_settings;

		BWindow *window = Window();
		if (window != NULL) {
			window->Lock();
			block_size_control->SetValue(settings.block_size);
			reduce_resolution_box->SetValue(settings.reduce_resolution);
			window->Unlock();
		}
	}
}
