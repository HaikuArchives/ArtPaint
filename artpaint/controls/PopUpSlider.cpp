/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2008, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <karsten.heimrich@gmx.de>
 *
 */

#include "PopUpSlider.h"


#include <Message.h>
#include <Messenger.h>
#include <Slider.h>


PopUpSlider::PopUpSlider(BRect rect)
	:
	BWindow(rect, "popUpSliderWindow", B_BORDERED_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL,
		B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE),
	fSlider(NULL)
{
}


PopUpSlider*
PopUpSlider::Instantiate(
	const BMessenger& target, BMessage* message, int32 minRange, int32 maxRange)
{
	BSlider* slider = new BSlider(BRect(0.0, 0.0, 200.0, 0.0), "popUpSlider", NULL, message,
		minRange, maxRange, B_TRIANGLE_THUMB);

	slider->SetTarget(target);
	slider->ResizeToPreferred();
	slider->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	PopUpSlider* popUpSlider = new PopUpSlider(slider->Bounds());
	popUpSlider->fSlider = slider;
	popUpSlider->AddChild(slider);

	return popUpSlider;
}


void
PopUpSlider::Go()
{
	Show();
	PostMessage(B_MOUSE_DOWN, fSlider);
}


BSlider*
PopUpSlider::Slider() const
{
	return fSlider;
}
