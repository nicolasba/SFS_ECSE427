#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "in_mem_data_struct.h"
#include "on_disk_data_struct.h"
#include "disk_emu.h"
#include "sfs_api.h"

#define	SFS_FILENAME_TEST "my_sfs_20.txt"
#define BLK_SIZE 1024
#define NB_BLKS 10000
#define ROOT_DIR_ADDR 1

int root_dir_addr;

void mksfs(int fresh) {

	if (fresh) {
		init_fresh_disk(SFS_FILENAME_TEST, BLK_SIZE, NB_BLKS); //Initialize fresh disk
		init_super_blk(BLK_SIZE, NB_BLKS, ROOT_DIR_ADDR, 0); //Initialize super block content
		init_root_dir(fresh);						//Initialize root dir cache
	} else {
		init_disk(SFS_FILENAME_TEST, BLK_SIZE, NB_BLKS);	//Initialize disk
		read_super_blk();					//Read existing super block content
		print_super_blk();

		inode *root_dir_inode = read_i_node(super_blk.root_dir_i_node);
		print_i_node(root_dir_inode);

		init_root_dir(fresh);						//Initialize root dir cache
		read_root_dir(root_dir_inode);//Read existing root dir content to cache
		print_root_dir();
	}

	init_mem_table(fresh);		//Initialize free blocks table
	init_fd_table();			//Initialize fd table
}

//Returns size of a given file
int sfs_getfilesize(const char *path) {

	int inode_index = get_i_node_index(path);
	inode *file_inode = read_i_node(inode_index);

	if (inode_index == -1)
		return -1;

	return file_inode->size;
}

//Opens a file (or creates it if it does not exist)
int sfs_fopen(char *name) {

	int inode_index = get_i_node_index(name);
	int fd;
	inode *root_dir_inode = read_i_node(super_blk.root_dir_i_node);

	//File does not exist -> create file
	if (inode_index == -1) {

		printf("Creating file with name : %s\n", name);

		//Allocate free block for inode
		inode_index = allocate_blocks(1);

		printf("Allocating new file.\n");
		inode *new_file_inode = init_i_node(inode_index);  	//Create inode
		print_i_node(new_file_inode);

		add_new_file_root_dir_entry(inode_index, name); //Add entry to root directory table
		//	int fd2 = sfs_fopen("test3");
		//	int fd3 = sfs_fopen("test4");
		write_root_dir(root_dir_inode, super_blk.root_dir_i_node);
		print_root_dir();

		//Add entry to fd table
		fd = add_fd_entry(inode_index);
		print_fd_table();
	} else { 	//Open existing file

		printf("File with name \"%s\" exists.\n", name);

		if (is_open(inode_index)) {
			printf("File is already open.\n");
			return -1;
		}

		//Add entry to fd table
		fd = add_fd_entry(inode_index);
		print_fd_table();
	}

	return fd;
}

//Closes a file
int sfs_fclose(int fileID) {

	return remove_fd_entry(fileID);
}

//Moves the read pointer of a file to loc
int sfs_frseek(int fileID, int loc) {

	fd_table_entry *entry = get_fd_entry(fileID);

	if (entry == NULL)
		return -1;

	entry->r_offset = loc;
	return 0;
}

//Moves the write pointer of a file to loc
int sfs_fwseek(int fileID, int loc) {

	fd_table_entry *entry = get_fd_entry(fileID);

	if (entry == NULL)
		return -1;

	entry->w_offset = loc;
	return 0;
}

//Reads "length" bytes from a file starting from read pointer into buf
int sfs_fread(int fileID, char *buf, int length) {

	fd_table_entry *entry = get_fd_entry(fileID);
	int i_node;//If there is only one element in the list, all block indices after the index in the element
	//are available
	inode *file_inode;			//File inode
	int r_offset;				//Read pointer

	int first_block_index;		//First data block to read index
	int last_block_index;		//Last data block to read index
	int nb_blocks;				//Number of blocks to read
	char *temp_buf;
	char *concat_buf;
	int temp_offset;

	i_node = entry->i_node;
	file_inode = read_i_node(i_node);
	r_offset = entry->r_offset;

	if (r_offset + length > file_inode->size) {
		printf("You're trying to read more than there is data.\n");
		return -1;
	}

	first_block_index = r_offset / super_blk.blk_size;
	last_block_index = (r_offset + length) / super_blk.blk_size;
	nb_blocks = last_block_index - first_block_index + 1;

	concat_buf = (char*) malloc(nb_blocks * super_blk.blk_size);
	temp_buf = (char*) malloc(BLOCK_SIZE);
	temp_offset = 0;

	//Read block by block and concatenate data
	for (int i = first_block_index; i <= last_block_index; i++) {
		if (file_inode->data_blk[i] != -1) {
			read_blocks(file_inode->data_blk[i], 1, temp_buf);
			memcpy(concat_buf + temp_offset, temp_buf, super_blk.blk_size);
			temp_offset += super_blk.blk_size;
		}
	}

	temp_offset = r_offset % super_blk.blk_size;
	memcpy(buf, concat_buf + temp_offset, length);

	entry->r_offset = r_offset + length;

	free(concat_buf);
	free(temp_buf);
	return length;
}

//Writes "length" bytes from buf into file starting at write pointer
int sfs_fwrite(int fileID, char *buf, int length) {

	fd_table_entry *entry = get_fd_entry(fileID);
	int i_node;
	inode *file_inode;			//File inode
	int w_offset;				//Read pointer

	int first_block_index;		//First data block to read index
	int last_block_index;		//Last data block to read index
	char *temp_buf;
	int temp_offset;
	int buf_offset;

	i_node = entry->i_node;
	file_inode = read_i_node(i_node);
	w_offset = entry->w_offset;

	first_block_index = w_offset / super_blk.blk_size;
	last_block_index = (w_offset + length) / super_blk.blk_size;

	if (file_inode->data_blk[first_block_index] == -1
			&& first_block_index > 0) {
		printf(
				"You are trying to write outside of the file allocated space.\n");
		return -1;
	}

	temp_buf = (char*) malloc(super_blk.blk_size);
	buf_offset = 0;

	for (int i = first_block_index; i <= last_block_index; i++) {
		if (file_inode->data_blk[i] == -1)
			file_inode->data_blk[i] = allocate_blocks(1);
//			file_inode->data_blk[i] = 10;

		//For first block to be written
		if (i == first_block_index) {
			read_blocks(file_inode->data_blk[i], 1, temp_buf);

			temp_offset = w_offset - (first_block_index * super_blk.blk_size);
			memcpy(temp_buf + temp_offset, buf + buf_offset,
					super_blk.blk_size - temp_offset);
			buf += super_blk.blk_size - temp_offset;

			write_blocks(file_inode->data_blk[i], 1, temp_buf);
		}
		//For last block to be written
		else if (i == last_block_index) {
			read_blocks(file_inode->data_blk[i], 1, temp_buf);

			temp_offset = (w_offset + length)
					- (last_block_index * super_blk.blk_size);
			memcpy(temp_buf, buf + buf_offset, temp_offset);

			write_blocks(file_inode->data_blk[i], 1, temp_buf);
		}
		//For blocks in the middle
		else {
			write_blocks(file_inode->data_blk[i], 1, buf + buf_offset);
		}
	}

	//Update size of file in case it was expanded, otherwise it stays the same
	if (w_offset + length > file_inode->size)
		file_inode->size = w_offset + length;

	entry->w_offset = w_offset + length;

	write_i_node(file_inode, i_node);
	return length;
}

//Removes a file from the sfs
int sfs_remove(char *file) {

	int i_node = get_i_node_index(file);

	if (i_node == -1) {
		printf("File does not exist.\n");
		return -1;
	}

	int fd = get_fd(i_node);
	inode *root_dir_inode = read_i_node(super_blk.root_dir_i_node);
	inode *file_i_node = read_i_node(i_node);
	int i = 0;

	while (file_i_node->data_blk[i] != -1) {
		deallocate_block(file_i_node->data_blk[i]);		//Deallocate data blocks
	}

	deallocate_block(i_node);						//Deallocate i node block

	if (remove_root_dir_entry(file) == -1)			//Remove from root directory
		return -1;

	write_root_dir(root_dir_inode, super_blk.root_dir_i_node);

	if (is_open(i_node))		//Remove entry from fd table
		remove_fd_entry(fd);

	free(file_i_node);
	return 0;
}

int main(void) {

	//Test1.1
	//Init root dir
	mksfs(0);

//	for (int i = 0; i < 50; i++) {
//		snprintf(buf, 7, "test%d", i);
//		sfs_fopen(buf);
//	}
	int fd1 = sfs_fopen("test102");
//	int fd2 = sfs_fopen("test3");
//	sfs_remove("test101");
	print_root_dir();
	print_i_node(read_i_node(super_blk.root_dir_i_node));
	print_fd_table();

	char buf1[] =
			"In computer science, a pointer is a programming language object that stores the memory address of another value located in computer memory. A pointer references a location in memory, and obtaining the value stored at that location is known as dereferencing the pointer. As an analogy, a page number in a book's index could be considered a pointer to the corresponding page; dereferencing such a pointer would be done by flipping to the page with the given page number and reading the text found on that page. The actual format and content of a pointer variable is dependent on the underlying computer "
			"architecture. Using pointers significantly improves performance for repetitive operations like traversing iterable data structures, e.g. strings, lookup tables, control tables and tree structures. In particular, it is often much cheaper in time and space to copy and dereference pointers than it is to copy and access the data to which the pointers point. Pointers are also used to hold the addresses of entry points for called subroutines in procedural programming and for run-time linking to dynamic link libraries (DLLs). In object-oriented programming, pointers to functions are used for binding methods, often using what are called"
			" virtual method tables.";
	char buf2[1260];

	printf("length: %d\n", strlen(buf1));

//	sfs_frseek(fd1, 2);
//	sfs_fwrite(fd1, buf1, 1260);
	sfs_fread(fd1, buf2, 1260);
//
//	printf("buf1: %s\n\n\n\n", buf1);
	printf("buf2: %s\n", buf2);
//	int fd3 = sfs_fopen("test4");
//	sfs_fopen("test5");
//	sfs_fopen("test6666612345.exe");
//	sfs_fopen("test6");
//	sfs_fopen("test8");

	return EXIT_SUCCESS;
}

