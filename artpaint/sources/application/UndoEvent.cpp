/* 

	Filename:	UndoEvent.cpp	
	Contents:	UndoEvent-class definitions	
	Author:		Heikki Suhonen
	
*/

#include <stdio.h>

#include "UndoEvent.h"


UndoEvent::UndoEvent(const char *name, const BBitmap*)
	: event_name(name)
{
	actions = NULL;
	action_count = 0;
	max_action_count = 0;

	thumbnail_image = NULL;
	
	next_event = NULL;
	previous_event = NULL;

	selection_data = NULL;
}


UndoEvent::~UndoEvent()
{
	delete thumbnail_image;
	
	for (int32 i=0;i<action_count;i++) {
		delete actions[i];
		actions[i] = NULL;
	}
	delete[] actions;

	delete selection_data;
}


void UndoEvent::AddAction(UndoAction *action)
{
	if (actions == NULL) {
		max_action_count = 1;
		actions = new UndoAction*[max_action_count];
	}

	if (action_count == max_action_count) {
		max_action_count *= 2;
		UndoAction **new_actions = new UndoAction*[max_action_count];
		for (int32 i=0;i<action_count;i++) {
			new_actions[i] = actions[i];
			actions[i]= NULL;
		}
		delete[] actions;
		actions = new_actions;
	}
	action->SetQueue(queue);
	action->SetEvent(this);	
	actions[action_count++] = action;
}


UndoAction** UndoEvent::ReturnActions()
{
	return actions;
}


int32 UndoEvent::ActionCount()
{
	return action_count;
}



bool UndoEvent::IsEmpty()
{
	if (action_count == 0)
		return TRUE;

	bool is_empty = TRUE;
	for (int32 i=0;i<action_count;i++) {
		if (actions[i]->IsEmpty() == FALSE)
			is_empty = FALSE;	
	}	
	return is_empty;
}


void UndoEvent::SetSelectionData(const SelectionData *s)
{
	delete selection_data;
	selection_data = new SelectionData(s);
}