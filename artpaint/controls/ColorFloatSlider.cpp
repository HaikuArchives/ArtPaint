/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "ColorFloatSlider.h"

#include "FloatControl.h"


#include <GroupLayoutBuilder.h>
#include <Message.h>
#include <String.h>
#include <Window.h>


#include <new>
#include <stdlib.h>


namespace ArtPaint {
	namespace Interface {

enum {
	kNumberControlFinished		= 'kncf',
	kSliderValueModified		= 'ksvm',
	kSliderModificationFinished	= 'ksmf'
};


ColorFloatSlider::ColorFloatSlider(const char* label, const char* text,
		BMessage* message, float minRange, float maxRange, bool layout,
		bool continuous, border_style borderStyle, thumb_style thumbStyle,
		uint8 resolution)
	: BBox(borderStyle, NULL),
	fMinRange(minRange),
	fMaxRange(maxRange),
	fContinuous(continuous),
	fSlider(NULL),
	fMessage(message),
	fFloatControl(NULL),
	fMult(10)
{
	_InitMessage();

	BFont font;

	fMult = pow(10, resolution);

	fFormat.SetToFormat("%%0.%df", resolution);
	fFloatControl = new (std::nothrow) FloatControl(label, text,
		new BMessage(kNumberControlFinished), 5, minRange < 0);
	fSlider = new (std::nothrow) ColorSlider(NULL, NULL,
		new BMessage(kSliderModificationFinished), (int32)minRange * fMult,
		(int32)maxRange * fMult, B_HORIZONTAL, thumbStyle);

	if (fFloatControl != NULL && fSlider != NULL && layout) {
		SetLayout(new BGroupLayout(B_VERTICAL));
		AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 10.0)
			.Add(fFloatControl)
			.Add(fSlider)
		);

		SetExplicitMinSize(BSize(fFloatControl->PreferredSize().Width() * 2.5,
			MinSize().Height()));
	}

	if (fSlider != NULL)
		fSlider->SetModificationMessage(new BMessage(kSliderValueModified));
}


ColorFloatSlider::~ColorFloatSlider()
{
	delete fMessage;
}


void
ColorFloatSlider::AllAttached()
{
	fSlider->SetTarget(this);
	fFloatControl->SetTarget(this);

	if (fMessage != NULL)
		SetValue(fMessage->FindFloat("value"));
}


void
ColorFloatSlider::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kNumberControlFinished: {
			SetValue(atof(fFloatControl->Text()));
			_SendMessage(atof(fFloatControl->Text()));
		} break;

		case kSliderValueModified: {
			BString value;
			value.SetToFormat(fFormat, (float)(fSlider->Value() / fMult));
			fFloatControl->SetText(value.String());
			if (modifiers() & B_SHIFT_KEY)
				fMessage->ReplaceInt32("modifiers", modifiers());
			else
				fMessage->ReplaceInt32("modifiers", 0);

			if (fContinuous)
				_SendMessage((float)(fSlider->Value() / fMult), false);
		} break;

		case kSliderModificationFinished: {
			BString value;
			value.SetToFormat(fFormat, (float)(fSlider->Value() / fMult));
			fFloatControl->SetText(value.String());
			if (modifiers() & B_SHIFT_KEY)
				fMessage->ReplaceInt32("modifiers", modifiers());
			else
				fMessage->ReplaceInt32("modifiers", 0);

			_SendMessage((float)(fSlider->Value() / fMult), true);
		} break;

		default: {
			BBox::MessageReceived(message);
		} break;
	}
}


void
ColorFloatSlider::SetEnabled(bool enabled)
{
	if (fSlider != NULL)
		fSlider->SetEnabled(enabled);

	if (fFloatControl != NULL)
		fFloatControl->SetEnabled(enabled);
}


void
ColorFloatSlider::SetValue(float value)
{
	value = _FixValue(value);

	if (fSlider != NULL)
		fSlider->SetValue((int32)(value * fMult));

	if (fFloatControl != NULL) {
		fFloatControl->SetValue(value);
		BString valueString;
		valueString.SetToFormat(fFormat, value);
		fFloatControl->SetText(valueString.String());
	}
}


float
ColorFloatSlider::Value() const
{
	if (fSlider != NULL)
		return (float)(fSlider->Value() / fMult);

	if (fFloatControl != NULL)
		return fFloatControl->Value();

	return -1;
}


void
ColorFloatSlider::SetTarget(const BMessenger& target)
{
	fTarget = target;
}


void
ColorFloatSlider::SetMessage(BMessage* message)
{
	delete fMessage;
	fMessage = message;

	_InitMessage();
}


void
ColorFloatSlider::SetMinMax(float min, float max)
{
	fSlider->SetLimits((int32)min * fMult,
		(int32)max * fMult);
}


void
ColorFloatSlider::SetResolution(uint8 resolution)
{
	int32 min, max;
	fSlider->GetLimits(&min, &max);

	float minRange = (float)min / fMult;
	float maxRange = (float)max / fMult;

	fMult = pow(10, resolution);;
	fFormat.SetToFormat("%%0.%df", resolution);
	BString value;
	value.SetToFormat(fFormat, (float)(fSlider->Value() / fMult));
	fFloatControl->SetText(value.String());

	SetMinMax(minRange, maxRange);
}


void
ColorFloatSlider::SetToolTip(const char* tip)
{
	fSlider->SetToolTip(tip);
	fFloatControl->SetToolTip(tip);
}


ColorSlider*
ColorFloatSlider::Slider() const
{
	return fSlider;
}


FloatControl*
ColorFloatSlider::TextControl() const
{
	return fFloatControl;
}


BLayoutItem*
ColorFloatSlider::LabelLayoutItem() const
{
	if (fFloatControl != NULL)
		return fFloatControl->CreateLabelLayoutItem();
	return NULL;
}


BLayoutItem*
ColorFloatSlider::TextViewLayoutItem() const
{
	if (fFloatControl != NULL)
		return fFloatControl->CreateTextViewLayoutItem();
	return NULL;
}


void
ColorFloatSlider::_InitMessage()
{
	// The message returned by the control will have at least
	// the two items: 'value' and 'final'.
	// 'value' is of course the current value of the control;
	// It may have been given an initial value at construction.
	// 'final' will be false while the slider is being moved,
	// and set true at mouse-up.
	if (fMessage == NULL)
		fMessage = new BMessage;

	if (fMessage != NULL) {
		if (!fMessage->HasFloat("value"))	// may have been set by creator
			fMessage->AddFloat("value", 0.0);

		fMessage->AddBool("final", true);
		fMessage->AddInt32("modifiers", 0);
		fMessage->AddPointer("id", this);
	}
}


float
ColorFloatSlider::_FixValue(float value)
{
	if (value > fMaxRange)
		value = fMaxRange;

	if (value < fMinRange)
		value = fMinRange;

	return value;
}


void
ColorFloatSlider::_SendMessage(float value, bool final)
{
	if (fMessage != NULL) {
		fMessage->ReplaceBool("final", final);
		fMessage->ReplaceFloat("value", value);

		if (!fTarget.IsValid()) {
			if (BWindow* window = Window())
				window->PostMessage(fMessage, window);
		} else
			fTarget.SendMessage(fMessage);
	}
}

	}	// namespace Interface
}	// namespace ArtPaint
