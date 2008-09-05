/* 

	Filename:	PreviewView.h
	Contents:	Declaration for a class that shows a preview of an effect as a bitmap.	
	Author:		Heikki Suhonen
	
*/



#ifndef PREVIEW_VIEW_H
#define	PREVIEW_VIEW_H

#include <View.h>

class PreviewView : public BView {
		BBitmap	*preview_bitmap;
		BPoint	bitmap_offset;
public:
		PreviewView(BRect frame,int32 preview_width,int32 preview_height);
virtual	~PreviewView();
		
void	Draw(BRect);
		
void		RedisplayBitmap();
BBitmap*	ReturnBitmap();
};


#endif