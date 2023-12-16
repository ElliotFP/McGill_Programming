
#include "constants.h"

/* INode */
struct inode
{
    int size;
    int direct[12];
    int indirect;
};

struct inode_block
{
    struct inode inodes[num_of_inodes_per_block];
};

int i_fname_to_num(char *name, int check_root_dir);       // check_root_dir = 1 if we want to check the root directory
struct inode i_get_inode(int inode_number);               // Returns the inode with the given inode number
int i_update_inode(int inode_number, struct inode inode); // Updates the inode with the given inode number
int i_get_free_inode();                                   // Returns the inode number of the next free inode
int i_init_root_inode(int fresh);                         // Initializes the root inode
int i_check_link(struct inode inode, int link_index);     // Returns 1 if the link is valid, 0 otherwise
void i_create_inode(int inode_number, char *name);        // Creates an inode with the given inode number and name
void i_remove_inode(int inode_number);                    // Removes the inode with the given inode number
int i_add_blocks_to_fbm(struct inode inode);              // Adds the blocks of the given inode to the free bitmap
int i_remove_inode_blocks_from_fbm(struct inode inode);

/* Superblock */

struct superblock
{
    int magic;
    int block_size;
    int num_blocks;
    int num_inodes;
    int root_inode;
};

int init_superblock(int); // Initializes the superblock

struct free_bitmap
{
    char bits[994]; // Number of data blocks
};

struct free_bitmap *bitmap; // Pointer to the free bitmap

int get_next_free_block();  // Returns the index of the next free block
int init_bitmap(int fresh); // Initializes the free bitmap

/* Directory */

struct directory_entry
{
    int valid;
    char name[16];
    int inode_num;
};

struct directory_block
{
    struct directory_entry entries[42];
};

struct inode *root_inode;                      // Pointer to the root inode
struct directory_entry dir_cache[max_dir_len]; // Directory cache

int d_get_free_entry(); // Returns the index of the next free directory cache entry
int init_dir_cache();   // Initializes the directory cache

/* File Descriptor Table */

struct fdt_entry
{
    int valid;
    int fd;
    int inode;
    int rw_ptr;
};
struct fdt_entry fdt[max_open_files];

int init_fdt();           // Initializes the file descriptor table
int f_get_free_entry();   // Returns the index of the next free file descriptor table entry
int f_add(int inode_num); // Adds the file with the given inode number to the file descriptor table

struct indirect_block
{
    int entries[max_ints_in_block];
};
struct block
{
    char data[block_size_]; // Size of a block
};

// helper functions for writing to blocks
int write_to_block(int block_number, int offset, const char *buffer, int size);
int write_to_indirect(int ind_block_num, int block_index, int offset, const char *buffer, int size);

// helper functions for reading from blocks
int read_from_block(int block_number, int offset, char *buffer, int size);
int read_from_indirect(int ind_block_num, int block_index, int offset, char *buffer, int size);