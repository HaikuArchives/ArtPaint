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


class BView;
class DrawingTool;


class ToolSetupWindow : public BWindow {
public:
	static	void				ShowToolSetupWindow(int32 tool);
	static	void				SetWindowFeel(window_feel feel);
	static	void				CurrentToolChanged(int32 newTool);

private:
								ToolSetupWindow(BRect frame);
	virtual						~ToolSetupWindow();

			void				_UpdateConfigurationView(int32 newTool);

private:
			int32				fCurrentTool;

			BView*				fContainer;
			DrawingTool*		fDrawingTool;


	static	ToolSetupWindow*	fToolSetupWindow;
};


#endif
