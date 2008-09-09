/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef MANIPULATOR_WINDOW_H
#define MANIPULATOR_WINDOW_H

#include <Window.h>

class ManipulatorWindow : public BWindow {
static	BList		*window_list;
static	sem_id		list_mutex;

static	window_feel	feel;
static	window_look	look;

		BView		*manipulator_view;
		BMessenger	*target;

public:
		ManipulatorWindow(BRect,BView*,char*,BWindow *master,BMessenger*);
		~ManipulatorWindow();



void	static	setFeel(window_feel);
};



class ManipulatorWindowBackgroundView : public BView {
public:
		ManipulatorWindowBackgroundView(BRect rect)
			: BView(rect,"background-view",B_FOLLOW_ALL_SIDES,B_WILL_DRAW) {}


void	Draw(BRect r) {
			BView::Draw(r);
			BView *view = FindView("ok_button");
			if (view != NULL) {
				float top = view->Frame().top - 10;
				SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),B_DARKEN_4_TINT));
				StrokeLine(BPoint(0,top),BPoint(Bounds().right,top));
				SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),B_LIGHTEN_2_TINT));
				StrokeLine(BPoint(0,top+1),BPoint(Bounds().right,top+1));
			}
		}
};
#endif

