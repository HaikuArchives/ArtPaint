/* 

	Filename:	BackgroundView.cpp
	Contents:	BackgroundView constructor and Draw() function	
	Author:		Heikki Suhonen
	
*/

#include "BackgroundView.h"
#include "MessageConstants.h"
#include "ImageView.h"
#include "UtilityClasses.h"
#include "PaintWindow.h"
#include "Patterns.h"

BackgroundView::BackgroundView(BRect frame)
			: BView (frame,"background",B_FOLLOW_ALL_SIDES,B_WILL_DRAW|B_FRAME_EVENTS)
{
	// some user control for appearance of background should be allowed
	// this is the place to set it
	SetHighColor(200,200,200);
	SetLowColor(180,180,180);	
	SetViewColor(B_TRANSPARENT_32_BIT);
	SetDrawingMode(B_OP_COPY);
}			


void BackgroundView::Draw(BRect)
{	
	BRegion a_region;
	GetClippingRegion(&a_region);
	for (int32 i=0;i<a_region.CountRects();i++)
		FillRect(a_region.RectAt(i),HS_THIN_STRIPES);
}


void BackgroundView::FrameResized(float width,float height)
{
	// Here we should resize the image-view. It is better to do it here
	// than in window's similar function because this view's dimensions are
	// now known.
	
	BView *image = FindView("image_view");

	if (image != NULL) {
		Window()->DisableUpdates();
		((ImageView*)image)->adjustSize();
		((ImageView*)image)->adjustPosition();
		((ImageView*)image)->adjustScrollBars();	
		Window()->EnableUpdates();
	}
	
	// Width + 1 and Height + 1 are the real dimension of this view.
	((PaintWindow*)Window())->DisplayCoordinates(BPoint(width+1,height+1),BPoint(0,0),FALSE);
}


void BackgroundView::MouseDown(BPoint point)
{
	// We call image-view's MouseDown-function if the view exists.
	BView *image_view = FindView("image_view");
	if (image_view != NULL) {
		BPoint image_view_point = image_view->ConvertFromScreen(ConvertToScreen(point));
		image_view->MouseDown(image_view_point);
	}	
}