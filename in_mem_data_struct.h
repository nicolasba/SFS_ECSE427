/*
 * in_mem_data_struct.h
 *
 *  Created on: Nov 28, 2019
 *      Author: nicolas
 */

#ifndef IN_MEM_DATA_STRUCT_H_
#define IN_MEM_DATA_STRUCT_H_

struct fd_table_entry {
	int fd;
	int i_node;
	int r_offset;
	int w_offset;
};


#endif /* IN_MEM_DATA_STRUCT_H_ */
