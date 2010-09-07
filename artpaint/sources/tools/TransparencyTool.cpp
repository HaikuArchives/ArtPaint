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

#include "TransparencyTool.h"

#include "Cursors.h"
#include "Image.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "StringServer.h"
#include "ToolScript.h"


#include <Bitmap.h>
#include <GridLayoutBuilder.h>
#include <Window.h>


using ArtPaint::Interface::NumberSliderControl;


TransparencyTool::TransparencyTool()
	: DrawingTool(StringServer::ReturnString(TRANSPARENCY_TOOL_NAME_STRING),
		TRANSPARENCY_TOOL)
{
	// The pressure option controls the speed of transparency change.
	fOptions = SIZE_OPTION | PRESSURE_OPTION;
	fOptionsCount = 3;

	SetOption(SIZE_OPTION,1);
	SetOption(PRESSURE_OPTION,1);
}


TransparencyTool::~TransparencyTool()
{
}


ToolScript*
TransparencyTool::UseTool(ImageView* view, uint32 buttons, BPoint point, BPoint)
{
	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	BWindow* window = view->Window();
	BBitmap* bitmap = view->ReturnImage()->ReturnActiveBitmap();

	ToolScript* the_script = new ToolScript(Type(), fToolSettings,
		((PaintApplication*)be_app)->Color(true));

	BRect bounds = bitmap->Bounds();
	uint32* bits_origin = (uint32*)bitmap->Bits();
	int32 bpr = bitmap->BytesPerRow() / 4;

	// for the quick calculation of square-roots
	float sqrt_table[5500];
	for (int32 i=0;i<5500;i++)
		sqrt_table[i] = sqrt(i);

	float half_size = fToolSettings.size/2;
	BRect rc = BRect(floor(point.x - half_size), floor(point.y - half_size),
		ceil(point.x + half_size), ceil(point.y + half_size));
	rc = rc & bounds;
	SetLastUpdatedRect(rc);

	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	while (buttons) {
		the_script->AddPoint(point);
		uint32 transparency_value = ((PaintApplication*)be_app)->Color(true).alpha;
		int32 x_dist,y_sqr;

		int32 width = rc.IntegerWidth();
		int32 height = rc.IntegerHeight();
		for (int32 y=0;y<height+1;y++) {
			y_sqr = (int32)(point.y - rc.top - y);
			y_sqr *= y_sqr;
			int32 real_y = (int32)(rc.top + y);
			int32 real_x;
			for (int32 x=0;x<width+1;x++) {
				x_dist = (int32)(point.x-rc.left-x);
				real_x = (int32)(rc.left+x);
				if (sqrt_table[x_dist*x_dist + y_sqr] <= half_size) {
//					for (int32 i=0;i<(float)GetCurrentValue(PRESSURE_OPTION)/4.0;i++) {
						color.word = *(bits_origin + real_y*bpr + real_x);
						if (color.bytes[3] < transparency_value) {
							color.bytes[3] = (uint8)min_c(color.bytes[3] +
								GetCurrentValue(PRESSURE_OPTION)/4.0,transparency_value);
							*(bits_origin + real_y*bpr + real_x) = color.word;
						} else if (color.bytes[3] > transparency_value) {
							color.bytes[3] = (uint8)max_c(color.bytes[3] -
								GetCurrentValue(PRESSURE_OPTION)/4.0,transparency_value);
							*(bits_origin + real_y*bpr + real_x) = color.word;
						}
//					}
				}
			}
		}

		if (rc.IsValid()) {
			window->Lock();
			view->UpdateImage(rc);
			view->Sync();
			view->getCoords(&point,&buttons);
			window->Unlock();
			half_size = fToolSettings.size/2;
			rc = BRect(floor(point.x - half_size), floor(point.y - half_size),
				ceil(point.x + half_size), ceil(point.y + half_size));
			rc = rc & bounds;
			SetLastUpdatedRect(LastUpdatedRect() | rc);
			//snooze(20.0 * 1000.0);
		} else {
			window->Lock();
			view->getCoords(&point,&buttons);
			window->Unlock();
			half_size = fToolSettings.size/2;
			rc = BRect(floor(point.x - half_size), floor(point.y - half_size),
				ceil(point.x + half_size), ceil(point.y + half_size));
			rc = rc & bounds;
			SetLastUpdatedRect(LastUpdatedRect() | rc);
			snooze(20 * 1000);
		}
	}

	view->ReturnImage()->Render(rc);
	window->Lock();
	view->Draw(view->convertBitmapRectToView(rc));
	SetLastUpdatedRect(LastUpdatedRect() | rc);	// ???
	window->Unlock();

	return the_script;
}


int32
TransparencyTool::UseToolWithScript(ToolScript*, BBitmap*)
{
	return B_OK;
}


BView*
TransparencyTool::makeConfigView()
{
	return (new TransparencyToolConfigView(this));
}


const void*
TransparencyTool::ToolCursor() const
{
	return HS_TRANSPARENCY_CURSOR;
}


const char*
TransparencyTool::HelpString(bool isInUse) const
{
	return StringServer::ReturnString(isInUse ? TRANSPARENCY_TOOL_IN_USE_STRING
		: TRANSPARENCY_TOOL_READY_STRING);
}


// #pragma mark -- TransparencyToolConfigView


TransparencyToolConfigView::TransparencyToolConfigView(DrawingTool* tool)
	: DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SIZE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(SIZE_OPTION));

		fSizeSlider =
			new NumberSliderControl(StringServer::ReturnString(SIZE_STRING),
			"1", message, 1, 100, false);
		layout->AddView(fSizeSlider);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", PRESSURE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(PRESSURE_OPTION));

		fSpeedSlider =
			new NumberSliderControl(StringServer::ReturnString(SPEED_STRING),
			"1", message, 1, 100, false);
		layout->AddView(fSpeedSlider);

		BGridLayout* gridLayout = BGridLayoutBuilder(5.0, 5.0)
			.Add(fSizeSlider->LabelLayoutItem(), 0, 0)
			.Add(fSizeSlider->TextViewLayoutItem(), 1, 0)
			.Add(fSizeSlider->Slider(), 2, 0)
			.Add(fSpeedSlider->LabelLayoutItem(), 0, 1)
			.Add(fSpeedSlider->TextViewLayoutItem(), 1, 1)
			.Add(fSpeedSlider->Slider(), 2, 1);
		gridLayout->SetMaxColumnWidth(1, StringWidth("1000"));
		gridLayout->SetMinColumnWidth(2, StringWidth("SLIDERSLIDERSLIDER"));
		layout->AddView(gridLayout->View());
	}
}


void
TransparencyToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fSizeSlider->SetTarget(this);
	fSpeedSlider->SetTarget(this);
}
