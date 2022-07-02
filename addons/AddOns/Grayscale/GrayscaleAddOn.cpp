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
#include <Message.h>
#include <StatusBar.h>
#include <Window.h>

#include "AddOns.h"
#include "ManipulatorInformer.h"
#include "GrayscaleAddOn.h"
#include "Selection.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddOns_Grayscale"


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = B_TRANSLATE_MARK("Grayscale");
	char menu_help_string[255] = B_TRANSLATE_MARK("Converts the active layer to grayscale.");
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = COLOR_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new GrayscaleAddOnManipulator(bm);
}


GrayscaleAddOnManipulator::GrayscaleAddOnManipulator(BBitmap*)
		: Manipulator()
{
}


GrayscaleAddOnManipulator::~GrayscaleAddOnManipulator()
{

}


BBitmap* GrayscaleAddOnManipulator::ManipulateBitmap(BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	// We may create another bitmap and return it instead of original, but we may
	// also do the manipulation on the original and return it. We Should send messages
	// to progress_view that contain B_UPDATE_STATUS_BAR as their 'what'. The sum of
	// message deltas should equal 100*prog_step.
	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta",0.0);
	if (status_bar != NULL) {
		progress_message.ReplaceFloat("delta",100);
		status_bar->Window()->PostMessage(&progress_message,status_bar);
	}

	int32 bits_length = original->BitsLength()/4;
	uint32 *bits = (uint32*)original->Bits();
	float blue,red,green;
	union {
		uint8 bytes[4];
		uint32 word;
	} color;
	if ((selection == NULL) || (selection->IsEmpty() == TRUE)) {
		for (int32 i=0;i<bits_length;i++) {
			color.word = *bits;
			blue = color.bytes[0] * 0.114;
			green = color.bytes[1] * 0.587;
			red = color.bytes[2] * 0.299;
			float sum = blue + red + green;
			color.bytes[0] = sum;
			color.bytes[1] = sum;
			color.bytes[2] = sum;
			*bits++ = color.word;
		}
	}
	else {
		int32 width = original->Bounds().Width();
		int32 height = original->Bounds().Height();
		for (int32 y=0;y<=height;y++) {
			for (int32 x=0;x<=width;x++) {
				if (selection->ContainsPoint(x,y)) {
					color.word = *bits;
					blue = color.bytes[0] * 0.114;
					green = color.bytes[1] * 0.587;
					red = color.bytes[2] * 0.299;
					float sum = blue + red + green;
					color.bytes[0] = sum;
					color.bytes[1] = sum;
					color.bytes[2] = sum;
					*bits++ = color.word;
				}
				else {
					++bits;
				}
			}
		}
	}
	return original;
}


const char* GrayscaleAddOnManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Converts the active layer to grayscale.");
}


const char*	GrayscaleAddOnManipulator::ReturnName()
{
	return B_TRANSLATE("Grayscale");
}
