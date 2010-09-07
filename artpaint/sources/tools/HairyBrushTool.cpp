/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "HairyBrushTool.h"

#include "BitmapDrawer.h"
#include "CoordinateReader.h"
#include "Cursors.h"
#include "Image.h"
#include "ImageUpdater.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "RandomNumberGenerator.h"
#include "StringServer.h"
#include "ToolScript.h"
#include "UtilityClasses.h"


#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <Slider.h>


#include <stdlib.h>


#define COLOR_VARIANCE_CHANGED	'Cvar'
#define	COLOR_AMOUNT_CHANGED	'Camt'


using ArtPaint::Interface::NumberSliderControl;


HairyBrushTool::HairyBrushTool()
	: DrawingTool(StringServer::ReturnString(HAIRY_BRUSH_TOOL_NAME_STRING),
		HAIRY_BRUSH_TOOL)
{
	options = SIZE_OPTION | PRESSURE_OPTION | TOLERANCE_OPTION
		| CONTINUITY_OPTION;
	number_of_options = 3;

	SetOption(SIZE_OPTION, 5);
	SetOption(PRESSURE_OPTION, 2);
	SetOption(TOLERANCE_OPTION, 10);
	SetOption(CONTINUITY_OPTION, 20);
}


HairyBrushTool::~HairyBrushTool()
{
}


ToolScript*
HairyBrushTool::UseTool(ImageView *view, uint32 buttons, BPoint point, BPoint)
{
	// here we first get the necessary data from view
	// and then start drawing while mousebutton is held down

	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

//	coordinate_queue = new CoordinateQueue();
//	image_view = view;
//	thread_id coordinate_reader = spawn_thread(CoordinateReader,
//		"read coordinates",B_NORMAL_PRIORITY,this);
//	resume_thread(coordinate_reader);
//	reading_coordinates = TRUE;

	BBitmap* buffer = view->ReturnImage()->ReturnActiveBitmap();
	if (!buffer)
		return NULL;

	ToolScript *the_script = new ToolScript(Type(), settings,
		((PaintApplication*)be_app)->Color(true));

	Selection *selection = view->GetSelection();
	BitmapDrawer *drawer = new BitmapDrawer(buffer);
	CoordinateReader *reader = new CoordinateReader(view, NO_INTERPOLATION,
		false, true);
	ImageUpdater* imageUpdater = new ImageUpdater(view, 0);
	RandomNumberGenerator *random_stream =
		new RandomNumberGenerator(107 + int32(point.x), 1024);

	BPoint prev_point = point;
	BRect updated_rect;

	float initial_width = GetCurrentValue(PRESSURE_OPTION);
	float maximum_width = initial_width;//ceil(initial_width*1.5);
	float minimum_width = 0.5;//floor(max_c(2,initial_width*0.5));
	float current_width = initial_width;
	float color_randomness = GetCurrentValue(TOLERANCE_OPTION);
	float initial_color_amount = GetCurrentValue(CONTINUITY_OPTION) / 10.0;
	float color_amount_randomness = 4;
	rgb_color color =  ((PaintApplication*)be_app)->Color(true);
	int32 hair_count = GetCurrentValue(SIZE_OPTION);

	float *color_amount_array = new float[hair_count];
	rgb_color *color_array = new rgb_color[hair_count];
	BPoint *start_point_array = new BPoint[hair_count];
	int32 *index_array = new int32[hair_count];

	for (int32 i=0;i<hair_count;i++) {
		float red,green,blue,alpha;
		color_amount_array[i] = initial_color_amount - color_amount_randomness + random()%1000/1000.0*color_amount_randomness*2.0;
		color_array[i] = ((PaintApplication*)be_app)->Color(true);
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

	SetLastUpdatedRect(BRect(point, point));
	the_script->AddPoint(point);

	bool initialized = false;
	initial_width = 1;
	BPoint original_point = point;
	while (!initialized && (reader->GetPoint(point) == B_OK)) {
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
			imageUpdater->AddRect(updated_rect);
			SetLastUpdatedRect(updated_rect);
		}
		else {
			initial_width = min_c(maximum_width,initial_width+0.5);
		}
	}
	if (!initialized) {

	}
	imageUpdater->ForceUpdate();

	float number_of_consecutive_growths = 0;
//	while (((status_of_read = coordinate_queue->Get(point)) == B_OK) || (reading_coordinates == true)) {
	while (reader->GetPoint(point) == B_OK) {
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

			SetLastUpdatedRect(LastUpdatedRect() | updated_rect);

			imageUpdater->AddRect(updated_rect);
			imageUpdater->ForceUpdate();

			prev_point = point;
		}
		else {
//			current_width = max_c(current_width-0.5,minimum_width);
		}
	}
	imageUpdater->ForceUpdate();

	delete[] index_array;
	delete[] color_amount_array;
	delete[] color_array;
	delete[] start_point_array;
	delete drawer;
	delete reader;
	delete imageUpdater;
	delete random_stream;

	return the_script;
}

int32
HairyBrushTool::UseToolWithScript(ToolScript*, BBitmap*)
{
	return B_ERROR;
}


BView*
HairyBrushTool::makeConfigView()
{
	return (new HairyBrushToolConfigView(this));
}


const void*
HairyBrushTool::ToolCursor() const
{
	return HS_HAIRY_BRUSH_CURSOR;
}


const char*
HairyBrushTool::HelpString(bool isInUse) const
{
	return StringServer::ReturnString(isInUse ? HAIRY_BRUSH_TOOL_IN_USE_STRING
		: HAIRY_BRUSH_TOOL_READY_STRING);
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
//	reading_coordinates = true;
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


// #pragma mark -- HairyBrushToolConfigView


HairyBrushToolConfigView::HairyBrushToolConfigView(DrawingTool* tool)
	: DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", PRESSURE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(PRESSURE_OPTION));

		fBrushSize =
			new NumberSliderControl(StringServer::ReturnString(SIZE_STRING),
			"0", message, 2, 50, false);
		layout->AddView(fBrushSize);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SIZE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(SIZE_OPTION));

		fBrushHairs =
			new NumberSliderControl(StringServer::ReturnString(HAIRS_STRING),
			"0", message, 5, 100, false);
		layout->AddView(fBrushHairs);

		fColorAmount =
			new BSlider("", StringServer::ReturnString(COLOR_AMOUNT_STRING),
			new BMessage(COLOR_AMOUNT_CHANGED), 1, 500, B_HORIZONTAL,
			B_TRIANGLE_THUMB);
		fColorAmount->SetLimitLabels(StringServer::ReturnString(LITTLE_STRING),
			StringServer::ReturnString(MUCH_STRING));
		fColorAmount->SetValue(tool->GetCurrentValue(CONTINUITY_OPTION));

		fColorVariance =
			new BSlider("", StringServer::ReturnString(COLOR_VARIANCE_STRING),
			new BMessage(COLOR_VARIANCE_CHANGED), 0, 128, B_HORIZONTAL,
			B_TRIANGLE_THUMB);
		fColorVariance->SetLimitLabels(StringServer::ReturnString(NONE_STRING),
			StringServer::ReturnString(RANDOM_STRING));
		fColorVariance->SetValue(tool->GetCurrentValue(TOLERANCE_OPTION));

		BGridLayout* gridLayout = BGridLayoutBuilder(5.0, 5.0)
			.Add(fBrushSize->LabelLayoutItem(), 0, 0)
			.Add(fBrushSize->TextViewLayoutItem(), 1, 0)
			.Add(fBrushSize->Slider(), 2, 0)
			.Add(fBrushHairs->LabelLayoutItem(), 0, 1)
			.Add(fBrushHairs->TextViewLayoutItem(), 1, 1)
			.Add(fBrushHairs->Slider(), 2, 1)
			.SetInsets(5.0, 0.0, 0.0, 0.0);
		gridLayout->SetMaxColumnWidth(1, StringWidth("1000"));
		gridLayout->SetMinColumnWidth(2, StringWidth("SLIDERSLIDERSLIDER"));

		layout->AddItem(BGroupLayoutBuilder(B_VERTICAL)
			.Add(gridLayout)
			.Add(fColorAmount)
			.Add(fColorVariance)
		);
	}
}


void
HairyBrushToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fBrushSize->SetTarget(this);
	fBrushHairs->SetTarget(this);

	fColorAmount->SetTarget(this);
	fColorVariance->SetTarget(this);
}


void
HairyBrushToolConfigView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case COLOR_VARIANCE_CHANGED: {
			Tool()->SetOption(TOLERANCE_OPTION, fColorVariance->Value());
		}	break;

		case COLOR_AMOUNT_CHANGED: {
			Tool()->SetOption(CONTINUITY_OPTION, fColorAmount->Value());
		}	break;	// TODO: check since before it did fall through

		default: {
			DrawingToolConfigView::MessageReceived(message);
		}	break;
	}
}
