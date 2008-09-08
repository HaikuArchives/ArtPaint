/*

	Filename:	ToolButton.h
	Contents:	ToolButton-class declaration.
	Author:		Heikki Suhonen

*/


#ifndef TOOL_BUTTON_H
#define TOOL_BUTTON_H

#include <PictureButton.h>
#include <Window.h>

// This class keeps track of all its instances.
class ToolButton : public BPictureButton {
static	BList		*tool_button_list;
		int32		tool_type;

const	char			*tool_name;

static	int32		active_tool;
static	ToolButton	*active_button;

		BWindow		*help_window;
		BPoint		opening_point;

public:
		ToolButton(BRect frame,int32 tool,const char *name);
		~ToolButton();

		void	MouseDown(BPoint);
		void	MouseMoved(BPoint,uint32,const BMessage*);
		void	Pulse();

static	void	ChangeActiveButton(int32);
};

#endif
