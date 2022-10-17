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
#include "EmbossAddOn.h"
#include "ManipulatorInformer.h"
#include "Selection.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddOns_Emboss"


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = B_TRANSLATE_MARK("Emboss");
	char menu_help_string[255] = B_TRANSLATE_MARK("Creates an emboss effect.");
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = EFFECT_FILTER_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap*,ManipulatorInformer *i)
{
	delete i;
	return new EmbossManipulator();
}




EmbossManipulator::EmbossManipulator()
		: Manipulator(),
		selection(NULL)
{
}


EmbossManipulator::~EmbossManipulator()
{
}

// these are used when calculating the new pixel-value
#define LT (*(spare_bits-spare_bpr-1))
#define RB (*(spare_bits+spare_bpr+1))

BBitmap* EmbossManipulator::ManipulateBitmap(BBitmap *original,
	BStatusBar *progress_view)
{
	// We may create another bitmap and return it instead of original, but we may
	// also do the manipulation on the original and return it. We Should send messages
	// to pregress_view that contain B_UPDATE_STATUS_BAR as their 'what'. The sum of
	// message deltas should equal 100*prog_step.

	// Here manipulate the bitmap in what way is required
	// , but do not delete it.
	/*

		this will do a simple emboss-effect by using the following matrix

				-1	0	0
				0	0	0
				0	0	1

		it should also be checked that the pixel values do not go to negative
		, if that happens we should replace them with 0

	*/

	original->Lock();

	// first we will create a spare-buffer and copy data from original to it
	// the spare-buffer should be 1 pixel larger in each direction than the
	// actual buffer
	BRect a_rect = original->Bounds();
//	a_rect.InsetBy(-1,-1);
//	BBitmap *spare_buffer = new BBitmap(a_rect,B_RGB_32_BIT);
	BBitmap *spare_buffer = DuplicateBitmap(original,-1);
	// here we should copy the buffer to spare and pad the edges twice
	int32 *buffer_bits = (int32*)original->Bits();
	int32 buffer_bpr = original->BytesPerRow()/4;

	int32 *spare_bits = (int32*)spare_buffer->Bits();
	int32 spare_bpr = spare_buffer->BytesPerRow()/4;

	// here we can start doing the actual convolution
	spare_bits += spare_bpr + 1;

	int32 target_value;

	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta",0.0);

	union {
		uint8 bytes[4];
		uint32 word;
	} left_top,right_bottom,result;
	if (selection->IsEmpty() == TRUE) {
		// here we iterate through each pixel in spare_buffer omitting the border pixels
		for (int32 y=0;y<a_rect.Height()+1;y++) {
			for (int32 x=0;x<a_rect.Width()+1;x++) {
				/*
					as this is a very simple filter we should only look at pixels
					at left top and right bottom and calculate the answer

							X1	0	0

							0	P	0

							0	0	-X1

					here P == X1 - X2
				*/
//				target_value = (((LT>>24)&0xFF) > ((RB>>24)&0xFF) ? (((LT>>24)&0xFF)-((RB>>24)&0xFF))<<24	 : 0x00000000)
//									| (((LT>>16)&0xFF) > ((RB>>16)&0xFF) ? (((LT>>16)&0xFF)-((RB>>16)&0xFF))<<16	 : 0x00000000)
//									| (((LT>>8)&0xFF) > ((RB>>8)&0xFF) ? (((LT>>8)&0xFF)-((RB>>8)&0xFF))<<8	 : 0x00000000)
//									| (*spare_bits & 0x000000FF);
//				*buffer_bits++ = target_value;
				left_top.word = LT;
				right_bottom.word = RB;
				result.word = *spare_bits;
				result.bytes[0] = max_c(min_c(255,127+right_bottom.bytes[0] - left_top.bytes[0]),0);
				result.bytes[1] = max_c(min_c(255,127+right_bottom.bytes[1] - left_top.bytes[1]),0);
				result.bytes[2] = max_c(min_c(255,127+right_bottom.bytes[2] - left_top.bytes[2]),0);

				*buffer_bits++ = result.word;
				spare_bits++;
			}
			spare_bits+=2;
			if ((progress_view != NULL) && ((y % 20) == 0)) {
				progress_message.ReplaceFloat("delta",20*100.0/(a_rect.Height()+1));
				progress_view->Window()->PostMessage(&progress_message,progress_view);
			}
		}
	}
	else {
		BRect frame = selection->GetBoundingRect();

		int32 left = frame.left;
		int32 right = frame.right;
		int32 top = frame.top;
		int32 bottom = frame.bottom;
		int32 *spare_origin = spare_bits;
		for (int32 y=top;y<=bottom;y++) {
			int32 y_buffer_bpr = y*buffer_bpr;
			for (int32 x=left;x<=right;x++) {
				if (selection->ContainsPoint(x,y) == TRUE) {
					spare_bits = spare_origin + y*spare_bpr + x;
//					target_value = (((LT>>24)&0xFF) > ((RB>>24)&0xFF) ? (((LT>>24)&0xFF)-((RB>>24)&0xFF))<<24	 : 0x00000000)
//									| (((LT>>16)&0xFF) > ((RB>>16)&0xFF) ? (((LT>>16)&0xFF)-((RB>>16)&0xFF))<<16	 : 0x00000000)
//									| (((LT>>8)&0xFF) > ((RB>>8)&0xFF) ? (((LT>>8)&0xFF)-((RB>>8)&0xFF))<<8	 : 0x00000000)
//									| (*spare_bits & 0x000000FF);
//					*(buffer_bits + y*buffer_bpr + x) = target_value;
					left_top.word = LT;
					right_bottom.word = RB;
					result.word = *spare_bits;
					result.bytes[0] = max_c(min_c(255,127+right_bottom.bytes[0] - left_top.bytes[0]),0);
					result.bytes[1] = max_c(min_c(255,127+right_bottom.bytes[1] - left_top.bytes[1]),0);
					result.bytes[2] = max_c(min_c(255,127+right_bottom.bytes[2] - left_top.bytes[2]),0);

					*(buffer_bits + y_buffer_bpr + x) = result.word;
				}
			}
		}
	}
	original->Unlock();

	// we should also delete the spare-bitmap
	delete spare_buffer;


	return original;
}


const char* EmbossManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Creates an emboss effect.");
}


const char*	EmbossManipulator::ReturnName()
{
	return B_TRANSLATE("Emboss");
}
