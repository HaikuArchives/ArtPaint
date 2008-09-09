/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef POP_UP_SLIDER_H
#define	POP_UP_SLIDER_H

#include <Window.h>

class PopUpSlider : public BWindow {
		BSlider	*the_slider;

		// A private constructor. The object is created through the
		// Instantiate-function
		PopUpSlider(BRect);
public:

static	PopUpSlider*	Instantiate(BPoint,BMessenger*,BMessage*,int32,int32);
		BSlider*		ReturnSlider() { return the_slider; }
		void			Go();
};

#endif
