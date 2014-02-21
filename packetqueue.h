/*
 * CSC 450 Programming Assignment 3
 * 
 * Written By: John Hawkins & Jacob Burt
 * Date: 2/19/14
 */
#ifndef __PACKETQUEUE
#define __PACKETQUEUE

#include <stdlib.h>

/*
 * Defines a single node in the queue
 */
struct queue_node {
	
	void *elem;
	struct queue_node *prev;
	struct queue_node *next;
	
};

/*
 * Defines a queue
 */
struct queue {

	unsigned int size;
	struct queue_node *head;
	struct queue_node *tail;

};

/*
 * Initialize the queue to empty
 * 
 * Parameters:
 * 		- q : The queue to initialize
 */
void initQueue(struct queue *q);

/*
 * Add an element to the tail of the queue
 * 
 * Parameters:
 * 		- q : The queue to enqueue to
 * 		- elem : The element to enqueue
 */
void enqueue(struct queue *q, void *elem);

/*
 * Get the value of the element at the head of the queue, without removing it
 * 
 * Parameters:
 * 		- q : The queue to peek at
 * 
 * Returns:
 * 		- void* : The element at the head
 */
void* peek(struct queue *q);

/*
 * Get the value of the element at the head of the queue and remove it
 * 
 * Parameters:
 * 		- q : The queue to dequeue from
 * 
 * Returns:
 * 		- void* : The element at the head
 */
void* dequeue(struct queue *q);

#endif
