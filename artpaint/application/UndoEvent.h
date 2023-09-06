/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef UNDO_EVENT_H
#define	UNDO_EVENT_H


#include <String.h>


#include "UndoAction.h"
#include "Layer.h"


class UndoQueue;
class SelectionData;


class UndoEvent {
friend 	class		UndoQueue;
		UndoAction	**actions;
		int32		action_count;
		int32		max_action_count;
		UndoQueue	*queue;

		BString		event_name;
		BBitmap		*thumbnail_image;


		UndoEvent	*next_event;
		UndoEvent	*previous_event;

	SelectionData	*selection_data;
		BBitmap*	selection_map;

		Layer*		layer_data;

public:
		UndoEvent(const BString& name, const BBitmap *thumbnail);
		~UndoEvent();


void			AddAction(UndoAction *action);

UndoAction**	ReturnActions();
int32			ActionCount();

void			SetQueue(UndoQueue *q) { queue = q; }
bool			IsEmpty();

BString			ReturnName() { return event_name; }
BBitmap*		ReturnThumbnail() { return thumbnail_image; }

SelectionData*	ReturnSelectionData() { return selection_data; }
void			SetSelectionData(const SelectionData*);

BBitmap*		ReturnSelectionMap() { return selection_map; }
void			SetSelectionMap(const BBitmap* new_map);

void			SetLayerData(Layer*);
Layer*			ReturnLayerData() { return layer_data; }
};

#endif
