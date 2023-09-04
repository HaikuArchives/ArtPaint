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
#include "CoordinateQueue.h"
#include "CoordinateReader.h"
#include "Cursors.h"
#include "Image.h"
#include "ImageUpdater.h"
#include "ImageView.h"
#include "PaintApplication.h"
#include "PixelOperations.h"
#include "Selection.h"
#include "ToolManager.h"
#include "ToolScript.h"
#include "UtilityClasses.h"


#include <Catalog.h>
#include <File.h>
#include <Layout.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


BrushTool::BrushTool()
	:
	DrawingTool(B_TRANSLATE("Brush tool"), "b", BRUSH_TOOL)
{
	// Options will also have some brush-data options.
	fOptions = 0;
	fOptionsCount = 0;
}


BrushTool::~BrushTool()
{
}


ToolScript*
BrushTool::UseTool(ImageView* view, uint32 buttons, BPoint point, BPoint viewPoint)
{
	B_UNUSED(buttons)
	B_UNUSED(viewPoint)

	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	Brush* brush = ToolManager::Instance().GetCurrentBrush();
	if (brush == NULL)
		return NULL;

	CoordinateReader* coordinate_reader
		= new (std::nothrow) CoordinateReader(view, LINEAR_INTERPOLATION, false);
	if (coordinate_reader == NULL)
		return NULL;

	ToolScript* the_script = new (std::nothrow)
		ToolScript(Type(), fToolSettings, ((PaintApplication*)be_app)->Color(true));
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

	BRect bitmap_bounds = buffer->Bounds();

	BBitmap* tmpBuffer = new (std::nothrow) BBitmap(bitmap_bounds, buffer->ColorSpace());
	if (tmpBuffer == NULL) {
		delete coordinate_reader;
		delete the_script;
		delete srcBuffer;
		return NULL;
	}

	float brush_width_per_2 = floor(brush->Width() / 2);
	float brush_height_per_2 = floor(brush->Height() / 2);

	BPoint prev_point;

	union color_conversion new_color;

	bool use_fg_color = true;
	if (buttons == B_SECONDARY_MOUSE_BUTTON)
		use_fg_color = false;

	new_color.word = RGBColorToBGRA(((PaintApplication*)be_app)->Color(use_fg_color));

	union color_conversion clear_color;
	clear_color.word = new_color.word;
	clear_color.bytes[3] = 0x01;

	BitmapUtilities::ClearBitmap(tmpBuffer, clear_color.word);

	prev_point = last_point = point;
	BRect updated_rect;

	the_script->AddPoint(point);

	if (coordinate_reader->GetPoint(point) == B_OK) {
		brush->draw(tmpBuffer, BPoint(point.x - brush_width_per_2, point.y - brush_height_per_2),
			selection);
	}

	updated_rect = BRect(point.x - brush_width_per_2, point.y - brush_height_per_2,
		point.x + brush_width_per_2, point.y + brush_height_per_2);
	SetLastUpdatedRect(updated_rect);
	buffer->Lock();
	BitmapUtilities::CompositeBitmapOnSource(
		buffer, srcBuffer, tmpBuffer, updated_rect, src_over_fixed, new_color.word);
	buffer->Unlock();
	prev_point = point;

	ImageUpdater* imageUpdater = new ImageUpdater(view, 20000);
	imageUpdater->AddRect(updated_rect);

	while (coordinate_reader->GetPoint(point) == B_OK) {
		brush->draw(tmpBuffer, BPoint(point.x - brush_width_per_2, point.y - brush_height_per_2),
			selection);
		updated_rect = BRect(point.x - brush_width_per_2, point.y - brush_height_per_2,
			point.x + brush_width_per_2, point.y + brush_height_per_2);
		imageUpdater->AddRect(updated_rect);
		SetLastUpdatedRect(updated_rect | LastUpdatedRect());
		buffer->Lock();
		BitmapUtilities::CompositeBitmapOnSource(
			buffer, srcBuffer, tmpBuffer, updated_rect, src_over_fixed, new_color.word);
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
	return (isInUse ? B_TRANSLATE("Painting with a brush.") : B_TRANSLATE("Brush tool"));
}


status_t
BrushTool::readSettings(BFile& file, bool isLittleEndian)
{
	int32 length;
	if (file.Read(&length, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	if (isLittleEndian)
		length = B_LENDIAN_TO_HOST_INT32(length);
	else
		length = B_BENDIAN_TO_HOST_INT32(length);

	int32 version;
	if (file.Read(&version, sizeof(int32)) != sizeof(int32))
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

	if (info.width == 0 || info.width > 500
		|| info.height == 0 || info.height > 500
		|| info.shape > HS_IRREGULAR_BRUSH || info.hardness > 1.0 || info.angle > 180) {
		info.width = 30;
		info.height = 30;
		info.angle = 0;
		info.shape = HS_ELLIPTICAL_BRUSH;
		info.hardness = 0;
	}

	ToolManager::Instance().SetCurrentBrush(&info);

	return B_OK;
}


status_t
BrushTool::writeSettings(BFile& file)
{
	int32 type = Type();
	if (file.Write(&type, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	int32 settings_size = sizeof(struct brush_info) + sizeof(int32);
	if (file.Write(&settings_size, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	int32 settings_version = TOOL_SETTINGS_STRUCT_VERSION;
	if (file.Write(&settings_version, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	brush_info info;
	info = ToolManager::Instance().GetCurrentBrush()->GetInfo();
	if (file.Write(&info, sizeof(struct brush_info)) != sizeof(struct brush_info))
		return B_ERROR;

	return B_OK;
}


// #pragma mark -- BrushToolConfigView


BrushToolConfigView::BrushToolConfigView(DrawingTool* tool)
	:
	DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		// layout->AddView(BrushEditor::CreateBrushEditor(brush));
	}
}
