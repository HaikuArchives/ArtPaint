/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2008, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <karsten.heimrich@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "MagnificationView.h"

#include "Cursors.h"
#include "ImageView.h"
#include "MessageConstants.h"
#include "PopUpSlider.h"


#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MessageFilter.h>
#include <Slider.h>
#include <StringView.h>


#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MagnificationView"


PopUpSlider* gPopUpSlider = NULL;
filter_result MouseUpFilter(BMessage*, BHandler**, BMessageFilter*);
filter_result MouseDownFilter(BMessage*, BHandler**, BMessageFilter*);


// #pragma mark - MagStringView


class MagStringView : public BStringView {
public:
							MagStringView(const char* name,
								const char* label);
	virtual					~MagStringView();

	virtual	void			MouseMoved(BPoint point, uint32 transit,
								const BMessage* message);
};


MagStringView::MagStringView(const char* name,
		const char* label)
	: BStringView(name, label)
{
	AddFilter(new BMessageFilter(B_MOUSE_UP, MouseUpFilter));
	AddFilter(new BMessageFilter(B_MOUSE_DOWN, MouseDownFilter));
}


MagStringView::~MagStringView()
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


MagnificationView::MagnificationView()
	: BBox("magnificationView", B_WILL_DRAW | B_FRAME_EVENTS | B_SUPPORTS_LAYOUT,
	B_PLAIN_BORDER)
{
	BFont font;

	char string[256];
	sprintf(string,"%s: %.0f%%", B_TRANSLATE("Zoom"), 1600.0);

	fMagStringView = new MagStringView("magStringView", string);

	fMinusButton = new BButton("minusButton", "-",
		new BMessage(HS_ZOOM_IMAGE_OUT));

	fPlusButton = new BButton("plusButton", "+",
		new BMessage(HS_ZOOM_IMAGE_IN));

	float button_size = font.StringWidth("XXX");
	fMinusButton->SetExplicitMaxSize(BSize(button_size, button_size));
	fMinusButton->SetExplicitMinSize(BSize(button_size, button_size));

	fPlusButton->SetExplicitMaxSize(BSize(button_size, button_size));
	fPlusButton->SetExplicitMinSize(BSize(button_size, button_size));

	BGridLayout* mainLayout = BLayoutBuilder::Grid<>(this, 5.0, 5.0)
		.Add(fMagStringView, 0, 0)
		.Add(fMinusButton, 1, 0)
		.Add(fPlusButton, 2, 0)
		.SetInsets(3.0, 2.0, 3.0, 2.0);
	mainLayout->SetMinColumnWidth(0, StringWidth(string));
	mainLayout->SetMaxColumnWidth(1, 25);
	mainLayout->SetMaxColumnWidth(2, 25);
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
	sprintf(string, "%s: %.0f%%", B_TRANSLATE("Zoom"),
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

	gPopUpSlider = PopUpSlider::Instantiate(BMessenger(imageView, window),
		new BMessage(HS_SET_MAGNIFYING_SCALE), 10, 1600);

	// Convert nonlinear from [0.1,16] -> [10,1600]
	float value = 1590.0 * log10((9.0 * (imageView->getMagScale() - 0.1) / 15.9)
		+ 1.0) + 10.0;

	BSlider* slider = gPopUpSlider->Slider();
	slider->SetValue(int32(value));
	slider->SetModificationMessage(new BMessage(HS_SET_MAGNIFYING_SCALE));

	BRect rect(slider->BarFrame());
	float offset = (slider->Position() * (rect.Width() - 1.0)) + 8.0;

	rect = gPopUpSlider->Bounds();
	bounds = magStringView->ConvertToScreen(bounds);

	message->FindPoint("screen_where", &point);
	gPopUpSlider->MoveTo(point.x - offset, bounds.top - rect.Height());
	gPopUpSlider->Go();

	return B_SKIP_MESSAGE;
}
