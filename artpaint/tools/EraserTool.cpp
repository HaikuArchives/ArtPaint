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

#include "EraserTool.h"

#include "BitmapDrawer.h"
#include "BitmapUtilities.h"
#include "Brush.h"
#include "CoordinateQueue.h"
#include "CoordinateReader.h"
#include "Cursors.h"
#include "Image.h"
#include "ImageUpdater.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "PixelOperations.h"
#include "ToolManager.h"
#include "ToolScript.h"


#include <Catalog.h>
#include <CheckBox.h>
#include <GridLayout.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>
#include <SeparatorView.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


using ArtPaint::Interface::NumberSliderControl;


EraserTool::EraserTool()
	:
	DrawingTool(B_TRANSLATE("Eraser tool"), "e", ERASER_TOOL)
{
	fOptions = SIZE_OPTION | MODE_OPTION | USE_BRUSH_OPTION | PRESSURE_OPTION;
	fOptionsCount = 4;

	SetOption(SIZE_OPTION, 1);
	SetOption(MODE_OPTION, HS_ERASE_TO_BACKGROUND_MODE);
	SetOption(USE_BRUSH_OPTION, B_CONTROL_OFF);
	SetOption(PRESSURE_OPTION, 100);
}


ToolScript*
EraserTool::UseTool(ImageView* view, uint32 buttons, BPoint point, BPoint)
{
	// here we first get the necessary data from view
	// and then start drawing while mousebutton is held down

	uint32 (*composite_func)(uint32, uint32) = dst_out_fixed;

	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	image_view = view;
	CoordinateReader* coordinate_reader
		= new (std::nothrow) CoordinateReader(view, NO_INTERPOLATION, false);
	if (coordinate_reader == NULL)
		return NULL;
	reading_coordinates = true;

	ToolScript* the_script
		= new ToolScript(Type(), fToolSettings, ((PaintApplication*)be_app)->Color(true));
	if (the_script == NULL) {
		delete coordinate_reader;

		return NULL;
	}

	BBitmap* buffer = view->ReturnImage()->ReturnActiveBitmap();
	if (buffer == NULL) {
		delete coordinate_reader;
		delete the_script;
		return NULL;
	}
	BBitmap* srcBuffer = new (std::nothrow) BBitmap(buffer);
	if (srcBuffer == NULL) {
		delete coordinate_reader;
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

	Selection* selection = view->GetSelection();
	BitmapDrawer* drawer = new BitmapDrawer(tmpBuffer);
	if (drawer == NULL) {
		delete coordinate_reader;
		delete the_script;

		return NULL;
	}

	float pressure = (float)fToolSettings.pressure / 100.;

	union color_conversion background;
	background.word = 0xFFFFFFFF;

	if (fToolSettings.mode == HS_ERASE_TO_BACKGROUND_MODE) {
		rgb_color c = ((PaintApplication*)be_app)->Color(false);
		background.bytes[0] = c.blue;
		background.bytes[1] = c.green;
		background.bytes[2] = c.red;
		background.bytes[3] = c.alpha * pressure;
		composite_func = src_over_fixed;
	} else
		background.bytes[3] *= pressure;

	BPoint prev_point;

	prev_point = point;
	BRect updated_rect;
	status_t status_of_read;
	Brush* brush;

	float brush_width_per_2;
	float brush_height_per_2;
	int diameter = fToolSettings.size;
	if ((diameter % 2) == 0)
		diameter++;

	if (fToolSettings.use_current_brush == true) {
		brush = ToolManager::Instance().GetCurrentBrush();
		brush_width_per_2 = floor(brush->Width() / 2);
		brush_height_per_2 = floor(brush->Height() / 2);

		brush->draw(tmpBuffer,
			BPoint(prev_point.x - brush_width_per_2, prev_point.y - brush_height_per_2), selection);
	} else {
		brush_width_per_2 = floor(fToolSettings.size / 2);
		brush_height_per_2 = brush_width_per_2;

		if (diameter != 1) {
			drawer->DrawCircle(prev_point, diameter / 2, tmp_draw_color.word, true, true, selection,
				src_over_fixed);
		} else {
			drawer->DrawHairLine(
				prev_point, point, tmp_draw_color.word, true, selection, src_over_fixed);
		}
	}

	// This makes sure that the view is updated even if just one point is drawn
	updated_rect.left = min_c(point.x - brush_width_per_2, prev_point.x - brush_width_per_2);
	updated_rect.top = min_c(point.y - brush_height_per_2, prev_point.y - brush_height_per_2);
	updated_rect.right = max_c(point.x + brush_width_per_2, prev_point.x + brush_width_per_2);
	updated_rect.bottom = max_c(point.y + brush_height_per_2, prev_point.y + brush_height_per_2);

	// We should do the composite picture and re-draw the window in
	// a separate thread.
	buffer->Lock();
	BitmapUtilities::CompositeBitmapOnSource(
		buffer, srcBuffer, tmpBuffer, updated_rect, composite_func, background.word);
	buffer->Unlock();

	SetLastUpdatedRect(updated_rect);
	the_script->AddPoint(point);

	ImageUpdater* imageUpdater = new ImageUpdater(view, 2000);
	imageUpdater->AddRect(updated_rect);

	while (coordinate_reader->GetPoint(point) == B_OK) {
		if (prev_point != point) {
			the_script->AddPoint(point);
			if (fToolSettings.use_current_brush == true && brush != NULL) {
				brush->draw(tmpBuffer,
					BPoint(point.x - brush_width_per_2, point.y - brush_height_per_2), selection);
				brush->draw_line(tmpBuffer, point, prev_point, selection);
			} else {
				if (diameter != 1) {
					drawer->DrawCircle(point, diameter / 2, tmp_draw_color.word, true, true,
						selection, src_over_fixed);
					drawer->DrawLine(prev_point, point, tmp_draw_color.word, diameter, true,
						selection, src_over_fixed);
				} else
					drawer->DrawHairLine(
						prev_point, point, background.word, true, selection, composite_func);
			}

			updated_rect.left
				= min_c(point.x - brush_width_per_2 - 1, prev_point.x - brush_width_per_2 - 1);
			updated_rect.top
				= min_c(point.y - brush_height_per_2 - 1, prev_point.y - brush_height_per_2 - 1);
			updated_rect.right
				= max_c(point.x + brush_width_per_2 + 1, prev_point.x + brush_width_per_2 + 1);
			updated_rect.bottom
				= max_c(point.y + brush_height_per_2 + 1, prev_point.y + brush_height_per_2 + 1);

			imageUpdater->AddRect(updated_rect);

			SetLastUpdatedRect(LastUpdatedRect() | updated_rect);

			buffer->Lock();
			BitmapUtilities::CompositeBitmapOnSource(
				buffer, srcBuffer, tmpBuffer, updated_rect, composite_func, background.word);
			buffer->Unlock();

			prev_point = point;
		}
	}

	imageUpdater->ForceUpdate();
	delete imageUpdater;

	delete srcBuffer;
	delete tmpBuffer;

	delete drawer;
	delete coordinate_reader;
	return the_script;
}


int32
EraserTool::UseToolWithScript(ToolScript*, BBitmap*)
{
	return B_ERROR;
}


BView*
EraserTool::ConfigView()
{
	return new EraserToolConfigView(this);
}


const void*
EraserTool::ToolCursor() const
{
	return HS_ERASER_CURSOR;
}


const char*
EraserTool::HelpString(bool isInUse) const
{
	return (isInUse ? B_TRANSLATE("Erasing pixels.") : B_TRANSLATE("Eraser tool"));
}


// #pragma mark -- EraserToolConfigView


EraserToolConfigView::EraserToolConfigView(DrawingTool* tool)
	:
	DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SIZE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(SIZE_OPTION));

		fSizeSlider = new NumberSliderControl(B_TRANSLATE("Size:"), "1", message, 1, 100, false);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", MODE_OPTION);
		message->AddInt32("value", HS_ERASE_TO_BACKGROUND_MODE);

		fBackground = new BRadioButton(B_TRANSLATE("Background color"), new BMessage(*message));

		message->ReplaceInt32("value", HS_ERASE_TO_TRANSPARENT_MODE);
		fTransparent = new BRadioButton(B_TRANSLATE("Transparency"), message);

		BGridLayout* sizeLayout = LayoutSliderGrid(fSizeSlider);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", USE_BRUSH_OPTION);
		message->AddInt32("value", 0x00000000);
		fUseBrush = new BCheckBox(B_TRANSLATE("Use current brush"), message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", PRESSURE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(PRESSURE_OPTION));

		fPressureSlider
			= new NumberSliderControl(B_TRANSLATE("Pressure:"), "100", message, 1, 100, false);

		BGridLayout* pressureLayout = LayoutSliderGrid(fPressureSlider);

		layout->AddView(BGroupLayoutBuilder(B_VERTICAL, kWidgetSpacing)
			.Add(sizeLayout)
			.Add(pressureLayout)
			.Add(fUseBrush)
			.AddStrut(kWidgetSpacing)
			.Add(SeparatorView(B_TRANSLATE("Replace pixels with")))
			.AddGroup(B_VERTICAL, kWidgetSpacing)
				.Add(fBackground)
				.Add(fTransparent)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.TopView()
		);

		if (tool->GetCurrentValue(MODE_OPTION) == HS_ERASE_TO_BACKGROUND_MODE)
			fBackground->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(MODE_OPTION) == HS_ERASE_TO_TRANSPARENT_MODE)
			fTransparent->SetValue(B_CONTROL_ON);

		fUseBrush->SetValue(tool->GetCurrentValue(USE_BRUSH_OPTION));
		if (tool->GetCurrentValue(USE_BRUSH_OPTION) != B_CONTROL_OFF)
			fSizeSlider->SetEnabled(FALSE);

		fPressureSlider->SetValue(tool->GetCurrentValue(PRESSURE_OPTION));
	}
}


void
EraserToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fSizeSlider->SetTarget(this);
	fBackground->SetTarget(this);
	fTransparent->SetTarget(this);
	fUseBrush->SetTarget(this);
	fPressureSlider->SetTarget(this);
}


void
EraserToolConfigView::MessageReceived(BMessage* message)
{
	DrawingToolConfigView::MessageReceived(message);

	switch (message->what) {
		case OPTION_CHANGED:
		{
			if (message->FindInt32("option") == USE_BRUSH_OPTION) {
				if (fUseBrush->Value() == B_CONTROL_OFF)
					fSizeSlider->SetEnabled(TRUE);
				else
					fSizeSlider->SetEnabled(FALSE);
			}
		} break;
	}
}
