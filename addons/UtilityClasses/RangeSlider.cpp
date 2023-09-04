/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Window.h>
#include <stdlib.h>

#include "RangeSlider.h"


RangeSlider::RangeSlider(BRect frame, const char* name, const char* label, BMessage* message,
	int32 minValue, int32 maxValue)
	:
	BSlider(frame, name, label, message, minValue, maxValue, B_TRIANGLE_THUMB)
{
	min_value = lower_value = minValue;
	max_value = higher_value = maxValue;

	lower_thumb_color.red = 200;
	lower_thumb_color.green = 100;
	lower_thumb_color.blue = 100;
	lower_thumb_color.alpha = 255;

	higher_thumb_color.red = 100;
	higher_thumb_color.green = 100;
	higher_thumb_color.blue = 200;
	higher_thumb_color.alpha = 255;
}


void
RangeSlider::DrawBar()
{
	BSlider::DrawBar();
	BRect rect = BarFrame();

	rect.top += 2;
	rect.left += 2;
	rect.right -= 1;
	rect.bottom -= 1;
	BView* view = OffscreenView();
	view->SetHighColor(BarColor());
	view->FillRect(rect);

	rgb_color fill_color;
	if (FillColor(&fill_color)) {
		view->SetHighColor(fill_color);
		rect.left = max_c(rect.left, LowerValueX());
		rect.right = min_c(rect.right, HigherValueX());
		view->FillRect(rect);
	}
}


void
RangeSlider::DrawThumb()
{
	BRect bar_frame = BarFrame();
	BView* view = OffscreenView();

	float bottom = bar_frame.bottom;

	int32 x;

	// First draw the lower slider
	x = LowerValueX();
	BPoint point1, point2, point3;
	point1.x = x - 3;
	point1.y = bottom + 4;

	point2.x = x;
	point2.y = bottom + 1;

	point3.x = x + 3;
	point3.y = bottom + 4;

	view->SetHighColor(lower_thumb_color);
	view->FillTriangle(point1, point2, point3);

	point1 = point1 + BPoint(-2, 1);
	point2 = point2 + BPoint(0, -1);
	point3 = point3 + BPoint(2, 1);

	view->SetHighColor(tint_color(lower_thumb_color, B_DARKEN_2_TINT));
	view->StrokeTriangle(point1, point2, point3);
	view->SetHighColor(tint_color(lower_thumb_color, B_LIGHTEN_2_TINT));
	view->StrokeLine(point1, point2);

	// Then draw the higher slider
	x = HigherValueX();
	point1.x = x - 3;
	point1.y = bottom + 4;

	point2.x = x;
	point2.y = bottom + 1;

	point3.x = x + 3;
	point3.y = bottom + 4;

	view->SetHighColor(higher_thumb_color);
	view->FillTriangle(point1, point2, point3);

	point1 = point1 + BPoint(-2, 1);
	point2 = point2 + BPoint(0, -1);
	point3 = point3 + BPoint(2, 1);

	view->SetHighColor(tint_color(higher_thumb_color, B_DARKEN_2_TINT));
	view->StrokeTriangle(point1, point2, point3);
	view->SetHighColor(tint_color(higher_thumb_color, B_LIGHTEN_2_TINT));
	view->StrokeLine(point1, point2);
}


void RangeSlider::MouseDown(BPoint)
{
	thread_id track_thread
		= spawn_thread(track_entry, "track_thread", B_NORMAL_PRIORITY, (void*)this);
	resume_thread(track_thread);
}


int32
RangeSlider::track_entry(void* p)
{
	return ((RangeSlider*)p)->track_mouse();
}


int32
RangeSlider::track_mouse()
{
	uint32 buttons;
	BPoint point;
	BWindow* window = Window();
	if (window == NULL)
		return B_ERROR;

	BHandler* target_handler = Target();
	if (target_handler == NULL)
		return B_ERROR;

	BLooper* target_looper = target_handler->Looper();

	if (target_looper == NULL)
		return B_ERROR;

	BMessenger messenger = BMessenger(target_handler, target_looper);


	// Here we should choose which side to track, max or min.
	// The selected side is based on which is nearer the point's value.
	window->Lock();
	GetMouse(&point, &buttons);
	int32 value = ValueForPoint(point);
	window->Unlock();

	if ((modifiers() & B_SHIFT_KEY) || (buttons & B_SECONDARY_MOUSE_BUTTON)) {
		// track both values
		int32 original_value = value;
		int32 original_low = lower_value;
//		int32 original_high = higher_value;
		int32 upper_limit = (max_value - min_value) - (higher_value - lower_value) + min_value;
		int32 lower_limit = min_value;
		int32 previous_value = value;
		int32 interval_length = higher_value - lower_value;
		while (buttons) {
			window->Lock();
			GetMouse(&point, &buttons);
			value = ValueForPoint(point);
			if (value != previous_value) {
				int32 new_low = original_low + value - original_value;
				int32 new_high;
				new_low = min_c(new_low, upper_limit);
				new_low = max_c(new_low, lower_limit);
				new_high = new_low + interval_length;
				if (ModificationMessage() != NULL)
					messenger.SendMessage(ModificationMessage());

				lower_value = new_low;
				SetHigherValue(new_high);

				previous_value = value;
			}
			window->Unlock();

			snooze(SnoozeAmount());
		}
	} else if ((value < LowerValue()) || (abs(value - LowerValue()) < abs(value - HigherValue()))) {
		// track the low value
		if (value != LowerValue()) {
			if (ModificationMessage() != NULL)
				messenger.SendMessage(ModificationMessage());

			window->Lock();
			SetLowerValue(value);
			window->Unlock();
		}
		while (buttons) {
			window->Lock();
			GetMouse(&point, &buttons);
			value = ValueForPoint(point);
			if (value != LowerValue()) {
				if (ModificationMessage() != NULL)
					messenger.SendMessage(ModificationMessage());

				SetLowerValue(value);
			}
			window->Unlock();

			snooze(SnoozeAmount());
		}
	} else {
		// track the high value
		if (value != HigherValue()) {
			if (ModificationMessage() != NULL)
				messenger.SendMessage(ModificationMessage());

			window->Lock();
			SetHigherValue(value);
			window->Unlock();
		}
		while (buttons) {
			window->Lock();
			GetMouse(&point, &buttons);
			value = ValueForPoint(point);
			if (value != HigherValue()) {
				if (ModificationMessage() != NULL)
					messenger.SendMessage(ModificationMessage());

				SetHigherValue(value);
			}
			window->Unlock();

			snooze(SnoozeAmount());
		}
	}

	if (Message() != NULL)
		messenger.SendMessage(Message());

	return B_OK;
}


void
RangeSlider::SetHigherValue(int32 new_value)
{
	new_value = max_c(new_value, LowerValue());
	new_value = min_c(new_value, max_value);

	if (new_value != higher_value) {
		higher_value = new_value;
		Draw(Bounds());
	}
}


void
RangeSlider::SetLowerValue(int32 new_value)
{
	new_value = max_c(new_value, min_value);
	new_value = min_c(new_value, HigherValue());

	if (new_value != lower_value) {
		lower_value = new_value;
		Draw(Bounds());
	}
}


void
RangeSlider::SetHigherThumbColor(rgb_color c)
{
	higher_thumb_color = c;
	Invalidate();
}


void
RangeSlider::SetLowerThumbColor(rgb_color c)
{
	lower_thumb_color = c;
	Invalidate();
}


int32
RangeSlider::HigherValueX()
{
	BRect rect = BarFrame();
	int32 left = int32(rect.left);
	int32 width = rect.IntegerWidth();
	return width / (max_value - min_value) * higher_value + left;
}


int32
RangeSlider::LowerValueX()
{
	BRect rect = BarFrame();
	int32 left = int32(rect.left);
	int32 width = rect.IntegerWidth();
	return width / (max_value - min_value) * lower_value + left;
}
