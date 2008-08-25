/* 

	Filename:	PaintApplication.h
	Contents:	PaintApplication class declaration
	Author:	Heikki Suhonen
	
*/

#ifndef PAINT_APPLICATION_H
#define PAINT_APPLICATION_H

#include <Application.h>
#include <MessageFilter.h>

class ColorPaletteWindow;
class ColorSet;
class DrawingTool;

struct global_settings;

//// This defines how many different drawing tools we have, it will be only used
//// in the apps functions.
//#define	HS_NUMBER_OF_TOOLS	14

// These are help-messages that might be displayed in the help-view of a paint-window.
//#define	HS_DRAW_MODE_HELP_MESSAGE	"Use the tools to paint or select an action from the menus."

// PaintApplication class declaration
class PaintApplication : public BApplication {
private:		
		// these variables hold the various open-panels
		BFilePanel		*image_open_panel;
		BFilePanel		*project_open_panel;				

//		// this variable holds the pop-up-menu that displays
//		// tools to the user
//		BPopUpMenu		*tool_pop_up_menu;		

		global_settings	*settings;
		
		// this tells if we can use the datatypes
		bool			datatypes_available;	

//		// this array holds pointers to tool-instances
//		DrawingTool	 	*drawing_tools[HS_NUMBER_OF_TOOLS];
		
//		// this variable holds the info on how many untitled windows we have created
//		// in this session
//		int32			untitled_window_number;
							
// this funtion show the color palette window whether it is hidden, minimized
// or closed
//void		showColorWindow();

// these functions set up some pop-up menus		
//void		setupToolMenu();

//// this function creates the drawing tool instances and places them to the array
//void		createDrawingTools();
//void		createDrawingTool(int32 index, int32 type);

//// this function creates a paint-window
//void		createPaintWindow(BBitmap *a_bitmap=NULL,char *file_name=NULL,int32 type=0, entry_ref &ref=entry_ref());

// these functions read and write the preferences-file
void		readPreferences();
void		writePreferences();


// This function reads a project-file and creates new window for it
status_t	readProject(BFile&,entry_ref&);
status_t	readProjectOldStyle(BFile&,entry_ref&);
// This function reads all the add-ons from add-ons -directory and all it's
// subdirectories. It places the in the add_on_array;
status_t	readAddOns();


public:
		PaintApplication();
		~PaintApplication();
		void				MessageReceived(BMessage *message);
		bool				QuitRequested();
		void				ReadyToRun();
		void				RefsReceived(BMessage *message);

// These functions get and set color for particular button. The ability to have different colors
// for each button is removed and thus only foreground and background-color can be defined.
		bool				SetColor(rgb_color color,bool foreground);
		rgb_color			GetColor(bool foreground);

// this function returns pointer to tool pop-up-menu
//		BPopUpMenu* 		GetToolMenu() { return tool_pop_up_menu; };

// this will put the home-directory of the application to path parameter
static	void				HomeDirectory(BPath &path);

// this function returns a pointer to settings struct
		global_settings*	Settings() { return settings; }	
};



filter_result AppKeyFilterFunction(BMessage *message,BHandler **handler,BMessageFilter*);

#endif
