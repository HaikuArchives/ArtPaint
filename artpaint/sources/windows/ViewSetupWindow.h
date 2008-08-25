/* 

	Filename:	ViewSetupWindow.h
	Contents:	ViewSetupWindow class declaration	
	Author:		Heikki Suhonen
	
*/


#ifndef	VIEW_SETUP_WINDOW_H
#define	VIEW_SETUP_WINDOW_H

#include <Window.h>

class RGBControl;

// this class constructs a window that is used to set up
// the ImageView's and PaintWindow's parameters such as
// selected area mask color and pattern 
// Paintwindows control this window with the static member functions.

class ViewSetupWindow : public BWindow {
static	BWindow			*target_window;
static	BView			*target_view;

static	ViewSetupWindow	*setup_window;

		RGBControl		*rgb_control;
	
public:
	ViewSetupWindow(BRect frame);
	~ViewSetupWindow();

// The first of these functions will show the set-up window
// and it will change the contents to the parameter window's
// settings. The second function will close this window if the
// parameter window is same as target_window. Settings are always
// stored whenever the target_window changes or this window is
// closed.
static	void	showViewSetupWindow(BWindow*,BView*);
static	void	closeViewSetupWindow(BWindow*);
};


#endif