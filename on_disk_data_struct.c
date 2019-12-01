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

//SUPER BLOCK section
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

//I-NODE section
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

//ROOT DIR section
int init_root_dir() {

	inode *root_dir_i_node = init_i_node(1); //Create root dir i-node (MUST CACHE)

	root_dir.i_node_index = -1;
	root_dir.filename = NULL;
	root_dir.next = NULL;

	last_entry = &root_dir;
	return 0;
}

void* read_root_dir() {

	return NULL;
}

int write_root_dir() {


	return 0;
}

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

		root_dir_entry->filename = (char*) calloc(strlen(filename) + 1, sizeof(char));
		memcpy(root_dir_entry->filename, filename, strlen(filename));
		root_dir_entry->i_node_index = i_node;
		root_dir_entry->next = NULL;

		//Assign pointer to next file in the directory (linked list)
		last_entry->next = root_dir_entry;
		last_entry = last_entry->next;
	}

	return 0;
}

