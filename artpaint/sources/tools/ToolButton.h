/*
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef TOOL_BUTTON_H
#define TOOL_BUTTON_H

#include <Control.h>
#include <String.h>


class ToolButton : public BControl {
public:
								ToolButton(const char* name, BMessage* message,
									BBitmap* icon = NULL);
								~ToolButton();

	virtual	void				SetValue(int32 value);

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseUp(BPoint point);
	virtual	void				MouseDown(BPoint point);
	virtual	void				MouseMoved(BPoint point, uint32 transit,
									const BMessage* message);

	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	virtual	BSize				MaxSize();
	virtual	void				GetPreferredSize(float* width, float* height);

private:
			void				_SelectNextToolButton(uchar key);

private:
			BString				fName;
			bool				fInside;
			uint32				fMouseButton;

			BBitmap*			fIcon;
};

#endif
