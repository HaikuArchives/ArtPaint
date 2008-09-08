
/*

	Filename:	AirBrushTool.cpp
	Contents:	AirBrushTool-class definitions
	Author:		Heikki Suhonen

*/


#include <Debug.h>
#include <math.h>
#include <RadioButton.h>
#include <stdlib.h>

#define PI M_PI


#include "AirBrushTool.h"
#include "PixelOperations.h"
#include "Selection.h"
#include "StringServer.h"
#include "Cursors.h"
#include "CoordinateReader.h"
#include "RandomNumberGenerator.h"
#include "ImageUpdater.h"

AirBrushTool::AirBrushTool()
	: DrawingTool(StringServer::ReturnString(AIR_BRUSH_TOOL_NAME_STRING),AIR_BRUSH_TOOL)
{
	options = SIZE_OPTION | PRESSURE_OPTION | MODE_OPTION;
	number_of_options = 3;

	SetOption(SIZE_OPTION,1);
	SetOption(PRESSURE_OPTION,1);
	SetOption(MODE_OPTION,HS_AIRBRUSH_MODE);

	sqrt_table = new int32[5500];
	for (int32 i=0;i<5500;i++) {
		sqrt_table[i] = (int32)sqrt(i);
	}
}


AirBrushTool::~AirBrushTool()
{
	delete[] sqrt_table;
}


ToolScript* AirBrushTool::UseTool(ImageView *view,uint32 buttons,BPoint point,BPoint)
{
	// Wait for the last_updated_region to become empty
	while (last_updated_rect.IsValid() == TRUE)
		snooze(50 * 1000);

	BPoint prev_point;
	BWindow *window = view->Window();
	BBitmap *bitmap = view->ReturnImage()->ReturnActiveBitmap();
	BitmapDrawer *drawer = new BitmapDrawer(bitmap);
	Selection *selection = view->GetSelection();

	ToolScript *the_script = new ToolScript(type,settings,((PaintApplication*)be_app)->GetColor(TRUE));

	if (settings.mode == HS_AIRBRUSH_MODE) {		// Do the airbrush
		BRect bounds = bitmap->Bounds();
		BRect rc;

		prev_point = point - BPoint(1,1);
		last_updated_rect = BRect(point,point);
		uint32 target_color;
		while (buttons) {
			the_script->AddPoint(point);

			float half_size = settings.size/2;
			rgb_color c = ((PaintApplication*)be_app)->GetColor(TRUE);
			target_color = RGBColorToBGRA(c);
			// we should only consider points that are inside this rectangle
			rc = BRect(point.x-half_size,point.y-half_size,point.x+half_size,point.y+half_size);
			rc = rc & bounds;
			rc = rc & selection->GetBoundingRect();

			if (rc.IsValid() == TRUE) {
				int32 height = rc.IntegerHeight();
				int32 width = rc.IntegerWidth();
				int32 left = (int32)rc.left;
				int32 top = (int32)rc.top;
				if (selection->IsEmpty()) {
					for (int32 y=0;y<=height;y++) {
						int32 y_sqr = (int32)((point.y-rc.top-y)*(point.y-rc.top-y));
						for (int32 x=0;x<=width;x++) {
							int32 dx = (int32)(point.x-rc.left-x);
							float distance = sqrt_table[dx*dx + y_sqr];
							if (distance <= half_size) {
								float change = (half_size-distance)/half_size;
								change = change*(((float)settings.pressure)/100.0);


								// This is experimental for doing a real transparency
								// Seems to work quite well
								union {
									uint8 bytes[4];
									uint32 word;
								} color;
								color.word = drawer->GetPixel(left+x,top+y);
								if (color.bytes[3] != 0x00)
									drawer->SetPixel(left+x,top+y,mix_2_pixels_fixed(target_color,color.word,(uint32)(32768*change)));
								else {
									color.word = target_color;
									color.bytes[3] = 0x00;
									drawer->SetPixel(left+x,top+y,mix_2_pixels_fixed(target_color,color.word,(uint32)(32768*change)));
								}
							}
						}
					}
				}
				else {
					for (int32 y=0;y<=height;y++) {
						int32 y_sqr = (int32)((point.y-rc.top-y)*(point.y-rc.top-y));
						for (int32 x=0;x<=width;x++) {
							int32 dx = (int32)(point.x-rc.left-x);
							float distance = sqrt_table[dx*dx + y_sqr];
							if ((distance <= half_size) && (selection->ContainsPoint(left+x,top+y))) {
								float change = (half_size-distance)/half_size;
								change = change*(((float)settings.pressure)/100.0);

								// This is experimental for doing a real transparency
								// Seems to work quite well
								union {
									uint8 bytes[4];
									uint32 word;
								} color;
								color.word = drawer->GetPixel(left+x,top+y);
								if (color.bytes[3] != 0x00)
									drawer->SetPixel(left+x,top+y,mix_2_pixels_fixed(target_color,color.word,(uint32)(32768*change)));
								else {
									color.word = target_color;
									color.bytes[3] = 0x00;
									drawer->SetPixel(left+x,top+y,mix_2_pixels_fixed(target_color,color.word,(uint32)(32768*change)));
								}
							}
						}
					}
				}
			}

			window->Lock();
			if (rc.IsValid()) {
				view->UpdateImage(rc);
				view->Sync();
				last_updated_rect = last_updated_rect | rc;
			}
			view->getCoords(&point,&buttons);
			window->Unlock();
			snooze(20 * 1000);
		}
	}
	else if (settings.mode == HS_SPRAY_MODE) {	// Do the spray
		CoordinateReader *coordinate_reader = new CoordinateReader(view,NO_INTERPOLATION,FALSE,TRUE);
		RandomNumberGenerator *generator = new RandomNumberGenerator(0,10000);
		ImageUpdater *updater = new ImageUpdater(view,20000.0);
		prev_point = point;

		while (coordinate_reader->GetPoint(point) == B_NO_ERROR) {
			int32 flow = settings.pressure + 1;
			float width = settings.size;
			float angle;
			float opacity = 0.4;
			rgb_color c = ((PaintApplication*)be_app)->GetColor(TRUE);
			uint32 target_color = RGBColorToBGRA(c);

			BRect rc(point,point);

			if (selection->IsEmpty()) {
				if (point == prev_point) {
					for (int32 i=0;i<flow;i++) {
						float x = generator->UniformDistribution(0,width*.5);
						float y = generator->UniformDistribution(0,sqrt((width*.5)*(width*.5)-x*x));

						angle = generator->UniformDistribution(0,1.0)*2*PI;
						float old_x = x;
						x = cos(angle)*x - sin(angle)*y;
						y = sin(angle)*old_x + cos(angle)*y;
						BPoint new_point = point+BPoint(x,y);
						new_point.x = round(new_point.x);
						new_point.y = round(new_point.y);
						rc = rc | BRect(new_point,new_point);

						drawer->SetPixel(new_point,mix_2_pixels_fixed(target_color,drawer->GetPixel(new_point),(uint32)(32768*opacity)));
					}
				}
				else {
					BPoint center;
					for (int32 i=0;i<flow;i++) {
						float x = generator->UniformDistribution(0,width*.5);
						float y = generator->UniformDistribution(0,sqrt((width*.5)*(width*.5)-x*x));

						// Select center randomly from the line between prev_point and point.
						// This is done by doing a linear interpolation between the
						// two points and rounding the result to nearest integer.
						float a = generator->UniformDistribution(0,1.0);
						center.x = round(a*prev_point.x + (1.0-a)*point.x);
						center.y = round(a*prev_point.y + (1.0-a)*point.y);

						angle = generator->UniformDistribution(0,1.0)*2*PI;
						float old_x = x;
						x = cos(angle)*x - sin(angle)*y;
						y = sin(angle)*old_x + cos(angle)*y;
						BPoint new_point = center + BPoint(x,y);
						new_point.x = round(new_point.x);
						new_point.y = round(new_point.y);
						rc = rc | BRect(new_point,new_point);

						drawer->SetPixel(new_point,mix_2_pixels_fixed(target_color,drawer->GetPixel(new_point),(uint32)(32768*opacity)));
					}
				}
			}
			else {
				if (point == prev_point) {
					for (int32 i=0;i<flow;i++) {
						float x = generator->UniformDistribution(0,width*.5);
						float y = generator->UniformDistribution(0,sqrt((width*.5)*(width*.5)-x*x));

						angle = generator->UniformDistribution(0,1.0)*2*PI;
						float old_x = x;
						x = cos(angle)*x - sin(angle)*y;
						y = sin(angle)*old_x + cos(angle)*y;
						BPoint new_point = point+BPoint(x,y);
						new_point.x = round(new_point.x);
						new_point.y = round(new_point.y);
						rc = rc | BRect(new_point,new_point);

						if (selection->ContainsPoint(new_point))
							drawer->SetPixel(new_point,mix_2_pixels_fixed(target_color,drawer->GetPixel(new_point),(uint32)(32768*opacity)));
					}
				}
				else {
					BPoint center;
					for (int32 i=0;i<flow;i++) {
						float x = generator->UniformDistribution(0,width*.5);
						float y = generator->UniformDistribution(0,sqrt((width*.5)*(width*.5)-x*x));

						// Select center randomly from the line between prev_point and point.
						// This is done by doing a linear interpolation between the
						// two points and rounding the result to nearest integer.
						float a = generator->UniformDistribution(0,1.0);
						center.x = round(a*prev_point.x + (1.0-a)*point.x);
						center.y = round(a*prev_point.y + (1.0-a)*point.y);

						angle = generator->UniformDistribution(0,1.0)*2*PI;
						float old_x = x;
						x = cos(angle)*x - sin(angle)*y;
						y = sin(angle)*old_x + cos(angle)*y;
						BPoint new_point = center + BPoint(x,y);
						new_point.x = round(new_point.x);
						new_point.y = round(new_point.y);
						rc = rc | BRect(new_point,new_point);

						if (selection->ContainsPoint(new_point))
							drawer->SetPixel(new_point,mix_2_pixels_fixed(target_color,drawer->GetPixel(new_point),(uint32)(32768*opacity)));
					}
				}
			}
			prev_point = point;

			updater->AddRect(rc);
			last_updated_rect = last_updated_rect | rc;
		}
		updater->ForceUpdate();

		delete coordinate_reader;
		delete generator;
		delete updater;
	}

	delete drawer;

	return the_script;
}


int32 AirBrushTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_NO_ERROR;
}


BView* AirBrushTool::makeConfigView()
{
	AirBrushToolConfigView *target_view = new AirBrushToolConfigView(BRect(0,0,150,0),this);
	return target_view;
}

const void* AirBrushTool::ReturnToolCursor()
{
	return HS_SPRAY_CURSOR;
}

const char* AirBrushTool::ReturnHelpString(bool is_in_use)
{
	if (!is_in_use)
		return StringServer::ReturnString(AIR_BRUSH_TOOL_READY_STRING);
	else
		return StringServer::ReturnString(AIR_BRUSH_TOOL_IN_USE_STRING);
}



AirBrushToolConfigView::AirBrushToolConfigView(BRect rect, DrawingTool *t)
	: DrawingToolConfigView(rect,t)
{
	BMessage *message;
	BRect controller_frame = BRect(EXTRA_EDGE,EXTRA_EDGE,150+EXTRA_EDGE,EXTRA_EDGE);

	float divider;

	// First add the controller for size.
	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",SIZE_OPTION);
	message->AddInt32("value",tool->GetCurrentValue(SIZE_OPTION));
	size_slider = new ControlSliderBox(controller_frame,"size",StringServer::ReturnString(SIZE_STRING),"1",message,1,100);
	AddChild(size_slider);
	controller_frame = size_slider->Frame();
	controller_frame.OffsetBy(0,controller_frame.Height()+EXTRA_EDGE);

	// Then add the controller for flow.
	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",PRESSURE_OPTION);
	message->AddInt32("value",tool->GetCurrentValue(PRESSURE_OPTION));
	flow_slider = new ControlSliderBox(controller_frame,"size",StringServer::ReturnString(FLOW_STRING),"1",message,1,100);
	AddChild(flow_slider);
	controller_frame = flow_slider->Frame();
	controller_frame.OffsetBy(0,controller_frame.Height()+EXTRA_EDGE);
	divider = max_c(flow_slider->Divider(),size_slider->Divider());
	size_slider->SetDivider(divider);
	flow_slider->SetDivider(divider);

	controller_frame = flow_slider->Frame();
	controller_frame.OffsetBy(0,controller_frame.Height()+EXTRA_EDGE);
	controller_frame.bottom = controller_frame.top;
	// Add two radio-buttons for defining the mode of selector.
	// Add a menu for defining the shape of the selector.

	// Create a BBox for the radio-buttons.
	BBox *container = new BBox(controller_frame,"airbrush_tool_type");
	container->SetLabel(StringServer::ReturnString(MODE_STRING));
	AddChild(container);

	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",MODE_OPTION);
	message->AddInt32("value",HS_AIRBRUSH_MODE);

	// Create the first radio-button.
	font_height fHeight;
	container->GetFontHeight(&fHeight);
	mode_button_1 = new BRadioButton(BRect(EXTRA_EDGE,EXTRA_EDGE+fHeight.descent+fHeight.ascent/2,EXTRA_EDGE,EXTRA_EDGE+fHeight.descent+fHeight.ascent/2),"a radio button",StringServer::ReturnString(AIRBRUSH_STRING),new BMessage(*message));
	mode_button_1->ResizeToPreferred();
	container->AddChild(mode_button_1);
	if (tool->GetCurrentValue(MODE_OPTION) == HS_AIRBRUSH_MODE)
		mode_button_1->SetValue(B_CONTROL_ON);

	// Create the second radio-button.
	message->ReplaceInt32("value",HS_SPRAY_MODE);
	mode_button_2 = new BRadioButton(BRect(EXTRA_EDGE,mode_button_1->Frame().bottom,EXTRA_EDGE,mode_button_1->Frame().bottom)," a radio button",StringServer::ReturnString(SPRAY_STRING),message);
	mode_button_2->ResizeToPreferred();
	container->AddChild(mode_button_2);
	if (tool->GetCurrentValue(MODE_OPTION) == HS_SPRAY_MODE)
		mode_button_2->SetValue(B_CONTROL_ON);

	container->ResizeBy(0,mode_button_2->Frame().bottom+EXTRA_EDGE);
	controller_frame.OffsetBy(0,container->Frame().Height() + EXTRA_EDGE);

	ResizeTo(container->Bounds().Width()+2*EXTRA_EDGE,container->Frame().bottom + EXTRA_EDGE);
}


void AirBrushToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();
	size_slider->SetTarget(new BMessenger(this));
	flow_slider->SetTarget(new BMessenger(this));
	mode_button_1->SetTarget(BMessenger(this));
	mode_button_2->SetTarget(BMessenger(this));
}
