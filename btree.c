/*
 * CSC 450 Programming Assignment 3
 * 
 * Written By: John Hawkins & Jacob Burt
 * Date: 2/19/14
 */
#include "btree.h"

/*
 * Clears a node and its children and frees them from memory
 * 
 * Parameters:
 * 		- node : The node to clear
 */
void clearNode(struct ordered_btree_node *node)
{
	if(node != NULL) {
		clearNode(node->left);
		clearNode(node->right);
		free(node->data);
		free(node);
	}
}

/*
 * Writes the data of the node and its children in the correct sequence
 * 
 * Parameters:
 * 		- node : The node to write data from
 * 		- file : The file stream to write to
 */
void writeNode(struct ordered_btree_node *node, FILE *file)
{
	if(node != NULL) {
		writeNode(node->left, file);
		int tot = 0;
		while(tot < node->data_len) {
			int temp = fwrite(node->data, 1, node->data_len - tot, file);
			fflush(file);
			if(temp < 0) {
				perror("Unable to write node to file");
				exit(errno);
			}
			tot += temp;
		}
		writeNode(node->right, file);
	}
}

/*
 * Check if a node or its children has the sequence number given
 * 
 * Parameters:
 * 		- node : The node to check
 * 		- seq_num : The sequence number to search for
 * 
 * Returns:
 * 		- int : 1 if sequence number was found, 0 otherwise
 */
int nodeContains(struct ordered_btree_node *node, unsigned int seq_num)
{
	if(node != NULL) {
		int num = node->seq_num - seq_num;
		if(num < 0) {
			return nodeContains(node->right, seq_num);
		}else if(num > 0) {
			return nodeContains(node->left, seq_num);
		}else {
			return 1;
		}
	}
	return 0;
}

/*
 * Add an new node as a correctly child node of this node
 * 
 * Parameters:
 * 		- node : The root node
 * 		- nnode : the new node to add
 */
void addToNode(struct ordered_btree_node *node, struct ordered_btree_node *nnode)
{
	if(node != NULL && nnode != NULL) {
		if(nnode->seq_num < node->seq_num) {
			if(node->left == NULL) {
				nnode->parent = node;
				node->left = nnode;
			}else {
				addToNode(node->left, nnode);
			}
		}else {
			if(node->right == NULL) {
				nnode->parent = node;
				node->right = nnode;
			}else {
				addToNode(node->right, nnode);
			}
		}
	}
}

/*
 * Find the smallest sequence number within this node and its children
 * 
 * Parameters:
 * 		- node : The node to search in
 * 
 * Returns:
 * 		- unsigned int : the smallest sequence number found in this node and its children
 */
unsigned int getSmallestSequenceNumberInNode(struct ordered_btree_node *node)
{
	unsigned int num = node->seq_num;
	if(node->left != NULL) {
		unsigned int num2 = getSmallestSequenceNumberInNode(node->left);
		if(num2 < num) {
			return num2;
		}
	}
	return num;
}

/*
 * Find the largest sequence number within this node and its children
 * 
 * Parameters:
 * 		- node : The node to search in
 * 
 * Returns:
 * 		- unsigned int : the largest sequence number found in this node and its children
 */
unsigned int getLargestSequenceNumberInNode(struct ordered_btree_node *node)
{
	unsigned int num = node->seq_num;
	if(node->right != NULL) {
		unsigned int num2 = getLargestSequenceNumberInNode(node->right);
		if(num2 > num) {
			return num2;
		}
	}
	return num;
}

/*
 * Initializes an empty binary tree
 * 
 * Parameters:
 * 		- tree : The tree to intialize
 */
void initBtree(struct ordered_btree *tree)
{
	tree->size = 0;
	tree->root = NULL;
}

/*
 * Writes the contents of the binary tree to a file in order of sequence number
 * 
 * Parameters:
 * 		- tree : The binary tree to write to file
 * 		- file : The file to write to
 */ 
void writeTree(struct ordered_btree *tree, FILE *file)
{
	writeNode(tree->root, file);
}

/*
 * Clears the whole tree and re-initializes it to empty
 * 
 * Parameters:
 * 		- tree : the binary tree to clear
 */ 
void clear(struct ordered_btree *tree)
{
	clearNode(tree->root);
	initBtree(tree);
}

/*
 * Adds data to the binary tree in the appropriate order
 * 
 * Parameters:
 * 		- tree : The binary tree to add to
 * 		- seq_num : The sequence number to search for
 * 		- data : The data to be stored
 * 		- data_len : The size of the data in bytes
 */
void add(struct ordered_btree *tree, unsigned int seq_num, char *data, int data_len)
{
	struct ordered_btree_node *nnode = (struct ordered_btree_node*) malloc(sizeof(struct ordered_btree_node));
	nnode->data_len = data_len;
	nnode->data = data;
	nnode->seq_num = seq_num;
	nnode->left = NULL;
	nnode->right = NULL;
	
	if(tree->root == NULL) {
		tree->root = nnode;
	}else {
		addToNode(tree->root, nnode);
	}
	tree->size++;
}

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
int contains(struct ordered_btree *tree, unsigned int seq_num)
{
	return nodeContains(tree->root, seq_num);
}

/*
 * Get the smallest sequence number stored in the binary tree
 * 
 * Parameters:
 * 		- tree : the binary tree to search
 * 
 * Returns:
 * 		- unsigned int : the smallest sequence number in the tree
 */
unsigned int getMinSequenceNumber(struct ordered_btree *tree)
{
	if(tree->root != NULL) {
		return getSmallestSequenceNumberInNode(tree->root);
	}
	return 0;
}

/*
 * Get the largest sequence number stored in the binary tree
 * 
 * Parameters:
 * 		- tree : the binary tree to search
 * 
 * Returns:
 * 		- unsigned int : the largest sequence number in the tree
 */
unsigned int getMaxSequenceNumber(struct ordered_btree *tree)
{
	if(tree->root != NULL) {
		return getLargestSequenceNumberInNode(tree->root);
	}
	return 0;
}
