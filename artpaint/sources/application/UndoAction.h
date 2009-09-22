/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef UNDO_ACTION_H
#define	UNDO_ACTION_H


#include <Rect.h>

#include "ToolScript.h"
#include "Manipulator.h"


// Some of the action types are distinguished just to allow
// the repeating of those actions. For example MERGE_LAYER_ACTION is
// such.
enum action_type {
	NO_ACTION,
	TOOL_ACTION,
	MANIPULATOR_ACTION,
	ADD_LAYER_ACTION,
	DELETE_LAYER_ACTION,
	CLEAR_LAYER_ACTION,
	CHANGE_LAYER_CONTENT_ACTION,
	MERGE_LAYER_ACTION
};



class UndoQueue;
class UndoEvent;

class UndoAction {
	friend 	class	UndoQueue;
	BRect	 			bounding_rect;
	BBitmap 			**undo_bitmaps;
	BRect				*undo_rects;
	int32 				undo_bitmap_count;

	int32				layer_id;
	int32				merged_layer_id;
	action_type			type;


	ToolScript			*tool_script;
	ManipulatorSettings	*manipulator_settings;
	manipulator_type	manip_type;
	int32				add_on_id;

	UndoQueue			*queue;
	UndoEvent			*event;

	bool				size_has_changed;

	void				StoreDifferences(BBitmap *old,BBitmap *current,BRect area);
	BRect				RestoreDifference(BBitmap*,BBitmap*);

	enum bitmap_difference {
		NO_DIFFERENCE,
		SLIGHT_DIFFERENCE,
		GREAT_DIFFERENCE
	};


public:
	UndoAction(int32 layer,action_type t=NO_ACTION,BRect rect=BRect(0,0,-1,-1));
	UndoAction(int32 layer,int32 merged_layer,BRect rect=BRect(0,0,-1,-1));
	UndoAction(int32 layer,ToolScript *script,BRect rect);
	UndoAction(int32 layer,ManipulatorSettings*,BRect rect,manipulator_type type,int32 aoid=-1);

	~UndoAction();

status_t	StoreUndo(BBitmap*);
BBitmap*	ApplyUndo(BBitmap*,BRect&);


void		SetEvent(UndoEvent *e) { event = e; }
void		SetQueue(UndoQueue *q) { queue = q; }

int32		LayerId() { return layer_id; }
bool		IsEmpty() { return type == NO_ACTION; }
};


#endif
