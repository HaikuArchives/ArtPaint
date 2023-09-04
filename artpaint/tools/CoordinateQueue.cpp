/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "CoordinateQueue.h"


CoordinateQueue::CoordinateQueue()
{
	front = rear = 0;
	queue_length = 0;
	queue_semaphore = create_sem(1, "coordinate queue semaphore");
}


CoordinateQueue::~CoordinateQueue()
{
	delete_sem(queue_semaphore);
}


status_t
CoordinateQueue::Get(BPoint& point)
{
	acquire_sem(queue_semaphore);
	if (queue_length == 0) {
		release_sem(queue_semaphore);
		return B_ERROR;
	} else {
		point = queue[rear];
		rear = (rear + 1) % MAX_QUEUE_LENGTH;
		queue_length--;
		release_sem(queue_semaphore);
		return B_OK;
	}
}


status_t
CoordinateQueue::Put(BPoint& point)
{
	acquire_sem(queue_semaphore);
	if (queue_length == MAX_QUEUE_LENGTH) {
		release_sem(queue_semaphore);
		return B_ERROR;
	} else {
		queue[front] = point;
		front = (front + 1) % MAX_QUEUE_LENGTH;
		queue_length++;
		release_sem(queue_semaphore);
		return B_OK;
	}
}
