/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2008, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <karsten.heimrich@gmx.de>
 *
 */

#include "MagnificationView.h"

#include "Cursors.h"
#include "ImageView.h"
#include "MessageConstants.h"
#include "PopUpSlider.h"
#include "StringServer.h"


#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <MessageFilter.h>
#include <Slider.h>
#include <StringView.h>


#include <stdio.h>


PopUpSlider* gPopUpSlider = NULL;
filter_result MouseUpFilter(BMessage*, BHandler**, BMessageFilter*);
filter_result MouseDownFilter(BMessage*, BHandler**, BMessageFilter*);


// #pragma mark - MagStringView


class MagStringView : public BStringView {
public:
							MagStringView(BRect rect, const char* name,
								const char* label);
	virtual					~MagStringView();

	virtual void			AllAttached();
	virtual	void			MouseMoved(BPoint point, uint32 transit,
								const BMessage* message);
};


MagStringView::MagStringView(BRect rect, const char* name,
		const char* label)
	: BStringView(rect, name, label)
{
	AddFilter(new BMessageFilter(B_MOUSE_UP, MouseUpFilter));
	AddFilter(new BMessageFilter(B_MOUSE_DOWN, MouseDownFilter));
}


MagStringView::~MagStringView()
{
}


void
MagStringView::AllAttached()
{

}


void
MagStringView::MouseMoved(BPoint point, uint32 transit,
	const BMessage* message)
{
	if (transit == B_ENTERED_VIEW)
		be_app->SetCursor(HS_MINUS_PLUS_HAND_CURSOR);

	if (transit == B_EXITED_VIEW)
		be_app->SetCursor(B_HAND_CURSOR);
}


// #pragma mark - MagnificationView


MagnificationView::MagnificationView(BRect rect)
	: BBox(rect, "magnificationView", B_FOLLOW_TOP | B_FOLLOW_LEFT,
		B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER)
{
	char string[256];
	sprintf(string,"%s: %.1f%%", StringServer::ReturnString(MAG_STRING), 1600.0);

	float width, height;
	rect.OffsetTo(4.0, 1.0);
	fMagStringView = new MagStringView(rect, "magStringView", string);
	fMagStringView->GetPreferredSize(&width, &height);
	fMagStringView->ResizeTo(width, rect.Height() - 2.0);
	AddChild(fMagStringView);

	height = rect.Height() - 2.0;
	fMinusButton = new BButton(rect, "minusButton", "-",
		new BMessage(HS_ZOOM_IMAGE_OUT));
	AddChild(fMinusButton);
	fMinusButton->ResizeTo(height, height);

	fPlusButton = new BButton(rect, "plusButton", "+",
		new BMessage(HS_ZOOM_IMAGE_IN));
	AddChild(fPlusButton);
	fPlusButton->ResizeTo(height, height);

	fMinusButton->MoveBy(width + 5.0, 0.0);
	fPlusButton->MoveBy(width + 5.0 + height, 0.0);

	ResizeTo(fPlusButton->Frame().right + 1.0, Bounds().Height());
}


void
MagnificationView::AttachedToWindow()
{
	BBox::AttachedToWindow();

	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
MagnificationView::Draw(BRect updateRect)
{
	BBox::Draw(updateRect);
}


void
MagnificationView::SetMagnificationLevel(float magLevel)
{
	char string[256];
	sprintf(string, "%s: %.1f%%", StringServer::ReturnString(MAG_STRING),
		100.0 * magLevel);
	fMagStringView->SetText(string);
}


void
MagnificationView::SetTarget(const BMessenger& target)
{
	fPlusButton->SetTarget(target);
	fMinusButton->SetTarget(target);
}


// #pragma mark - MouseUpFilter, MouseDownFilter


filter_result
MouseUpFilter(BMessage* message, BHandler** handlers, BMessageFilter* filter)
{
	MagStringView* magStringView = dynamic_cast<MagStringView*> (handlers[0]);
	if (magStringView)
		magStringView->SetEventMask(magStringView->EventMask() &~ B_POINTER_EVENTS);

	if (gPopUpSlider) {
		gPopUpSlider->PostMessage(B_QUIT_REQUESTED);
		gPopUpSlider = NULL;
		return B_SKIP_MESSAGE;
	}

	return B_DISPATCH_MESSAGE;
}


filter_result
MouseDownFilter(BMessage* message, BHandler** handlers, BMessageFilter* filter)
{
	BWindow* window = dynamic_cast<BWindow*> (filter->Looper());
	if (window == NULL)
		return B_DISPATCH_MESSAGE;

	MagStringView* magStringView = dynamic_cast<MagStringView*> (handlers[0]);
	if (magStringView == NULL)
		return B_DISPATCH_MESSAGE;

	BPoint point;
	message->FindPoint("be:view_where", &point);
	BRect bounds(magStringView->Bounds());
	if (!bounds.Contains(point))
		return B_DISPATCH_MESSAGE;

	ImageView* imageView = dynamic_cast<ImageView*>(window->FindView("image_view"));
	if (imageView == NULL)
		return B_DISPATCH_MESSAGE;

	magStringView->SetEventMask(B_POINTER_EVENTS);

	float value = (sqrt(imageView->getMagScale()) * 16.0 / 15.9 - 0.1) * 100.0;

	gPopUpSlider = PopUpSlider::Instantiate(BMessenger(imageView, window),
		new BMessage(HS_SET_MAGNIFYING_SCALE), 10, 1600);

	BSlider* slider = gPopUpSlider->Slider();
	slider->SetValue(int32(value));
	slider->SetModificationMessage(new BMessage(HS_SET_MAGNIFYING_SCALE));

	BRect rect(slider->BarFrame());
	// this code depends heavily on BSlider implementation
	float offset = ceil((rect.left + 1.0) + (value - 10.0) / (1600 - 10) *
		(rect.right - 1.0) - (rect.left + 1.0));
	offset = ceil(offset * 3.885714286) + 8.0;

	rect = gPopUpSlider->Bounds();
	bounds = magStringView->ConvertToScreen(bounds);

	message->FindPoint("screen_where", &point);
	gPopUpSlider->MoveTo(point.x - offset, bounds.top - rect.Height());
	gPopUpSlider->Go();

	return B_SKIP_MESSAGE;
}
