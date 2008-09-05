/* 

	Filename:	HSPictureButton.h
	Contents:	HSPictureButton-class declaration	
	Author:		Heikki Suhonen
	
*/

#ifndef HS_PICTURE_BUTTON_H
#define HS_PICTURE_BUTTON_H

#include <PictureButton.h>

class HelpWindow;
// this is a class that defines a PictureButton
// that also records the mouse-button that was used
// and also displays help message if mouse is on top of it and still
// for long enough
class HSPictureButton : public BPictureButton {
	char		*help_message;
	char		*long_help_message;

	HelpWindow	*help_window;
	BPoint		opening_point;
		
public:
	HSPictureButton(BRect,BPicture*,BPicture*,BMessage*,const char *help_msg=NULL,const char *longer_help=NULL,uint32 behavior = B_ONE_STATE_BUTTON,uint32 resizingMode = B_FOLLOW_LEFT|B_FOLLOW_TOP,uint32 flags = B_WILL_DRAW|B_NAVIGABLE);
	HSPictureButton(BRect,BBitmap*,BBitmap*,BMessage*,const char *help_msg=NULL,const char *longer_help=NULL,uint32 behavior = B_ONE_STATE_BUTTON,uint32 resizingMode = B_FOLLOW_LEFT|B_FOLLOW_TOP,uint32 flags = B_WILL_DRAW|B_NAVIGABLE);

	~HSPictureButton();
		
void	MouseDown(BPoint location);	
void	MouseMoved(BPoint point, uint32 transit,const BMessage*);

void	Pulse();
};

#endif