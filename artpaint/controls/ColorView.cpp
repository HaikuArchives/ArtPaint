/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <StringView.h>

#include "ColorPalette.h"
#include "ColorView.h"
#include "UtilityClasses.h"


ColorView::ColorView(BRect frame, char* label, BMessage* message, rgb_color initial_color)
	:
	BControl(frame, "color view", label, message, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
	label_view
		= new BStringView(BRect(16, 0, frame.Width(), frame.Height()), "color view label", label);
	font_height fHeight;

	label_view->GetFontHeight(&fHeight);
	font_leading = (frame.Height() - (fHeight.descent + fHeight.ascent)) / 2;

	AddChild(label_view);
	label_view->MoveTo((Bounds().bottom - font_leading) - (font_leading - 1) + 6, 0);
	current_color = initial_color;
}


void ColorView::Draw(BRect)
{
	rgb_color low = current_color;
	rgb_color high = low;

	float coeff = high.alpha / 255.0;
	low.red = (uint8)(coeff * low.red);
	low.green = (uint8)(coeff * low.green);
	low.blue = (uint8)(coeff * low.blue);
	low.alpha = 255;

	high.red = (uint8)(coeff * high.red + (1 - coeff) * 255);
	high.green = (uint8)(coeff * high.green + (1 - coeff) * 255);
	high.blue = (uint8)(coeff * high.blue + (1 - coeff) * 255);
	high.alpha = 255;

	SetHighColor(high);
	SetLowColor(low);

//	BRect filled_area = BRect(1, font_leading,
//		(Bounds().bottom - font_leading) - (font_leading - 1), Bounds().bottom - font_leading);
	BRect filled_area = BRect(1, 1, Bounds().Height() - 2, Bounds().Height() - 2);
	FillRect(filled_area, B_MIXED_COLORS);
	SetPenSize(1);

	SetHighColor(192, 192, 192, 255);
	StrokeLine(filled_area.LeftBottom(), filled_area.RightBottom());
	StrokeLine(filled_area.RightTop(), filled_area.RightBottom());

	SetHighColor(255, 255, 255, 255);
	StrokeLine(filled_area.LeftBottom(), filled_area.LeftTop());
	StrokeLine(filled_area.LeftTop(), filled_area.RightTop());
}


void
ColorView::MessageReceived(BMessage* message)
{
	rgb_color* new_color;
	ssize_t color_size = sizeof(rgb_color);
	switch (message->what) {
		case B_PASTE:
		{
			if (message->FindData(
				"RGBColor", B_RGB_COLOR_TYPE, (const void**)&new_color, &color_size) == B_OK) {
				current_color = *new_color;
				Invalidate();
				Message()->ReplaceInt32("value", RGBColorToBGRA(current_color));
				Invoke();
			}
		} break;
		default:
			BControl::MessageReceived(message);
	}
}


void
ColorView::MouseDown(BPoint point)
{
	bigtime_t click_speed;
	get_click_speed(&click_speed);
	bigtime_t start_time = real_time_clock_usecs();
	uint32 buttons;
	GetMouse(&point, &buttons);
	while ((buttons) && ((real_time_clock_usecs() - start_time) < click_speed)) {
		GetMouse(&point, &buttons);
		snooze(20 * 1000);
	}
	ColorPaletteWindow::showPaletteWindow();
	ColorPaletteWindow::ChangePaletteColor(current_color);
}


void
ColorView::ResizeToPreferred()
{
	font_height fHeight;
	label_view->GetFontHeight(&fHeight);
	float height = fHeight.ascent + fHeight.descent + fHeight.leading;
	label_view->ResizeTo(label_view->Bounds().Width(), height);
	height += 8;
	ResizeTo(Bounds().Width(), height);
	label_view->MoveTo(height + 4, 4);
	font_leading = (Frame().Height() - (fHeight.descent + fHeight.ascent)) / 2;
}


void
ColorView::SetColor(rgb_color c)
{
	current_color = c;
	Invalidate();
	Message()->ReplaceInt32("value", RGBColorToBGRA(current_color));
	Invoke();
}
