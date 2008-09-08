/*

	Filename:	ToolSelectionWindow.h
	Contents:	Class declaration for ToolSelectionWindow
	Author:		Heikki Suhonen

*/
#ifndef TOOL_SELECTION_WINDOW_H
#define	TOOL_SELECTION_WINDOW_H


#include <Window.h>


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

private:
	static	ToolSelectionWindow*	fSelectionWindow;
};


#endif
