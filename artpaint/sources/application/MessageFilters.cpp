/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Looper.h>
#include <stdio.h>
#include <View.h>
#include <Window.h>
#include <ClassInfo.h>

#include "MessageFilters.h"

filter_result window_activation_filter(BMessage*, BHandler**, BMessageFilter *messageFilter)
{
	BWindow *window = cast_as(messageFilter->Looper(),BWindow);
	if (window != NULL)
		window->Activate(TRUE);

	return B_DISPATCH_MESSAGE;
}


filter_result message_to_parent(BMessage *msg,BHandler **handler,BMessageFilter *messageFilter)
{
	BView *view = dynamic_cast<BView*>(*handler);
	BLooper *looper = messageFilter->Looper();

	if ((view != NULL) && (view->Parent() != NULL)) {
		view = view->Parent();
		if (looper != NULL)
				looper->PostMessage(msg,view);

	}

	return B_DISPATCH_MESSAGE;
}


filter_result test_filter(BMessage*,BHandler**,BMessageFilter*)
{
	printf("Test Filter\n");

	return B_SKIP_MESSAGE;
}
