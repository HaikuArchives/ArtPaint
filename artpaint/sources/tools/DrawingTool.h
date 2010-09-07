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
#ifndef DRAWING_TOOL_H
#define DRAWING_TOOL_H

#include "Tools.h"


#include <Box.h>
#include <String.h>


class BBitmap;
class BFile;
class BHandler;
class ImageView;
class ToolScript;


class DrawingTool {
public:
							DrawingTool(const BString& name, int32 type);
	virtual					~DrawingTool();

	virtual	int32			UseToolWithScript(ToolScript*, BBitmap*);
	virtual	ToolScript*		UseTool(ImageView*, uint32 buttons, BPoint point,
								BPoint viewPoint);

	virtual	BView*			makeConfigView();
	virtual	void			UpdateConfigView(BView*) {}

	inline	int32			Options() { return options; }
	virtual	void			SetOption(int32 option, int32 value,
								BHandler* source = NULL);

	virtual	int32			GetCurrentValue(int32 option);

			BBitmap*		Icon() const;
			BString			Name() const { return fName; }
			int32			Type() const { return fType; }


	// these functions read and write tool's settings to a file
	virtual	status_t		readSettings(BFile &file,bool is_little_endian);
	virtual	status_t		writeSettings(BFile &file);

			BRect			LastUpdatedRect() const;
			void			SetLastUpdatedRect(const BRect& rect);

	virtual	const void*		ToolCursor() const;
	virtual	const char*		HelpString(bool isInUse) const;

protected:
			int32			options;
			int32			number_of_options;

			// this struct contains the tool's settings
			tool_settings	settings;

private:
			BBitmap*		fIcon;
			BString			fName;
			int32			fType;

			// The UseTool-function should set this region. Before starting the
			// UseTool-function should wait for this region to become empty
			BRect			fLastUpdatedRect;
};


class DrawingToolConfigView : public BBox {
public:
							DrawingToolConfigView(DrawingTool* newTool);
	virtual					~DrawingToolConfigView();

	virtual	void			AttachedToWindow();
	virtual	void			MessageReceived(BMessage* message);

			DrawingTool*	Tool() const { return tool; }

private:
			DrawingTool*	tool;
};

#endif	// DRAWING_TOOL_H
