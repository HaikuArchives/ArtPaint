/* 

	Filename:	HistogramAddOn.cpp
	Contents:	An add-on that generates images histogram.	
	Author:		Heikki Suhonen
	 
*/

#include "HistogramAddOn.h"
#include "ViewManipulator.h"
#include "AddOnTypes.h"

extern "C" bool manipulate_buffer(BBitmap*);
extern "C" ViewManipulator* start_manipulator(BView*,manipulator_data &data);


#pragma export on
bool creates_gui = TRUE;
char name[255] = "Histogram…";
int32 add_on_api_version = ADD_ON_API_VERSION;
add_on_types add_on_type = ANALYZER_ADD_ON;

bool manipulate_buffer(BBitmap *bitmap)
{
	return TRUE;
}


ViewManipulator* start_manipulator(BView *target_view,manipulator_data &data)
{
	// Here create a view-manipulator. The class should inherit
	// from the ViewManipulator base-class. It will be deleted
	// in the application program.
	
//	printf("Add-on start_manipulator\n");
	return new HistogramManipulator(target_view,data);	
}

#pragma export off




HistogramManipulator::HistogramManipulator(BView *target,manipulator_data &data)
	: ViewManipulator(target)
{	
	the_buffer = data.composite_picture;
	the_window = new HistogramWindow(target,this);
	the_window->Show();
	node = data.node;
}



HistogramManipulator::~HistogramManipulator()
{
	// The window might already have been deleted.
	// delete the_window;
	delete node;
}


void HistogramManipulator::MouseDown(BPoint location,uint32 buttons,uint32 modifiers,GET_MOUSE)
{
}


BBitmap* HistogramManipulator::generateHistogram(int32 color)
{

	int32 histogram_array[256];
	int32 max_value = 0;

	// Initialize the histogram array	
	for (int32 i=0;i<256;i++)
		histogram_array[i] = 0;
		
	uint32 *bits = (uint32*)the_buffer->Bits();
	int32 bits_length = the_buffer->BitsLength() / 4;
	int32 index;
	uint32 graph_color;
	if (color == RED_HISTOGRAM)
		graph_color = 0x0000FF00;
	else if (color == GREEN_HISTOGRAM)
		graph_color = 0x00FF0000;
	else
		graph_color = 0xFF000000;
	
	// Generate the histogram		
	for (int32 i=0;i<bits_length;i++) {
		if (color == RED_HISTOGRAM)
			index = (*bits >> 8) & 0xff;
		else if (color == GREEN_HISTOGRAM)
			index = (*bits >> 16) & 0xff;
		else if (color == BLUE_HISTOGRAM)
			index = (*bits >> 24) & 0xff;
			
		index = min_c(index,255);
		index = max_c(index,0);
		
		histogram_array[index]++;
		max_value = max_c(histogram_array[index],max_value);
		bits++;
	}		
	

	BBitmap *histogram = new BBitmap(BRect(0,0,255,99),B_RGB_32_BIT);
	bits = (uint32*)histogram->Bits();
	
	for (int32 y=0;y<histogram->Bounds().Height()+1;y++) {
		for (int32 x=0;x<histogram->Bounds().Width()+1;x++) {
			if (((float)histogram_array[x]/(float)max_value*100) >= (100 - y) )
				*bits++ = graph_color;
			else {
				if ((y%10 == 0) && (x%2 == 0))
					*bits++ = 0xFFFFFF00;
				else
					*bits++ = 0xC0C0C000;
			}
		}
	}

	return histogram;
}


HistogramWindow::HistogramWindow(BView *target_view,HistogramManipulator *manipulator)
	: BWindow(BRect(300,300,300,300),"Histogram",B_TITLED_WINDOW,B_NOT_RESIZABLE)
{
	target = target_view;
	the_manipulator = manipulator;
	histogram = the_manipulator->generateHistogram(RED_HISTOGRAM);
	
	bgview = new BitmapView(histogram,BPoint(1,1));
	
	BMenuBar *color_menu = new BMenuBar(BRect(3,3,3,3),"color menu",B_FOLLOW_LEFT|B_FOLLOW_TOP);
	BPopUpMenu *color_pop_up_menu = new BPopUpMenu("color pop up menu");
	color_pop_up_menu->AddItem(new BMenuItem("Red",new BMessage(RED_HISTOGRAM)));
	color_pop_up_menu->AddItem(new BMenuItem("Blue",new BMessage(BLUE_HISTOGRAM)));
	color_pop_up_menu->AddItem(new BMenuItem("Green",new BMessage(GREEN_HISTOGRAM)));
	color_pop_up_menu->SetRadioMode(TRUE);
	color_pop_up_menu->ItemAt(0)->SetMarked(TRUE);
	color_menu->AddItem(color_pop_up_menu);
		
	bgview->AddChild(color_menu);
	AddChild(bgview);	
	ResizeTo(bgview->Bounds().Width()+2,bgview->Bounds().Height()+2);
}


HistogramWindow::~HistogramWindow()
{
	BMessage *a_message = new BMessage(HS_OPERATION_FINISHED);
	bool status = FALSE;
	a_message->AddBool("status",status);
	target->Window()->PostMessage(a_message,target);
	delete a_message;

	bgview->RemoveSelf();
	delete bgview;
	delete histogram;
}


void HistogramWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case RED_HISTOGRAM: case GREEN_HISTOGRAM: case BLUE_HISTOGRAM:
			BBitmap *old_hist = histogram;
			histogram = the_manipulator->generateHistogram(message->what);
			bgview->ChangeBitmap(histogram);
			bgview->Invalidate();
			delete old_hist;
			break;
		default:
			inherited::MessageReceived(message);
			break;
	}
}
