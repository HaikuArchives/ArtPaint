/* 

	Filename:	ContrastManipulator.cpp
	Contents:	A template for the ArtPaint add-ons.	
	Author:		Heikki Suhonen
	
*/

#include "AddOnTypes.h"
#include "ContrastManipulator.h"

extern "C" ViewManipulator* start_manipulator(BView*,manipulator_data&);

#pragma export on
bool creates_gui = FALSE;		// If this is true, we should inherit from GUIManipilator.
char name[255] = "Enhance Contrast";
int32 add_on_api_version = ADD_ON_API_VERSION;
add_on_types add_on_type = COLOR_ADD_ON;


ViewManipulator* start_manipulator(BView *target_view,manipulator_data &data)
{
	// Here create a view-manipulator. The class should inherit
	// from the ViewManipulator base-class. It will be deleted
	// in the application program.
	
	printf("Add-on start_manipulator\n");
	return new ContrastManipulator(target_view,data);	
}

#pragma export off



ContrastManipulator::ContrastManipulator(BView *target,manipulator_data &data)
		: BasicManipulator(target)
{
	target_bitmap = data.current_layer;
}


ContrastManipulator::~ContrastManipulator()
{

}


BBitmap* ContrastManipulator::ManipulateBitmap(BBitmap *original, BView *progress_view, float prog_step)
{
	// We may create another bitmap and return it instead of original, but we may
	// also do the manipulation on the original and return it. We Should send messages
	// to pregress_view that contain B_UPDATE_STATUS_BAR as their 'what'. The sum of
	// message deltas should equal 100*prog_step.
	uint32 *bits = (uint32*)original->Bits();
	int32 bits_length = original->BitsLength()/4;
	
	
	// This manipulator will re-map the image's color-values using a logarithmic function.
	
	// These maps contain the values that particular values of component map to.
	// The mapping is logarithmic (i.e r_map[i] == log(f(i)), where f(i) is linear.	
	uint32 r_map[256];
	uint32 g_map[256];
	uint32 b_map[256];
		
	// Here generate the mappings of colors.
	// The value i of red should map to log(i+1)/log(256)*255
	for (float i=0;i<256;i++) {
		r_map[(int32)i] = (((uint32)((float)log(i+1)/(float)log(256)*255.0)) << 8) & 0x0000FF00;
		g_map[(int32)i] = (((uint32)((float)log(i+1)/(float)log(256)*255.0)) << 16) & 0x00FF0000;
		b_map[(int32)i] = (((uint32)((float)log(i+1)/(float)log(256)*255.0)) << 24) & 0xFF000000;
	}

	// Here change the pixel values according to mapping.
	bits = (uint32*)original->Bits();
	for (int32 i=0;i<bits_length;i++) {
		*bits = r_map[(*bits >> 8) & 0xFF] | g_map[(*bits >> 16) & 0xFF] | b_map[(*bits >> 24) & 0xFF] | (*bits & 0xFF);
		bits++;
	}

	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta",0.0);
	if (progress_view != NULL) {
		progress_message.ReplaceFloat("delta",100*prog_step);
		progress_view->Window()->PostMessage(&progress_message,progress_view);
	}

	return original;
}