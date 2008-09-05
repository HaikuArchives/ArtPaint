/* 

	Filename:	NegativeAddOn.cpp
	Contents:	A template for the ArtPaint add-ons.	
	Author:		Heikki Suhonen
	
*/

#include <Message.h>
#include <StatusBar.h>
#include <Window.h>

#include "AddOns.h"
#include "NegativeAddOn.h"

extern "C" __declspec(dllexport) char name[255] = "Negative";
extern "C" __declspec(dllexport) char menu_help_string[255] = "Makes a negative of the active layer.";
extern "C" __declspec(dllexport) int32 add_on_api_version = ADD_ON_API_VERSION;
extern "C" __declspec(dllexport) add_on_types add_on_type = COLOR_ADD_ON;


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new NegativeAddOnManipulator(bm);	
}




NegativeAddOnManipulator::NegativeAddOnManipulator(BBitmap*)
		: Manipulator()
{
}


NegativeAddOnManipulator::~NegativeAddOnManipulator()
{

}


BBitmap* NegativeAddOnManipulator::ManipulateBitmap(BBitmap *original, Selection *selection, BStatusBar *status_bar)
{
	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta",0.0);
	if (status_bar != NULL) {
		progress_message.ReplaceFloat("delta",100);
		status_bar->Window()->PostMessage(&progress_message,status_bar);
	}
	
	int32 bits_length = original->BitsLength()/4;
	uint32 *bits = (uint32*)original->Bits();
	union {
		uint8 bytes[4];
		uint32 word;
	} color;
	
	if (selection->IsEmpty() == TRUE) {
		for (int32 i=0;i<bits_length;i++) {
			color.word = *bits;
			color.bytes[0] = 255 - color.bytes[0];
			color.bytes[1] = 255 - color.bytes[1];
			color.bytes[2] = 255 - color.bytes[2];
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
					color.bytes[0] = 255 - color.bytes[0];
					color.bytes[1] = 255 - color.bytes[1];
					color.bytes[2] = 255 - color.bytes[2];
					*bits++ = color.word;
				}
				else
					++bits;
			}		
		}
	}	
	return original;
}