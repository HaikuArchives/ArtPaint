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

#include "AirBrushTool.h"

#include "BitmapDrawer.h"
#include "CoordinateReader.h"
#include "Cursors.h"
#include "Image.h"
#include "ImageUpdater.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "PixelOperations.h"
#include "RandomNumberGenerator.h"
#include "Selection.h"
#include "StringServer.h"
#include "ToolScript.h"
#include "UtilityClasses.h"


#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>
#include <SeparatorView.h>
#include <Window.h>


using ArtPaint::Interface::NumberSliderControl;


AirBrushTool::AirBrushTool()
	: DrawingTool(StringServer::ReturnString(AIR_BRUSH_TOOL_NAME_STRING),
		AIR_BRUSH_TOOL)
{
	fOptions = SIZE_OPTION | PRESSURE_OPTION | MODE_OPTION;
	fOptionsCount = 3;

	SetOption(SIZE_OPTION, 1);
	SetOption(PRESSURE_OPTION, 1);
	SetOption(MODE_OPTION, HS_AIRBRUSH_MODE);

	sqrt_table = new int32[5500];
	for (int32 i = 0; i < 5500; ++i)
		sqrt_table[i] = (int32)sqrt(i);
}


AirBrushTool::~AirBrushTool()
{
	delete [] sqrt_table;
}


ToolScript*
AirBrushTool::UseTool(ImageView *view, uint32 buttons, BPoint point, BPoint)
{
	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	BPoint prev_point;
	BWindow *window = view->Window();
	BBitmap *bitmap = view->ReturnImage()->ReturnActiveBitmap();
	BitmapDrawer *drawer = new BitmapDrawer(bitmap);
	Selection *selection = view->GetSelection();

	ToolScript *the_script = new ToolScript(Type(), fToolSettings,
		((PaintApplication*)be_app)->Color(true));

	if (fToolSettings.mode == HS_AIRBRUSH_MODE) {		// Do the airbrush
		BRect bounds = bitmap->Bounds();
		BRect rc;

		prev_point = point - BPoint(1,1);
		SetLastUpdatedRect(BRect(point, point));
		uint32 target_color;
		while (buttons) {
			the_script->AddPoint(point);

			float half_size = fToolSettings.size/2;
			rgb_color c = ((PaintApplication*)be_app)->Color(true);
			target_color = RGBColorToBGRA(c);
			// we should only consider points that are inside this rectangle
			rc = BRect(point.x-half_size,point.y-half_size,point.x+half_size,point.y+half_size);
			rc = rc & bounds;
			rc = rc & selection->GetBoundingRect();

			if (rc.IsValid() == true) {
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
								change = change*(((float)fToolSettings.pressure)/100.0);


								// This is experimental for doing a real transparency
								// Seems to work quite well
								union {
									uint8 bytes[4];
									uint32 word;
								} color;
								color.word = drawer->GetPixel(left+x,top+y);
								if (color.bytes[3] != 0x00) {
									drawer->SetPixel(left+x, top+y,
										mix_2_pixels_fixed(target_color,
											color.word, (uint32)(32768*change)));
								} else {
									color.word = target_color;
									color.bytes[3] = 0x00;
									drawer->SetPixel(left+x, top+y,
										mix_2_pixels_fixed(target_color,
											color.word, (uint32)(32768*change)));
								}
							}
						}
					}
				} else {
					for (int32 y=0;y<=height;y++) {
						int32 y_sqr = (int32)((point.y-rc.top-y)*(point.y-rc.top-y));
						for (int32 x=0;x<=width;x++) {
							int32 dx = (int32)(point.x-rc.left-x);
							float distance = sqrt_table[dx*dx + y_sqr];
							if ((distance <= half_size)
								&& (selection->ContainsPoint(left+x,top+y))) {
								float change = (half_size-distance)/half_size;
								change = change*(((float)fToolSettings.pressure)/100.0);

								// This is experimental for doing a real transparency
								// Seems to work quite well
								union {
									uint8 bytes[4];
									uint32 word;
								} color;
								color.word = drawer->GetPixel(left+x,top+y);
								if (color.bytes[3] != 0x00) {
									drawer->SetPixel(left+x, top+y,
										mix_2_pixels_fixed(target_color,
											color.word, (uint32)(32768*change)));
								} else {
									color.word = target_color;
									color.bytes[3] = 0x00;
									drawer->SetPixel(left+x, top+y,
										mix_2_pixels_fixed(target_color,
											color.word, (uint32)(32768*change)));
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
				SetLastUpdatedRect(LastUpdatedRect() | rc);
			}
			view->getCoords(&point,&buttons);
			window->Unlock();
			snooze(20 * 1000);
		}
	} else if (fToolSettings.mode == HS_SPRAY_MODE) {	// Do the spray
		CoordinateReader *coordinate_reader = new CoordinateReader(view,
			NO_INTERPOLATION, false, true);
		RandomNumberGenerator *generator = new RandomNumberGenerator(0,10000);
		ImageUpdater* imageUpdater = new ImageUpdater(view, 20000);
		prev_point = point;

		while (coordinate_reader->GetPoint(point) == B_OK) {
			int32 flow = fToolSettings.pressure + 1;
			float width = fToolSettings.size;
			float angle;
			float opacity = 0.4;
			rgb_color c = ((PaintApplication*)be_app)->Color(true);
			uint32 target_color = RGBColorToBGRA(c);

			BRect rc(point,point);

			if (selection->IsEmpty()) {
				if (point == prev_point) {
					for (int32 i=0;i<flow;i++) {
						float x = generator->UniformDistribution(0,width*.5);
						float y = generator->UniformDistribution(0,
							sqrt((width*.5)*(width*.5)-x*x));

						angle = generator->UniformDistribution(0,1.0)*2*M_PI;
						float old_x = x;
						x = cos(angle)*x - sin(angle)*y;
						y = sin(angle)*old_x + cos(angle)*y;
						BPoint new_point = point+BPoint(x,y);
						new_point.x = round(new_point.x);
						new_point.y = round(new_point.y);
						rc = rc | BRect(new_point,new_point);

						drawer->SetPixel(new_point,
							mix_2_pixels_fixed(target_color,
								drawer->GetPixel(new_point), (uint32)(32768*opacity)));
					}
				} else {
					BPoint center;
					for (int32 i=0;i<flow;i++) {
						float x = generator->UniformDistribution(0,width*.5);
						float y = generator->UniformDistribution(0,
							sqrt((width*.5)*(width*.5)-x*x));

						// Select center randomly from the line between prev_point and point.
						// This is done by doing a linear interpolation between the
						// two points and rounding the result to nearest integer.
						float a = generator->UniformDistribution(0,1.0);
						center.x = round(a*prev_point.x + (1.0-a)*point.x);
						center.y = round(a*prev_point.y + (1.0-a)*point.y);

						angle = generator->UniformDistribution(0,1.0)*2*M_PI;
						float old_x = x;
						x = cos(angle)*x - sin(angle)*y;
						y = sin(angle)*old_x + cos(angle)*y;
						BPoint new_point = center + BPoint(x,y);
						new_point.x = round(new_point.x);
						new_point.y = round(new_point.y);
						rc = rc | BRect(new_point,new_point);

						drawer->SetPixel(new_point,
							mix_2_pixels_fixed(target_color,
								drawer->GetPixel(new_point), (uint32)(32768*opacity)));
					}
				}
			} else {
				if (point == prev_point) {
					for (int32 i=0;i<flow;i++) {
						float x = generator->UniformDistribution(0,width*.5);
						float y = generator->UniformDistribution(0,
							sqrt((width*.5)*(width*.5)-x*x));

						angle = generator->UniformDistribution(0,1.0)*2*M_PI;
						float old_x = x;
						x = cos(angle)*x - sin(angle)*y;
						y = sin(angle)*old_x + cos(angle)*y;
						BPoint new_point = point+BPoint(x,y);
						new_point.x = round(new_point.x);
						new_point.y = round(new_point.y);
						rc = rc | BRect(new_point,new_point);

						if (selection->ContainsPoint(new_point)) {
							drawer->SetPixel(new_point,
								mix_2_pixels_fixed(target_color,
									drawer->GetPixel(new_point), (uint32)(32768*opacity)));
						}
					}
				} else {
					BPoint center;
					for (int32 i=0;i<flow;i++) {
						float x = generator->UniformDistribution(0,width*.5);
						float y = generator->UniformDistribution(0,
							sqrt((width*.5)*(width*.5)-x*x));

						// Select center randomly from the line between prev_point and point.
						// This is done by doing a linear interpolation between the
						// two points and rounding the result to nearest integer.
						float a = generator->UniformDistribution(0,1.0);
						center.x = round(a*prev_point.x + (1.0-a)*point.x);
						center.y = round(a*prev_point.y + (1.0-a)*point.y);

						angle = generator->UniformDistribution(0,1.0)*2*M_PI;
						float old_x = x;
						x = cos(angle)*x - sin(angle)*y;
						y = sin(angle)*old_x + cos(angle)*y;
						BPoint new_point = center + BPoint(x,y);
						new_point.x = round(new_point.x);
						new_point.y = round(new_point.y);
						rc = rc | BRect(new_point,new_point);

						if (selection->ContainsPoint(new_point)) {
							drawer->SetPixel(new_point,
								mix_2_pixels_fixed(target_color,
									drawer->GetPixel(new_point), (uint32)(32768*opacity)));
						}
					}
				}
			}
			prev_point = point;

			imageUpdater->AddRect(rc);
			SetLastUpdatedRect(LastUpdatedRect() | rc);
		}
		imageUpdater->ForceUpdate();

		delete coordinate_reader;
		delete generator;
		delete imageUpdater;
	}

	delete drawer;

	return the_script;
}


int32
AirBrushTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_OK;
}


BView*
AirBrushTool::makeConfigView()
{
	return (new AirBrushToolConfigView(this));
}


const void*
AirBrushTool::ToolCursor() const
{
	return HS_SPRAY_CURSOR;
}


const char*
AirBrushTool::HelpString(bool isInUse) const
{
	return StringServer::ReturnString(isInUse ? AIR_BRUSH_TOOL_IN_USE_STRING
		: AIR_BRUSH_TOOL_READY_STRING);
}


// #pragma mark -- AirBrushToolConfigView


AirBrushToolConfigView::AirBrushToolConfigView(DrawingTool* tool)
	: DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SIZE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(SIZE_OPTION));

		fBrushSize =
			new NumberSliderControl(StringServer::ReturnString(SIZE_STRING),
			"1", message, 1, 100, false);
		layout->AddView(fBrushSize);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", PRESSURE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(PRESSURE_OPTION));

		fBrushFlow =
			new NumberSliderControl(StringServer::ReturnString(FLOW_STRING),
			"1", message, 1, 100, false);
		layout->AddView(fBrushFlow);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", MODE_OPTION);
		message->AddInt32("value", HS_SPRAY_MODE);

		fSpray = new BRadioButton(StringServer::ReturnString(SPRAY_STRING),
			message);

		message->ReplaceInt32("value", HS_AIRBRUSH_MODE);
		fAirBrush = new BRadioButton(StringServer::ReturnString(AIRBRUSH_STRING),
			new BMessage(*message));

		BSeparatorView* separator =
			new BSeparatorView(StringServer::ReturnString(MODE_STRING),
			B_HORIZONTAL, B_FANCY_BORDER, BAlignment(B_ALIGN_LEFT,
			B_ALIGN_VERTICAL_CENTER));
		separator->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		BGridLayout* gridLayout = BGridLayoutBuilder(5.0, 5.0)
			.Add(fBrushSize->LabelLayoutItem(), 0, 0)
			.Add(fBrushSize->TextViewLayoutItem(), 1, 0)
			.Add(fBrushSize->Slider(), 2, 0)
			.Add(fBrushFlow->LabelLayoutItem(), 0, 1)
			.Add(fBrushFlow->TextViewLayoutItem(), 1, 1)
			.Add(fBrushFlow->Slider(), 2, 1)
			.SetInsets(5.0, 0.0, 0.0, 0.0);
		gridLayout->SetMaxColumnWidth(1, StringWidth("1000"));
		gridLayout->SetMinColumnWidth(2, StringWidth("SLIDERSLIDERSLIDER"));

		layout->AddItem(BGroupLayoutBuilder(B_VERTICAL)
			.Add(gridLayout)
			.Add(separator)
			.Add(BGroupLayoutBuilder(B_VERTICAL, 0.0)
				.Add(fSpray)
				.Add(fAirBrush)
				.SetInsets(5.0, 0.0, 0.0, 0.0))
		);

		if (tool->GetCurrentValue(MODE_OPTION) == HS_SPRAY_MODE)
			fSpray->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(MODE_OPTION) == HS_AIRBRUSH_MODE)
			fAirBrush->SetValue(B_CONTROL_ON);
	}
}


void
AirBrushToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fBrushSize->SetTarget(this);
	fBrushFlow->SetTarget(this);

	fSpray->SetTarget(this);
	fAirBrush->SetTarget(this);
}
