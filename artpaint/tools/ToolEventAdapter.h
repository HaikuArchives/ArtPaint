/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef TOOL_EVENT_ADAPTER_H
#define	TOOL_EVENT_ADAPTER_H

#include <Point.h>
#include <View.h>

/*
	This class defines an interface that ToolManager can use to
	inform tools about events that happen in the views while the
	tool is in the UseTool-function. If a tool wants to receive such
	information it should inherit also from this class.
*/

class ToolEventAdapter {
protected:
		bool	is_clicks_data_valid;
		BPoint	last_click_bitmap_location;
		BPoint	last_click_view_location;
		uint32	last_click_buttons;
		int32	last_click_clicks;

		bool 	is_keys_data_valid;
		char*	last_key_event_bytes;
		int32	last_key_event_num_bytes;


		bool	is_view_data_valid;
		BView*	active_image_view;
		bool	is_mouse_inside_the_active_view;


public:
				ToolEventAdapter();
				~ToolEventAdapter();

		void	SetClickEvent(BPoint, BPoint, uint32, int32);
		void	SetKeyEvent(const char*, int32);

		bool	SetActiveImageView(BView*);
		bool	SetIsMouseInsideActiveView(bool);
};


#endif // TOOL_EVENT_ADAPTER_H
