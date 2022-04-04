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

#include "DrawingTool.h"

#include "Cursors.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "ResourceServer.h"
#include "StringServer.h"
#include "UtilityClasses.h"


#include <Bitmap.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <File.h>
#include <GroupLayoutBuilder.h>
#include <GridLayoutBuilder.h>
#include <Handler.h>
#include <SeparatorView.h>
#include <StringView.h>


#include <new>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


DrawingTool::DrawingTool(const BString& name, int32 type)
	: fIcon(NULL)
	, fName(name)
	, fType(type)
	, fLastUpdatedRect(BRect())
{
	// In derived classes set whatever options tool happens to use.
	fOptions = 0;
	fOptionsCount = 0;

	ResourceServer::Instance()->GetBitmap(B_VECTOR_ICON_TYPE, type,
		LARGE_TOOL_ICON_SIZE, LARGE_TOOL_ICON_SIZE, &fIcon);
}


DrawingTool::~DrawingTool()
{
	delete fIcon;
}


/* !
	This function will do the drawing in the derived classes. ImageView must
	provide necessary data with a function that can be called from here.
*/
ToolScript*
DrawingTool::UseTool(ImageView* view, uint32 buttons, BPoint point,
	BPoint viewPoint)
{
	B_UNUSED(view)
	B_UNUSED(buttons)
	B_UNUSED(point)
	B_UNUSED(viewPoint)

	return NULL;
}


int32
DrawingTool::UseToolWithScript(ToolScript*, BBitmap*)
{
	return B_OK;
}


BView*
DrawingTool::ConfigView()
{
	return NULL;
}


void
DrawingTool::SetOption(int32 option, int32 value, BHandler *source)
{
	// If option is valid for this tool, set it.
	// If handler is NULL, the boolean options should use value as the new value.
	// Otherwise they should use value that can be gotten from the source.

	if (option & fOptions) {
		switch (option) {
			case SIZE_OPTION: {
				fToolSettings.size = value;
			}	break;

			case PRESSURE_OPTION: {
				fToolSettings.pressure = value;
			}	break;

			case MODE_OPTION: {
				fToolSettings.mode = value;
				if (BCheckBox* booleanBox = dynamic_cast<BCheckBox*> (source))
					fToolSettings.mode = booleanBox->Value();
			} break;

			case SHAPE_OPTION: {
				fToolSettings.shape = value;
			}	break;

			case GRADIENT_ENABLED_OPTION: {
				fToolSettings.gradient_enabled = value;
				if (BControl* control = dynamic_cast<BCheckBox*> (source))
					fToolSettings.gradient_enabled = control->Value();
			} break;

			case GRADIENT_COLOR_OPTION: {
				fToolSettings.gradient_color = value;
			}	break;

			case PREVIEW_ENABLED_OPTION: {
				fToolSettings.preview_enabled = value;
				if (BControl* control = dynamic_cast<BCheckBox*> (source))
					fToolSettings.preview_enabled = control->Value();
			}	break;

			case FILL_ENABLED_OPTION: {
				fToolSettings.fill_enabled = value;
				if (BControl* control = dynamic_cast<BCheckBox*> (source))
					fToolSettings.fill_enabled = control->Value();
			}	break;

			case ROTATION_ENABLED_OPTION: {
				fToolSettings.rotation_enabled = value;
				if (BControl* control = dynamic_cast<BCheckBox*> (source))
					fToolSettings.rotation_enabled = control->Value();
			}	break;

			case ANTI_ALIASING_LEVEL_OPTION: {
				fToolSettings.anti_aliasing_level = value;
				if (BControl* control = dynamic_cast<BCheckBox*> (source))
					fToolSettings.anti_aliasing_level = control->Value();
			}	break;

			case CONTINUITY_OPTION: {
				fToolSettings.continuity = value;
				if (BControl* control = dynamic_cast<BCheckBox*> (source))
					fToolSettings.continuity = control->Value();
			}	break;

			case TOLERANCE_OPTION: {
				fToolSettings.tolerance = value;
			}	break;

			case TRANSPARENCY_OPTION: {
				fToolSettings.transparency = value;
			}	break;

			default:	break;
		}
	}
}


int32
DrawingTool::GetCurrentValue(int32 option)
{
	if (option & fOptions) {
		switch (option) {
			case SIZE_OPTION:
				return fToolSettings.size;
			case PRESSURE_OPTION:
				return fToolSettings.pressure;
			case MODE_OPTION:
				return fToolSettings.mode;
			case SHAPE_OPTION:
				return fToolSettings.shape;
			case GRADIENT_ENABLED_OPTION:
				return fToolSettings.gradient_enabled;
			case GRADIENT_COLOR_OPTION:
				return fToolSettings.gradient_color;
			case PREVIEW_ENABLED_OPTION:
				return fToolSettings.preview_enabled;
			case FILL_ENABLED_OPTION:
				return fToolSettings.fill_enabled;
			case ROTATION_ENABLED_OPTION:
				return fToolSettings.rotation_enabled;
			case ANTI_ALIASING_LEVEL_OPTION:
				return fToolSettings.anti_aliasing_level;
			case CONTINUITY_OPTION:
				return fToolSettings.continuity;
			case TOLERANCE_OPTION:
				return fToolSettings.tolerance;
			case TRANSPARENCY_OPTION:
				return fToolSettings.transparency;
			default:
				return 0;
		}
	}
	return 0;
}


BBitmap*
DrawingTool::Icon() const
{
	return new (std::nothrow) BBitmap(fIcon);
}


status_t
DrawingTool::readSettings(BFile &file, bool isLittleEndian)
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
	} else {
		// This should also be converted to right endianness
		int32 settingsSize = sizeof(struct tool_settings);
		if (file.Read(&fToolSettings, settingsSize) != settingsSize)
			return B_ERROR;
	}

	return B_OK;
}


status_t
DrawingTool::writeSettings(BFile &file)
{
	if (file.Write(&fType,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	int32 settingsSize = sizeof(struct tool_settings) + sizeof(int32);
	if (file.Write(&settingsSize,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	int32 settingsVersion = TOOL_SETTINGS_STRUCT_VERSION;
	if (file.Write(&settingsVersion,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	settingsSize = sizeof(struct tool_settings);
	if (file.Write(&fToolSettings, settingsSize) != settingsSize)
		return B_ERROR;

	return B_OK;
}


BRect
DrawingTool::LastUpdatedRect() const
{
	return fLastUpdatedRect;
}


void
DrawingTool::SetLastUpdatedRect(const BRect& rect)
{
	fLastUpdatedRect = rect;
}


const void*
DrawingTool::ToolCursor() const
{
	return HS_CROSS_CURSOR;
}


const char*
DrawingTool::HelpString(bool isInUse) const
{
	return B_TRANSLATE(isInUse ? "Using the tool."
		: "Use the tool by pressing the mouse-button.");
}


// #pragma mark -- DrawingToolConfigView


DrawingToolConfigView::DrawingToolConfigView(DrawingTool* drawingTool)
	: BBox(B_FANCY_BORDER, NULL)
	, fTool(drawingTool)
{
	SetLayout(BGroupLayoutBuilder(B_VERTICAL)
		.SetInsets(10.0, be_bold_font->Size()+10.0, 10.0, 10.0)
		.TopLayout());
	SetLabel(drawingTool->Name().String());
}


DrawingToolConfigView::~DrawingToolConfigView()
{
}


void
DrawingToolConfigView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
DrawingToolConfigView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case OPTION_CHANGED: {
			// This comes from one of the controls in this window, it tells us
			// that the value for tool control(s) have changed, it contains
			// int32 "option" and int32 "value" data members
			BHandler* handler;
			message->FindPointer("source", (void**)&handler);
			if (fTool) {
				fTool->SetOption(message->FindInt32("option"),
					message->FindInt32("value"), handler);
			}
		}	break;

		default: {
			BView::MessageReceived(message);
		}	break;
	}
}


BSeparatorView*
DrawingToolConfigView::SeparatorView(const char* label) const
{
	BSeparatorView* view =
		new BSeparatorView(label, B_HORIZONTAL, B_FANCY_BORDER,
			BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));
	view->SetExplicitMinSize(BSize(200.0, B_SIZE_UNSET));
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	return view;
}


BGridLayout*
DrawingToolConfigView::LayoutSliderGrid(
	ArtPaint::Interface::NumberSliderControl* slider) const
{
	BGridLayout* gridLayout = BGridLayoutBuilder(
		kWidgetSpacing, kWidgetSpacing)
			.Add(slider, 0, 0, 0, 0)
			.Add(slider->LabelLayoutItem(), 0, 0)
			.Add(slider->TextViewLayoutItem(), 1, 0)
			.Add(slider->Slider(), 2, 0)
			.SetInsets(kWidgetInset, 0.0, 0.0, 0.0);
		gridLayout->SetMinColumnWidth(0, StringWidth("LABELSIZE"));
		gridLayout->SetMaxColumnWidth(1, StringWidth("100"));
		gridLayout->SetMinColumnWidth(2, StringWidth("SLIDERSLIDERSLIDER"));

	return gridLayout;
}

