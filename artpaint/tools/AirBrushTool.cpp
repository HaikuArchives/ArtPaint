/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "AirBrushTool.h"

#include "BitmapDrawer.h"
#include "BitmapUtilities.h"
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
#include "ToolScript.h"
#include "UtilityClasses.h"


#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>
#include <SeparatorView.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


using ArtPaint::Interface::NumberSliderControl;


AirBrushTool::AirBrushTool()
	: DrawingTool(B_TRANSLATE("Airbrush tool"),
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
	BBitmap *srcBuffer = new (std::nothrow) BBitmap(bitmap);
	if (srcBuffer == NULL)
		return NULL;
	BBitmap *tmpBuffer = new (std::nothrow) BBitmap(bitmap);
	if (tmpBuffer == NULL) {
		delete srcBuffer;
		return NULL;
	}

	rgb_color c = ((PaintApplication*)be_app)->Color(true);
	uint32 target_color = RGBColorToBGRA(c);

	union color_conversion clear_color;
	clear_color.word = target_color;
	clear_color.bytes[3] = 0x01;

	BitmapUtilities::ClearBitmap(tmpBuffer, clear_color.word);

	BitmapDrawer *drawer = new BitmapDrawer(tmpBuffer);
	Selection *selection = view->GetSelection();

	ToolScript *the_script = new ToolScript(Type(), fToolSettings,
		((PaintApplication*)be_app)->Color(true));

	if (fToolSettings.mode == HS_AIRBRUSH_MODE) {		// Do the airbrush
		BRect bounds = bitmap->Bounds();
		BRect rc;

		prev_point = point - BPoint(1,1);
		SetLastUpdatedRect(BRect(point, point));
		while (buttons) {
			the_script->AddPoint(point);

			float half_size = fToolSettings.size/2;
			// we should only consider points that are inside this rectangle
			rc = BRect(point.x-half_size,point.y-half_size,point.x+half_size,point.y+half_size);
			rc = rc & bounds;
			rc = rc & selection->GetBoundingRect();

			if (rc.IsValid() == true) {
				int32 height = rc.IntegerHeight();
				int32 width = rc.IntegerWidth();
				int32 left = (int32)rc.left;
				int32 top = (int32)rc.top;

				for (int32 y=0;y<=height;y++) {
					int32 y_sqr = (int32)((point.y-rc.top-y)*(point.y-rc.top-y));
					for (int32 x=0;x<=width;x++) {
						int32 dx = (int32)(point.x-rc.left-x);
						float distance = sqrt_table[dx*dx + y_sqr];
						if ((distance <= half_size) &&
							(selection->IsEmpty() ||
							selection->ContainsPoint(left + x, top + y))) {
							float change = (half_size - distance) / half_size;
							change *= ((float)fToolSettings.pressure) / 100.0;

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
										color.word, (uint32)(32768*change)),
										selection, NULL);
							} else {
								color.word = target_color;
								color.bytes[3] = 0x00;
								drawer->SetPixel(left+x, top+y,
									mix_2_pixels_fixed(target_color,
										color.word, (uint32)(32768*change)),
										selection, NULL);
							}
						}
					}
				}
			}

			window->Lock();
			BitmapUtilities::CompositeBitmapOnSource(bitmap, srcBuffer,
				tmpBuffer, rc);
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

			BRect rc(point,point);

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

					if (selection->IsEmpty() ||
						selection->ContainsPoint(new_point)) {
						drawer->SetPixel(new_point,
							mix_2_pixels_fixed(target_color,
								drawer->GetPixel(new_point), (uint32)(32768*opacity)),
								selection, NULL);
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

					if (selection->IsEmpty() ||
						selection->ContainsPoint(new_point)) {
						drawer->SetPixel(new_point,
							mix_2_pixels_fixed(target_color,
								drawer->GetPixel(new_point), (uint32)(32768*opacity)),
								selection, NULL);
					}
				}
			}
			prev_point = point;

			imageUpdater->AddRect(rc);
			SetLastUpdatedRect(LastUpdatedRect() | rc);
			BitmapUtilities::CompositeBitmapOnSource(bitmap, srcBuffer,
				tmpBuffer, rc);
		}
		imageUpdater->ForceUpdate();

		delete coordinate_reader;
		delete generator;
		delete imageUpdater;
	}

	delete drawer;
	delete srcBuffer;
	delete tmpBuffer;

	return the_script;
}


int32
AirBrushTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_OK;
}


BView*
AirBrushTool::ConfigView()
{
	return new AirBrushToolConfigView(this);
}


const void*
AirBrushTool::ToolCursor() const
{
	return HS_SPRAY_CURSOR;
}


const char*
AirBrushTool::HelpString(bool isInUse) const
{
	return (isInUse
		? B_TRANSLATE("Using the airbrush.")
		: B_TRANSLATE("Click to paint with airbrush."));
}


// #pragma mark -- AirBrushToolConfigView


AirBrushToolConfigView::AirBrushToolConfigView(DrawingTool* tool)
	:
	DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SIZE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(SIZE_OPTION));

		fBrushSize =
			new NumberSliderControl(B_TRANSLATE("Size:"),
			"1", message, 1, 100, false);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", PRESSURE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(PRESSURE_OPTION));

		fBrushFlow =
			new NumberSliderControl(B_TRANSLATE("Flow:"),
			"1", message, 1, 100, false);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", MODE_OPTION);
		message->AddInt32("value", HS_SPRAY_MODE);

		fSpray = new BRadioButton(B_TRANSLATE("Spray"),
			message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", MODE_OPTION);
		message->AddInt32("value", HS_AIRBRUSH_MODE);
		fAirBrush = new BRadioButton(B_TRANSLATE("Airbrush"),
			message);

		BGridLayout* sizeLayout = LayoutSliderGrid(fBrushSize);
		BGridLayout* flowLayout = LayoutSliderGrid(fBrushFlow);

		layout->AddItem(BGroupLayoutBuilder(B_VERTICAL, kWidgetSpacing)
			.Add(sizeLayout)
			.Add(flowLayout)
			.AddStrut(kWidgetSpacing)
			.Add(SeparatorView(B_TRANSLATE("Mode")))
			.Add(BGroupLayoutBuilder(B_VERTICAL, kWidgetSpacing)
				.Add(fSpray)
				.Add(fAirBrush)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0))
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
