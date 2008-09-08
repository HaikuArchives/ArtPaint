/*

	Filename:	MessageFilters.h
	Contents:	MessageeFilter-function declarations
	Author:		Heikki Suhonen

*/



#ifndef MESSAGE_FILTERS_H
#define MESSAGE_FILTERS_H

#include <MessageFilter.h>

filter_result	window_activation_filter(BMessage*, BHandler**, BMessageFilter*);
filter_result	message_to_parent(BMessage*,BHandler**,BMessageFilter*);
filter_result	test_filter(BMessage*,BHandler**,BMessageFilter*);

#endif
