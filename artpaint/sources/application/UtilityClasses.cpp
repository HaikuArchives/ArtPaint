/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "UtilityClasses.h"

#include "StringServer.h"


#include <Bitmap.h>
#include <Screen.h>
#include <StringView.h>


#include <new>
#include <string.h>


BitmapView::BitmapView(BBitmap* bitmap, BRect frame)
	: BView(frame, "bitmap view", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
	, fBitmap(bitmap)
{
}


BitmapView::BitmapView(BBitmap* bitmap, BPoint leftTop)
	: BView(BRect(leftTop, leftTop), "bitmap view", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW)
	, fBitmap(bitmap)
{
	if (fBitmap)
		ResizeTo(fBitmap->Bounds().Width(), fBitmap->Bounds().Height());
}


BitmapView::~BitmapView()
{
	delete fBitmap;
}


void
BitmapView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
BitmapView::Draw(BRect updateRect)
{
	if (fBitmap)
		DrawBitmap(fBitmap, updateRect, updateRect);
}


BBitmap*
BitmapView::Bitmap() const
{
	return fBitmap;
}


void
BitmapView::SetBitmap(BBitmap* bitmap)
{
	delete fBitmap;
	fBitmap = bitmap;
}



BRect FitRectToScreen(BRect source)
{
	// This function returns the source moved and resized so that it fits onto
	// current screen.
	BRect result = source;
	BRect screenFrame = BScreen().Frame();

	if (!screenFrame.Contains(result.RightBottom()))
		result.OffsetTo(BPoint(15.0, 15.0));

	if (!screenFrame.Contains(result.RightBottom())) {
		result.bottom = min_c(screenFrame.bottom, result.bottom);
		result.right = min_c(screenFrame.right, result.right);
	}

	return result;
}


BRect
CenterRectOnScreen(BRect source)
{
	BRect screenFrame = BScreen().Frame();

	BPoint leftTop((screenFrame.Width() + source.Width()) / 2.0,
		(screenFrame.Height() + source.Height()) / 2.0);

	if (leftTop.x < 0.0) leftTop.x = 0.0;
	if (leftTop.y < 0.0) leftTop.y = 0.0;

	return source.OffsetToCopy(leftTop);
}


BRect make_rect_from_points(BPoint &point1,BPoint &point2)
{
	BRect rect;
	rect.left = min_c(point1.x,point2.x);
	rect.right = max_c(point1.x,point2.x);
	rect.top = min_c(point1.y,point2.y);
	rect.bottom = max_c(point1.y,point2.y);

	return rect;
}
