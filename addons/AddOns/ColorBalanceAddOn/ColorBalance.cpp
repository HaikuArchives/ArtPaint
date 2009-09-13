/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "AddOns.h"
#include "ColorBalance.h"

#include <Slider.h>


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = "Color Balance…";
	char menu_help_string[255] = "Starts adjusting the color balance of the active layer.";
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = COLOR_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new ColorBalanceManipulator(bm);
}




ColorBalanceManipulator::ColorBalanceManipulator(BBitmap *bm)
		: WindowGUIManipulator()
{
	preview_bitmap = NULL;
	copy_of_the_preview_bitmap = NULL;
	config_view = NULL;

	SetPreviewBitmap(bm);
	lowest_allowed_quality = 2;
	last_used_quality = 2;
}


ColorBalanceManipulator::~ColorBalanceManipulator()
{
	delete copy_of_the_preview_bitmap;

	if (config_view != NULL) {
		config_view->RemoveSelf();
		delete config_view;
		config_view = NULL;
	}
}

BBitmap* ColorBalanceManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	ColorBalanceManipulatorSettings *new_settings = dynamic_cast<ColorBalanceManipulatorSettings*>(set);

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	if ((new_settings->red_difference == 0) &&(new_settings->green_difference == 0) && (new_settings->blue_difference == 0))
		return NULL;

	BBitmap *source_bitmap;
	BBitmap *target_bitmap;
	BBitmap *new_bitmap = NULL;

	if (original == preview_bitmap) {
		target_bitmap = original;
		source_bitmap = copy_of_the_preview_bitmap;
	}
	else {
		target_bitmap = original;
		new_bitmap =  DuplicateBitmap(original,0);
		source_bitmap = new_bitmap;
	}


	uint32 *source_bits = (uint32*)source_bitmap->Bits();
	uint32 *target_bits = (uint32*)target_bitmap->Bits();
	int32 source_bpr = source_bitmap->BytesPerRow()/4;
	int32 target_bpr = target_bitmap->BytesPerRow()/4;


	int32 left = target_bitmap->Bounds().left;
	int32 right = target_bitmap->Bounds().right;
	int32 top = target_bitmap->Bounds().top;
	int32 bottom = target_bitmap->Bounds().bottom;

	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	if (selection->IsEmpty() == TRUE) {
		for (int32 y=top;y<=bottom;y++) {
			for (int32 x=left;x<=right;x++) {
				color.word = *source_bits++;
				color.bytes[0] = max_c(min_c(255,color.bytes[0]+new_settings->blue_difference),0);
				color.bytes[1] = max_c(min_c(255,color.bytes[1]+new_settings->green_difference),0);
				color.bytes[2] = max_c(min_c(255,color.bytes[2]+new_settings->red_difference),0);
				*target_bits++ = color.word;
			}
		}
	}
	else {
		for (int32 y=top;y<=bottom;y++) {
			for (int32 x=left;x<=right;x++) {
				if (selection->ContainsPoint(x,y)) {
					color.word = *source_bits++;
					color.bytes[0] = max_c(min_c(255,color.bytes[0]+new_settings->blue_difference),0);
					color.bytes[1] = max_c(min_c(255,color.bytes[1]+new_settings->green_difference),0);
					color.bytes[2] = max_c(min_c(255,color.bytes[2]+new_settings->red_difference),0);
					*target_bits++ = color.word;
				}
				else {
					*target_bits++ = *source_bits++;
				}
			}
		}
	}
	if (new_bitmap != NULL) {
		delete new_bitmap;
	}

	return original;
}


int32 ColorBalanceManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
{
	if ((settings == previous_settings) == FALSE) {
		previous_settings = settings;
		last_used_quality = lowest_allowed_quality;
	}
	else {
		last_used_quality = floor(last_used_quality/2.0);
	}
	if (full_quality == TRUE)
		last_used_quality = min_c(last_used_quality,1);

	uint32 *source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
	uint32 *target_bits = (uint32*)preview_bitmap->Bits();
	int32 source_bpr = copy_of_the_preview_bitmap->BytesPerRow()/4;
	int32 target_bpr = preview_bitmap->BytesPerRow()/4;

	int32 left = preview_bitmap->Bounds().left;
	int32 right = preview_bitmap->Bounds().right;
	int32 top = preview_bitmap->Bounds().top;
	int32 bottom = preview_bitmap->Bounds().bottom;


	if (last_used_quality > 0) {
		union {
			uint8 bytes[4];
			uint32 word;
		} color;
		uint8 blue_array[256];
		uint8 green_array[256];
		uint8 red_array[256];

		for (int32 i=0;i<256;i++) {
			blue_array[i] = max_c(min_c(255,i+settings.blue_difference),0);
			green_array[i] = max_c(min_c(255,i+settings.green_difference),0);
			red_array[i] = max_c(min_c(255,i+settings.red_difference),0);
		}

		if (selection->IsEmpty() == TRUE) {
			for (int32 y=top;y<=bottom;y += last_used_quality) {
				for (int32 x=left;x<=right;x += last_used_quality) {
					color.word = *(source_bits + x + y*source_bpr);
					color.bytes[0] = blue_array[color.bytes[0]];
					color.bytes[1] = green_array[color.bytes[1]];
					color.bytes[2] = red_array[color.bytes[2]];
					*(target_bits + x + y*target_bpr) = color.word;
				}
			}
		}
		else {
			for (int32 y=top;y<=bottom;y += last_used_quality) {
				for (int32 x=left;x<=right;x += last_used_quality) {
					if (selection->ContainsPoint(x,y)) {
						color.word = *(source_bits + x + y*source_bpr);
						color.bytes[0] = blue_array[color.bytes[0]];
						color.bytes[1] = green_array[color.bytes[1]];
						color.bytes[2] = red_array[color.bytes[2]];
						*(target_bits + x + y*target_bpr) = color.word;
					}
					else {
						*(target_bits + x + y*target_bpr) = *(source_bits + x + y*source_bpr);
					}
				}
			}
		}
	}

	updated_region->Set(preview_bitmap->Bounds());
	return last_used_quality;
}

void ColorBalanceManipulator::SetPreviewBitmap(BBitmap *bm)
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


void ColorBalanceManipulator::Reset(Selection*)
{
	if (preview_bitmap != NULL) {
		uint32 *source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target_bits = (uint32*)preview_bitmap->Bits();

		int32 bits_length = preview_bitmap->BitsLength()/4;

		for (int32 i=0;i<bits_length;i++)
			*target_bits++  = *source_bits++;
	}
}


BView* ColorBalanceManipulator::MakeConfigurationView(const BMessenger& target)
{
	config_view = new ColorBalanceManipulatorView(BRect(0,0,0,0),this,target);
	config_view->ChangeSettings(settings);

	return config_view;
}


ManipulatorSettings* ColorBalanceManipulator::ReturnSettings()
{
	return new ColorBalanceManipulatorSettings(&settings);
}

void ColorBalanceManipulator::ChangeSettings(ManipulatorSettings *s)
{
	ColorBalanceManipulatorSettings *new_settings = dynamic_cast<ColorBalanceManipulatorSettings*>(s);

	if (new_settings != NULL) {
		previous_settings = settings;
		settings = *new_settings;
	}
}



ColorBalanceManipulatorView::ColorBalanceManipulatorView(BRect rect,
		ColorBalanceManipulator *manip, const BMessenger& t)
	: WindowGUIManipulatorView()
{
	target = new BMessenger(t);
	manipulator = manip;
	preview_started = FALSE;
	rgb_color color;

	red_slider = new BSlider(BRect(0,0,150,0), "red_slider", NULL,
		new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), -255, 255,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	red_slider->SetLimitLabels("Less Red","More Red");
	red_slider->ResizeToPreferred();
	red_slider->SetModificationMessage(new BMessage(HS_MANIPULATOR_ADJUSTING_STARTED));
	color.red = 255;
	color.blue = 0;
	color.green = 0;
	color.alpha = 255;
	red_slider->SetBarColor(color);
	AddChild(red_slider);
	red_slider->MoveTo(4,4);
	BRect frame = red_slider->Frame();
	frame.OffsetBy(0,frame.Height()+4);

	green_slider = new BSlider(frame, "green_slider", NULL,
		new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), -255, 255,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	green_slider->SetLimitLabels("Less Green","More Green");
	green_slider->ResizeToPreferred();
	green_slider->SetModificationMessage(new BMessage(HS_MANIPULATOR_ADJUSTING_STARTED));
	color.red = 0;
	color.blue = 0;
	color.green = 255;
	color.alpha = 255;
	green_slider->SetBarColor(color);
	AddChild(green_slider);
	frame = green_slider->Frame();
	frame.OffsetBy(0,frame.Height()+4);

	blue_slider = new BSlider(frame, "blue_slider", NULL,
		new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), -255, 255,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	blue_slider->SetLimitLabels("Less Blue","More Blue");
	blue_slider->ResizeToPreferred();
	blue_slider->SetModificationMessage(new BMessage(HS_MANIPULATOR_ADJUSTING_STARTED));
	color.red = 0;
	color.blue = 255;
	color.green = 0;
	color.alpha = 255;
	blue_slider->SetBarColor(color);
	AddChild(blue_slider);

	ResizeTo(blue_slider->Bounds().Width()+8,blue_slider->Frame().bottom + 4);
}

ColorBalanceManipulatorView::~ColorBalanceManipulatorView()
{
	delete target;
}

void ColorBalanceManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();

	red_slider->SetTarget(BMessenger(this));
	blue_slider->SetTarget(BMessenger(this));
	green_slider->SetTarget(BMessenger(this));
}



void ColorBalanceManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case HS_MANIPULATOR_ADJUSTING_FINISHED:
			preview_started = FALSE;
			settings.red_difference = red_slider->Value();
			settings.blue_difference = blue_slider->Value();
			settings.green_difference = green_slider->Value();
			manipulator->ChangeSettings(&settings);
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case HS_MANIPULATOR_ADJUSTING_STARTED:
			settings.red_difference = red_slider->Value();
			settings.blue_difference = blue_slider->Value();
			settings.green_difference = green_slider->Value();
			manipulator->ChangeSettings(&settings);
			if (preview_started == FALSE) {
				preview_started = TRUE;
				target->SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
			}
			break;

		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}



void ColorBalanceManipulatorView::ChangeSettings(ColorBalanceManipulatorSettings s)
{
	settings = s;
}
