/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "FloatSliderControl.h"

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
	kNumberControlFinished = 'kncf',
	kSliderValueModified = 'ksvm',
	kSliderModificationFinished = 'ksmf'
};


FloatSliderControl::FloatSliderControl(const char* label, const char* text, BMessage* message,
	float minRange, float maxRange, bool layout, bool continuous, border_style borderStyle,
	thumb_style thumbStyle, uint8 resolution)
	:
	BBox(borderStyle, NULL),
	fMinRange(minRange),
	fMaxRange(maxRange),
	fContinuous(continuous),
	fSlider(NULL),
	fMessage(message),
	fFloatControl(NULL),
	fMult(10)
{
	_InitMessage();

	fMult = pow(10, resolution);

	fFloatControl = new (std::nothrow)
		FloatControl(label, text, new BMessage(kNumberControlFinished), 5, minRange < 0);
	fSlider = new (std::nothrow) BSlider(NULL, NULL, new BMessage(kSliderModificationFinished),
		(int32)minRange * fMult, (int32)maxRange * fMult, B_HORIZONTAL, thumbStyle);

	if (fFloatControl && fSlider && layout) {
		SetLayout(new BGroupLayout(B_VERTICAL));
		AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 10.0)
			.Add(fFloatControl)
			.Add(fSlider));

		SetExplicitMinSize(BSize(fFloatControl->PreferredSize().Width() * 2.5, MinSize().Height()));
	}

	if (fSlider)
		fSlider->SetModificationMessage(new BMessage(kSliderValueModified));
}


FloatSliderControl::~FloatSliderControl()
{
	delete fMessage;
}


void
FloatSliderControl::AllAttached()
{
	fSlider->SetTarget(this);
	fFloatControl->SetTarget(this);

	if (fMessage)
		SetValue(fMessage->FindFloat("value"));
}


void
FloatSliderControl::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kNumberControlFinished:
		{
			fSlider->SetValue(_FixValue(atof(fFloatControl->Text())) * fMult);
			_SendMessage(atof(fFloatControl->Text()));
		} break;
		case kSliderValueModified:
		{
			BString value;
			value << (float)(fSlider->Value() / fMult);
			fFloatControl->SetText(value.String());
			if (fContinuous)
				_SendMessage((float)(fSlider->Value() / fMult), false);
		} break;
		case kSliderModificationFinished:
		{
			BString value;
			value << (float)(fSlider->Value() / fMult);
			fFloatControl->SetText(value.String());
			_SendMessage((float)(fSlider->Value() / fMult), true);
		} break;
		default:
			BBox::MessageReceived(message);
	}
}


void
FloatSliderControl::SetEnabled(bool enabled)
{
	if (fSlider)
		fSlider->SetEnabled(enabled);

	if (fFloatControl)
		fFloatControl->SetEnabled(enabled);
}


void
FloatSliderControl::SetValue(float value)
{
	value = _FixValue(value);

	if (fSlider)
		fSlider->SetValue((int32)(value * fMult));

	if (fFloatControl)
		fFloatControl->SetValue(value);
}


float
FloatSliderControl::Value() const
{
	if (fSlider)
		return (float)(fSlider->Value() / fMult);

	if (fFloatControl)
		return fFloatControl->Value();

	return -1;
}


void
FloatSliderControl::SetTarget(const BMessenger& target)
{
	fTarget = target;
}


void
FloatSliderControl::SetMessage(BMessage* message)
{
	delete fMessage;
	fMessage = message;

	_InitMessage();
}


void
FloatSliderControl::SetMinMax(float min, float max)
{
	fSlider->SetLimits((int32)min * fMult, (int32)max * fMult);
}


void
FloatSliderControl::SetResolution(uint8 resolution)
{
	int32 min, max;
	fSlider->GetLimits(&min, &max);

	float minRange = (float)min / fMult;
	float maxRange = (float)max / fMult;

	fMult = resolution;

	SetMinMax(minRange, maxRange);
}


BString
FloatSliderControl::Label() const
{
	return fFloatControl->Label();
}


BSlider*
FloatSliderControl::Slider() const
{
	return fSlider;
}


FloatControl*
FloatSliderControl::TextControl() const
{
	return fFloatControl;
}


BLayoutItem*
FloatSliderControl::LabelLayoutItem() const
{
	if (fFloatControl)
		return fFloatControl->CreateLabelLayoutItem();
	return NULL;
}


BLayoutItem*
FloatSliderControl::TextViewLayoutItem() const
{
	if (fFloatControl)
		return fFloatControl->CreateTextViewLayoutItem();
	return NULL;
}


void
FloatSliderControl::_InitMessage()
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
		if (!fMessage->HasFloat("value")) // may have been set by creator
			fMessage->AddFloat("value", 0.0);

		fMessage->AddBool("final", true);
	}
}


float
FloatSliderControl::_FixValue(float value)
{
	if (value > fMaxRange)
		value = fMaxRange;

	if (value < fMinRange)
		value = fMinRange;

	return value;
}


void
FloatSliderControl::_SendMessage(float value, bool final)
{
	if (fMessage) {
		fMessage->ReplaceBool("final", final);
		fMessage->ReplaceFloat("value", value);

		if (!fTarget.IsValid()) {
			if (BWindow* window = Window())
				window->PostMessage(fMessage, window);
		} else
			fTarget.SendMessage(fMessage);
	}
}

} // namespace Interface
} // namespace ArtPaint
