/*
 * CSC 450 Programming Assignment 3
 * 
 * Written By: John Hawkins & Jacob Burt
 * Date: 2/19/14
 */
#ifndef __ORDERDED_BTREE
#define __ORDERDED_BTREE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

struct ordered_btree_node {
	
	unsigned int seq_num;
	int data_len;
	char *data;
	struct ordered_btree_node *parent;
	struct ordered_btree_node *left;
	struct ordered_btree_node *right;
	
};

struct ordered_btree {
	
	int size;
	struct ordered_btree_node *root;
	
};

/*
 * Initializes an empty binary tree
 * 
 * Parameters:
 * 		- tree : The tree to intialize
 */
void initBtree(struct ordered_btree *tree);

/*
 * Writes the contents of the binary tree to a file in order of sequence number
 * 
 * Parameters:
 * 		- tree : The binary tree to write to file
 * 		- file : The file to write to
 */ 
void writeTree(struct ordered_btree *tree, FILE *file);

/*
 * Clears the whole tree and re-initializes it to empty
 * 
 * Parameters:
 * 		- tree : the binary tree to clear
 */ 
void clear(struct ordered_btree *tree);

/*
 * Adds data to the binary tree in the appropriate order
 * 
 * Parameters:
 * 		- tree : The binary tree to add to
 * 		- seq_num : The sequence number to search for
 * 		- data : The data to be stored
 * 		- data_len : The size of the data in bytes
 */
void add(struct ordered_btree *tree, unsigned int seq_num, char *data, int data_len);

/*
 * Check if the binary tree has the sequence number given
 * 
 * Parameters:
 * 		- tree : The binary tree to check in
 * 		- seq_num : The sequence number to search for
 * 
 * Returns:
 * 		- int : 1 if sequence number was found, 0 otherwise
 */
int contains(struct ordered_btree *tree, unsigned int seq_num);

/*
 * Get the smallest sequence number stored in the binary tree
 * 
 * Parameters:
 * 		- tree : the binary tree to search
 * 
 * Returns:
 * 		- unsigned int : the smallest sequence number in the tree
 */
unsigned int getMinSequenceNumber(struct ordered_btree *tree);

/*
 * Get the largest sequence number stored in the binary tree
 * 
 * Parameters:
 * 		- tree : the binary tree to search
 * 
 * Returns:
 * 		- unsigned int : the largest sequence number in the tree
 */
unsigned int getMaxSequenceNumber(struct ordered_btree *tree);

#endif
