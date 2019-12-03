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

int init_super_blk(int blk_size, int sfs_size, int root_dir_i_node,
		int nb_files) {

	printf("Initializing super block.\n");
	BLOCK_SIZE = blk_size;

	super_blk.magic = MAGIC;
	super_blk.blk_size = blk_size;
	super_blk.sfs_size = sfs_size;
	super_blk.root_dir_i_node = root_dir_i_node;
	super_blk.nb_files = nb_files;

	return 0;
}

int read_super_blk() {

	struct super_blk_t *super_blk_temp = (struct super_blk_t*) malloc(
			BLOCK_SIZE);
	read_blocks(SUPER_BLK_ADDR, 1, super_blk_temp);

	memcpy(&super_blk, super_blk_temp, sizeof(struct super_blk_t));
	free(super_blk_temp);
	return 0;
}

int write_super_blk() {

	printf("Writing super block.\n");
	struct super_blk_t *buffer_temp = (struct super_blk_t*) malloc(BLOCK_SIZE);
	memcpy(buffer_temp, &super_blk, sizeof(struct super_blk_t));
//	*buffer = super_blk.magic;
//	*(buffer + 1) = super_blk.blk_size;
//	*(buffer + 2) = super_blk.sfs_size;
//	*(buffer + 3) = super_blk.root_dir_i_node;
//	*(buffer + 4) = super_blk.nb_files;

	write_blocks(SUPER_BLK_ADDR, 1, buffer_temp);
	free(buffer_temp);
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

	for (int i = 0; i < 50; i++)
		new_i_node->data_blk[i] = -1;

	new_i_node->ind_pointer = -1;

	write_i_node(new_i_node, blk);

	return new_i_node;
}

//Will read the contents of i-node at input block index
inode* read_i_node(int blk) {

	inode *i_node_in_blk = (inode*) malloc(sizeof(inode));

	inode *buffer_temp = (inode*) malloc(BLOCK_SIZE);
	read_blocks(blk, 1, buffer_temp);

	memcpy(i_node_in_blk, buffer_temp, sizeof(inode));

	return i_node_in_blk;
}

//Writes i_node struct to input block index
int write_i_node(inode *i_node, int blk) {

	printf("Writing inode to blk %d\n", blk);
	inode *buffer_temp = (inode*) malloc(BLOCK_SIZE);
	memcpy(buffer_temp, i_node, sizeof(inode));

	write_blocks(blk, 1, buffer_temp);
	free(buffer_temp);
	return 0;
}

void print_i_node(inode *i_node) {

	printf("mode: %d, ln count: %d, uid: %d, gid: %d, size: %d, data_blks: ",
			i_node->mode, i_node->ln_count, i_node->uid, i_node->gid,
			i_node->size);

	for (int i = 0; i < 50; i++) {
		printf("%d ", i_node->data_blk[i]);
	}

	printf("indirect pointer: %d\n", i_node->ind_pointer);
}

/*------------------------------------------------------------------*/
/*Root Dir section										            */
/*------------------------------------------------------------------*/

int init_root_dir() {

	inode *root_dir_i_node = init_i_node(1); //Create root dir i-node (MUST CACHE) TODO replace 1 with allocate

	root_dir.dir_entry.i_node_index = -1;
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
//	fname = (char*) calloc(strlen(current_entry->filename) + 1, sizeof(char));
	memcpy(fname, current_entry->dir_entry.filename,
			strlen(current_entry->dir_entry.filename));

	return 0;
}

//Transforms the directory table into a buffer that will be written to disk
//Returns number of blocks needed to store buffer
int get_rootdir_buffer(root_dir_entry *buffer) {

//	root_dir_entry *buffer;
	int offset = 0;
	int nb_blocks = 1;		// #blocks currently needed to store buffer in disk

	//If there are no files, buffer should be empty
	if (super_blk.nb_files == 0) {
		printf("No files, rootdir buffer empty\n");
		return 0;
	}

//	buffer = (root_dir_entry*) malloc(BLOCK_SIZE);//Start by allocating only 1 block

	do {
		//Check for overflow before copying
		if (offset + sizeof(root_dir_entry) > nb_blocks * BLOCK_SIZE)
			buffer = realloc(buffer, ++nb_blocks * BLOCK_SIZE);

		memcpy(buffer + offset, &(current_entry->dir_entry),
				sizeof(root_dir_entry));
		offset += sizeof(root_dir_entry);

		current_entry = current_entry->next;

	} while (current_entry != NULL);

	current_entry = &root_dir; //Restore current entry to first entry
	return nb_blocks;
}

int write_root_dir(inode *root_dir_inode) {

	root_dir_entry *root_dir_buffer = (root_dir_entry*) malloc(BLOCK_SIZE);
	int nb_blocks_buffer = get_rootdir_buffer(root_dir_buffer);

	for (int i = 0; i < nb_blocks_buffer; i++) {

	}

	free(root_dir_buffer);
	return 0;
}

//Adds entries (2 elements per entry: i_node, filename) to the root directory
int add_root_dir_entry(int i_node, char *filename) {

	if (super_blk.nb_files == 0) { //First entry in the directory
//		printf("first entry\n");

		memcpy(root_dir.dir_entry.filename, filename, strlen(filename));
		root_dir.dir_entry.i_node_index = i_node;
		root_dir.next = NULL;
	} else {

//		printf("not first entry\n");
		root_dir_t *root_dir_entry = (root_dir_t*) malloc(sizeof(root_dir_t));

		memcpy(root_dir_entry->dir_entry.filename, filename, strlen(filename));
		root_dir_entry->dir_entry.i_node_index = i_node;
		root_dir_entry->next = NULL;

		//last_entry points to file that was just added to linked list
		last_entry->next = root_dir_entry;
		last_entry = last_entry->next;
	}
	super_blk.nb_files++;
	write_super_blk();

	return 0;
}

int remove_root_dir_entry(char *filename) {

	return 0;
}

