#ifndef SFS_API_H
#define SFS_API_H

// // You can add more into this file.
extern const int block_size;
extern const int max_blocks;
extern const int max_ints_in_block;

extern const int max_dir_len;
extern const int num_of_inodes;
extern const int num_of_inodes_per_block;

extern const int max_fname_size;
extern const int max_number_of_open_files;

extern int data_block_start;
extern int num_of_data_blocks;

extern const int num_files_per_dir_block;

extern int root_inode_num;


void mksfs(int);

int sfs_getnextfilename(char*);

int sfs_getfilesize(const char*);

int sfs_fopen(char*);

int sfs_fclose(int);

int sfs_fwrite(int, const char*, int);

int sfs_fread(int, char*, int);

int sfs_fseek(int, int);

int sfs_remove(char*);

#endif
