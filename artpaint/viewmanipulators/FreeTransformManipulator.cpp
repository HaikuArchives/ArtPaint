/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "FreeTransformManipulator.h"
#include "PixelOperations.h"
#include "MessageConstants.h"


#include <Catalog.h>
#include <CheckBox.h>
#include <ClassInfo.h>
#include <Bitmap.h>
#include <Button.h>
#include <StatusBar.h>
#include <Window.h>


#include <math.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define PI M_PI

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FreeTransformManipulator"


FreeTransformManipulator::FreeTransformManipulator(BBitmap *bm)
	:	WindowGUIManipulator()
{
	configuration_view = NULL;

	preview_bitmap = NULL;
	copy_of_the_preview_bitmap = NULL;

	SetPreviewBitmap(bm);

	transformation_mode = 0;
}



FreeTransformManipulator::~FreeTransformManipulator()
{
	if (configuration_view != NULL) {
		configuration_view->RemoveSelf();
		delete configuration_view;
	}

	delete copy_of_the_preview_bitmap;
}



BBitmap*
FreeTransformManipulator::ManipulateBitmap(ManipulatorSettings *set,
	BBitmap *original, Selection*, BStatusBar *status_bar)
{
	// TODO: check what's the idea behind this

//	FreeTransformManipulatorSettings *new_settings =
//		cast_as(set,FreeTransformManipulatorSettings);
//	if (new_settings == NULL)
		return NULL;
}


int32 FreeTransformManipulator::PreviewBitmap(Selection*,bool,BRegion *region)
{
	if (preview_bitmap == NULL)
		return 0;

	FreeTransformManipulatorSettings current_settings = settings;
	if (current_settings == previous_settings)
		return 0;

	union {
		uint8 bytes[4];
		uint32 word;
	} white;

	white.bytes[0] = 0xFF;
	white.bytes[1] = 0xFF;
	white.bytes[2] = 0xFF;
	white.bytes[3] = 0x00;

	//int32 width = preview_bitmap->Bounds().IntegerWidth();
	//int32 height = preview_bitmap->Bounds().IntegerHeight();

	//uint32 *source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
	//uint32 *target_bits = (uint32*)preview_bitmap->Bits();
	//int32 bpr = preview_bitmap->BytesPerRow()/4;

	//float rad_angle = current_settings.rotation/180.0*PI;

	// 1. Translate by -width/2, -height/2
	// 2. Rotate by rad_angle
	// 3. Scale by dx and dy
	// 4. Translate by translation_x + width/2, translation_y + height/2

	return 1;
}

void FreeTransformManipulator::MouseDown(BPoint point,uint32 buttons,BView*,bool first_click)
{
	if (first_click == TRUE) {
		// Select the transformation-mode and record the starting-point.
		starting_point = point;

		uint32 mods = modifiers();
		if (mods & B_LEFT_SHIFT_KEY)
			transformation_mode = RESIZING_MODE;
		else if (mods & B_LEFT_CONTROL_KEY)
			transformation_mode = ROTATING_MODE;
		else
			transformation_mode = TRANSLATING_MODE;

	}
	else {
		// Do the appropriate transformation.
		switch (transformation_mode) {
			case RESIZING_MODE:
				settings.x_scale_factor = point.x/preview_bitmap->Bounds().Width();
				settings.y_scale_factor = point.y/preview_bitmap->Bounds().Height();
				break;
			case TRANSLATING_MODE:
				settings.x_translation += (starting_point.x - point.x);
				settings.y_translation += (starting_point.y - point.y);
				starting_point = point;
				break;
			case ROTATING_MODE:
				settings.rotation = starting_point.x - point.x;
				break;
		}
	}
}



void FreeTransformManipulator::ChangeSettings(ManipulatorSettings *s)
{
	// change the values for controls whose values have changed
	FreeTransformManipulatorSettings* newSettings = cast_as(s, FreeTransformManipulatorSettings);
	if (newSettings)
		settings = *newSettings;
}


void FreeTransformManipulator::Reset(Selection*)
{
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with loop.
		uint32 *source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target,source,bits_length);
	}
}

void FreeTransformManipulator::SetPreviewBitmap(BBitmap *bitmap)
{
	if ((bitmap == NULL) || (preview_bitmap == NULL)
		|| (bitmap->Bounds() != preview_bitmap->Bounds())) {
		try {
			if (preview_bitmap)
				delete copy_of_the_preview_bitmap;

			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;

			if (bitmap) {
				preview_bitmap = bitmap;
				copy_of_the_preview_bitmap = DuplicateBitmap(preview_bitmap);
			}
		} catch (std::bad_alloc e) {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap=NULL;
			throw e;
		}
	} else {
		// Just update the copy_of_the_preview_bitmap
		preview_bitmap = bitmap;
		uint32 *source = (uint32*)preview_bitmap->Bits();
		uint32 *target = (uint32*)copy_of_the_preview_bitmap->Bits();
		int32 bitslength = min_c(preview_bitmap->BitsLength(),
			copy_of_the_preview_bitmap->BitsLength());
		memcpy(target, source, bitslength);
	}
}


const char* FreeTransformManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Click on the image to rotate, move or stretch it.");
}


const char* FreeTransformManipulator::ReturnName()
{
	return B_TRANSLATE("Free 2D transform");
}


ManipulatorSettings* FreeTransformManipulator::ReturnSettings()
{
	return new FreeTransformManipulatorSettings(settings);
}

BView* FreeTransformManipulator::MakeConfigurationView(const BMessenger& target)
{
	configuration_view = new (std::nothrow) FreeTransformManipulatorView(this,
		target);
	if (configuration_view)
		configuration_view->ChangeSettings(&settings);
	return configuration_view;
}


// #pragma mark -- FreeTransformManipulatorView


FreeTransformManipulatorView::FreeTransformManipulatorView(
		FreeTransformManipulator* manipulator, const BMessenger& target)
	: WindowGUIManipulatorView()
{
}


FreeTransformManipulatorView::~FreeTransformManipulatorView()
{
}


void
FreeTransformManipulatorView::AttachedToWindow()
{
}


void
FreeTransformManipulatorView::MessageReceived(BMessage *message)
{
	WindowGUIManipulatorView::MessageReceived(message);
}


/*
void
FreeTransformManipulatorView::SetValues(float width, float height)
{
	original_width = width;
	original_height = height;
	current_width = width;
	current_height = height;

	char text[256];

	BWindow *window = Window();
	if (window != NULL)
		window->Lock();

	sprintf(text,"%.0f",original_width);
	BTextControl *text_control = cast_as(FindView("width_control"),BTextControl);
	if (text_control != NULL) {
		text_control->SetText(text);
	}
	sprintf(text,"%.0f",original_height);

	text_control = cast_as(FindView("height_control"),BTextControl);
	if (text_control != NULL) {
		text_control->SetText(text);
	}

	if (window != NULL)
		window->Unlock();
}
*/


void
FreeTransformManipulatorView::ChangeSettings(ManipulatorSettings *s)
{
	// change the values for controls whose values have changed
	FreeTransformManipulatorSettings* newSettings =
		dynamic_cast<FreeTransformManipulatorSettings*> (s);
	if (newSettings && *newSettings != settings)
		settings = *newSettings;
}
