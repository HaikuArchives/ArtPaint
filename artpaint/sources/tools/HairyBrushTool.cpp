/*

	Filename:	HairyBrushTool.cpp
	Contents:	HairyBrushTool-class definitions.
	Author:		Heikki Suhonen

*/

#include <stdlib.h>


#include "HairyBrushTool.h"
#include "Selection.h"
#include "StringServer.h"
#include "Cursors.h"
#include "PixelOperations.h"
#include "CoordinateReader.h"
#include "ImageUpdater.h"
#include "RandomNumberGenerator.h"

HairyBrushTool::HairyBrushTool()
		: DrawingTool(StringServer::ReturnString(HAIRY_BRUSH_TOOL_NAME_STRING),HAIRY_BRUSH_TOOL)
{
	options = SIZE_OPTION | PRESSURE_OPTION | TOLERANCE_OPTION | CONTINUITY_OPTION;
	number_of_options = 3;
	SetOption(SIZE_OPTION,5);
	SetOption(PRESSURE_OPTION,2);
	SetOption(TOLERANCE_OPTION,10);
	SetOption(CONTINUITY_OPTION,20);
}


HairyBrushTool::~HairyBrushTool()
{
	// free whatever storage this class allocated
}


ToolScript* HairyBrushTool::UseTool(ImageView *view,uint32 buttons,BPoint point,BPoint)
{
	// here we first get the necessary data from view
	// and then start drawing while mousebutton is held down

	// Wait for the last_updated_region to become empty
	while (last_updated_rect.IsValid() == TRUE)
		snooze(50 * 1000);

//	coordinate_queue = new CoordinateQueue();
	image_view = view;
//	thread_id coordinate_reader = spawn_thread(CoordinateReader,"read coordinates",B_NORMAL_PRIORITY,this);
//	resume_thread(coordinate_reader);
//	reading_coordinates = TRUE;
	ToolScript *the_script = new ToolScript(type,settings,((PaintApplication*)be_app)->GetColor(TRUE));

	Selection *selection = view->GetSelection();

	BBitmap* buffer = view->ReturnImage()->ReturnActiveBitmap();
	BitmapDrawer *drawer = new BitmapDrawer(buffer);
	CoordinateReader *reader = new CoordinateReader(image_view,NO_INTERPOLATION,false,true);
	ImageUpdater *updater = new ImageUpdater(image_view,0.0);

	RandomNumberGenerator *random_stream = new RandomNumberGenerator(107 + (int32)point.x,1024);

	if (buffer == NULL) {
		delete the_script;
		return NULL;
	}

	BPoint prev_point;
	prev_point = point;
	BRect updated_rect;

	float initial_width = GetCurrentValue(PRESSURE_OPTION);
	float maximum_width = initial_width;//ceil(initial_width*1.5);
	float minimum_width = 0.5;//floor(max_c(2,initial_width*0.5));
	float current_width = initial_width;
	float color_randomness = GetCurrentValue(TOLERANCE_OPTION);
	float initial_color_amount = GetCurrentValue(CONTINUITY_OPTION) / 10.0;
	float color_amount_randomness = 4;
	rgb_color color =  ((PaintApplication*)be_app)->GetColor(TRUE);
	int32 hair_count = GetCurrentValue(SIZE_OPTION);

	float *color_amount_array = new float[hair_count];
	rgb_color *color_array = new rgb_color[hair_count];
	BPoint *start_point_array = new BPoint[hair_count];
	int32 *index_array = new int32[hair_count];

	for (int32 i=0;i<hair_count;i++) {
		float red,green,blue,alpha;
		color_amount_array[i] = initial_color_amount - color_amount_randomness + random()%1000/1000.0*color_amount_randomness*2.0;
		color_array[i] = ((PaintApplication*)be_app)->GetColor(TRUE);
		red = color_array[i].red;
		green = color_array[i].green;
		blue = color_array[i].blue;
		alpha = color_array[i].alpha;

		red = red - color_randomness + ((rand() % 1000) / 1000.0 * color_randomness * 2.0);
		red = min_c(255,max_c(red,0));

		green = green - color_randomness + ((rand() % 1000) / 1000.0 * color_randomness * 2.0);
		green = min_c(255,max_c(green,0));

		blue = blue - color_randomness + ((rand() % 1000) / 1000.0 * color_randomness * 2.0);
		blue = min_c(255,max_c(blue,0));

		alpha = alpha - color_randomness + ((rand() % 1000) / 1000.0 * color_randomness * 2.0);
		alpha = min_c(255,max_c(alpha,0));

		color_array[i].red = (uint8)red;
		color_array[i].green = (uint8)green;
		color_array[i].blue = (uint8)blue;

		start_point_array[i] = point;
	}

	last_updated_rect = BRect(point,point);
	the_script->AddPoint(point);

	bool initialized = false;
	initial_width = 1;
	BPoint original_point = point;
	while (!initialized && (reader->GetPoint(point) == B_NO_ERROR)) {
		if (point != original_point) {
			updated_rect = BRect(point,point);
			initialized = true;
			current_width = initial_width;
			BPoint line_normal;
			line_normal.x = -(point-original_point).y;
			line_normal.y = (point-original_point).x;
			float normal_length = sqrt(pow(line_normal.x,2) + pow(line_normal.y,2));
			line_normal.x /= normal_length;
			line_normal.y /= normal_length;

			for (int32 i=0;i<hair_count;i++)
				index_array[i] = i;

			for (int32 j=hair_count-1;j>=0;j--) {
				// First calculate the points where to draw this line
				int32 random_index;
				if (j != 0)
					random_index = random() % j;
				else
					random_index = 1;

				int32 i = index_array[random_index];
				index_array[random_index] = index_array[j];
				BPoint offset = line_normal;
				offset.x = offset.x * current_width - offset.x * ((float)i/(float)hair_count)*2*current_width;
				offset.y = offset.y * current_width - offset.y * ((float)i/(float)hair_count)*2*current_width;
				BPoint start_point = start_point_array[i] + offset;


				// Randomly weight the rounding
				start_point.x = random_round(start_point.x,random_stream->UniformDistribution(0.0,1.0));
				start_point.y = random_round(start_point.y,random_stream->UniformDistribution(0.0,1.0));

//				start_point.x = round(start_point.x);
//				start_point.y = round(start_point.y);

				BPoint end_point = point + offset;
				end_point.x = round(end_point.x);
				end_point.y = round(end_point.y);
				start_point_array[i] = end_point;

				// Then calculate how much and what color will be used. Also
				// calculate how the colors will be mixed with previous color.
				if (color_amount_array[i] > 0)
					color_amount_array[i] -= min_c(normal_length / 100.0,0.1);
				else {
					// Borrow some color from our neighbours.
					int32 neighbour_index = ((i>=1) ? i-1 : hair_count-1);
					if (color_amount_array[neighbour_index] > 0) {
						color_amount_array[i] = color_amount_array[neighbour_index]/4;
						color_amount_array[neighbour_index] -= color_amount_array[i];
					}
					neighbour_index = (i+1)%hair_count;
					if (color_amount_array[neighbour_index] > 0) {
						color_amount_array[i] += color_amount_array[neighbour_index]/4;
						color_amount_array[neighbour_index] -= color_amount_array[neighbour_index]/4;
					}

				}

				if (color_amount_array[i] > 0) {
					drawer->DrawHairLine(start_point,end_point,RGBColorToBGRA(color_array[i]),true,selection);
//					drawer->DrawLine(start_point,end_point,RGBColorToBGRA(color_array[i]),2,false,selection);
				}
				else {
				}
				updated_rect = updated_rect | BRect(start_point,start_point);
				updated_rect = updated_rect | BRect(end_point,end_point);
			}
			updated_rect.left = floor(updated_rect.left);
			updated_rect.top = floor(updated_rect.top);
			updated_rect.right = ceil(updated_rect.right);
			updated_rect.bottom = ceil(updated_rect.bottom);
			updated_rect.InsetBy(-1,-1);
			updater->AddRect(updated_rect);
			last_updated_rect = updated_rect;
		}
		else {
			initial_width = min_c(maximum_width,initial_width+0.5);
		}
	}
	if (!initialized) {

	}
	updater->ForceUpdate();

	float number_of_consecutive_growths = 0;
//	while (((status_of_read = coordinate_queue->Get(point)) == B_OK) || (reading_coordinates == TRUE)) {
	while (reader->GetPoint(point) == B_NO_ERROR) {
//		if ( (status_of_read == B_OK) && (prev_point != point) ) {
		if (prev_point != point) {
			the_script->AddPoint(point);

			BPoint line_normal;
			line_normal.x = -(point-prev_point).y;
			line_normal.y = (point-prev_point).x;
			float normal_length = sqrt(pow(line_normal.x,2) + pow(line_normal.y,2));
			line_normal.x /= normal_length;
			line_normal.y /= normal_length;

			// This controls the width of resulting line
			float width_coeff = min_c(normal_length/30.0,1.0);
			float new_width = current_width * 0.5 + 0.5*(width_coeff*minimum_width + (1.0-width_coeff)*maximum_width);
			if (new_width > current_width) {
				current_width += min_c(number_of_consecutive_growths/20.0,1.0) * (new_width-current_width);
				current_width = min_c(current_width,maximum_width);
				number_of_consecutive_growths++;
			}
			else if (new_width <= current_width) {
				current_width = new_width;
				number_of_consecutive_growths = 0;
			}
			updated_rect = BRect(point,point);


			// Here we make an array of indexes and also swap hairs with
			// their neighbours randomly.
			for (int32 i=0;i<hair_count;i++) {
				index_array[i] = i;
				float number = rand() % 1000 / 1000.0;
				if (number > 0.9) {
					float old_amount = color_amount_array[i];
					rgb_color old_color = color_array[i];

					color_amount_array[i] = color_amount_array[(i+1)%hair_count];
					color_array[i] = color_array[(i+1)%hair_count];
					color_amount_array[(i+1)%hair_count] = old_amount;
					color_array[(i+1)%hair_count] = old_color;
				}
				else if (number > 0.8) {
					// NOT: (i-1)%hair_count might be also negative if i happens to be 0.
					int32 other_index = ((i>=1) ? i-1 : hair_count-1);
					float old_amount = color_amount_array[i];
					rgb_color old_color = color_array[i];

					color_amount_array[i] = color_amount_array[other_index];
					color_array[i] = color_array[other_index];
					color_amount_array[other_index] = old_amount;
					color_array[other_index] = old_color;
				}
			}

			for (int32 j=hair_count-1;j>=0;j--) {
				// First calculate the points where to draw this line
				int32 random_index;
				if (j != 0)
					random_index = random() % j;
				else
					random_index = 1;
				int32 i = index_array[random_index];
				index_array[random_index] = index_array[j];
				BPoint offset = line_normal;
				offset.x = offset.x * current_width - offset.x * ((float)i/(float)hair_count)*2*current_width;
				offset.y = offset.y * current_width - offset.y * ((float)i/(float)hair_count)*2*current_width;
				BPoint start_point = start_point_array[i];
				BPoint end_point = point + offset;


				// Randomly weight the rounding
				end_point.x = random_round(end_point.x,random_stream->UniformDistribution(0.0,1.0));
				end_point.y = random_round(end_point.y,random_stream->UniformDistribution(0.0,1.0));

//				end_point.x = round(end_point.x);
//				end_point.y = round(end_point.y);


				start_point_array[i] = end_point;

				// Then calculate how much and what color will be used. Also
				// calculate how the colors will be mixed with previous color.
				if (color_amount_array[i] > 0)
					color_amount_array[i] -= min_c(normal_length / 100.0,0.1);
				else {
					// Borrow some color from our neighbours.
					int32 neighbour_index = ((i>=1) ? i-1 : hair_count-1);
					if (color_amount_array[neighbour_index] > 0) {
						color_amount_array[i] = color_amount_array[neighbour_index]/4;
						color_amount_array[neighbour_index] -= color_amount_array[i];
					}
					neighbour_index = (i+1)%hair_count;
					if (color_amount_array[neighbour_index] > 0) {
						color_amount_array[i] += color_amount_array[neighbour_index]/4;
						color_amount_array[neighbour_index] -= color_amount_array[neighbour_index]/4;
					}

				}

				if (color_amount_array[i] > 0) {
					drawer->DrawHairLine(start_point,end_point,RGBColorToBGRA(color_array[i]),true,selection);
//					drawer->DrawLine(start_point,end_point,RGBColorToBGRA(color_array[i]),2,false,selection);
				}
				else {
				}
				updated_rect = updated_rect | BRect(start_point,start_point);
				updated_rect = updated_rect | BRect(end_point,end_point);
			}
			updated_rect.left = floor(updated_rect.left);
			updated_rect.top = floor(updated_rect.top);
			updated_rect.right = ceil(updated_rect.right);
			updated_rect.bottom = ceil(updated_rect.bottom);
			updated_rect.InsetBy(-1,-1);

			last_updated_rect = last_updated_rect | updated_rect;

			updater->AddRect(updated_rect);
			updater->ForceUpdate();

			prev_point = point;
		}
		else {
//			current_width = max_c(current_width-0.5,minimum_width);
		}
	}
	updater->ForceUpdate();

	delete[] index_array;
	delete[] color_amount_array;
	delete[] color_array;
	delete[] start_point_array;
	delete drawer;
	delete reader;
	delete updater;
	delete random_stream;

	return the_script;
}

int32 HairyBrushTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_ERROR;
}

BView* HairyBrushTool::makeConfigView()
{
	HairyBrushToolConfigView *target_view = new HairyBrushToolConfigView(BRect(0,0,150,0),this);
	return target_view;
}

const char* HairyBrushTool::ReturnHelpString(bool is_in_use)
{
	if (!is_in_use)
		return StringServer::ReturnString(HAIRY_BRUSH_TOOL_READY_STRING);
	else
		return StringServer::ReturnString(HAIRY_BRUSH_TOOL_IN_USE_STRING);
}


const void* HairyBrushTool::ReturnToolCursor()
{
	return HS_HAIRY_BRUSH_CURSOR;
}

//int32 HairyBrushTool::CoordinateReader(void *data)
//{
//	HairyBrushTool *this_pointer = (HairyBrushTool*)data;
//	return this_pointer->read_coordinates();
//}
//
//
//int32 HairyBrushTool::read_coordinates()
//{
//	reading_coordinates = TRUE;
//	uint32 buttons;
//	BPoint point,prev_point;
//	BPoint view_point;
//	image_view->Window()->Lock();
//	image_view->getCoords(&point,&buttons,&view_point);
//	image_view->MovePenTo(view_point);
//	image_view->Window()->Unlock();
//	prev_point = point + BPoint(1,1);
//
//	while (buttons) {
//		image_view->Window()->Lock();
//		if (point != prev_point) {
//			coordinate_queue->Put(point);
//			image_view->StrokeLine(view_point);
//			prev_point = point;
//		}
//		image_view->getCoords(&point,&buttons,&view_point);
//		image_view->Window()->Unlock();
//		snooze(20.0 * 1000.0);
//	}
//
//	reading_coordinates = FALSE;
//	return B_OK;
//}



HairyBrushToolConfigView::HairyBrushToolConfigView(BRect rect,DrawingTool *t)
	: DrawingToolConfigView(rect,t)
{
	BMessage *message;
	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",SIZE_OPTION);
	message->AddInt32("value",tool->GetCurrentValue(SIZE_OPTION));

	float divider = 0;
	hair_amount_slider = new ControlSliderBox(BRect(EXTRA_EDGE,EXTRA_EDGE,150+EXTRA_EDGE,EXTRA_EDGE),"slider",StringServer::ReturnString(HAIRS_STRING),"0",message,5,100);
	hair_amount_slider->ResizeToPreferred();
	AddChild(hair_amount_slider);
	BRect frame = hair_amount_slider->Frame();
	frame.OffsetBy(0,frame.Height()+EXTRA_EDGE);
	divider = hair_amount_slider->Divider();

	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",PRESSURE_OPTION);
	message->AddInt32("value",tool->GetCurrentValue(PRESSURE_OPTION));
	width_slider = new ControlSliderBox(frame,"size_slider",StringServer::ReturnString(SIZE_STRING),"0",message,2,50);
	AddChild(width_slider);
	divider = max_c(divider,width_slider->Divider());

	frame.OffsetBy(0,frame.Height()+EXTRA_EDGE);
	message = new BMessage(COLOR_VARIANCE_CHANGED);
	color_variance_slider = new ControlSlider(frame,"color_variance_slider",StringServer::ReturnString(COLOR_VARIANCE_STRING),message,0,128,B_BLOCK_THUMB);
	color_variance_slider->SetLimitLabels(StringServer::ReturnString(NONE_STRING),StringServer::ReturnString(RANDOM_STRING));
	color_variance_slider->SetValue(tool->GetCurrentValue(TOLERANCE_OPTION));
	color_variance_slider->ResizeToPreferred();
	AddChild(color_variance_slider);

	frame = color_variance_slider->Frame();
	frame.OffsetBy(0,frame.Height());
	message = new BMessage(COLOR_AMOUNT_CHANGED);
	color_amount_slider = new ControlSlider(frame,"color_amount_slider",StringServer::ReturnString(COLOR_AMOUNT_STRING),message,1,500,B_BLOCK_THUMB);
	color_amount_slider->SetLimitLabels(StringServer::ReturnString(LITTLE_STRING),StringServer::ReturnString(MUCH_STRING));
	color_amount_slider->SetValue(tool->GetCurrentValue(CONTINUITY_OPTION));
	color_amount_slider->ResizeToPreferred();
	AddChild(color_amount_slider);

	hair_amount_slider->SetDivider(divider);
	width_slider->SetDivider(divider);
	ResizeTo(color_amount_slider->Bounds().Width()+2*EXTRA_EDGE,color_amount_slider->Frame().bottom+EXTRA_EDGE);
}


void HairyBrushToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();
	hair_amount_slider->SetTarget(new BMessenger(this));
	width_slider->SetTarget(new BMessenger(this));
	color_variance_slider->SetTarget(BMessenger(this));
	color_amount_slider->SetTarget(BMessenger(this));
}



void HairyBrushToolConfigView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case COLOR_VARIANCE_CHANGED:
			tool->SetOption(TOLERANCE_OPTION,color_variance_slider->Value());
			break;
		case COLOR_AMOUNT_CHANGED:
			tool->SetOption(CONTINUITY_OPTION,color_amount_slider->Value());
		default:
			DrawingToolConfigView::MessageReceived(message);
			break;
	}
}
