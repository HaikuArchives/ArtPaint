/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "TextTool.h"

#include "Cursors.h"


#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


TextTool::TextTool()
	: DrawingTool(B_TRANSLATE("Text tool"), "t",
		TEXT_TOOL)
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
	return (isInUse
		? B_TRANSLATE("Drag the text to correct position and set its appearance.")
		: B_TRANSLATE("Click to place the text."));
}
