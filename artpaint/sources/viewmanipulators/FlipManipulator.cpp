/*

	Filename:	FlipManipulator.cpp
	Contents:	FlipManipulator -class definitions
	Author:		Heikki Suhonen

*/

#include <AppDefs.h>
#include <Message.h>
#include <StatusBar.h>
#include <Window.h>

#include "MessageConstants.h"
#include "FlipManipulator.h"
#include "ImageView.h"
#include "StringServer.h"

HorizFlipManipulator::HorizFlipManipulator()
	:	Manipulator()
{
}

BBitmap* HorizFlipManipulator::ManipulateBitmap(BBitmap *original,Selection*,BStatusBar *status_bar)
{
	int32 height = original->Bounds().IntegerHeight();
	int32 width = original->Bounds().IntegerWidth();
	int32 width_per_2 = width/2;
	uint32 spare;
	uint32 *bits = (uint32*)original->Bits();
	uint32 bpr = original->BytesPerRow()/4;

	for (int32 y=0;y<=height;y++) {
		for (int32 x=0;x<=width_per_2;x++) {
			spare = *(bits+width-x);
			*(bits + width-x) = *(bits + x);
			*(bits + x) = spare;
		}
		bits += bpr;

		if ((y%40 == 0) && (status_bar != NULL) && (status_bar->Window() != NULL)) {
			BMessage *a_message = new BMessage(B_UPDATE_STATUS_BAR);
			a_message->AddFloat("delta",40.0*100.0/(float)height);
			status_bar->Window()->PostMessage(a_message,status_bar);
			delete a_message;
		}
	}
	return original;
}

const char* HorizFlipManipulator::ReturnName()
{
	return StringServer::ReturnString(FLIP_HORIZONTAL_STRING);
}

VertFlipManipulator::VertFlipManipulator()
	:	Manipulator()
{
}

BBitmap* VertFlipManipulator::ManipulateBitmap(BBitmap *original,Selection*,BStatusBar *status_bar)
{
	int32 height = original->Bounds().IntegerHeight();
	int32 width = original->Bounds().IntegerWidth();
	int32 height_per_2 = height/2;
	uint32 spare;
	uint32 *bits = (uint32*)original->Bits();
	uint32 bpr = original->BytesPerRow()/4;

	for (int32 y=0;y<=height_per_2;y++) {
		for (int32 x=0;x<=width;x++) {
			spare = *bits;
			*bits = *(bits + (height - y-y)*bpr);
			*(bits + (height - y - y)*bpr) = spare;
			bits++;
		}

		if ((y%40 == 0) && (status_bar != NULL) && (status_bar->Window() != NULL)) {
			BMessage *a_message = new BMessage(B_UPDATE_STATUS_BAR);
			a_message->AddFloat("delta",40.0*100.0/(float)height_per_2);
			status_bar->Window()->PostMessage(a_message,status_bar);
			delete a_message;
		}
	}

	return original;
}

const char* VertFlipManipulator::ReturnName()
{
	return StringServer::ReturnString(FLIP_VERTICAL_STRING);
}
