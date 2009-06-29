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

// these are used for ControlSliderBox's internal messaging
#define HS_NUMBER_CONTROL_INVOKED	'NcIv'
#define HS_CONTROL_SLIDER_INVOKED	'CslI'


ControlSliderBox::ControlSliderBox(const char* name, const char* label,
		const char* text, BMessage* message, int32 rangeMin, int32 rangeMax,
		border_style border, bool continuos, thumb_style knob)
	: BBox(border, NULL)
	, continuos_messages(continuos)
	, target(NULL)
	, min_value(rangeMin)
	, max_value(rangeMax)
{
	AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 10.0)
		.Add(number_control = new NumberControl(label, text,
				new BMessage(HS_NUMBER_CONTROL_INVOKED), 3, (rangeMin < 0)))
		.Add(slider = new ControlSlider(name, NULL,
				new BMessage(CONTROL_SLIDER_FINISHED), rangeMin, rangeMax,
				B_BLOCK_THUMB))
	);
	slider->SetSnoozeAmount(50 * 1000);
	slider->SetModificationMessage(new BMessage(HS_CONTROL_SLIDER_INVOKED));

	if (message) {
		msg = message;
		if (msg->HasInt32("value") == false)
			msg->AddInt32("value", 0);

		if (msg->HasBool("final") == false)
			msg->AddBool("final", true);
	} else {
		// We have to create a dummy-message.
		msg = new BMessage();
		msg->AddInt32("value", 0);
		msg->AddBool("final", true);
	}
	SetExplicitMinSize(BSize(number_control->PreferredSize().Width() * 2.5,
		MinSize().Height()));
}


ControlSliderBox::ControlSliderBox(BRect frame, const char* name,
		const char* label, const char* text, BMessage* message, int32 rangeMin,
		int32 rangeMax, border_style border, bool continuos, thumb_style)
	: BBox(frame, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW |
		B_FRAME_EVENTS | B_NAVIGABLE_JUMP, border)
{
	target = NULL;
	number_control = new NumberControl(BRect(2,2,2,2),name,label,text,
		new BMessage(HS_NUMBER_CONTROL_INVOKED),3,(rangeMin<0));
	number_control->ResizeToPreferred();
	number_control->TextView()->ResizeTo(number_control->TextView()->StringWidth("0000"),
			number_control->TextView()->Bounds().Height());
	number_control->ResizeTo(number_control->TextView()->Frame().right+2,
		number_control->Frame().Height());
	AddChild(number_control);
	divider = Divider();

	BRect slider_frame = BRect(number_control->Frame().right+2,
		number_control->Frame().top,frame.Width()-4,number_control->Frame().bottom);
	slider = new ControlSlider(slider_frame,name,NULL,
		new BMessage(CONTROL_SLIDER_FINISHED),rangeMin,rangeMax,B_BLOCK_THUMB);
	slider->SetSnoozeAmount(50 * 1000);
	slider->SetModificationMessage(new BMessage(HS_CONTROL_SLIDER_INVOKED));

	AddChild(slider);
	ResizeTo(frame.Width(),number_control->Bounds().Height()+4);

	continuos_messages = continuos;

	min_value = rangeMin;
	max_value = rangeMax;

	if (message) {
		msg = message;
		if (msg->HasInt32("value") == false)
			msg->AddInt32("value", 0);

		if (msg->HasBool("final") == false)
			msg->AddBool("final", true);
	} else {
		// We have to create a dummy-message.
		msg = new BMessage();
		msg->AddInt32("value", 0);
		msg->AddBool("final", true);
	}
}


ControlSliderBox::~ControlSliderBox()
{
	if (target)
		delete target;
	if (msg)
		delete msg;
}


void
ControlSliderBox::AllAttached()
{
	number_control->SetTarget(this);
	slider->SetTarget(this);
	setValue(msg->FindInt32("value"));
}


void
ControlSliderBox::MessageReceived(BMessage *message)
{
	switch (message->what) {

	// this comes from the number_control and tells us that it's value has changed
	// we should then set the new value for slider and inform the window also
	case HS_NUMBER_CONTROL_INVOKED:
		printf("number control invoked\n");
		slider->SetValue(CheckValue(atoi(number_control->Text())));
		sendMessage(atoi(number_control->Text()));
		break;
	// this comes from the slider and tells us that it's value has changed we
	// should then set the new value for number_control and inform the window
	case HS_CONTROL_SLIDER_INVOKED:
		char value[10];
		sprintf(value, "%ld", slider->Value());
		number_control->SetText(value);
		if (continuos_messages == TRUE)
			sendMessage(slider->Value(),FALSE);
		break;
	case CONTROL_SLIDER_FINISHED:
		sprintf(value, "%ld", slider->Value());
		number_control->SetText(value);
		sendMessage(slider->Value(),TRUE);
		break;
	default:
		BBox::MessageReceived(message);
		break;
	}
}


void
ControlSliderBox::setValue(int32 value)
{
	// this sets the value for both of the views
	value = CheckValue(value);

//	Next line causes some odd crashes.
	slider->SetValue(value);
	char val[10];
	sprintf(val, "%ld", value);
	number_control->SetText(val);
}


int32
ControlSliderBox::CheckValue(int32 value)
{
	if (value>max_value)
		value = (int32)max_value;
	else if (value<min_value)
		value = (int32)min_value;

	return value;
}


// this posts a message that contains the new value to the window
void
ControlSliderBox::sendMessage(int32 value, bool final)
{
	msg->ReplaceInt32("value", value);
	msg->ReplaceBool("final", final);

	if (target == NULL)
		Window()->PostMessage(msg,Window());
	else
		target->SendMessage(msg);
}


void
ControlSliderBox::SetMessage(BMessage *new_message)
{
	delete msg;
	msg = new BMessage(*new_message);
}


float
ControlSliderBox::Divider()
{
	return number_control->Frame().right;
}


void
ControlSliderBox::SetDivider(float position,bool resize_text_field)
{
	float delta = position - Divider();
	number_control->ResizeTo(position-number_control->Frame().left,
		number_control->Bounds().Height());
	if (resize_text_field)
		number_control->TextView()->ResizeBy(delta,0);
	else
		number_control->TextView()->MoveBy(delta,0);

	slider->MoveBy(delta,0);
	slider->ResizeBy(-delta,0);

	divider = position;
}
