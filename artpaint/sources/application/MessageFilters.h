/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef MESSAGE_FILTERS_H
#define MESSAGE_FILTERS_H


#include <MessageFilter.h>


filter_result	window_activation_filter(BMessage*, BHandler**, BMessageFilter*);
filter_result	message_to_parent(BMessage*,BHandler**,BMessageFilter*);
filter_result	test_filter(BMessage*,BHandler**,BMessageFilter*);


#endif
