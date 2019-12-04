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
int nb_files_root_dir_cache;
root_dir_t *last_entry;
root_dir_t *current_entry;

/*------------------------------------------------------------------*/
/*Super block section											    */
/*------------------------------------------------------------------*/

//Initializes super block at fresh start
int init_super_blk(int blk_size, int sfs_size, int root_dir_i_node,
		int nb_files) {

	printf("Initializing super block.\n");
	BLOCK_SIZE = blk_size;

	super_blk.magic = MAGIC;
	super_blk.blk_size = blk_size;
	super_blk.sfs_size = sfs_size;
	super_blk.root_dir_i_node = root_dir_i_node;
	super_blk.nb_files = nb_files;

	write_super_blk();
	return 0;
}

//Reads from block at SUPER_BLK_ADDR (default 0), and copies the content into
//super block cache
int read_super_blk() {

	struct super_blk_t *super_blk_temp = (struct super_blk_t*) malloc(
			BLOCK_SIZE);
	read_blocks(SUPER_BLK_ADDR, 1, super_blk_temp);

	memcpy(&super_blk, super_blk_temp, sizeof(struct super_blk_t));
	free(super_blk_temp);

	return 0;
}

//Writes content from super block cache to disk at SUPER_BLK_ADDR
int write_super_blk() {

	printf("Writing super block.\n");
	struct super_blk_t *buffer_temp = (struct super_blk_t*) malloc(BLOCK_SIZE);
	memcpy(buffer_temp, &super_blk, sizeof(struct super_blk_t));

	write_blocks(SUPER_BLK_ADDR, 1, buffer_temp);
	free(buffer_temp);
	return 0;
}

//Prints content from super block cache
void print_super_blk() {

	printf(
			"magic: %d, blk size: %d, sfs size: %d, root dir i-node: %d, nb files: %d\n",
			super_blk.magic, super_blk.blk_size, super_blk.sfs_size,
			super_blk.root_dir_i_node, super_blk.nb_files);
}

/*------------------------------------------------------------------*/
/*Inode section											            */
/*------------------------------------------------------------------*/

//Creates new i-node with default content at block of input index
inode* init_i_node(int blk) {

	inode *new_i_node = (inode*) malloc(sizeof(inode));

	new_i_node->mode = 0;
	new_i_node->ln_count = 0;
	new_i_node->gid = 0;
	new_i_node->uid = 0;
	new_i_node->size = 0;

	for (int i = 0; i < NB_SINGLE_POINTERS; i++)
		new_i_node->data_blk[i] = -1;

	new_i_node->ind_pointer = -1;

	write_i_node(new_i_node, blk);

	return new_i_node;
}

//Will read the contents of i-node at input block index into cache
inode* read_i_node(int blk) {

	inode *i_node_in_blk = (inode*) malloc(sizeof(inode));

	inode *buffer_temp = (inode*) malloc(BLOCK_SIZE);
	read_blocks(blk, 1, buffer_temp);

	memcpy(i_node_in_blk, buffer_temp, sizeof(inode));

	free(buffer_temp);
	return i_node_in_blk;
}

//Writes content of i_node struct cache to input block index
int write_i_node(inode *i_node, int blk) {

	printf("Writing inode to blk %d\n", blk);
	inode *buffer_temp = (inode*) malloc(BLOCK_SIZE);
	memcpy(buffer_temp, i_node, sizeof(inode));

	write_blocks(blk, 1, buffer_temp);
	free(buffer_temp);
	return 0;
}

//Prints content of i-node cache
void print_i_node(inode *i_node) {

	printf("mode: %d, ln count: %d, uid: %d, gid: %d, size: %d, data_blks: ",
			i_node->mode, i_node->ln_count, i_node->uid, i_node->gid,
			i_node->size);

	for (int i = 0; i < NB_SINGLE_POINTERS; i++) {
		printf("%d ", i_node->data_blk[i]);
	}

	printf("indirect pointer: %d\n", i_node->ind_pointer);
}

/*------------------------------------------------------------------*/
/*Root Dir section										            */
/*------------------------------------------------------------------*/

int init_root_dir(int fresh) {

	inode *root_dir_i_node;

	if (fresh)
		root_dir_i_node = init_i_node(1); //Create root dir i-node (MUST CACHE) TODO replace 1 with allocate

	root_dir.dir_entry.i_node_index = -1;
	root_dir.next = NULL;

	nb_files_root_dir_cache = 0; //At start, cache is empty
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

//Transforms the directory table cache into a buffer that will be written to disk
//Returns number of blocks needed to store buffer
int get_rootdir_buffer(root_dir_entry **buffer) {

	int offset = 0;
	int nb_blocks = 1;		// #blocks currently needed to store buffer in disk

	//If there are no files, buffer should be empty
	if (nb_files_root_dir_cache == 0) {
		printf("No files, rootdir buffer empty\n");
		return 0;
	}

	do {
		//Check for overflow before copying
		if (offset * sizeof(root_dir_entry) + sizeof(root_dir_entry)
				> nb_blocks * BLOCK_SIZE)
			*buffer = realloc(*buffer, ++nb_blocks * BLOCK_SIZE);

//		printf(
//				"get buffer : current entry inode: %d, filename: %s, saved at %ld location, offset: %d, sizeof root_dir_entry: %ld\n",
//				current_entry->dir_entry.i_node_index,
//				current_entry->dir_entry.filename, ((void*)(*buffer)) + offset, offset,
//				sizeof(root_dir_entry));
		memcpy(*buffer + offset, &(current_entry->dir_entry),
				sizeof(root_dir_entry));
		offset++;

		current_entry = current_entry->next;

	} while (current_entry != NULL);

	if (offset == 0) {
		printf("Even though super block has nb_files > 0, root_dir is empty.");
		return 0;
	}

	current_entry = &root_dir; //Restore current entry to first entry
	return nb_blocks;
}

int read_root_dir(inode *root_dir_inode) {

	int i = 0;
	int offset = 0;
	root_dir_entry *entries_across_blocks = (root_dir_entry*) malloc(
			BLOCK_SIZE);
	root_dir_entry *buffer_temp = (root_dir_entry*) malloc(BLOCK_SIZE);

	//As long as there are data blocks to copy from
	while (root_dir_inode->data_blk[i] != -1) {

		//Need to allocate more blocks when there root directory spans across more than 1 block
		if (i > 0)
			entries_across_blocks = (root_dir_entry*) realloc(
					entries_across_blocks, (i + 1) * BLOCK_SIZE);

		read_blocks(root_dir_inode->data_blk[i], 1, buffer_temp);

		memcpy(((char*) entries_across_blocks) + offset, buffer_temp,
				BLOCK_SIZE);
		offset += BLOCK_SIZE;
		i++;
	}

	if (i == 0) {
		printf("Root dir has no content on disk.\n");
		return 0;
	}

	//MIGHT NEED TO CLEAR CURRENT ROOT DIR CACHE TODO
//	root_dir.dir_entry.i_node_index = -1;

	for (i = 0; i < super_blk.nb_files; i++) {
		add_root_dir_entry((entries_across_blocks + i)->i_node_index,
				(entries_across_blocks + i)->filename);
	}

	free(entries_across_blocks);
	free(buffer_temp);
	return 0;
}

int write_root_dir(inode *root_dir_inode, int inode_index) {

	root_dir_entry *root_dir_buffer = (root_dir_entry*) malloc(BLOCK_SIZE);
	int nb_blocks_buffer = get_rootdir_buffer(&root_dir_buffer);

	printf("we need %d blocks to store the content of root dir.\n",
			nb_blocks_buffer);

//	for (int i = 0; i < super_blk.nb_files; i++) {
//		root_dir_entry *temp = (root_dir_entry*) malloc(sizeof(root_dir_entry));
//		memcpy(temp, root_dir_buffer + i, sizeof(root_dir_entry));
//
//		printf("Writing to root data block: %d - inode: %d, filename: %s\n", i,
//				temp->i_node_index, temp->filename);
//	}

	int offset = 0;

	if (nb_blocks_buffer > NB_SINGLE_POINTERS) {
		printf(
				"Root directory is too large. Takes more than %d blocks to store.\n",
				NB_SINGLE_POINTERS);
		return -1;
	}

	int allocated = 2; //TODO REMOOOOOOOVE

	//Iterate through the single pointers to data blocks
	for (int i = 0; i < nb_blocks_buffer; i++) {
		if (root_dir_inode->data_blk[i] == -1) {
			//Allocate new data block (TODO)
			allocated = (i == 0) ? 2 : 3;
			root_dir_inode->data_blk[i] = allocated;
		}

		write_blocks(root_dir_inode->data_blk[i], 1,
				((char*) (root_dir_buffer)) + offset);
		offset += BLOCK_SIZE;
	}

	for (int i = nb_blocks_buffer; i < NB_SINGLE_POINTERS; i++) {
		root_dir_inode->data_blk[i] = -1;
	}

	write_i_node(root_dir_inode, inode_index);	//Save changes to inode on disk
	free(root_dir_buffer);
	return 0;
}

//Adds entries (2 elements per entry: i_node, filename) to the root directory
int add_root_dir_entry(int i_node, char *filename) {

	if (strlen(filename) > 20) {
		printf("Filename too long.\n");
		return -1;
	}

	if (nb_files_root_dir_cache == 0) { //First entry in the directory
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

	nb_files_root_dir_cache++;
	return 0;
}

//Call this function to add entries to the root dir entry when a new file is created (as opposed
//to when initializing existing file system -> adding entries to cache root dir from existing files)
int add_new_file_root_dir_entry(int i_node, char *filename) {

	if (add_root_dir_entry(i_node, filename) == -1)
		return -1;
	super_blk.nb_files++;
	write_super_blk();

	return 0;
}

int remove_root_dir_entry(char *filename) {

	root_dir_t *prev_entry;
	if (super_blk.nb_files == 0) {
		printf("Root directory cache is empty.\n");
		return -1;
	}

	current_entry = &root_dir;
	prev_entry = &root_dir;

	while (current_entry != NULL) {

		//Found entry to remove
		if (strcmp(current_entry->dir_entry.filename, filename) == 0) {

			//Remove from linked list
			prev_entry->next = current_entry->next;

			//Entry to be removed is the first in the directory
			if (current_entry == &root_dir) {
				//Entry to be removed is the only entry in the directory
				if (current_entry->next == NULL) {
					init_root_dir(0);
					return 0;
				}
				else
					root_dir = *(current_entry->next);

				printf("Entry to remove is the first in the directory.\n");
			}
			//Entry to be removed is the last in the directory
			else if (current_entry == last_entry)
				last_entry = prev_entry;

			//I cant free root dir because it was not malloc
			if (current_entry != &root_dir)
				free(current_entry);

			current_entry = &root_dir;
			printf("Entry was removed.\n");
			nb_files_root_dir_cache--;
			super_blk.nb_files--;
			write_super_blk();

			return 0;
		}

		prev_entry = current_entry;
		current_entry = current_entry->next;
	}

	current_entry = &root_dir;
	printf("File to remove was not found.\n");
	return -1;
}

int get_i_node_index(char *filename) {

	if (nb_files_root_dir_cache == 0) {
		printf("Directory cache is empty.\n");
		return -1;
	}

	current_entry = &root_dir;

	while (current_entry != NULL) {

		if (strcmp(current_entry->dir_entry.filename, filename) == 0) {

			printf("Found entry: inode: %d\n",
					current_entry->dir_entry.i_node_index);
			current_entry = &root_dir;
			return current_entry->dir_entry.i_node_index;
		}

		current_entry = current_entry->next;
	}

	current_entry = &root_dir;
	printf("Entry could not be found for \"%s\"\n", filename);
	return -1;
}

int get_number_of_files() {
	return super_blk.nb_files;
}

void print_root_dir() {

	root_dir_entry *root_dir_buffer = (root_dir_entry*) malloc(BLOCK_SIZE);

	//0 blocks from root_dir_buffer
	if (get_rootdir_buffer(&root_dir_buffer) == 0) {
		printf("Root_dir is empty.\n");
		free(root_dir_buffer);
		return;
	}

	for (int i = 0; i < nb_files_root_dir_cache; i++) {
		root_dir_entry entry = *(root_dir_buffer + i);
		printf("inode: %d, filename: %s\n", entry.i_node_index, entry.filename);
	}

	printf("Number of files in rootdir cache: %d\n", nb_files_root_dir_cache);
	free(root_dir_buffer);
}

