/*
 * on_disk_data_struct.c
 *
 *  Created on: Nov 29, 2019
 *      Author: nicolas
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "on_disk_data_struct.h"
#include "disk_emu.h"

//SUPER BLOCK vars
#define MAGIC 0x4e49434f	//"NICO"
#define SUPER_BLK_ADDR 0
int BLOCK_SIZE;

//ROOT DIR vars
root_dir_t *last_entry;
root_dir_t *current_entry;

/*------------------------------------------------------------------*/
/*Super block section											    */
/*------------------------------------------------------------------*/

int init_super_blk(int blk_size, int sfs_size, int root_dir_i_node) {

	printf("Initializing super block.\n");
	BLOCK_SIZE = blk_size;

	super_blk.magic = MAGIC;
	super_blk.blk_size = blk_size;
	super_blk.sfs_size = sfs_size;
	super_blk.root_dir_i_node = root_dir_i_node;

	return 0;
}

int* read_super_blk() {

	int *buffer = (int*) malloc(BLOCK_SIZE);

	read_blocks(SUPER_BLK_ADDR, 1, buffer);
	return buffer;
}

int write_super_blk() {

	printf("Writing super block.\n");
	int *buffer = (int*) malloc(BLOCK_SIZE);

	*buffer = super_blk.magic;
	*(buffer + 1) = super_blk.blk_size;
	*(buffer + 2) = super_blk.sfs_size;
	*(buffer + 3) = super_blk.root_dir_i_node;

	write_blocks(SUPER_BLK_ADDR, 1, buffer);
	free(buffer);
	return 0;
}

/*------------------------------------------------------------------*/
/*Inode section											            */
/*------------------------------------------------------------------*/

inode* init_i_node(int blk) {

	inode *new_i_node = (inode*) malloc(sizeof(inode));

	new_i_node->mode = 0;
	new_i_node->ln_count = 0;
	new_i_node->gid = 0;
	new_i_node->uid = 0;
	new_i_node->size = 0;

	for (int i = 0; i < 12; i++)
		new_i_node->data_blk[i] = -1;

	new_i_node->ind_pointer = -1;

	write_i_node(new_i_node, blk);

	return new_i_node;
}

int* read_i_node(int blk) {

	int *buffer = (int*) malloc(BLOCK_SIZE);

	read_blocks(blk, 1, buffer);
	return buffer;
}

int write_i_node(inode *i_node, int blk) {

	int *buffer = (int*) malloc(BLOCK_SIZE);

	buffer[0] = i_node->mode;
	buffer[1] = i_node->ln_count;
	buffer[2] = i_node->gid;
	buffer[3] = i_node->uid;
	buffer[4] = i_node->size;

	for (int i = 0; i < 12; i++)
		buffer[5 + i] = i_node->data_blk[i];

	buffer[17] = i_node->ind_pointer;

	write_blocks(blk, 1, buffer);
	free(buffer);
	return 0;
}

/*------------------------------------------------------------------*/
/*Root Dir section										            */
/*------------------------------------------------------------------*/

int init_root_dir() {

	inode *root_dir_i_node = init_i_node(1); //Create root dir i-node (MUST CACHE) TODO replace 1 with allocate

	root_dir.i_node_index = -1;
	root_dir.filename = NULL;
	root_dir.next = NULL;

	last_entry = &root_dir;
	current_entry = &root_dir;
	return 0;
}

//Iterator for root directory
int sfs_getnextfilename(char *fname) {

	//No more files
	if (current_entry->next == NULL)
		return -1;

	current_entry = current_entry->next;
	fname = (char*) calloc(strlen(current_entry->filename) + 1, sizeof(char));
	memcpy(fname, current_entry->filename, strlen(current_entry->filename));

	return 0;
}

//Transforms the directory table into a buffer that will be written to disk
//Format: inode1,filename1,inode2,filename2,inode3,filename3...
char* get_rootdir_buffer() {

	char *buffer;
	char *entry_filename;
	int entry_inode;

	int offset = 0;
	int nb_blocks = 1;		// #blocks currently needed to store buffer in disk

	//If there are no files, buffer should be empty
	if (current_entry->filename == NULL)
		return NULL;

	buffer = (char*) malloc(BLOCK_SIZE);	//Start by allocating only 1 block

	do {
		entry_filename = current_entry->filename;
		entry_inode = current_entry->i_node_index;

		printf("filename: %s, entry_inode: %d\n", entry_filename, entry_inode);
		//Check for overflow before copying
		if (offset + sizeof(int) + strlen(entry_filename) + 2 > nb_blocks * BLOCK_SIZE)
			buffer = realloc(buffer, ++nb_blocks * BLOCK_SIZE);

		//Copy i_node index of entry to buffer
		memcpy(buffer + offset, &entry_inode, sizeof(int));
		offset += sizeof(int);

		//Add comma
		*(buffer + offset++) = ',';

		//Copy filename of entry to buffer
		memcpy(buffer + offset, entry_filename, strlen(entry_filename));
		offset += strlen(entry_filename);

		//Add comma
		*(buffer + offset++) = ',';

		current_entry = current_entry->next;
	} while (current_entry != NULL);

	current_entry = &root_dir; //Restore current entry to first entry
	return buffer;
}

int write_root_dir(inode *root_dir_inode) {

//	char *buffer = (char*) calloc(BLOCK_SIZE, sizeof(char));
//	int count
//
//	while (sfs_getnextfilename())

	return 0;
}

//Adds entries (2 elements per entry: i_node, filename) to the root directory
int add_root_dir_entry(int i_node, char *filename) {

	if (root_dir.filename == NULL) {	//First entry in the directory
		printf("first entry\n");

		root_dir.filename = (char*) calloc(strlen(filename) + 1, sizeof(char));
		memcpy(root_dir.filename, filename, strlen(filename));
		root_dir.i_node_index = i_node;
		root_dir.next = NULL;
	} else {

		printf("not first entry\n");
		root_dir_t *root_dir_entry = (root_dir_t*) malloc(sizeof(root_dir_t));

		root_dir_entry->filename = (char*) calloc(strlen(filename) + 1,
				sizeof(char));
		memcpy(root_dir_entry->filename, filename, strlen(filename));
		root_dir_entry->i_node_index = i_node;
		root_dir_entry->next = NULL;

		//last_entry points to file that was just added to linked list
		last_entry->next = root_dir_entry;
		last_entry = last_entry->next;
	}

	return 0;
}

