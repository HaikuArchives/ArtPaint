/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef TOOL_SELECTION_WINDOW_H
#define	TOOL_SELECTION_WINDOW_H

#include <Window.h>


class DrawingTool;
class MatrixView;


class ToolSelectionWindow : public BWindow {
public:
	virtual	void					FrameResized(float width, float height);
	virtual	void					MessageReceived(BMessage* message);

	static	void					showWindow();
	static	void					setFeel(window_feel feel);

	static	void					ChangeTool(int32 tool);

private:
									ToolSelectionWindow(BRect frame);
	virtual							~ToolSelectionWindow();

			void					_AddTool(const DrawingTool* tool);

private:
			MatrixView*				fMatrixView;
	static	ToolSelectionWindow*	fSelectionWindow;
};


#endif // TOOL_SELECTION_WINDOW_H
