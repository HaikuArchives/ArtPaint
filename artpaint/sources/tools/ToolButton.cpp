/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "ToolButton.h"

#include "Tools.h"


#include <Bitmap.h>
#include <ControlLook.h>
#include <LayoutUtils.h>
#include <Window.h>


const float gInset = 2.0;


ToolButton::ToolButton(const char* name, BMessage* message, BBitmap* icon)
	: BControl(name, "?", message, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS)
	, fInside(false)
	, fMouseButton(0)
	, fIcon(icon)
{
	SetToolTip(name);
}


ToolButton::~ToolButton()
{
	delete fIcon;
}


void
ToolButton::SetValue(int32 value)
{
	if (value != Value()) {
		BControl::SetValueNoUpdate(value);
		Invalidate(Bounds());
	}

	if (!value)
		return;

	BView* child = NULL;
	BView* parent = Parent();
	if (parent)
		child = parent->ChildAt(0);
	else if (Window())
		child = Window()->ChildAt(0);

	while (child) {
		ToolButton* button = dynamic_cast<ToolButton*> (child);
		if (button && (button != this))
			button->SetValue(B_CONTROL_OFF);
		child = child->NextSibling();
	}
}


void
ToolButton::AttachedToWindow()
{
	BControl::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
}


void
ToolButton::Draw(BRect updateRect)
{
	if (be_control_look) {
		rgb_color base = LowColor();
		rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
		if (Parent())
			background = Parent()->ViewColor();

		uint32 flags = be_control_look->Flags(this);
		if (fInside)
			flags |= BControlLook::B_ACTIVATED;

		BRect rect = Bounds();
		be_control_look->DrawButtonFrame(this, rect, updateRect,
			base, background, flags);
		be_control_look->DrawButtonBackground(this, rect, updateRect,
			base, flags);

		if (!fIcon) {
			if (be_control_look) {
				SetFont(be_bold_font);
				SetFontSize(LARGE_TOOL_ICON_SIZE);

				rect.InsetBy(gInset, gInset);
				be_control_look->DrawLabel(this, Label(), rect, updateRect,
					base, flags, BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));
			}
		} else {
			SetDrawingMode(B_OP_ALPHA);
			DrawBitmap(fIcon, BPoint(gInset, gInset));
		}
	} else {
		SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		FillRect(Bounds());

		SetHighColor(0, 0, 0, 255);
		StrokeRect(Bounds());

		if (fIcon) {
			SetDrawingMode(B_OP_ALPHA);
			DrawBitmap(fIcon, BPoint(gInset, gInset));
		}
	}
}


void
ToolButton::MouseUp(BPoint point)
{
	if (!IsTracking())
		return;

	if (Bounds().Contains(point)) {
		SetValue(B_CONTROL_ON);
		Message()->ReplaceUInt32("buttons", fMouseButton);
		Invoke();
	}

	fInside = false;
	Invalidate();
	SetTracking(false);
}


void
ToolButton::MouseDown(BPoint point)
{
	if (!IsEnabled())
		return;

	fInside = true;
	GetMouse(&point, &fMouseButton);

	Invalidate();
	SetTracking(true);
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}


void
ToolButton::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if (!IsTracking())
		return;

	bool inside = Bounds().Contains(point);
	if (fInside != inside) {
		fInside = inside;
		Invalidate();
	}
}


void
ToolButton::KeyDown(const char* bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_SPACE: {
		case B_RETURN:
			if (IsEnabled() && !Value()) {
				SetValue(B_CONTROL_ON);
				Invoke();
			}
		}	break;

		case B_UP_ARROW: {
		case B_DOWN_ARROW:
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
			_SelectNextToolButton(bytes[0]);
		}	break;

		default: {
			BControl::KeyDown(bytes, numBytes);
		}	break;
	}
}


BSize
ToolButton::MaxSize()
{
	float width, height;
	GetPreferredSize(&width, &height);

	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), BSize(width, height));
}


void
ToolButton::GetPreferredSize(float* width, float* height)
{
	if (!width || !height)
		return;

	*width = *height = LARGE_TOOL_ICON_SIZE + (2.0 * gInset);

	if (fIcon) {
		*width = fIcon->Bounds().Width() + (2.0 * gInset) + 1.0;
		*height = fIcon->Bounds().Height() + (2.0 * gInset) + 1.0;
	}
}


void
ToolButton::_SelectNextToolButton(uchar key)
{
	if (BView* parent = Parent()) {
		BRect frame = Frame();
		float difference = 10000;
		ToolButton* nextButton = NULL;
		for (int32  i = 0; i < parent->CountChildren(); ++i) {
			ToolButton* button = dynamic_cast<ToolButton*> (parent->ChildAt(i));
			if (button && button != this) {
				float diff = 0.0;
				float offset = 0.0;
				switch (key) {
					case B_UP_ARROW: {
						BRect nextFrame = button->Frame();
						diff = frame.top - nextFrame.bottom;
						offset = nextFrame.left - frame.left;
					}	break;

					case B_DOWN_ARROW: {
						BRect nextFrame = button->Frame();
						diff = nextFrame.top - frame.bottom;
						offset = nextFrame.left - frame.left;
					}	break;

					case B_LEFT_ARROW: {
						BRect nextFrame = button->Frame();
						offset = nextFrame.top - frame.top;
						diff = frame.left - nextFrame.right;
					}	break;

					case B_RIGHT_ARROW: {
						BRect nextFrame = button->Frame();
						offset = nextFrame.top - frame.top;
						diff = nextFrame.left - frame.right;
					}	break;
				}

				if (offset > -5.0 && offset < 5.0) {
					if ((difference > diff) && (diff > 0.0)) {
						difference = diff;
						nextButton = button;
					}
				}
			}
		}

		if (nextButton)
			nextButton->MakeFocus(true);
	}
}
