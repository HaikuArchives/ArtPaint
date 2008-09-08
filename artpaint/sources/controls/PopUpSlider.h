/*

	Filename:	PopUpSlider.h
	Contents:	PopUpSlider-class declarations
	Author:		Heikki Suhonen

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
