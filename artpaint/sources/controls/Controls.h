/*

	Filename:	Controls.h
	Contents:	Declarations for various control-classes
	Author:		Heikki Suhonen

*/

#ifndef CONTROLS_H
#define CONTROLS_H

#include <Box.h>
#include <Slider.h>
#include <TextControl.h>

#define EXTRA_EDGE	4

class NumberControl : public BTextControl {

public:
		NumberControl(BRect frame, const char *name, const char *label, const char *text, BMessage *message,int32 maxBytes=5,bool allow_negative=false,bool continuos=true);

void	AttachedToWindow();
int32	Value();
void	SetValue(int32);
};


// this class creates an object derived from BControl
// that has horizontal slider and it reports its value
// to target whenever value is changed
//class ControlSlider : public BControl {
//	float max_range,min_range;
//	BRect knob_rect;
//	thread_id slider_thread;
//
//static	int32	thread_func(void *data);
//void	threadFunc();
//
//public:
//	ControlSlider(BRect frame,char *name,char *label,BMessage *message,int32 rangeMin,int32 rangeMax);
//	~ControlSlider();
//
//void	Draw(BRect);
//void	MouseDown(BPoint);
//};



class ControlSlider : public BSlider {

static	int32	track_entry(void*);
		int32	track_mouse();
public:
		ControlSlider(BRect,const char*,const char*,BMessage*,int32,int32,thumb_style);

void	MouseDown(BPoint);
};


// this class groups a NumberControl and ControlSlider - objects under the
// same parent and takes care that whenever the others value is changed, the
// other will be also updated
// then this informs the target about the change
class ControlSliderBox : public BBox {
	BSlider 	*slider;
	NumberControl 	*number_control;

// this stores the model-message that is to be sent to the target
	BMessage 		*msg;
	bool			continuos_messages;

	BMessenger		*target;		// If this is NULL, the target will be the window.
	float 			max_value,min_value;
	// this posts the message to the target
	void			sendMessage(int32 value,bool final=TRUE);

	float			divider;

	int32			CheckValue(int32);

public:
	// The message that is passed to this constructor must not be NULL. It must also contain
	// an int32 named "value". Whatever the value is will be used as the initial value for controller.
	ControlSliderBox(BRect frame,const char *name, const char *label, const char *text, BMessage *message,int32 rangeMin, int32 rangeMax, border_style border = B_PLAIN_BORDER,bool continuos=TRUE,thumb_style knob=B_TRIANGLE_THUMB);
	~ControlSliderBox();

void	AllAttached();
void	MessageReceived(BMessage *message);

void	setValue(int32 value);

// The caller of this function retains the ownership of the message.
void	SetMessage(BMessage*);

void	SetTarget(BMessenger *t) { target = t; }

// These functions return and set the point where the number-control and slider meet.
// They can be used to arrange many controls to have equal look.
float	Divider();
void	SetDivider(float,bool resize_text_field=FALSE);


};






#endif
