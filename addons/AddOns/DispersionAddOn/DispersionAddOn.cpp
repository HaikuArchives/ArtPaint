/* 

	Filename:	AddOnTemplate.cpp
	Contents:	A template for the ArtPaint add-ons.	
	Author:		Heikki Suhonen
	
*/

#include <Node.h>
#include <StatusBar.h>
#include <stdlib.h>
#include <string.h>
#include <Window.h>

#include "AddOns.h"
#include "DispersionAddOn.h"
#include "BitmapDrawer.h"

extern "C" __declspec(dllexport) char name[255] = "Dispersion";
extern "C" __declspec(dllexport) char menu_help_string[255] = "Changes pixel positions randomly a little bit.";
extern "C" __declspec(dllexport) int32 add_on_api_version = ADD_ON_API_VERSION;
extern "C" __declspec(dllexport) add_on_types add_on_type = EFFECT_FILTER_ADD_ON;


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new DispersionManipulator(bm);	
}




DispersionManipulator::DispersionManipulator(BBitmap*)
		: Manipulator()
{
}


DispersionManipulator::~DispersionManipulator()
{

}

BBitmap* DispersionManipulator::ManipulateBitmap(BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	BWindow *status_bar_window = NULL;
	if (status_bar != NULL)
		status_bar_window = status_bar->Window();

	BBitmap *spare_buffer = DuplicateBitmap(original);				
	BitmapDrawer *target = new BitmapDrawer(original);
	
	int32 width = original->Bounds().Width() + 1;
	int32 height = original->Bounds().Height() + 1;

	uint32 *spare_bits = (uint32*)spare_buffer->Bits();
	uint32 moved_pixel;

	int32 dx,dy;

	if ((selection == NULL) || (selection->IsEmpty() == TRUE)) {	
		for (int32 y=0;y<height;y++) {
			for (int32 x=0;x<width;x++) {
				moved_pixel = *spare_bits++;
				dx = (rand() % MAX_DISPERSION_X) * (rand() % 2 == 0 ? -1 : 1);				
				dy = (rand() % MAX_DISPERSION_Y) * (rand() % 2 == 0 ? -1 : 1);				
				target->SetPixel(BPoint(x+dx,y+dy),moved_pixel);	
			}
			if (((y % 20) == 0) && (status_bar_window != NULL) && (status_bar != NULL) && (status_bar_window->LockWithTimeout(0) == B_OK)) {
				status_bar->Update(100.0/height*20);
				status_bar_window->Unlock();
			}
		}	
	}
	else {
		for (int32 y=0;y<height;y++) {
			for (int32 x=0;x<width;x++) {
				moved_pixel = *spare_bits++;
				dx = (rand() % MAX_DISPERSION_X) * (rand() % 2 == 0 ? -1 : 1);				
				dy = (rand() % MAX_DISPERSION_Y) * (rand() % 2 == 0 ? -1 : 1);				
				target->SetPixel(BPoint(x+dx,y+dy),moved_pixel,selection);	
			}
			if (((y % 20) == 0) && (status_bar_window != NULL) && (status_bar != NULL) && (status_bar_window->LockWithTimeout(0) == B_OK)) {
				status_bar->Update(100.0/height*20);
				status_bar_window->Unlock();
			}
		}		
	}
	// we should also delete the spare-bitmap
	delete spare_buffer;
	delete target;

	return original;
}