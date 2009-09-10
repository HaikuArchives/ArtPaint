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
#ifndef TOOL_SETUP_WINDOW_H
#define TOOL_SETUP_WINDOW_H

#include <Window.h>


class DrawingTool;


class ToolSetupWindow : public BWindow {
public:
	static	void				ShowToolSetupWindow(int32 tool);
	static	void				SetWindowFeel(window_feel feel);
	static	void				CurrentToolChanged(int32 tool);

private:
								ToolSetupWindow(BRect frame);
	virtual						~ToolSetupWindow();

			void				_UpdateConfigurationView(int32 tool);

private:
			int32				fCurrentTool;

			DrawingTool*		fDrawingTool;
	static	ToolSetupWindow*	sfToolSetupWindow;
};

#endif	// TOOL_SETUP_WINDOW_H
