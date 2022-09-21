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
#include "Cursors.h"
#include "CoordinateQueue.h"
#include "Image.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "PixelOperations.h"
#include "ToolScript.h"
#include "UtilityClasses.h"


#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <Layout.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


using ArtPaint::Interface::NumberSliderControl;


FreeLineTool::FreeLineTool()
	: DrawingTool(B_TRANSLATE("Freehand line tool"),
		FREE_LINE_TOOL)
{
	fOptions = SIZE_OPTION;
	fOptionsCount = 1;

	SetOption(SIZE_OPTION, 1);
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
	union color_conversion clear_color;
	clear_color.word = 0xFFFFFFFF;
	clear_color.bytes[3] = 0x01;

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
	uint32 new_color_bgra;
//	int32 original_mouse_speed;
//	get_mouse_speed(&original_mouse_speed);

	BPoint prev_point;

	prev_point = point;
	BRect updated_rect;
	status_t status_of_read;
	int diameter = fToolSettings.size;
	if ((diameter%2) == 0)
		diameter++;

	new_color = ((PaintApplication*)be_app)->Color(true);
	new_color_bgra = RGBColorToBGRA(new_color);
	if (diameter != 1)
		drawer->DrawCircle(prev_point, diameter / 2, new_color_bgra,
			true, false, selection, NULL);
	else
		drawer->DrawHairLine(prev_point, point, new_color_bgra,
			false, selection, NULL);

	// This makes sure that the view is updated even if just one point is drawn
	updated_rect.left = min_c(point.x-diameter/2,prev_point.x-diameter/2);
	updated_rect.top = min_c(point.y-diameter/2,prev_point.y-diameter/2);
	updated_rect.right = max_c(point.x+diameter/2,prev_point.x+diameter/2);
	updated_rect.bottom = max_c(point.y+diameter/2,prev_point.y+diameter/2);

	// We should do the composite picture and re-draw the window in
	// a separate thread.
	view->ReturnImage()->Render(updated_rect);
	view->Window()->Lock();
	BitmapUtilities::CompositeBitmapOnSource(buffer, srcBuffer,
		tmpBuffer, updated_rect);
	// We have to use Draw, because Invalidate causes flickering by erasing
	// the area before calling Draw.
	view->Draw(view->convertBitmapRectToView(updated_rect));
	view->UpdateImage(updated_rect);
	view->Sync();
	view->Window()->Unlock();

	SetLastUpdatedRect(updated_rect);
	the_script->AddPoint(point);
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
			// first set the color
			new_color = ((PaintApplication*)be_app)->Color(true);
			new_color_bgra = RGBColorToBGRA(new_color);

			diameter = fToolSettings.size;
			if ((diameter%2) == 0)
				diameter++;

			if (diameter != 1) {
				drawer->DrawCircle(point,diameter / 2, new_color_bgra,
					true, false, selection, NULL);
				drawer->DrawLine(prev_point,point, new_color_bgra, diameter,
					false, selection, NULL);
			}
			else
				drawer->DrawHairLine(prev_point, point, new_color_bgra,
					false, selection);

			updated_rect.left = min_c(point.x-diameter/2-1,prev_point.x-diameter/2-1);
			updated_rect.top = min_c(point.y-diameter/2-1,prev_point.y-diameter/2-1);
			updated_rect.right = max_c(point.x+diameter/2+1,prev_point.x+diameter/2+1);
			updated_rect.bottom = max_c(point.y+diameter/2+1,prev_point.y+diameter/2+1);

			SetLastUpdatedRect(LastUpdatedRect() | updated_rect);

			// We should do the composite picture and re-draw the window in
			// a separate thread.
			view->Window()->Lock();
			BitmapUtilities::CompositeBitmapOnSource(buffer, srcBuffer,
				tmpBuffer, updated_rect);
			view->UpdateImage(updated_rect);
			view->Sync();
			view->Window()->Unlock();
			prev_point = point;
		}
		else
			snooze(20 * 1000);
	}

	delete srcBuffer;
	delete tmpBuffer;

	delete drawer;
	delete coordinate_queue;
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

		layout->AddView(lineSizeLayout->View());
	}
}


void
FreeLineToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fLineSize->SetTarget(this);
}
