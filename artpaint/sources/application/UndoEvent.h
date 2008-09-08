/*

	Filename:	UndoEvent.h
	Contents:	UndoEvent-class declarations
	Author:		Heikki Suhonen

*/



#ifndef UNDO_EVENT_H
#define	UNDO_EVENT_H


#include "UndoAction.h"

class UndoQueue;

class UndoEvent {
friend 	class		UndoQueue;
		UndoAction	**actions;
		int32		action_count;
		int32		max_action_count;
		UndoQueue	*queue;

const	char		*event_name;
		BBitmap		*thumbnail_image;


		UndoEvent	*next_event;
		UndoEvent	*previous_event;

	SelectionData	*selection_data;

public:
		UndoEvent(const char *name, const BBitmap *thumbnail);
		~UndoEvent();


void			AddAction(UndoAction *action);

UndoAction**	ReturnActions();
int32			ActionCount();

void			SetQueue(UndoQueue *q) { queue = q; }
bool			IsEmpty();


const	char*	ReturnName() { return event_name; }
BBitmap*		ReturnThumbnail() { return thumbnail_image; }

SelectionData*	ReturnSelectionData() { return selection_data; }
void			SetSelectionData(const SelectionData*);
};

#endif
