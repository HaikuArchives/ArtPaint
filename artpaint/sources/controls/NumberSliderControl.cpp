/*
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "NumberSliderControl.h"

#include "Controls.h"


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


NumberSliderControl::NumberSliderControl(const char* label, const char* text,
		BMessage* message, int32 minRange, int32 maxRange, bool layout,
		bool continuos, border_style borderStyle, thumb_style thumbStyle)
	: BBox(borderStyle, NULL)
	, fMinRange(minRange)
	, fMaxRange(maxRange)
	, fContinuos(continuos)
	, fSlider(NULL)
	, fMessage(message)
	, fNumberControl(NULL)
{
	_InitMessage();

	fNumberControl = new (std::nothrow) NumberControl(label, text,
		new BMessage(kNumberControlFinished), 3, minRange < 0);
	fSlider = new (std::nothrow) BSlider(NULL, NULL,
		new BMessage(kSliderModificationFinished), minRange,
		maxRange, B_HORIZONTAL, thumbStyle);

	if (fNumberControl && fSlider && layout) {
		AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 10.0)
			.Add(fNumberControl)
			.Add(fSlider)
		);

		SetExplicitMinSize(BSize(fNumberControl->PreferredSize().Width() * 2.5,
			MinSize().Height()));
	}

	if (fSlider)
		fSlider->SetModificationMessage(new BMessage(kSliderValueModified));
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
			fSlider->SetValue(_FixValue(atoi(fNumberControl->Text())));
			_SendMessage(atoi(fNumberControl->Text()));
		}	break;

		case kSliderValueModified: {
			BString value;
			value << fSlider->Value();
			fNumberControl->SetText(value.String());
			if (fContinuos)
				_SendMessage(fSlider->Value(), false);
		}	break;

		case kSliderModificationFinished: {
			BString value;
			value << fSlider->Value();
			fNumberControl->SetText(value.String());
			_SendMessage(fSlider->Value(), true);
		}	break;

		default: {
			BBox::MessageReceived(message);
		}	break;
	}
}


void
NumberSliderControl::SetValue(int32 value)
{
	value = _FixValue(value);

	if (fSlider)
		fSlider->SetValue(value);

	if (fNumberControl) {
		BString value;
		value << fSlider->Value();
		fNumberControl->SetText(value.String());
	}
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
	if (!fMessage)
		fMessage = new BMessage;

	if (fMessage) {
		if (fMessage->HasInt32("value"))
			fMessage->AddInt32("value", 0);

		if (fMessage->HasBool("final"))
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

	}	// namespace Interface
}	// namespace ArtPaint
