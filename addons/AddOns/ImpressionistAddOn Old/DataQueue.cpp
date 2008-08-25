/* 

	Filename:	DataQueue.cpp
	Contents:	DataQueue-class definitions	
	Author:		Heikki Suhonen
	
*/

#include "DataQueue.h"


template<class T> DataQueue<T>::DataQueue(int32 size)
{
	queue = new T[size];	
	
	queue_length = size;
	queue_mutex = create_sem(1,"queue mutex");

	front = 0;
	end = 0;
	
	items_in_the_queue = 0;
}



template<class T> DataQueue<T>::~DataQueue()
{
	delete[] queue;
}

template<class T> int32 DataQueue<T>::GetItem(T &item)
{
	acquire_sem(mutex);
	if (items_in_the_queue == 0) {
		release_sem(mutex);
		return 0;
	}
	else {
		item = queue[front];
		front = (front + 1) % queue_length;
		release_sem(mutex);				
		items_in_the_queue--;
		return 1;
	}		
}



template<class T> status_t DataQueue<T>::PutItem(T &item)
{
	// This will not return before it has put the item in the queue
	bool space_found=FALSE;
	while (!space_found) {
		while (items_in_the_queue == queue_length) {
			snooze(20 * 1000);	// Snooze a little bit if the queue is full.
		}
		acquire_sem(mutex);
		if (items_in_the_queue < queue_length) {
			space_found = TRUE;
			queue[end] = item;
			end = (end + 1) % queue_length;
			items_in_the_queue++;	
		}
		release_sem(mutex);
	}	
	return B_OK;
}


