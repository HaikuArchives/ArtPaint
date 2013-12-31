/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */

#include "TextTool.h"

#include "Cursors.h"
#include "StringServer.h"


TextTool::TextTool()
	: DrawingTool(StringServer::ReturnString(TEXT_TOOL_NAME_STRING), TEXT_TOOL)
{
}


TextTool::~TextTool()
{
}


int32
TextTool::UseToolWithScript(ToolScript*, BBitmap*)
{
	return B_ERROR;
}


ToolScript*
TextTool::UseTool(ImageView*, uint32, BPoint, BPoint)
{
	return NULL;
}


const void*
TextTool::ToolCursor() const
{
	return HS_TEXT_CURSOR;
}


const char*
TextTool::HelpString(bool isInUse) const
{
	return StringServer::ReturnString(isInUse ? TEXT_TOOL_IN_USE_STRING
		: TEXT_TOOL_READY_STRING);
}
