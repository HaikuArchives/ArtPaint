/* 

	Filename:	DataQueue.h
	Contents:	DataQueue-class declarations and definitions.	
	Author:		Heikki Suhonen
	
*/



/*
	DataQueue is a class that can be used to share data between threads.
	One thread can write the data to the queue and many threads can read it.
	The access to the queue is exclusive and only one thread gets to read particular
	data. The class uses templates and thus it can be used to store any kind of data.
	There is a possibility to write or read data in different sized batches.
*/

#ifndef DATA_QUEUE_H
#define DATA_QUEUE_H


template<class T> 
class DataQueue {
sem_id			queue_mutex;
int32			front;
int32			end;
int32			items_in_the_queue;
int32			queue_length;		
T				*queue;

public:
			DataQueue(int32 size);
			~DataQueue();

int32		GetItem(T&);
status_t	PutItem(T&);

int32		GetItems(T*,int32 count=1);
status_t	PutItems(T*,int32 count=1);			
};

template<class T> inline DataQueue<T>::DataQueue(int32 size)
{
	queue = new T[size];	
	
	queue_length = size;
	queue_mutex = create_sem(1,"queue mutex");

	front = 0;
	end = 0;
	
	items_in_the_queue = 0;
}



template<class T> inline DataQueue<T>::~DataQueue()
{
	delete[] queue;
}


template<class T> inline int32 DataQueue<T>::GetItem(T &item)
{
	acquire_sem(queue_mutex);
	if (items_in_the_queue == 0) {
		release_sem(queue_mutex);
		return 0;
	}
	else {
		item = queue[front];
		front = (front + 1) % queue_length;
		items_in_the_queue--;
		release_sem(queue_mutex);				
		return 1;
	}		
}



template<class T> inline status_t DataQueue<T>::PutItem(T &item)
{
	// This will not return before it has put the item in the queue
	bool space_found=FALSE;
	while (!space_found) {
		while (items_in_the_queue == queue_length) {
			snooze(20 * 1000);	// Snooze a little bit if the queue is full.
		}
		acquire_sem(queue_mutex);
		if (items_in_the_queue < queue_length) {
			space_found = TRUE;
			queue[end] = item;
			end = (end + 1) % queue_length;
			items_in_the_queue++;	
		}
		release_sem(queue_mutex);
	}	
	return B_OK;
}

template<class T> inline int32 DataQueue<T>::GetItems(T *items,int32 count)
{
	acquire_sem(queue_mutex);
	if (items_in_the_queue == 0) {
		release_sem(queue_mutex);
		return 0;
	}
	else {
		int32 number_of_returned_items = min_c(count,items_in_the_queue);
		for (int32 i=0;i<number_of_returned_items;i++) {
			items[i] = queue[front];
			front = (front + 1) % queue_length;
		}
		items_in_the_queue-=number_of_returned_items;
		release_sem(queue_mutex);				
		return number_of_returned_items;
	}		
}


template<class T> inline status_t DataQueue<T>::PutItems(T *items,int32 count)
{
	// This will not return before it has put the item in the queue
	bool space_found=FALSE;
	while (!space_found) {
		while (items_in_the_queue+count >= queue_length) {
			snooze(1000);	// Snooze a little bit if the queue is full.
		}
		acquire_sem(queue_mutex);
		if (items_in_the_queue+count <= queue_length) {
			space_found = TRUE;
			for (int32 i=0;i<count;i++) {
				queue[end] = items[i];
				end = (end + 1) % queue_length;
			}
			items_in_the_queue += count;	
		}
		release_sem(queue_mutex);
	}	
	return B_OK;
}
#endif