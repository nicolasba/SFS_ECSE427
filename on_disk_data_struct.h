#ifndef ON_DISK_DATA_STRUCT_H_
#define ON_DISK_DATA_STRUCT_H_

typedef struct i_node inode;
typedef struct root_dir_t root_dir_t;
typedef struct root_dir_entry root_dir_entry;

#define NB_SINGLE_POINTERS 50

//SUPER BLOCK section
int BLOCK_SIZE;
struct super_blk_t{
	int magic;
	int blk_size;
	int sfs_size;
	int root_dir_i_node;
	int nb_files;
} super_blk;

int init_super_blk(int blk_size, int sfs_size, int root_dir_i_node, int nb_files);
int read_super_blk();
int write_super_blk();
void print_super_blk();

//I-NODE section
struct i_node {
	int mode;
	int ln_count;
	int uid;
	int gid;
	int size;
	int data_blk[NB_SINGLE_POINTERS];
	int ind_pointer;
};

inode* init_i_node(int blk);
inode* read_i_node(int blk) ;
int write_i_node(inode* i_node, int blk);
void print_i_node(inode *i_node);


//ROOT DIRECTORY section (root dir is implemented as a linked list)
struct root_dir_entry {
	int i_node_index;
	char filename[20];
};

struct root_dir_t {
	root_dir_entry dir_entry;
	root_dir_t *next;
} root_dir;

int init_root_dir(int fresh);
int sfs_getnextfilename(char *fname);
int get_rootdir_buffer(root_dir_entry **buffer);
int read_root_dir();
int write_root_dir();
int add_root_dir_entry(int i_node, char *filename);
int add_new_file_root_dir_entry(int i_node, char *filename);
int remove_root_dir_entry(char *filename);
int get_i_node_index(char *filename);
int get_number_of_files();
void print_root_dir();

#endif /* ON_DISK_DATA_STRUCT_H_ */

