/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef UNDO_WINDOW_H
#define UNDO_WINDOW_H


#include <Rect.h>
#include <View.h>
#include <Window.h>


class UndoWindow : public BWindow {
public:
		UndoWindow(BRect,char*);
		~UndoWindow();

void		MessageReceived(BMessage*);
};


class UndoView : public BView {
public:
		UndoView(BRect);
		~UndoView();

void		AttachedToWindow();
void		MouseDown(BPoint);

};

#endif
