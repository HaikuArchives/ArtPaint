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

#include "BrushTool.h"

#include "BitmapUtilities.h"
#include "Brush.h"
#include "BrushEditor.h"
#include "CoordinateReader.h"
#include "CoordinateQueue.h"
#include "Cursors.h"
#include "Image.h"
#include "ImageUpdater.h"
#include "ImageView.h"
#include "PaintApplication.h"
#include "PixelOperations.h"
#include "Selection.h"
#include "ToolScript.h"
#include "UtilityClasses.h"


#include <Catalog.h>
#include <File.h>
#include <Layout.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


BrushTool::BrushTool()
	: DrawingTool(B_TRANSLATE("Brush tool"), BRUSH_TOOL)
{
	// Options will also have some brush-data options.
	fOptions = 0;
	fOptionsCount = 0;

	brush_info info;
	info.shape = HS_ELLIPTICAL_BRUSH;
	info.width = 30;
	info.height = 30;
	info.angle = 0;
	info.hardness = 2;

	brush = new Brush(info);
}


BrushTool::~BrushTool()
{
	delete brush;
}


ToolScript*
BrushTool::UseTool(ImageView *view, uint32 buttons, BPoint point, BPoint viewPoint)
{
	B_UNUSED(buttons)
	B_UNUSED(viewPoint)

	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	CoordinateReader* coordinate_reader =
		new (std::nothrow) CoordinateReader(view, LINEAR_INTERPOLATION, false);
	if (coordinate_reader == NULL)
		return NULL;

	ToolScript* the_script = new (std::nothrow) ToolScript(Type(),
		fToolSettings, ((PaintApplication*)be_app)->Color(true));
	if (the_script == NULL) {
		delete coordinate_reader;
		return NULL;
	}

	selection = view->GetSelection();

	BBitmap* buffer = view->ReturnImage()->ReturnActiveBitmap();
	BBitmap* srcBuffer = new (std::nothrow) BBitmap(buffer);
	if (srcBuffer == NULL) {
		delete coordinate_reader;
		delete the_script;
		return NULL;
	}

	bits = (uint32*)buffer->Bits();
	bpr = buffer->BytesPerRow()/4;
	BRect bitmap_bounds = buffer->Bounds();

	left_bound = (int32)bitmap_bounds.left;
	right_bound = (int32)bitmap_bounds.right;
	top_bound = (int32)bitmap_bounds.top;
	bottom_bound = (int32)bitmap_bounds.bottom;

	BBitmap* tmpBuffer = new (std::nothrow) BBitmap(bitmap_bounds,
		buffer->ColorSpace());
	if (tmpBuffer == NULL) {
		delete coordinate_reader;
		delete the_script;
		delete srcBuffer;
		return NULL;
	}

	float brush_width_per_2 = floor(brush->Width()/2);
	float brush_height_per_2 = floor(brush->Height()/2);

	BPoint prev_point;

	union color_conversion new_color;

	new_color.word =
		RGBColorToBGRA(((PaintApplication*)be_app)->Color(true));

	union color_conversion clear_color;
	clear_color.word = new_color.word;
	clear_color.bytes[3] = 0x01;

	BitmapUtilities::ClearBitmap(tmpBuffer, clear_color.word);

	prev_point = last_point = point;
	BRect updated_rect;

	the_script->AddPoint(point);

	if (coordinate_reader->GetPoint(point) == B_OK) {
		draw_brush(tmpBuffer, BPoint(point.x - brush_width_per_2,
			point.y - brush_height_per_2), 0, 0, new_color.word);
	}

	updated_rect = BRect(point.x - brush_width_per_2,
		point.y - brush_height_per_2, point.x + brush_width_per_2,
		point.y + brush_height_per_2);
	SetLastUpdatedRect(updated_rect);
	buffer->Lock();
	BitmapUtilities::CompositeBitmapOnSource(buffer, srcBuffer,
		tmpBuffer, updated_rect);
	buffer->Unlock();
	prev_point = point;

	ImageUpdater* imageUpdater = new ImageUpdater(view, 20000);
	imageUpdater->AddRect(updated_rect);

	while (coordinate_reader->GetPoint(point) == B_OK) {
		draw_brush(tmpBuffer, BPoint(point.x - brush_width_per_2,
			point.y - brush_height_per_2), int32(point.x - prev_point.x),
			int32(point.y - prev_point.y), new_color.word);
		updated_rect = BRect(point.x - brush_width_per_2,
			point.y - brush_height_per_2, point.x + brush_width_per_2,
			point.y + brush_height_per_2);
		imageUpdater->AddRect(updated_rect);
		SetLastUpdatedRect(updated_rect | LastUpdatedRect());
		buffer->Lock();
		BitmapUtilities::CompositeBitmapOnSource(buffer, srcBuffer,
			tmpBuffer, updated_rect);
		buffer->Unlock();
		prev_point = point;
	}

	imageUpdater->ForceUpdate();

	delete imageUpdater;
	delete coordinate_reader;

//	test_brush(point,new_color);
//	if (view->LockLooper() == true) {
//		view->UpdateImage(view->Bounds());
//		view->Invalidate();
//		view->UnlockLooper();
//	}

	delete srcBuffer;
	delete tmpBuffer;

	return the_script;
}


int32
BrushTool::UseToolWithScript(ToolScript*, BBitmap*)
{
	return B_OK;
}


BView*
BrushTool::ConfigView()
{
	return new BrushToolConfigView(this);
}


const void*
BrushTool::ToolCursor() const
{
	return HS_BRUSH_CURSOR;
}


const char*
BrushTool::HelpString(bool isInUse) const
{
	return (isInUse
		? B_TRANSLATE("Painting with a brush.")
		: B_TRANSLATE("Click to paint with a brush."));
}


BRect
BrushTool::draw_line(BBitmap* buffer, BPoint start,BPoint end,uint32 color)
{
	int32 brush_width_per_2 = (int32)floor(brush->Width()/2);
	int32 brush_height_per_2 = (int32)floor(brush->Height()/2);
	BRect a_rect = MakeRectFromPoints(start, end);
	a_rect.InsetBy(-brush_width_per_2-1,-brush_height_per_2-1);
	// first check whether the line is longer in x direction than y
	bool increase_x = fabs(start.x - end.x) >= fabs(start.y - end.y);
	// check which direction the line is going
	float sign_x;
	float sign_y;
	int32 number_of_points;
	if ((end.x-start.x) != 0) {
		sign_x = (end.x-start.x)/fabs(start.x - end.x);
	}
	else {
		sign_x = 0;
	}
	if ((end.y-start.y) != 0) {
		sign_y = (end.y-start.y)/fabs(start.y - end.y);
	}
	else {
		sign_y = 0;
	}
	int32 dx,dy;
	int32 last_x,last_y;
	int32 new_x,new_y;

	if (increase_x) {
		float y_add = ((float)fabs(start.y - end.y)) / ((float)fabs(start.x - end.x));
		number_of_points = (int32)fabs(start.x-end.x);
		for (int32 i=0;i<number_of_points;i++) {
			last_point = start;
			start.x += sign_x;
			start.y += sign_y * y_add;
			new_x = (int32)round(start.x);
			new_y = (int32)round(start.y);
			last_x = (int32)round(last_point.x);
			last_y = (int32)round(last_point.y);

			dx = new_x - last_x;
			dy = new_y - last_y;
			draw_brush(buffer, BPoint(new_x - brush_width_per_2, new_y -
				brush_height_per_2), dx, dy, color);

//			view->Window()->Lock();
//			view->Invalidate();
//			view->Window()->Unlock();
//			snooze(50 * 1000);
		}
	}

	else {
		float x_add = ((float)fabs(start.x - end.x)) / ((float)fabs(start.y - end.y));
		number_of_points = (int32)fabs(start.y-end.y);
		for (int32 i=0;i<number_of_points;i++) {
			last_point = start;
			start.y += sign_y;
			start.x += sign_x * x_add;
			new_x = (int32)round(start.x);
			new_y = (int32)round(start.y);
			last_x = (int32)round(last_point.x);
			last_y = (int32)round(last_point.y);

			dx = new_x - last_x;
			dy = new_y - last_y;
			draw_brush(buffer, BPoint(new_x-brush_width_per_2,new_y-brush_height_per_2),dx,dy,color);

//			view->Window()->Lock();
//			view->Invalidate();
//			view->Window()->Unlock();
//			snooze(50 * 1000);
		}
	}
	return a_rect;
}


void
BrushTool::draw_brush(BBitmap* buffer, BPoint point,
	int32 dx, int32 dy, uint32 c)
{
	span* spans;
	int32 px = (int32)point.x;
	int32 py = (int32)point.y;
	uint32** brush_matrix = brush->GetData(&spans, dx, dy);

	if (brush_matrix == NULL)
		return;

	bits = (uint32*)buffer->Bits();
	uint32* target_bits = bits;
	while ((spans != NULL) && (spans->row + py <= bottom_bound)) {
		int32 left = max_c(px + spans->span_start, left_bound) ;
		int32 right = min_c(px + spans->span_end, right_bound);
		int32 y = spans->row;
		if (y + py >= top_bound) {
			// This works even if there are many spans in one row.
			target_bits = bits + (y + py) * bpr + left;
			for (int32 x = left; x <= right; ++x) {
				if (selection->IsEmpty() || selection->ContainsPoint(x, y + py)) {

					*target_bits = mix_2_pixels_fixed(c, *target_bits,
						brush_matrix[y][x-px]);
				}
				target_bits++;
			}
		}
		spans = spans->next;
	}
}


status_t
BrushTool::readSettings(BFile &file, bool isLittleEndian)
{
	int32 length;
	if (file.Read(&length,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	if (isLittleEndian)
		length = B_LENDIAN_TO_HOST_INT32(length);
	else
		length = B_BENDIAN_TO_HOST_INT32(length);

	int32 version;
	if (file.Read(&version,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	if (isLittleEndian)
		version = B_LENDIAN_TO_HOST_INT32(version);
	else
		version = B_BENDIAN_TO_HOST_INT32(version);

	if (version != TOOL_SETTINGS_STRUCT_VERSION) {
		file.Seek(length - sizeof(int32), SEEK_CUR);
		return B_ERROR;
	}

	// Here we should take the endianness into account.
	brush_info info;
	if (file.Read(&info, sizeof(struct brush_info)) != sizeof(struct brush_info))
		return B_ERROR;

	delete brush;
	brush = new Brush(info);

	return B_OK;
}


status_t
BrushTool::writeSettings(BFile &file)
{
	int32 type = Type();
	if (file.Write(&type, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	int32 settings_size = sizeof(struct brush_info) + sizeof(int32);
	if (file.Write(&settings_size,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	int32 settings_version = TOOL_SETTINGS_STRUCT_VERSION;
	if (file.Write(&settings_version,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	brush_info info;
	info = brush->GetInfo();
	if (file.Write(&info,sizeof(struct brush_info)) != sizeof(struct brush_info))
		return B_ERROR;

	return B_OK;
}


// #pragma mark -- BrushToolConfigView


BrushToolConfigView::BrushToolConfigView(DrawingTool* tool)
	: DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BrushTool* brushTool = dynamic_cast<BrushTool*> (tool);
		layout->AddView(BrushEditor::CreateBrushEditor(brushTool->GetBrush()));
	}
}
