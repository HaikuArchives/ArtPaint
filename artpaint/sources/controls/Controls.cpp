/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "Controls.h"

#include "MessageConstants.h"
#include "UtilityClasses.h"


#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <String.h>
#include <Window.h>


#include <stdio.h>
#include <stdlib.h>


NumberControl::NumberControl(const char* label, const char* text,
		BMessage* message, int32 maxBytes, bool allowNegative, bool continuos)
	: BTextControl(label, text, message)
{
	_InitControl(maxBytes, allowNegative, continuos);
}


NumberControl::NumberControl(BRect frame, const char* name, const char* label,
		const char* text, BMessage* message, int32 maxBytes, bool allowNegative,
		bool continuos)
	: BTextControl(frame, name, label, text, message)
{
	_InitControl(maxBytes, allowNegative, continuos);
}


int32
NumberControl::Value() const
{
	return atoi(Text());
}


void
NumberControl::SetValue(int32 value)
{
	BTextControl::SetValue(value);

	BString text;
	text << value;

	SetText(text.String());
}


void
NumberControl::AttachedToWindow()
{
	BTextControl::AttachedToWindow();
}


void
NumberControl::_InitControl(int32 maxBytes, bool allowNegative, bool continuos)
{
	for (uint32 i = 0; i < 256; ++i)
		TextView()->DisallowChar(i);

	TextView()->AllowChar('0');
	TextView()->AllowChar('1');
	TextView()->AllowChar('2');
	TextView()->AllowChar('3');
	TextView()->AllowChar('4');
	TextView()->AllowChar('5');
	TextView()->AllowChar('6');
	TextView()->AllowChar('7');
	TextView()->AllowChar('8');
	TextView()->AllowChar('9');

	if (allowNegative)
		TextView()->AllowChar('-');

	TextView()->SetMaxBytes(maxBytes);
	SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
}


// #pragma mark -- ControlSlider


// This is for communication between control-slider and it's parent.
#define	CONTROL_SLIDER_FINISHED	'CslF'


ControlSlider::ControlSlider(const char* name, const char* label,
		BMessage* message, int32 minValue, int32 maxValue, thumb_style thumbType)
	: BSlider(name, label, message, minValue, maxValue, B_HORIZONTAL, thumbType)
{
}


ControlSlider::ControlSlider(BRect frame, const char *name, const char *label,
		BMessage *msg, int32 rangeMin, int32 rangeMax, thumb_style knob)
	: BSlider(frame,name,label,msg,rangeMin,rangeMax,knob)
{
}


void
ControlSlider::MouseDown(BPoint where)
{
	thread_id threadId = spawn_thread(track_entry, "track_thread",
		B_NORMAL_PRIORITY,(void*)this);
	resume_thread(threadId);
}


int32
ControlSlider::track_entry(void* p)
{
	return ((ControlSlider*)p)->track_mouse();
}


int32
ControlSlider::track_mouse()
{
	uint32 buttons;
	BPoint point;
	BWindow *window = Window();
	if (window == NULL)
		return B_ERROR;

	BHandler *target_handler = Target();
	if (target_handler == NULL)
		return B_ERROR;

	BLooper *target_looper = target_handler->Looper();

	if (target_looper == NULL)
		return B_ERROR;

	BMessenger messenger = BMessenger(target_handler,target_looper);

	window->Lock();
	GetMouse(&point,&buttons);
	int32 value	= ValueForPoint(point);
	window->Unlock();

	if (value != Value()) {
		if (ModificationMessage() != NULL)
			messenger.SendMessage(ModificationMessage());

		window->Lock();
		SetValue(value);
		window->Unlock();
	}
	while (buttons) {
		window->Lock();
		GetMouse(&point,&buttons);
		value = ValueForPoint(point);
		if (value != Value()) {
			if (ModificationMessage() != NULL)
				messenger.SendMessage(ModificationMessage());

			SetValue(value);
		}
		window->Unlock();

		snooze(SnoozeAmount());
	}

	if (Message() != NULL)
		messenger.SendMessage(Message());
	return B_OK;
}
