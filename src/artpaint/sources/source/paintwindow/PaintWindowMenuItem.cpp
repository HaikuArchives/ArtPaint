/* 

	Filename:	PaintWindowMenuItem.cpp
	Contents:	PaintWindowMenuItem-class definitions	
	Author:		Heikki Suhonen
	
*/

#include <string.h>

#include "PaintWindowMenuItem.h"
#include "PaintWindow.h"
#include "MessageConstants.h"

PaintWindowMenuItem::PaintWindowMenuItem(const char *label,BMessage *message,char shortcut,uint32 modifiers,PaintWindow *pw,const char *help)
	: BMenuItem(label,message,shortcut,modifiers)
{
	if (help != NULL) {
		help_message = new char[strlen(help)+1];
		strcpy(help_message,help);
	}
	else
		help_message = NULL;		

	paint_window = pw;
}


PaintWindowMenuItem::~PaintWindowMenuItem()
{
	delete help_message;
}

void PaintWindowMenuItem::Highlight(bool highlighted)
{
	BMenuItem::Highlight(highlighted);
	if ((highlighted == TRUE) && (help_message != NULL) && (paint_window != NULL)) {
		paint_window->Lock();
		paint_window->SetHelpString(help_message,HS_TEMPORARY_HELP_MESSAGE);
		paint_window->Unlock();
	}
	else if ((highlighted == FALSE) && (paint_window != NULL)) {
		paint_window->Lock();
		paint_window->SetHelpString("",HS_TEMPORARY_HELP_MESSAGE);
		paint_window->Unlock();		
	}
}