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
		init_super_blk(BLK_SIZE, NB_BLKS, ROOT_DIR_ADDR, 0);
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

//	super_blk.magic = 3021;
//	super_blk.blk_size = 75741;
//	super_blk.sfs_size = 23142;
//	super_blk.root_dir_i_node = 28548;
//	super_blk.nb_files = 21319;

	read_super_blk();

	printf(
			"magic: %d, blk_size: %d, sfs_size: %d, root_dir_i_node: %d, nbfiles: %d\n",
			super_blk.magic, super_blk.blk_size, super_blk.sfs_size,
			super_blk.root_dir_i_node, super_blk.nb_files);

//	printf("adding directories\n");

	init_root_dir();
	add_root_dir_entry(257, "test1");
	add_root_dir_entry(100, "test2");
	add_root_dir_entry(25, "test3");
	add_root_dir_entry(400, "test4");
	add_root_dir_entry(800, "test5");
	add_root_dir_entry(900, "test6666666612345.exe");
	add_root_dir_entry(145000, "test6");
	add_root_dir_entry(2300, "test8");

	read_super_blk();
//
	printf(
			"magic: %d, blk_size: %d, sfs_size: %d, root_dir_i_node: %d, nbfiles: %d\n",
			super_blk.magic, super_blk.blk_size, super_blk.sfs_size,
			super_blk.root_dir_i_node, super_blk.nb_files);

	root_dir_entry *root_dir_buffer = get_rootdir_buffer();

	for (int i = 0; i < super_blk.nb_files; i++) {

		root_dir_entry entry = *(root_dir_buffer + i * sizeof(root_dir_entry));
		printf("inode: %d, filename: %s\n", entry.i_node_index, entry.filename);
	}

//	current_entry = &root_dir;
//
//	printf("printing directory\n");
//	do {
//		if (current_entry->dir_entry.i_node_index != -1)
//			printf("filename: %s, inode: %d\n", current_entry->dir_entry.filename, current_entry->dir_entry.i_node_index);
//		current_entry = current_entry->next;
//	} while (current_entry != NULL);

	free(buffer);
	return EXIT_SUCCESS;
}

