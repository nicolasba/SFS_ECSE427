/*
 * in_mem_data_struct.h
 *
 *  Created on: Nov 28, 2019
 *      Author: nicolas
 */

#ifndef IN_MEM_DATA_STRUCT_H_
#define IN_MEM_DATA_STRUCT_H_

typedef struct fd_table_entry fd_table_entry;
typedef struct fd_table_t fd_table_t;

struct fd_table_entry {
	int fd;
	int i_node;
	int r_offset;
	int w_offset;
};

struct fd_table_t {
	fd_table_entry dir_entry;
	fd_table_t *next;
} fd_table_root;

int init_fd_table();
int add_fd_entry(int i_node);
int remove_fd_entry(int fd);
fd_table_entry* get_fd_entry(int fd);
int get_fd(int i_node);
int is_open(int i_node);
void print_fd_table();

int allocate_blocks(int nb_blocks);
int deallocate_block(int blk);

#endif /* IN_MEM_DATA_STRUCT_H_ */
