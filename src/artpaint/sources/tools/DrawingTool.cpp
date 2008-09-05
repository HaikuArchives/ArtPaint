/* 

	Filename:	DrawingTool.cpp
	Contents:	DrawingTool base-class definitions	
	Author:		Heikki Suhonen
	
*/

#include <CheckBox.h>
#include <ClassInfo.h>
#include <File.h>
#include <Handler.h>
#include <string.h>
#include <unistd.h>
#include <StringView.h>

#include "DrawingTool.h"
#include "Cursors.h"
#include "StringServer.h"

DrawingTool::DrawingTool(const char *tool_name, int32 tool_type)
{
	// Here copy the arguments to member variables
	type = tool_type;
	strcpy(name,tool_name);
	
	// In derived classes set whatever options tool happens to use.  
	// Base-class has none.
	options = 0;
	number_of_options = 0;
	last_updated_rect = BRect(0,0,-1,-1);
}

DrawingTool::~DrawingTool()
{
}

ToolScript* DrawingTool::UseTool(ImageView*,uint32,BPoint,BPoint)
{
	// this function will do the drawing in the derived classes
	// ImageView must provide necessary data with a function that can
	// be called from here	

	// this base-class version does nothing
	return NULL;
}

int32 DrawingTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_NO_ERROR;	
}

BView* DrawingTool::makeConfigView()
{
	BView *config_view = new DrawingToolConfigView(BRect(0,0,0,0),this);
	
	BStringView *string_view = new BStringView(BRect(0,0,0,0),"string_view",StringServer::ReturnString(NO_OPTIONS_STRING));
	string_view->ResizeBy(string_view->StringWidth(StringServer::ReturnString(NO_OPTIONS_STRING)),0);
	font_height fHeight;
	string_view->GetFontHeight(&fHeight);
	string_view->ResizeBy(0,fHeight.ascent+fHeight.descent);
	config_view->AddChild(string_view);
	config_view->ResizeTo(string_view->Bounds().Width(),string_view->Bounds().Height());
	
	return config_view;
}


void DrawingTool::SetOption(int32 option, int32 value, BHandler *source)
{
	// If option is valid for this tool, set it.
	// If handler is NULL, the boolean options should use value as the new value.
	// Otherwise they should use value that can be gotten from the source.
	BCheckBox *boolean_box;
	BControl *control;
	if (option & options) {
		switch (option) {
			case SIZE_OPTION:
				settings.size = value;
				break;
			case PRESSURE_OPTION:
				settings.pressure = value;
				break;
			case MODE_OPTION:
				boolean_box = cast_as(source,BCheckBox);
				if (boolean_box != NULL) {
					settings.mode = boolean_box->Value();						
				}
				else {				
					settings.mode = value;
				}
				break;
			case SHAPE_OPTION:
				settings.shape = value;
				break;
			case GRADIENT_ENABLED_OPTION:
				control = cast_as(source,BControl);
				if (control != NULL) {
					settings.gradient_enabled = control->Value();						
				}
				else {				
					settings.gradient_enabled = value;
				}
				break;
			case GRADIENT_COLOR_OPTION:
				settings.gradient_color = value;
				break;
			case PREVIEW_ENABLED_OPTION:
				control = cast_as(source,BControl);
				if (control != NULL) {
					settings.preview_enabled = control->Value();						
				}
				else {				
					settings.preview_enabled = value;
				}			
				break;
			case FILL_ENABLED_OPTION:
				control = cast_as(source,BControl);
				if (control != NULL) {
					settings.fill_enabled = control->Value();						
				}
				else {				
					settings.fill_enabled = value;
				}			
				break;			
			case ROTATION_ENABLED_OPTION:
				control = cast_as(source,BControl);
				if (control != NULL) {
					settings.rotation_enabled = control->Value();						
				}
				else {				
					settings.rotation_enabled = value;
				}			
				break;			
			case ANTI_ALIASING_LEVEL_OPTION:
				control = cast_as(source,BControl);
				if (control != NULL) {
					settings.anti_aliasing_level = control->Value();
				}
				else {
					settings.anti_aliasing_level = value;
				}
				break;
			case CONTINUITY_OPTION:
				control = cast_as(source,BControl);
				if (control != NULL) {
					settings.continuity = control->Value();
				}
				else {
					settings.continuity = value;
				}
				break;
			case TOLERANCE_OPTION:
				settings.tolerance = value;
				break;
			case TRANSPARENCY_OPTION:
				settings.transparency = value;
				break;
			default:
				break;
		}
	}
}

int32 DrawingTool::GetCurrentValue(int32 option)
{
	if (option & options) {
		switch (option) {
			case SIZE_OPTION:
				return settings.size;
			case PRESSURE_OPTION:
				return settings.pressure;
			case MODE_OPTION:
				return settings.mode;
			case SHAPE_OPTION:
				return settings.shape;
			case GRADIENT_ENABLED_OPTION:
				return settings.gradient_enabled;
			case GRADIENT_COLOR_OPTION:
				return settings.gradient_color;
			case PREVIEW_ENABLED_OPTION:
				return settings.preview_enabled;
			case FILL_ENABLED_OPTION:
				return settings.fill_enabled;
			case ROTATION_ENABLED_OPTION:
				return settings.rotation_enabled;
			case ANTI_ALIASING_LEVEL_OPTION:
				return settings.anti_aliasing_level;
			case CONTINUITY_OPTION:
				return settings.continuity;
			case TOLERANCE_OPTION:
				return settings.tolerance;
			case TRANSPARENCY_OPTION:
				return settings.transparency;
			default:
				return 0;
		}	
	}
	else
		return 0;
}

status_t DrawingTool::readSettings(BFile &file,bool is_little_endian)
{
	int32 length;
	if (file.Read(&length,sizeof(int32)) != sizeof(int32)) {
		return B_ERROR;
	}
	if (is_little_endian)
		length = B_LENDIAN_TO_HOST_INT32(length);
	else
		length = B_BENDIAN_TO_HOST_INT32(length);

	int32 version;
	if (file.Read(&version,sizeof(int32)) != sizeof(int32)) {
		return B_ERROR;
	}
	if (is_little_endian)
		version = B_LENDIAN_TO_HOST_INT32(version);
	else
		version = B_BENDIAN_TO_HOST_INT32(version);
	
	if (version != TOOL_SETTINGS_STRUCT_VERSION) {
		file.Seek(length-sizeof(int32),SEEK_CUR);
		return B_ERROR;
	}
	else {
		// This should also be converted to right endianness
		if (file.Read(&settings,sizeof(struct tool_settings)) != sizeof(struct tool_settings))
			return B_ERROR;
	}
			
	return B_OK;
}

status_t DrawingTool::writeSettings(BFile &file)
{
	if (file.Write(&type,sizeof(int32)) != sizeof(int32)) {
		return B_ERROR;
	}
	int32 settings_size = sizeof(struct tool_settings) + sizeof(int32);
	if (file.Write(&settings_size,sizeof(int32)) != sizeof(int32)) {
		return B_ERROR;
	}	
	int32 settings_version = TOOL_SETTINGS_STRUCT_VERSION;
	if (file.Write(&settings_version,sizeof(int32)) != sizeof(int32)) {
		return B_ERROR;
	}	
	
	if (file.Write(&settings,sizeof(struct tool_settings)) != sizeof(struct tool_settings))
		return B_ERROR;
	
	return B_OK;
}



BRect DrawingTool::LastUpdatedRect()
{
	BRect rect = last_updated_rect;
	last_updated_rect = BRect(0,0,-1,-1);
	return rect;
}




const void* DrawingTool::ReturnToolCursor()
{
	return HS_CROSS_CURSOR;
}


const char* DrawingTool::ReturnHelpString(bool is_in_use)
{
	if (!is_in_use)
		return StringServer::ReturnString(USE_THE_TOOL_STRING);
	else
		return StringServer::ReturnString(USING_THE_TOOL_STRING);
}



DrawingToolConfigView::DrawingToolConfigView(BRect rect,DrawingTool *t)
	: BView(rect,"drawing_tool_config_view",B_FOLLOW_ALL_SIDES,B_WILL_DRAW)
{
	tool = t;	
}


DrawingToolConfigView::~DrawingToolConfigView()
{

}

void DrawingToolConfigView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
}



void DrawingToolConfigView::MessageReceived(BMessage *message)
{
	BHandler *handler;
	message->FindPointer("source",(void**)&handler);
	
	switch (message->what) {
		// this comes from one of the controls in this window, it tells us that the value
		// for that slider has changed, it contains int32 "option" and int32 "value" data members
		case OPTION_CHANGED:
			tool->SetOption(message->FindInt32("option"),message->FindInt32("value"),handler);
			break;

		default:
			BView::MessageReceived(message);
			break;
	}	
}
