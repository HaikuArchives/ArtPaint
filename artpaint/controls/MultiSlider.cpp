/*
 * Copyright 2026, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "MultiSlider.h"

#include <ControlLook.h>
#include <Message.h>
#include <Window.h>

#include "BitmapUtilities.h"
#include "PixelOperations.h"
#include "SettingsServer.h"
#include "UtilityClasses.h"

#include <stdio.h>

#define RES 128


MultiSlider::MultiSlider(BRect frame, const char* name, const char* label, BMessage* message,
	int32 minValue, int32 maxValue, thumb_style thumbType, uint32 resizingMode, uint32 flags)
	:
	BSlider(frame, name, label, message, minValue, maxValue, thumbType, resizingMode, flags)
{
	Init();
}


MultiSlider::MultiSlider(BRect frame, const char* name, const char* label, BMessage* message,
	int32 minValue, int32 maxValue, orientation posture, thumb_style thumbType, uint32 resizingMode,
	uint32 flags)
	:
	BSlider(
		frame, name, label, message, minValue, maxValue, posture, thumbType, resizingMode, flags)
{
	Init();
}


MultiSlider::MultiSlider(const char* name, const char* label, BMessage* message, int32 minValue,
	int32 maxValue, orientation posture, thumb_style thumbType, uint32 flags)
	:
	BSlider(name, label, message, minValue, maxValue, posture, thumbType, flags)
{
	Init();
}


MultiSlider::MultiSlider(BMessage* archive)
	:
	BSlider(archive)
{
	//Init();
}


MultiSlider::~MultiSlider()
{

}

void
MultiSlider::AttachedToWindow()
{
	BSlider::AttachedToWindow();

	AddThumb(0.);
}


void
MultiSlider::Init()
{
	fSelectedThumb = -1;
}


void
MultiSlider::DrawBar()
{
	BSlider::DrawBar();
}


void
MultiSlider::DrawThumb(int32 thumb)
{
	if (Style() == B_BLOCK_THUMB)
		_DrawBlockThumb(thumb);
	else
		_DrawTriangleThumb(thumb);
}


void
MultiSlider::_DrawBlockThumb(int32 thumb)
{
	BView* view = OffscreenView();

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 flags = be_control_look->Flags(this);

	BRect frame = ThumbFrame(thumb);

	be_control_look->DrawSliderThumb(view, frame, frame, base, flags,
		Orientation());
}


void
MultiSlider::_DrawTriangleThumb(int32 thumb)
{
	BView* view = OffscreenView();

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 flags = be_control_look->Flags(this);

	BRect frame = ThumbFrame(thumb);

	be_control_look->DrawSliderTriangle(view, frame, frame, base, flags,
		Orientation());
}


BObjectList<float>
MultiSlider::Positions() const
{
	return fPositions;
}


float
MultiSlider::Position(int32 thumb) const
{
	int32 minValue, maxValue;
	GetLimits(&minValue, &maxValue);

	float range = (float)(maxValue - minValue);
	if (range == 0.0f)
		range = 1.0f;

	return (float)(*(fPositions.ItemAt(thumb)) - minValue) / range;
}


void
MultiSlider::SetPosition(int32 thumb, float position)
{
	int32 min, max;

	GetLimits(&min, &max);

	if (position <= 0.0f)
		SetValue(thumb, min);
	else if (position >= 1.0f)
		SetValue(thumb, max);
	else
		SetValue(thumb, (int32)(position * (max - min) + min));
}


void
MultiSlider::SetValue(int32 thumb, int32 value)
{
	int32 min, max;
	GetLimits (&min, &max);

	if (value < min)
		value = min;

	if (value > max)
		value = max;

	if (value == Value(thumb))
		return;

	_SetLocationForValue(thumb, value);

	BRect oldThumbFrame = ThumbFrame(thumb);
	*(fPositions.ItemAt(thumb)) = value;

	if (Orientation() == B_HORIZONTAL)
		oldThumbFrame.top = BarFrame().top;
	else
		oldThumbFrame.left = BarFrame().left;

	BRect invalid = oldThumbFrame | ThumbFrame(thumb);

	if (Style() == B_TRIANGLE_THUMB) {
		if (Orientation() == B_HORIZONTAL) {
			if (IsFocus())
				invalid.bottom += 2;
			invalid.InsetBy(-1, 0);
		} else {
			if (IsFocus())
				invalid.left -= 2;
			invalid.InsetBy(0, -1);
		}
	}

	Invalidate(invalid);

	UpdateTextChanged();
}


int32
MultiSlider::Value(int32 thumb)
{
	if (thumb < fPositions.CountItems())
		return *(fPositions.ItemAt(thumb));

	return 0;  // -1?
}


BRect
MultiSlider::ThumbFrame(int32 thumb) const
{
	BRect frame = Bounds();

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	float textHeight = ceilf(fontHeight.ascent) + ceilf(fontHeight.descent);

	if (Style() == B_BLOCK_THUMB) {
		if (Orientation() == B_HORIZONTAL) {
			frame.left = floorf(Position(thumb) * (_MaxPosition()
				- _MinPosition()) + _MinPosition()) - 8;
			frame.top = 2 + (Label() || UpdateText() ? textHeight + 4 : 0);
			frame.right = frame.left + 17;
			frame.bottom = frame.top + BarThickness() + 7;
		} else {
			frame.left = floor((frame.Width() - BarThickness()) / 2) - 4;
			frame.top = floorf(Position(thumb) * (_MaxPosition()
				- _MinPosition()) + _MinPosition()) - 8;
			frame.right = frame.left + BarThickness() + 7;
			frame.bottom = frame.top + 17;
		}
	} else {
		if (Orientation() == B_HORIZONTAL) {
			frame.left = floorf(Position(thumb) * (_MaxPosition()
				- _MinPosition()) + _MinPosition()) - 6;
			frame.top = 3 + BarThickness() + (Label() ? textHeight + 4 : 0);
			frame.right = frame.left + 12;
			frame.bottom = frame.top + 8;
		} else {
			frame.left = floor((frame.Width() + BarThickness()) / 2) - 3;
			frame.top = floorf(Position(thumb) * (_MaxPosition()
				- _MinPosition()) + _MinPosition()) - 6;
			frame.right = frame.left + 8;
			frame.bottom = frame.top + 12;
		}
	}

	return frame;
}


float
MultiSlider::_MinPosition() const
{
	if (Orientation() == B_HORIZONTAL)
		return BarFrame().left + 1.0f;

	return BarFrame().bottom - 1.0f;
}


float
MultiSlider::_MaxPosition() const
{
	if (Orientation() == B_HORIZONTAL)
		return BarFrame().right - 1.0f;

	return BarFrame().top + 1.0f;
}


void
MultiSlider::_SetLocationForValue(int32 thumb, int32 value)
{
	BPoint loc;
	int32 min, max;
	GetLimits(&min, &max);

	float range = (float)(max - min);
	if (range == 0)
		range = 1;

	float pos = (float)(value - min) / range *
		(_MaxPosition() - _MinPosition());

	if (Orientation() == B_HORIZONTAL) {
		loc.x = ceil(_MinPosition() + pos);
		loc.y = 0;
	} else {
		loc.x = 0;
		loc.y = floor(_MaxPosition() - pos);
	}

	BPoint* old_pos = fLocations.ItemAt(thumb);

	old_pos->x = loc.x;
	old_pos->y = loc.y;
}


BPoint
MultiSlider::_Location(int32 thumb)
{
	BPoint *loc = fLocations.ItemAt(thumb);

	if (*loc == BPoint(-1, -1))
		_SetLocationForValue(thumb, Value(thumb));

	return *loc;
}


void
MultiSlider::DrawThumbs()
{
	for (int i = 0; i < fPositions.CountItems(); ++i) {
		DrawThumb(i);
	}
}


void
MultiSlider::DrawSlider()
{
	if (LockLooper()) {

		DrawBar();
		DrawHashMarks();

		DrawThumbs();

		DrawFocusMark();
		DrawText();

		UnlockLooper();
	}
}


void
MultiSlider::MouseDown(BPoint point)
{
	if (!IsEnabled())
		return;

	//if (BarFrame().Contains(point))
	//	fInitialLocation = _Location(fSelectedThumb);

	fSelectedThumb = -1;
	for (int i = 0; i < fPositions.CountItems(); ++i) {
		if (ThumbFrame(i).Contains(point)) {
			fSelectedThumb = i;
			fInitialLocation = _Location(fSelectedThumb);
			break;
		}
	}

	if (fSelectedThumb < 0)
		return;

	uint32 buttons;
	GetMouse(&point, &buttons, true);

	_ConstrainPoint(point, fInitialLocation);
	if (_Location(fSelectedThumb) != fInitialLocation)
		InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);

	if (Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) {
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS,
			B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
	} else {
		BPoint prevPoint;

		while (buttons) {
			prevPoint = point;

			snooze(SnoozeAmount());
			GetMouse(&point, &buttons, true);

			if (_ConstrainPoint(point, prevPoint)) {
				int32 value = ValueForPoint(point);
				if (value != Value(fSelectedThumb)) {
					SetValue(fSelectedThumb, value);
					InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);
				}
			}
		}
		if (_Location(fSelectedThumb) != fInitialLocation)
			Invoke();
	}
}


void
MultiSlider::MouseUp(BPoint point)
{
	if (IsTracking()) {
		if (fSelectedThumb >= 0)
			if (_Location(fSelectedThumb) != fInitialLocation)
				Invoke();

		SetTracking(false);
	} else
		BControl::MouseUp(point);
}


void
MultiSlider::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if (IsTracking()) {
		int posCount = fPositions.CountItems();
		float min = _MinPosition();
		float max = _MaxPosition();
		if (fSelectedThumb > 0)
			min = (_Location(fSelectedThumb - 1)).x + 1;
		if (fSelectedThumb < posCount - 1)
			max = (_Location(fSelectedThumb + 1)).x - 1;
		if (_ConstrainPoint(point, _Location(fSelectedThumb), min, max)) {
			int32 value = ValueForPoint(point);
			if (value != Value(fSelectedThumb)) {
				SetValue(fSelectedThumb, value);
				InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);
			}
		}
	} else
		BControl::MouseMoved(point, transit, message);
}


void
MultiSlider::KeyDown(const char* bytes, int32 numBytes)
{
	if (!IsEnabled() || IsHidden())
		return;

	if (fSelectedThumb < 0)
		return;

	int32 newValue = Value(fSelectedThumb);

	int32 min, max;
	GetLimits(&min, &max);

	switch (bytes[0]) {
		case B_LEFT_ARROW:
		case B_DOWN_ARROW:
			newValue -= KeyIncrementValue();
			break;

		case B_RIGHT_ARROW:
		case B_UP_ARROW:
			newValue += KeyIncrementValue();
			break;

		case B_HOME:
			newValue = min;
			break;

		case B_END:
			newValue = max;
			break;

		default:
			BControl::KeyDown(bytes, numBytes);
			return;
	}

	if (newValue < min)
		newValue = min;

	if (newValue > max)
		newValue = max;

	if (newValue != Value(fSelectedThumb)) {
		fInitialLocation = _Location(fSelectedThumb);
		SetValue(fSelectedThumb, newValue);
		InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);
	}
}


void
MultiSlider::KeyUp(const char* bytes, int32 numBytes)
{
	if (fSelectedThumb < 0)
		return;

	if (fInitialLocation != _Location(fSelectedThumb)) {
		Invoke();
	}
}


bool
MultiSlider::_ConstrainPoint(BPoint& point, BPoint comparePoint, float minPos, float maxPos)
{
	if (minPos < 0)
		minPos = _MinPosition();

	if (maxPos < 0)
		maxPos = _MaxPosition();

	if (Orientation() == B_HORIZONTAL) {
		if (point.x != comparePoint.x) {
			if (point.x < minPos)
				point.x = minPos;
			else if (point.x > maxPos)
				point.x = maxPos;

			return true;
		}
	} else {
		if (point.y != comparePoint.y) {
			if (point.y > minPos)
				point.y = minPos;
			else if (point.y < maxPos)
				point.y = maxPos;

			return true;
		}
	}

	return false;
}


void
MultiSlider::AddThumb(float newPosition)
{
	float* position = new float;
	*position = newPosition;
	fPositions.AddItem(position);
	BPoint* location = new BPoint(-1, -1);
	fLocations.AddItem(location);
}


void
MultiSlider::RemoveThumb(int32 thumb)
{
}