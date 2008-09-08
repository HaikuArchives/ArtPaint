/*

	Filename:	Controls.cpp
	Contents:	Definitions for various control-classes
	Author:		Heikki Suhonen

*/

#include <stdio.h>
#include <stdlib.h>

#include "Controls.h"
#include "UtilityClasses.h"
#include "MessageConstants.h"
#include "ToolSetupWindow.h"

NumberControl::NumberControl(BRect frame, const char *name, const char *label, const char *text, BMessage *message, int32 maxBytes,bool allow_negative,bool continuos)
				: BTextControl(frame,name,label,text,message)
{
	// here we set the control to only accept numbers
	// here disallow all chars
	for (int32 i=0;i<256;i++)
		TextView()->DisallowChar(i);
	// and here allow the numbers
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
	if (allow_negative == TRUE)
		TextView()->AllowChar('-');

	// there is a bug in alignment function
	// SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	TextView()->SetMaxBytes(maxBytes);
}


void NumberControl::AttachedToWindow()
{
	BTextControl::AttachedToWindow();
}


int32 NumberControl::Value()
{
	int32 value;
	value = atoi(Text());
	return value;
}


void NumberControl::SetValue(int32 value)
{
	BTextControl::SetValue(value);

	char text[256];

	sprintf(text,"%d",value);
	SetText(text);
}

#define KNOB_WIDTH 10.0

// This is for communication between control-slider and it's parent.
#define	CONTROL_SLIDER_FINISHED	'CslF'


ControlSlider::ControlSlider(BRect frame,const char *name,const char *label,BMessage *msg,int32 rangeMin,int32 rangeMax,thumb_style knob)
	: BSlider(frame,name,label,msg,rangeMin,rangeMax,knob)
{
}

void ControlSlider::MouseDown(BPoint)
{
	thread_id track_thread = spawn_thread(track_entry,"track_thread",B_NORMAL_PRIORITY,(void*)this);
	resume_thread(track_thread);
}


int32 ControlSlider::track_entry(void *p)
{
	return ((ControlSlider*)p)->track_mouse();
}


int32 ControlSlider::track_mouse()
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

ControlSliderBox::ControlSliderBox(BRect frame,const char *name, const char *label, const char *text, BMessage *message,int32 rangeMin, int32 rangeMax, border_style border,bool continuos,thumb_style)
		: BBox(frame,name,B_FOLLOW_LEFT|B_FOLLOW_TOP,B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE_JUMP,border)
{
	target = NULL;
	number_control = new NumberControl(BRect(2,2,2,2),name,label,text,new BMessage(HS_NUMBER_CONTROL_INVOKED),3,(rangeMin<0));
	number_control->ResizeToPreferred();
	number_control->TextView()->ResizeTo(number_control->TextView()->StringWidth("0000"),number_control->TextView()->Bounds().Height());
	number_control->ResizeTo(number_control->TextView()->Frame().right+2,number_control->Frame().Height());
	AddChild(number_control);
	divider = Divider();

	BRect slider_frame = BRect(number_control->Frame().right+2,number_control->Frame().top,frame.Width()-4,number_control->Frame().bottom);
//	slider = new ControlSlider(BRect(number_control->Frame().right+5,number_control->Frame().top,frame.Width()-4,number_control->Frame().bottom),name,label,new BMessage(HS_CONTROL_SLIDER_INVOKED),rangeMin,rangeMax);
	slider = new ControlSlider(slider_frame,name,NULL,new BMessage(CONTROL_SLIDER_FINISHED),rangeMin,rangeMax,B_BLOCK_THUMB);
	slider->SetSnoozeAmount(50 * 1000);
	slider->SetModificationMessage(new BMessage(HS_CONTROL_SLIDER_INVOKED));

	AddChild(slider);
	ResizeTo(frame.Width(),number_control->Bounds().Height()+4);

	continuos_messages = continuos;

	min_value = rangeMin;
	max_value = rangeMax;

	if (message != NULL) {
		msg = message;
		if (msg->HasInt32("value") == FALSE) {
			msg->AddInt32("value",0);
		}
		if (msg->HasBool("final") == FALSE) {
			msg->AddBool("final",TRUE);
		}
	}
	else {
		// We have to create a dummy-message.
		msg = new BMessage();
		msg->AddInt32("value",0);
		msg->AddBool("final",TRUE);
	}
}

ControlSliderBox::~ControlSliderBox()
{
	if (target != NULL) {
		delete target;
		target = NULL;
	}
}

void ControlSliderBox::AllAttached()
{
	number_control->SetTarget(this);
	slider->SetTarget(this);
	setValue(msg->FindInt32("value"));

//	SetDivider(divider);
}


void ControlSliderBox::MessageReceived(BMessage *message)
{
	switch (message->what) {

	// this comes from the number_control and tells us that it's value has changed
	// we should then set the new value for slider and inform the window also
	case HS_NUMBER_CONTROL_INVOKED:
		printf("number control invoked\n");
		slider->SetValue(CheckValue(atoi(number_control->Text())));
		sendMessage(atoi(number_control->Text()));
		break;
	// this comes from the slider and tells us that it's value has changed
	// we should then set the new value for number_control and inform the window also
	case HS_CONTROL_SLIDER_INVOKED:
		char value[10];
		sprintf(value,"%d",slider->Value());
		number_control->SetText(value);
		if (continuos_messages == TRUE)
			sendMessage(slider->Value(),FALSE);
		break;
	case CONTROL_SLIDER_FINISHED:
		sprintf(value,"%d",slider->Value());
		number_control->SetText(value);
		sendMessage(slider->Value(),TRUE);
		break;
	default:
		BBox::MessageReceived(message);
		break;
	}
}


void ControlSliderBox::setValue(int32 value)
{
	// this sets the value for both of the views
	value = CheckValue(value);

//	Next line causes some odd crashes.
	slider->SetValue(value);
	char val[10];
	sprintf(val,"%d",value);
	number_control->SetText(val);
}


int32 ControlSliderBox::CheckValue(int32 value)
{
	if (value>max_value)
		value = (int32)max_value;
	else if (value<min_value)
		value = (int32)min_value;

	return value;
}


// this posts a message that contains the new value to the window
void ControlSliderBox::sendMessage(int32 value,bool final)
{
	msg->ReplaceInt32("value",value);
	msg->ReplaceBool("final",final);

	if (target == NULL)
		Window()->PostMessage(msg,Window());
	else
		target->SendMessage(msg);
}

void ControlSliderBox::SetMessage(BMessage *new_message)
{
	msg = new BMessage(*new_message);
}



float ControlSliderBox::Divider()
{
	return number_control->Frame().right;
}


void ControlSliderBox::SetDivider(float position,bool resize_text_field)
{
	float delta = position - Divider();
	number_control->ResizeTo(position-number_control->Frame().left,number_control->Bounds().Height());
	if (resize_text_field)
		number_control->TextView()->ResizeBy(delta,0);
	else
		number_control->TextView()->MoveBy(delta,0);

	slider->MoveBy(delta,0);
	slider->ResizeBy(-delta,0);

	divider = position;
}






