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

void mksfs(int fresh) {

	if (fresh) {
		init_fresh_disk(SFS_FILENAME_TEST, BLK_SIZE, NB_BLKS);
		init_super_blk(BLK_SIZE, NB_BLKS, ROOT_DIR_ADDR, 0);
		write_super_blk();
	} else {
		init_disk(SFS_FILENAME_TEST, BLK_SIZE, NB_BLKS);
	}
}

int sfs_fopen(char *name) {

	//SEARCH FOR EXISTING FILENAME
	//ONLY increase nb_files if file does not exist already
	add_root_dir_entry(100, name);
	super_blk.nb_files++;
	write_super_blk();

}

int main(void) {

	//Test1.1
	//Init root dir
	mksfs(1);

	read_super_blk();
	print_super_blk();

	init_root_dir(1);
	inode *root_dir_inode = read_i_node(1);
	print_i_node(root_dir_inode);
//
//	printf("adding directories\n");
//	for (int i = 0; i < 50; i++)
//		add_root_dir_entry(i, "test");
//	add_root_dir_entry(100, "test2");
//	add_root_dir_entry(25, "test3");
//	add_root_dir_entry(400, "test4");
//	add_root_dir_entry(800, "test5");
//	add_root_dir_entry(900, "test6666612345.exe");
//	add_root_dir_entry(145000, "test6");
//	add_root_dir_entry(2300, "test8");
//	super_blk.nb_files=57;
//	write_super_blk();

	printf("adding directories\n");
//	for (int i = 0; i < 50; i++)
//		sfs_fopen("test");
	sfs_fopen("test2");
	sfs_fopen("test3");
	sfs_fopen("test4");
	sfs_fopen("test5");
	sfs_fopen("test6666612345.exe");
	sfs_fopen("test6");
	sfs_fopen("test8");

	read_super_blk();
	print_super_blk();

	print_root_dir();

	remove_root_dir_entry("test2");
	super_blk.nb_files--;
	write_super_blk();

	print_root_dir();

	get_i_node_index("test6");

	write_root_dir(root_dir_inode, ROOT_DIR_ADDR);
	root_dir_inode = read_i_node(1);
	print_i_node(root_dir_inode);

//	//Test 1.2
//	mksfs(0);
//
//	read_super_blk();
//	print_super_blk();
//
//	inode *root_dir_inode = read_i_node(1);
//	print_i_node(root_dir_inode);
//
//	init_root_dir(0);
//	read_root_dir(root_dir_inode);
//	print_root_dir();

//	root_dir_i_node->mode = 21423;
//	root_dir_i_node->data_blk[5] = 4545;
//	write_i_node(root_dir_i_node, 1);
//
//	super_blk.magic = 3021;
//	super_blk.blk_size = 75741;
//	super_blk.sfs_size = 23142;
//	super_blk.root_dir_i_node = 28548;
//	super_blk.nb_files = 21319;

	return EXIT_SUCCESS;
}

