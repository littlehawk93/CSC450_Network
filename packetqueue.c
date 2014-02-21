/*
 * CSC 450 Programming Assignment 3
 * 
 * Written By: John Hawkins & Jacob Burt
 * Date: 2/19/14
 */
#include "packetqueue.h"

/*
 * Initialize the queue to empty
 * 
 * Parameters:
 * 		- q : The queue to initialize
 */
void initQueue(struct queue *q)
{
	q->size = 0;
	q->head = NULL;
	q->tail = NULL;
}

/*
 * Add an element to the tail of the queue
 * 
 * Parameters:
 * 		- q : The queue to enqueue to
 * 		- elem : The element to enqueue
 */
void enqueue(struct queue *q,void *elem)
{
	struct queue_node *node = (struct queue_node*) malloc(sizeof(struct queue_node));
	node->elem = elem;
	if(q->size < 1) {
		node->prev = NULL;
		node->next = NULL;
		q->head = node;
		q->tail = q->head;
	}else {
		q->tail->prev = node;
		node->next = q->tail;
		node->prev = NULL;
		q->tail = node;
	}
	q->size++;
}

/*
 * Get the value of the element at the head of the queue, without removing it
 * 
 * Parameters:
 * 		- q : The queue to peek at
 * 
 * Returns:
 * 		- void* : The element at the head
 */
void* dequeue(struct queue *q)
{
	if(q->size < 1) {
		return NULL;
	}
	void *ptr;
	
	ptr = q->head->elem;
	
	if(q->size == 1) {
		q->head = NULL;
		q->tail = NULL;
	}else {
		q->head = q->head->prev;
		q->head->next = NULL;
	}
	q->size--;
	return ptr;
}

/*
 * Get the value of the element at the head of the queue and remove it
 * 
 * Parameters:
 * 		- q : The queue to dequeue from
 * 
 * Returns:
 * 		- void* : The element at the head
 */
void* peek(struct queue *q)
{
	if(q->size < 1) {
		return NULL;
	}
	return q->head->elem;
}
