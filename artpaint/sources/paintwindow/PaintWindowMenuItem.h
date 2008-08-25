/* 

	Filename:	PaintWindowMenuItem.h
	Contents:	PaintWindowMenuItem-class declaration	
	Author:		Heikki Suhonen
	
*/


#ifndef PAINT_WINDOW_MENU_ITEM_H
#define PAINT_WINDOW_MENU_ITEM_H

#include <MenuItem.h>

class PaintWindow;

class PaintWindowMenuItem : public BMenuItem {
		char		*help_message;
		PaintWindow	*paint_window;

public:
		PaintWindowMenuItem(const char*,BMessage*,char =NULL,uint32 =NULL,PaintWindow* =NULL,const char* =NULL);
		~PaintWindowMenuItem();
	
void	Highlight(bool);
};

#endif