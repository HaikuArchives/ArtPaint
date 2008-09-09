/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef POP_UP_COLOR_WINDOW_H
#define POP_UP_COLOR_WINDOW_H

#define	BLUE_VALUE_CHANGED	'bvCh'
#define	COLOR_SELECTED		'clSe'

#include <View.h>
#include <Window.h>

/*
	PopUpColorWindow displays a window that can be used to pick one color.
	It will close itself when de-activated or after a color is selected.
	It should be possible to pass the mouse-down message straight to this window.
	When color is selected a message is passed to designated target.
*/

class ControlSliderBox;


class PopUpColorWindow : public BWindow {
		BMessenger	*target;
		BMessage	*default_message;

public:
		PopUpColorWindow(BRect,BMessenger*,BMessage *default_out_message=NULL);
		~PopUpColorWindow();

void	MessageReceived(BMessage*);
void	WindowActivated(bool);
};


class ColorWell : public BView {
		BBitmap 			*color_map;
		uint32				blue_value;
		BMessenger			*target;

		ControlSliderBox	*slider;
virtual	void	create_color_map();
public:
		ColorWell(BRect frame,BMessenger*);
		~ColorWell();
void	AttachedToWindow();
void	Draw(BRect);
void 	MessageReceived(BMessage*);
void	MouseDown(BPoint location);
};
#endif
