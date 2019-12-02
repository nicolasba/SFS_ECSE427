/*
 * my_sfs.c
 *
 *  Created on: Nov 28, 2019
 *      Author: nicolas
 */
#include <stdio.h>
#include <stdlib.h>
#include "in_mem_data_struct.h"
#include "on_disk_data_struct.h"
#include "disk_emu.h"
#include "sfs_api.h"

#define	SFS_FILENAME_TEST "my_sfs_20.txt"
#define BLK_SIZE 1024
#define NB_BLKS 20
#define ROOT_DIR_ADDR 1

int sfs_size;

void mksfs(int fresh) {

	if (fresh) {
		init_fresh_disk(SFS_FILENAME_TEST, BLK_SIZE, NB_BLKS);
		init_super_blk(BLK_SIZE, NB_BLKS, ROOT_DIR_ADDR);
		write_super_blk();
	} else {
		init_disk(SFS_FILENAME_TEST, BLK_SIZE, NB_BLKS);

	}
}

int main(void) {

	int *buffer;

	mksfs(1);

//	inode *root_dir_i_node = init_i_node(1);
	buffer = read_i_node(1);

//	buffer = (int*) read_super_blk();

	for (int i = 0; i < BLK_SIZE / sizeof(int); i++) {
		printf("%d\n", *(buffer + i));
	}

//	printf("adding directories\n");

	init_root_dir();
	add_root_dir_entry(257, "test1");
	add_root_dir_entry(100, "test2");
	add_root_dir_entry(25, "test3");
	add_root_dir_entry(400, "test4");
	add_root_dir_entry(800, "test5");

	char *buffer_root_dir = get_rootdir_buffer();
	printf("%d\n", *buffer_root_dir);

//	current_entry = &root_dir;

//	printf("printing directory\n");
//	do {
//		if (current_entry->filename != NULL)
//			printf("filename: %s, inode: %d\n", current_entry->filename, current_entry->i_node_index);
//		current_entry = current_entry->next;
//	} while (current_entry != NULL);

	free(buffer);
	return EXIT_SUCCESS;
}

