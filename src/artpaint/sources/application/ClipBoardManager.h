/* 

	Filename:	ClipBoardManager.h
	Contents:	ClipBoardManager-class declarations.	
	Author:		Heikki Suhonen
	
*/




#ifndef CLIP_BOARD_MANAGER_H
#define	CLIP_BOARD_MANAGER_H

/*
	ClipBoardManager-class keeps track of the stuff that is on the
	clipboard. It stores the bitmaps on disk and loads them, it supplies
	the thumnail-images for each clipping. The manager is a looper that
	responds to messages.
*/

#include <Path.h>
#include <SupportDefs.h>

class Clip;

class ClipBoardManager {
		
public:
		ClipBoardManager();
		~ClipBoardManager();
		
static	status_t	AddClip(BBitmap*);

};




class Clip {
		BPath	clip_path;
		BBitmap	*thumbnail;
		BBitmap	*real_image;
public:
			Clip(BBitmap*);
			~Clip();
		
status_t	StoreBitmap(BPath);
BBitmap*	ReturnBitmap();					
BBitmap*	ReturnThumbnail();		
};
#endif 