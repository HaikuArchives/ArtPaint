/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef TEXT_TOOL_H
#define TEXT_TOOL_H

#include "DrawingTool.h"


class TextTool : public DrawingTool {
public:
							TextTool();
	virtual					~TextTool();

			int32			UseToolWithScript(ToolScript*, BBitmap*);
			ToolScript*		UseTool(ImageView*, uint32, BPoint, BPoint);

			const void*		ToolCursor() const;
			const char*		HelpString(bool isInUse) const;
};

#endif
