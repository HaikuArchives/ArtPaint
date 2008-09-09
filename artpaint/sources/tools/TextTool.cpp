/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "Cursors.h"
#include "TextTool.h"
#include "StringServer.h"

TextTool::TextTool()
	: DrawingTool(StringServer::ReturnString(TEXT_TOOL_NAME_STRING),TEXT_TOOL)
{
}

TextTool::~TextTool()
{
}



ToolScript* TextTool::UseTool(ImageView*,uint32,BPoint,BPoint)
{
	return NULL;
}


int32 TextTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_ERROR;
}

const char* TextTool::ReturnHelpString(bool is_in_use)
{
	if (!is_in_use)
		return StringServer::ReturnString(TEXT_TOOL_READY_STRING);
	else
		return StringServer::ReturnString(TEXT_TOOL_IN_USE_STRING);
}

const void* TextTool::ReturnToolCursor()
{
	return HS_TEXT_CURSOR;
}
