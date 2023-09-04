/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "UndoWindow.h"
#include "Message.h"


UndoWindow::UndoWindow(BRect rect, char* title)
	:
	BWindow(rect, title, B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE)
{
}


UndoWindow::~UndoWindow()
{
}


void
UndoWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


UndoView::UndoView(BRect rect)
	:
	BView(rect, "undo_view", B_FOLLOW_ALL_SIDES, 0)
{
}


UndoView::~UndoView()
{
}


void
UndoView::AttachedToWindow()
{
	BView::AttachedToWindow();
}


void UndoView::MouseDown(BPoint)
{
}
