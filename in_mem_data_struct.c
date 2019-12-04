#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "on_disk_data_struct.h"
#include "in_mem_data_struct.h"
#include "disk_emu.h"

mem_table_t *last_available;
int nb_open_files;
int nb_blks_store_free_table;

int init_fd_table() {

	//Simulate fd table similar to a process in Linux (0-2 are associated with stdin,stdout,sterr)
	fd_table_t *fd_out_entry = (fd_table_t*) malloc(sizeof(fd_table_t));
	fd_table_t *fd_err_entry = (fd_table_t*) malloc(sizeof(fd_table_t));

	fd_table_root.dir_entry.fd = 0;
	//Dont want to set positive inode index and not -1 (-1 will be used to find available fd)
	fd_table_root.dir_entry.i_node = -2;
	fd_table_root.dir_entry.r_offset = 0;
	fd_table_root.dir_entry.w_offset = 0;

	fd_out_entry->dir_entry.fd = 1;
	fd_out_entry->dir_entry.i_node = -2;
	fd_out_entry->dir_entry.r_offset = 0;
	fd_out_entry->dir_entry.w_offset = 0;

	fd_err_entry->dir_entry.fd = 2;
	fd_err_entry->dir_entry.i_node = -2;
	fd_err_entry->dir_entry.r_offset = 0;
	fd_err_entry->dir_entry.w_offset = 0;

	//Assign linked list pointers
	fd_table_root.next = fd_out_entry;
	fd_out_entry->next = fd_err_entry;

	nb_open_files = 3;
	return 0;
}

//Returns lowest available fd for opened file
int add_fd_entry(int i_node) {

	fd_table_t *current_entry = &fd_table_root;
	fd_table_t *new_entry = (fd_table_t*) malloc(sizeof(fd_table_t));

	//Iterate through linked list
	for (int i = 0; i < nb_open_files; i++) {

		//Found unused fd
		if (current_entry->dir_entry.i_node == -1) {
			current_entry->dir_entry.i_node = i_node;
			current_entry->dir_entry.r_offset = 0;
			current_entry->dir_entry.w_offset = 0;

			nb_open_files++;
			return current_entry->dir_entry.fd;
		}

		if (i < nb_open_files - 1)
			current_entry = current_entry->next;
	}

	//Did not find unused fd, so create new entry
	new_entry->dir_entry.fd = nb_open_files;
	new_entry->dir_entry.i_node = i_node;
	new_entry->dir_entry.r_offset = 0;
	new_entry->dir_entry.w_offset = 0;

	current_entry->next = new_entry;

	nb_open_files++;
	return new_entry->dir_entry.fd;
}

//Marks entry from fd table as unused
int remove_fd_entry(int fd) {

	if (fd <= 2) {
		printf("fd reserved\n");
		return -1;
	}

	fd_table_t *current_entry = &fd_table_root;

	//Iterate through linked list
	for (int i = 0; i < fd; i++) {
		current_entry = current_entry->next;
	}

	if (current_entry->dir_entry.i_node == -1) {
		printf("fd is unused, cannot remove.\n");
		return -1;
	}

	//Mark fd as unused
	current_entry->dir_entry.i_node = -1;
	current_entry->dir_entry.r_offset = 0;
	current_entry->dir_entry.w_offset = 0;

//	nb_open_files--;
	return 0;
}

fd_table_entry* get_fd_entry(int fd) {

	if (fd > nb_open_files - 1)
		return NULL;

	fd_table_t *current_entry = &fd_table_root;

	//Iterate through linked list
	for (int i = 0; i < fd; i++) {
		current_entry = current_entry->next;
	}

	return &(current_entry->dir_entry);
}

int get_fd(int i_node) {

	fd_table_t *current_entry = &fd_table_root;

	//Iterate through linked list
	for (int i = 0; i < nb_open_files; i++) {
		fd_table_entry entry = current_entry->dir_entry;

		if (entry.i_node == i_node)
			return entry.fd;

		current_entry = current_entry->next;
	}

	return -1;
}

//Returns true if the file is open
int is_open(int i_node) {

	fd_table_t *current_entry = &fd_table_root;

	for (int i = 0; i < nb_open_files; i++) {
		fd_table_entry entry = current_entry->dir_entry;

		if (entry.i_node == i_node)
			return 1;

		current_entry = current_entry->next;
	}

	return 0;
}

//Prints content of fd_table
void print_fd_table() {

	fd_table_t *current_entry = &fd_table_root;

	for (int i = 0; i < nb_open_files; i++) {

		fd_table_entry entry = current_entry->dir_entry;

		printf("fd: %d, inode: %d, r_offset: %d, w_offset: %d.\n", entry.fd,
				entry.i_node, entry.r_offset, entry.w_offset);

		current_entry = current_entry->next;
	}
}

//Initializes free blocks table
int init_mem_table(int fresh) {

	nb_blks_store_free_table = super_blk.sfs_size  / super_blk.blk_size + 1;

	if (fresh) {
		free_blks[0] = -1;	//Super block
		free_blks[1] = -1;	//Root dir inode

		for (int i = 2; i < super_blk.sfs_size; i++)
			free_blks[i] = 1;

		//If sfs_size= 10k, last 10 blocks will store the free blocks table
		for (int i = 0; i < nb_blks_store_free_table; i++)
			free_blks[super_blk.sfs_size - i - 1] = -1;

		write_mem_table();
	} else
		read_blocks(super_blk.sfs_size - nb_blks_store_free_table, nb_blks_store_free_table, free_blks);

	return 0;
}

//Uses a linked list to find available blocks
int allocate_blocks(int nb_blocks) {

	int ret = -1;

	for (int i = 0; i < super_blk.sfs_size; i++) {
		if (free_blks[i] == 1) {
			free_blks[i] = -1;
			ret = i;
			write_mem_table();
			break;
		}
	}
	return ret;
}

//Deallocates block at input index
int deallocate_block(int blk) {

	char *buf = (char*) calloc(1, super_blk.blk_size);
	write_blocks(blk, 1, buf);

	free_blks[blk] = 1;
	write_mem_table();
	return 0;
}

//Writes free blocks table to disk
int write_mem_table() {

	write_blocks(super_blk.sfs_size - nb_blks_store_free_table, nb_blks_store_free_table, free_blks);
	return 0;
}
