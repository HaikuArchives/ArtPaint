/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */

/*
	This class gives an interface for a queue where a thread can add
	coordinates or get them. It works as FIFO-system. If no coordinates
	are available when trying to get new, it will return B_ERROR
	If there is no room to put new value, it will also return B_ERROR.
	In no case will the access to queue block.
*/
#ifndef COORDINATE_QUEUE_H
#define COORDINATE_QUEUE_H

#include <OS.h>
#include <Point.h>

#define	MAX_QUEUE_LENGTH 100


class CoordinateQueue {
	sem_id		queue_semaphore;

	int32		queue_length;	// The number of items in queue;

	BPoint		queue[MAX_QUEUE_LENGTH];
	int32		front,rear;

public:
				CoordinateQueue();
				~CoordinateQueue();		// This destroys the remaining points.

	status_t	Get(BPoint&);
	status_t	Put(BPoint&);
};


#endif // COORDINATE_QUEUE_H
