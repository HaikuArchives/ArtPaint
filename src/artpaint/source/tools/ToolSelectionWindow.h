/* 

	Filename:	ToolSelectionWindow.h
	Contents:	Class declaration for ToolSelectionWindow	
	Author:		Heikki Suhonen
	
*/

#ifndef TOOL_SELECTION_WINDOW_H
#define	TOOL_SELECTION_WINDOW_H

#include <Window.h>

class ToolSelectionWindow : public BWindow {
static	ToolSelectionWindow	*selection_window;

			ToolSelectionWindow(BRect frame);
virtual		~ToolSelectionWindow();
public:
	
		void	FrameResized(float,float);
		void	MessageReceived(BMessage *message);
		

static	void	showWindow();	
static	void	setFeel(window_feel);

static	void	ChangeTool(int32);
};

#endif