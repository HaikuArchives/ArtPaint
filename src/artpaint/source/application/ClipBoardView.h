/* 

	Filename:	ClipBoardView.h
	Contents:	ClipBoardView-class declarations.	
	Author:		Heikki Suhonen
	
*/




#ifndef CLIP_BOARD_VIEW_H
#define	CLIP_BOARD_VIEW_H

/*
	ClipBoardView-class displays thumnails of clips. A clip can be
	placed on the clipboard by cutting or copying from some image.
	The actual clipping is stored on disk. The thumbnils can be dragged
	on to some image and pasted there into an existing layer or as a new layer.
	When a clipping is dragged onto an image some kind of preview should
	be shown also.
*/

#include <View.h>

class ClipBoardView : public BView {
		
public:
		ClipBoardView();
		~ClipBoardView();
		
void	Draw(BRect);		
void	MessageReceived(BMessage*);
void	MouseDown(BPoint);
};




#endif