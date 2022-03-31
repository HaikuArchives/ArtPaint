/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
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


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = "Anti-Dither…";
	char menu_help_string[255] = "Attempts to reverse the effects of dithering.";
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
		: WindowGUIManipulator()
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


BBitmap* AntiDithererManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)
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

int32 AntiDithererManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
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


void AntiDithererManipulator::Reset(Selection*)
{
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with a loop.
		uint32 *source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target,source,bits_length);
	}
}


void AntiDithererManipulator::anti_dither()
{
	uint32 *source_bits = (uint32*)source_bitmap->Bits();
	uint32 *target_bits = (uint32*)target_bitmap->Bits();

	int32 source_bpr = source_bitmap->BytesPerRow()/4;
	int32 target_bpr = target_bitmap->BytesPerRow()/4;

	int32 width = source_bitmap->Bounds().IntegerWidth();
	int32 height = source_bitmap->Bounds().IntegerHeight();

	int32 block_size = settings.block_size;

	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	if (settings.reduce_resolution) {
		for (int32 y=0;y<=height;y += block_size) {
			for (int32 x=0;x<=width;x += block_size) {
				int32 right = min_c(x+block_size-1,width);
				int32 bottom = min_c(y+block_size-1,height);

				float red = 0;
				float green = 0;
				float blue = 0;

				float divider = (right-x+1)*(bottom-y+1);

				for (int32 dy=y;dy<=bottom;dy++) {
					for (int32 dx=x;dx<=right;dx++) {
						color.word = *(source_bits + dy*source_bpr + dx);
						red += color.bytes[2];
						green += color.bytes[1];
						blue += color.bytes[0];
					}
				}
				red /= divider;
				green /= divider;
				blue /= divider;

				color.bytes[2] = red;
				color.bytes[1] = green;
				color.bytes[0] = blue;
				color.bytes[3] = 255;

				for (int32 dy=y;dy<=bottom;dy++) {
					for (int32 dx=x;dx<=right;dx++) {
						*(target_bits + dy*target_bpr + dx) = color.word;
					}
				}
			}
		}
	}
	else {
		for (int32 y=0;y<=height;y++) {
			for (int32 x=0;x<=width;x++) {
				int32 right = min_c(x+block_size-1,width);
				int32 bottom = min_c(y+block_size-1,height);

				float red = 0;
				float green = 0;
				float blue = 0;

				float divider = (right-x+1)*(bottom-y+1);

				for (int32 dy=y;dy<=bottom;dy++) {
					for (int32 dx=x;dx<=right;dx++) {
						color.word = *(source_bits + dy*source_bpr + dx);
						red += color.bytes[2];
						green += color.bytes[1];
						blue += color.bytes[0];
					}
				}
				red /= divider;
				green /= divider;
				blue /= divider;

				color.bytes[2] = red;
				color.bytes[1] = green;
				color.bytes[0] = blue;
				color.bytes[3] = 255;

				*(target_bits + y*target_bpr + x) = color.word;
			}
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

char* AntiDithererManipulator::ReturnName()
{
	return "Anti-Dither";
}

char* AntiDithererManipulator::ReturnHelpString()
{
	return "Attempts to reverse the effects of dithering.";
}




// -------------------------------------
AntiDithererManipulatorView::AntiDithererManipulatorView(AntiDithererManipulator *manip,
		const BMessenger& t)
	: WindowGUIManipulatorView()
{
	target = t;
	manipulator = manip;
	started_adjusting = FALSE;

	block_size_control = new BSpinner("blocksize", "Block size:",
		new BMessage(BLOCK_SIZE_ADJUSTED));
	block_size_control->SetMinValue(1);
	block_size_control->SetValue(1);

	reduce_resolution_box = new BCheckBox(
		"reduce_resolution","Reduce resolution",new BMessage(REDUCE_RESOLUTION_ADJUSTED));
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
