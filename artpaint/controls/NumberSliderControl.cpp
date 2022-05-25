/*
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Karsten Heimrich <host.haiku@gmx.de>
 *		(fixes by Pete Goodeve 2017)
 *      Dale Cieslak <dcieslak@yahoo.com>
 */

#include "NumberSliderControl.h"

#include "NumberControl.h"


#include <GroupLayoutBuilder.h>
#include <Message.h>
#include <String.h>
#include <Window.h>


#include <new>
#include <stdlib.h>
#include <stdio.h>


namespace ArtPaint {
	namespace Interface {

enum {
	kNumberControlFinished		= 'kncf',
	kSliderValueModified		= 'ksvm',
	kSliderModificationFinished	= 'ksmf'
};


NumberSliderControl::NumberSliderControl(const char* label, const char* text,
		BMessage* message, int32 minRange, int32 maxRange, bool layout,
		bool continuous, border_style borderStyle, thumb_style thumbStyle,
		bool proportional)
	: BBox(borderStyle, NULL)
	, fMinRange(minRange)
	, fMaxRange(maxRange)
	, fContinuous(continuous)
	, fSlider(NULL)
	, fMessage(message)
	, fNumberControl(NULL)
	, fProportional(proportional)
	, fExp(4)
{
	_InitMessage();

	fNumberControl = new (std::nothrow) NumberControl(label, text,
		new BMessage(kNumberControlFinished), 3, minRange < 0);
	int32 range = fMaxRange - fMinRange;
	int32 inc = 1000. / range;

	fSlider = new (std::nothrow) BSlider(NULL, NULL,
		new BMessage(kSliderModificationFinished), 0,
		(inc * range), B_HORIZONTAL, thumbStyle);

	if (fNumberControl && fSlider && layout) {
		SetLayout(new BGroupLayout(B_VERTICAL));
		AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 10.0)
			.Add(fNumberControl)
			.Add(fSlider)
		);

		SetExplicitMinSize(BSize(fNumberControl->PreferredSize().Width() * 2.5,
			MinSize().Height()));
	}

	if (fSlider) {
		fSlider->SetModificationMessage(new BMessage(kSliderValueModified));
		fSlider->SetKeyIncrementValue(inc);
	}
}


NumberSliderControl::~NumberSliderControl()
{
	delete fMessage;
}


void
NumberSliderControl::AllAttached()
{
	fSlider->SetTarget(this);
	fNumberControl->SetTarget(this);

	if (fMessage)
		SetValue(fMessage->FindInt32("value"));
}


void
NumberSliderControl::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kNumberControlFinished: {
			fSlider->SetPosition(
				_PositionForValue(_FixValue(atoi(fNumberControl->Text())))
			);
			_SendMessage(_FixValue(atoi(fNumberControl->Text())), true);
		}	break;

		case kSliderValueModified: {
			int32 numValue = _ValueForPosition(fSlider->Position());
			BString value;
			value << numValue;
			fNumberControl->SetText(value.String());
			if (fContinuous)
				_SendMessage(numValue, true);
		}	break;

		case kSliderModificationFinished: {
			int32 numValue = _ValueForPosition(fSlider->Position());
			BString value;
			value << numValue;
			fNumberControl->SetText(value.String());
			_SendMessage(numValue, true);
		}	break;

		default: {
			BBox::MessageReceived(message);
		}	break;
	}
}


void
NumberSliderControl::SetEnabled(bool enabled)
{
	if (fSlider)
		fSlider->SetEnabled(enabled);

	if (fNumberControl)
		fNumberControl->SetEnabled(enabled);
}


void
NumberSliderControl::SetValue(int32 value)
{
	printf("value %d\n", value);

	value = _FixValue(value);
	printf("fix value %d\n", value);

	if (fSlider)
		fSlider->SetPosition(_PositionForValue(value));

	if (fNumberControl) {
		BString strValue;
		strValue << value;
		fNumberControl->SetText(strValue.String());
	}
}


int32
NumberSliderControl::Value() const
{
	if (!fSlider)
		return -1;

	int32 range = fMaxRange - fMinRange;
	if (!fProportional)
		return (fSlider->Position() * range) + fMinRange;

	float norm = (pow(fExp, fSlider->Position()) - 1.) / (fExp - 1.);

	return (norm * range) + fMinRange;
}


void
NumberSliderControl::SetTarget(const BMessenger& target)
{
	fTarget = target;
}


void
NumberSliderControl::SetMessage(BMessage* message)
{
	delete fMessage;
	fMessage = message;

	_InitMessage();
}


BSlider*
NumberSliderControl::Slider() const
{
	return fSlider;
}


NumberControl*
NumberSliderControl::TextControl() const
{
	return fNumberControl;
}


BLayoutItem*
NumberSliderControl::LabelLayoutItem() const
{
	if (fNumberControl)
		return fNumberControl->CreateLabelLayoutItem();
	return NULL;
}


BLayoutItem*
NumberSliderControl::TextViewLayoutItem() const
{
	if (fNumberControl)
		return fNumberControl->CreateTextViewLayoutItem();
	return NULL;
}


void
NumberSliderControl::_InitMessage()
{
	// The message returned by the control will have at least
	// the two items: 'value' and 'final'.
	// 'value' is of course the current value of the control;
	// It may have been given an initial value at construction.
	// 'final' will be false while the slider is being moved,
	// and set true at mouse-up.
	if (!fMessage)
		fMessage = new BMessage;

	if (fMessage) {
		if (!fMessage->HasInt32("value"))	// may have been set by creator
			fMessage->AddInt32("value", 0);

		fMessage->AddBool("final", true);
	}
}


int32
NumberSliderControl::_FixValue(int32 value)
{
	if (value > fMaxRange)
		value = fMaxRange;

	if (value < fMinRange)
		value = fMinRange;

	return value;
}


void
NumberSliderControl::_SendMessage(int32 value, bool final)
{
	if (fMessage) {
		fMessage->ReplaceBool("final", final);
		fMessage->ReplaceInt32("value", value);

		if (!fTarget.IsValid()) {
			if (BWindow* window = Window())
				window->PostMessage(fMessage, window);
		} else {
			fTarget.SendMessage(fMessage);
		}
	}
}


float
NumberSliderControl::_PositionForValue(int32 value)
{
	int32 range = fMaxRange - fMinRange;
	if (!fProportional)
		return (float)(value - fMinRange) / (float)range;

	float norm = (float)(value - fMinRange) / (float)range;

	return log(norm * (fExp - 1.) + 1.) / log(fExp);
}


int32
NumberSliderControl::_ValueForPosition(float position)
{
	int32 range = fMaxRange - fMinRange;
	if (!fProportional)
		return (position * range) + fMinRange;

	float norm = (pow(fExp, position) - 1.) / (fExp - 1.);

	return (norm * range) + fMinRange;

}


	}	// namespace Interface
}	// namespace ArtPaint
