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

#include "FreeLineTool.h"

#include "BitmapDrawer.h"
#include "BitmapUtilities.h"
#include "Brush.h"
#include "Cursors.h"
#include "CoordinateQueue.h"
#include "Image.h"
#include "ImageUpdater.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "PixelOperations.h"
#include "ToolManager.h"
#include "ToolScript.h"
#include "UtilityClasses.h"


#include <Catalog.h>
#include <CheckBox.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <Layout.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


using ArtPaint::Interface::NumberSliderControl;


FreeLineTool::FreeLineTool()
	: DrawingTool(B_TRANSLATE("Freehand line tool"), "f",
		FREE_LINE_TOOL)
{
	fOptions = SIZE_OPTION | USE_BRUSH_OPTION | PRESSURE_OPTION;
	fOptionsCount = 3;

	SetOption(SIZE_OPTION, 1);
	SetOption(USE_BRUSH_OPTION, B_CONTROL_OFF);
	SetOption(PRESSURE_OPTION, 100);
}


FreeLineTool::~FreeLineTool()
{
}


ToolScript*
FreeLineTool::UseTool(ImageView* view, uint32 buttons, BPoint point, BPoint)
{
	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	coordinate_queue = new (std::nothrow) CoordinateQueue();
	if (coordinate_queue == NULL)
		return NULL;

	image_view = view;
	thread_id coordinate_reader = spawn_thread(CoordinateReader,
		"read coordinates", B_NORMAL_PRIORITY, this);
	resume_thread(coordinate_reader);
	reading_coordinates = true;

	ToolScript *the_script = new (std::nothrow) ToolScript(Type(), fToolSettings,
		((PaintApplication*)be_app)->Color(true));
	if (the_script == NULL) {
		delete coordinate_queue;

		return NULL;
	}
	BBitmap* buffer = view->ReturnImage()->ReturnActiveBitmap();
	if (buffer == NULL) {
		delete coordinate_queue;
		delete the_script;

		return NULL;
	}
	BBitmap* srcBuffer = new (std::nothrow) BBitmap(buffer);
	if (srcBuffer == NULL) {
		delete coordinate_queue;
		delete the_script;

		return NULL;
	}
	BBitmap* tmpBuffer = new (std::nothrow) BBitmap(buffer);
	if (tmpBuffer == NULL) {
		delete coordinate_queue;
		delete the_script;
		delete srcBuffer;
		return NULL;
	}
	union color_conversion clear_color, tmp_draw_color;
	clear_color.word = 0xFFFFFFFF;
	clear_color.bytes[3] = 0x00;
	tmp_draw_color.word = 0xFFFFFFFF;

	BitmapUtilities::ClearBitmap(tmpBuffer, clear_color.word);

	Selection *selection = view->GetSelection();
//	BView *buffer_view = view->getBufferView();
	BitmapDrawer *drawer = new (std::nothrow) BitmapDrawer(tmpBuffer);
	if (drawer == NULL) {
		delete coordinate_queue;
		delete the_script;
		delete srcBuffer;
		delete tmpBuffer;

		return NULL;
	}

	rgb_color new_color;
	union color_conversion new_color_bgra;
//	int32 original_mouse_speed;
//	get_mouse_speed(&original_mouse_speed);

	BPoint prev_point;

	prev_point = point;
	BRect updated_rect;
	status_t status_of_read;
	Brush* brush;
	bool delete_brush = false;

	if (fToolSettings.use_current_brush == true)
		brush = ToolManager::Instance().GetCurrentBrush();
	else {
		brush_info default_free_brush;
		default_free_brush.shape = HS_ELLIPTICAL_BRUSH;
		default_free_brush.width = fToolSettings.size;
		default_free_brush.height = fToolSettings.size;
		default_free_brush.angle = 0;
		default_free_brush.hardness = 100;
		brush = new Brush(default_free_brush);
		delete_brush = true;
	}

	float brush_width_per_2 = floor(brush->Width()/2);
	float brush_height_per_2 = floor(brush->Height()/2);

	bool use_fg_color = true;
	if (buttons == B_SECONDARY_MOUSE_BUTTON)
		use_fg_color = false;

	float pressure = (float)fToolSettings.pressure / 100.;

	new_color = ((PaintApplication*)be_app)->Color(use_fg_color);
	new_color_bgra.word = RGBColorToBGRA(new_color);

	new_color_bgra.bytes[3] *= pressure;

	brush->draw(tmpBuffer, BPoint(prev_point.x - brush_width_per_2,
		prev_point.y - brush_height_per_2), selection);

	// This makes sure that the view is updated even if just one point is drawn
	updated_rect.left = min_c(point.x - brush_width_per_2,
		prev_point.x - brush_width_per_2);
	updated_rect.top = min_c(point.y - brush_height_per_2,
		prev_point.y - brush_height_per_2);
	updated_rect.right = max_c(point.x + brush_width_per_2,
		prev_point.x + brush_width_per_2);
	updated_rect.bottom = max_c(point.y + brush_height_per_2,
		prev_point.y + brush_height_per_2);

	buffer->Lock();
	BitmapUtilities::CompositeBitmapOnSource(buffer, srcBuffer,
		tmpBuffer, updated_rect, src_over_fixed, new_color_bgra.word);
	buffer->Unlock();

	SetLastUpdatedRect(updated_rect);
	the_script->AddPoint(point);

	ImageUpdater* imageUpdater = new ImageUpdater(view, 2000);
	imageUpdater->AddRect(updated_rect);

	while (((status_of_read = coordinate_queue->Get(point)) == B_OK)
		|| (reading_coordinates == true)) {
		if ( (status_of_read == B_OK) && (prev_point != point) ) {
			the_script->AddPoint(point);
//			if (modifiers() & B_LEFT_CONTROL_KEY) {
//				set_mouse_speed(0);
//			}
//			else {
//				set_mouse_speed(original_mouse_speed);
//			}

			brush->draw(tmpBuffer, BPoint(point.x - brush_width_per_2,
				point.y - brush_height_per_2), selection);
			brush->draw_line(tmpBuffer, point, prev_point, selection);

			updated_rect.left = min_c(point.x - brush_width_per_2 - 1,
				prev_point.x - brush_width_per_2 - 1);
			updated_rect.top = min_c(point.y - brush_height_per_2 - 1,
				prev_point.y - brush_height_per_2 - 1);
			updated_rect.right = max_c(point.x + brush_width_per_2 + 1,
				prev_point.x + brush_width_per_2 + 1);
			updated_rect.bottom = max_c(point.y + brush_height_per_2 + 1,
				prev_point.y + brush_height_per_2 + 1);

			imageUpdater->AddRect(updated_rect);

			SetLastUpdatedRect(LastUpdatedRect() | updated_rect);

			buffer->Lock();
			BitmapUtilities::CompositeBitmapOnSource(buffer, srcBuffer,
				tmpBuffer, updated_rect, src_over_fixed, new_color_bgra.word);
			buffer->Unlock();

			prev_point = point;
		}
	}

	imageUpdater->ForceUpdate();
	delete imageUpdater;

	delete srcBuffer;
	delete tmpBuffer;

	delete drawer;
	delete coordinate_queue;

	if (delete_brush == true)
		delete brush;

	return the_script;
}


int32
FreeLineTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_OK;
}


BView*
FreeLineTool::ConfigView()
{
	return new FreeLineToolConfigView(this);
}


const void*
FreeLineTool::ToolCursor() const
{
	return HS_FREE_LINE_CURSOR;
}


const char*
FreeLineTool::HelpString(bool isInUse) const
{
	return (isInUse
		? B_TRANSLATE("Drawing a freehand line.")
		: B_TRANSLATE("Freehand line tool"));
}


int32
FreeLineTool::CoordinateReader(void *data)
{
	FreeLineTool *this_pointer = (FreeLineTool*)data;
	return this_pointer->read_coordinates();
}


int32
FreeLineTool::read_coordinates()
{
	reading_coordinates = true;
	uint32 buttons;
	BPoint point,prev_point;
	BPoint view_point;
	image_view->Window()->Lock();
	image_view->getCoords(&point,&buttons,&view_point);
	image_view->MovePenTo(view_point);
	image_view->Window()->Unlock();
	prev_point = point + BPoint(1,1);

	while (buttons) {
		image_view->Window()->Lock();
		if (point != prev_point) {
			coordinate_queue->Put(point);
			image_view->StrokeLine(view_point);
			prev_point = point;
		}
		image_view->getCoords(&point,&buttons,&view_point);
		image_view->Window()->Unlock();
		snooze(20 * 1000);
	}

	reading_coordinates = false;
	return B_OK;
}


// #pragma mark -- FreeLineToolConfigView


FreeLineToolConfigView::FreeLineToolConfigView(DrawingTool* tool)
	: DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SIZE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(SIZE_OPTION));

		fLineSize =
			new NumberSliderControl(B_TRANSLATE("Width:"),
			"1", message, 1, 100, false);

		BGridLayout* lineSizeLayout = LayoutSliderGrid(fLineSize);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", USE_BRUSH_OPTION);
		message->AddInt32("value", 0x00000000);
		fUseBrush = new BCheckBox(B_TRANSLATE("Use current brush"),
			message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", PRESSURE_OPTION);
		message->AddInt32("value", 100);

		fPressureSlider =
			new NumberSliderControl(B_TRANSLATE("Pressure:"),
			"100", message, 1, 100, false);

		BGridLayout* pressureLayout = LayoutSliderGrid(fPressureSlider);

		layout->AddView(BGroupLayoutBuilder(B_VERTICAL, kWidgetSpacing)
			.Add(lineSizeLayout)
			.Add(pressureLayout)
			.Add(fUseBrush)
			.TopView()
		);

		fUseBrush->SetValue(tool->GetCurrentValue(USE_BRUSH_OPTION));
		if (tool->GetCurrentValue(USE_BRUSH_OPTION) != B_CONTROL_OFF) {
			fLineSize->SetEnabled(FALSE);
		}

		fPressureSlider->SetValue(tool->GetCurrentValue(PRESSURE_OPTION));
	}
}


void
FreeLineToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fLineSize->SetTarget(this);
	fUseBrush->SetTarget(this);
	fPressureSlider->SetTarget(this);
}


void
FreeLineToolConfigView::MessageReceived(BMessage* message)
{
	DrawingToolConfigView::MessageReceived(message);

	switch(message->what) {
		case OPTION_CHANGED: {
			if (message->FindInt32("option") == USE_BRUSH_OPTION) {
				if (fUseBrush->Value() == B_CONTROL_OFF)
					fLineSize->SetEnabled(TRUE);
				else
					fLineSize->SetEnabled(FALSE);
			}
		} break;
	}
}
