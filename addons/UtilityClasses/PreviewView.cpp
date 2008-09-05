/* 

	Filename:	PreviewView.cpp
	Contents:	Definitions for a PreviewView-class.	
	Author:		Heikki Suhonen
	
*/
#include <Bitmap.h>

#include "PreviewView.h"


PreviewView::PreviewView(BRect frame,int32 preview_width,int32 preview_height)
	:	BView(frame,"preview_view",B_FOLLOW_LEFT|B_FOLLOW_TOP,B_WILL_DRAW)
{
	bitmap_offset.x = (frame.IntegerWidth() - preview_width) / 2;
	bitmap_offset.y = (frame.IntegerHeight() - preview_height) / 2;

	preview_bitmap = new BBitmap(BRect(0,0,preview_width-1,preview_height-1),B_RGB32,true);
	BRect rc = preview_bitmap->Bounds();
	BView *bitmap_view = new BView(rc,"bitmap_view",B_FOLLOW_NONE,B_WILL_DRAW|B_SUBPIXEL_PRECISE);
	preview_bitmap->Lock();
	preview_bitmap->AddChild(bitmap_view);
	preview_bitmap->Unlock();
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


PreviewView::~PreviewView()
{
	delete preview_bitmap;
}


void PreviewView::Draw(BRect rect)
{
	BView::Draw(rect);
	
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),B_DARKEN_2_TINT));
	SetPenSize(1.0);
	BRect bound = Bounds();
	StrokeRect(bound);
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),B_LIGHTEN_2_TINT));
	StrokeLine(bound.LeftBottom(),bound.RightBottom());
	StrokeLine(bound.RightBottom(),bound.RightTop());		

	RedisplayBitmap();
}


void PreviewView::RedisplayBitmap()
{
	if (LockLooper() == true) {
		DrawBitmap(preview_bitmap,bitmap_offset);
		Sync();
		UnlockLooper();
	}	
}


BBitmap* PreviewView::ReturnBitmap()
{
	return preview_bitmap;
}	