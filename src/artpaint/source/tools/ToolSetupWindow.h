/* 

	Filename:	ToolSetupWindow.h
	Contents:	Class declaration for ToolSetupWindow	
	Author:		Heikki Suhonen
	
*/

#ifndef TOOL_SETUP_WINDOW_H
#define TOOL_SETUP_WINDOW_H


#include <Window.h>
#include <MenuField.h>
#include <Box.h>


class DrawingTool;

class ToolSetupWindow : public BWindow {
	// Pointers to the tool-name menufield and tool menu.
	BMenuField *a_menufield;
//	BPopUpMenu *tool_name_menu;

// these are containers that form the three main groups of the window
//	BBox *menu_container;
	BBox *settings_container;

// these variables are used when resizing the window
	int32 widest_element_width;

// this points to the tool-object that is being adjusted
	DrawingTool *target_tool;
	int32		current_tool_type;
	
	
// only one instance of this class will be created, this is a pointer to it
static	ToolSetupWindow	*setup_window;

			
public:
		ToolSetupWindow(BRect frame, int32 tool_type);
		~ToolSetupWindow();

void	MessageReceived(BMessage *message);


// this changes the window to display views for new_type tool
// it is also used by the constructor to create the initial views
void	changeTool(int32 new_tool);	




// If settings for a tool are changed outside this window, we must
// call this function to reflect updates in this window if the changed
// tool is being displayed here.
static	void	updateTool(int32 updated_tool);

// this static function brings up the toolsetupwindow with proper tool
static	void	showWindow(int32 tool_type);

// this static function can be used to change the tool
static	void	changeToolForTheSetupWindow(int32);


static	void	setFeel(window_feel);
};


#endif