/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */

#include "MessageFilters.h"
#include "FloaterManager.h"


#include <Looper.h>
#include <Message.h>
#include <View.h>
#include <Window.h>


filter_result
window_activation_filter(BMessage* msg, BHandler** handler, BMessageFilter* filter)
{
	if (BWindow* window = dynamic_cast<BWindow*> (filter->Looper()))
		window->Activate(true);

	return B_DISPATCH_MESSAGE;
}


filter_result
message_to_parent(BMessage* msg, BHandler** handler, BMessageFilter* filter)
{
	if (BLooper* looper = filter->Looper()) {
		BView* view = dynamic_cast<BView*>(*handler);
		if (view && view->Parent())
			looper->PostMessage(msg, view->Parent());
	}

	return B_DISPATCH_MESSAGE;
}


filter_result
AppKeyFilterFunction(BMessage* msg, BHandler** handler, BMessageFilter* filter)
{
	if ((modifiers() & B_COMMAND_KEY)) {
		const char* bytes;
		if (msg->FindString("bytes", &bytes) == B_OK) {
			if (bytes[0] == B_TAB)
				FloaterManager::ToggleFloaterVisibility();
		}
	}
	return B_DISPATCH_MESSAGE;
}
