/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */

#include "ToolEventAdapter.h"


#include <string.h>


ToolEventAdapter::ToolEventAdapter()
{
	is_clicks_data_valid = FALSE;
	last_click_bitmap_location = BPoint(0, 0);
	last_click_view_location = BPoint(0, 0);
	last_click_buttons = 0;
	last_click_clicks = 0;

	is_keys_data_valid = FALSE;
	last_key_event_bytes = NULL;
	last_key_event_num_bytes = 0;

	is_view_data_valid = FALSE;
	active_image_view = NULL;
	is_mouse_inside_the_active_view = FALSE;
}


ToolEventAdapter::~ToolEventAdapter()
{
	delete last_key_event_bytes;
}


void
ToolEventAdapter::SetClickEvent(
	BPoint view_point, BPoint bitmap_point, uint32 buttons, int32 clicks)
{
	last_click_bitmap_location = bitmap_point;
	last_click_view_location = view_point;
	last_click_buttons = buttons;
	last_click_clicks = clicks;
	is_clicks_data_valid = TRUE;
}


void
ToolEventAdapter::SetKeyEvent(const char* bytes, int32 num_bytes)
{
	delete last_key_event_bytes;

	last_key_event_bytes = new char[num_bytes];
	strncpy(last_key_event_bytes, bytes, num_bytes);
	last_key_event_num_bytes = num_bytes;
	is_keys_data_valid = TRUE;
}


bool
ToolEventAdapter::SetActiveImageView(BView* view)
{
	if (active_image_view != view) {
		active_image_view = view;
		is_view_data_valid = TRUE;
		return TRUE;
	} else {
		is_view_data_valid = TRUE;
		return FALSE;
	}
}


bool
ToolEventAdapter::SetIsMouseInsideActiveView(bool is_inside)
{
	if (is_mouse_inside_the_active_view != is_inside) {
		is_mouse_inside_the_active_view = is_inside;
		is_view_data_valid = TRUE;
		return TRUE;
	} else {
		is_view_data_valid = TRUE;
		return FALSE;
	}
}
