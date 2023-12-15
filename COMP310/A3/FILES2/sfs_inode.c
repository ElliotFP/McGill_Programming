#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sfs_api.h"
#include "sfs_inode.h"
#include "sfs_dir.h"
#include "disk_emu.h"

/*------------------------------------------------------------------*/
/* Initializes the superblock                                       */
/*------------------------------------------------------------------*/
int init_superblock(int fresh) {
    struct superblock* sblock = (struct superblock*) calloc(1, sizeof(struct block)); 

    // Handle superblock creation
    if (fresh) {
        // Set the magic number
        int random_number = rand() % 1000;
        sblock->magic = random_number;

        // Set the block size to 1024
        sblock->block_size = 1024;

        // Set the number of blocks to 1024
        sblock->num_blocks = 1024;

        // Set the number of inodes to 512 (Maximum number of files)
        sblock->num_inodes = 501;

        // Set root directory i-Node number to i-Node 0
        sblock->root_inode = 0;
        // Write superblock to disk
        write_blocks(0, 1, (void *) sblock);
    }

    // Handle data from superblock
    read_blocks(0, 1, sblock);
    root_inode_num = sblock->root_inode;
    return 0;
}

/*------------------------------------------------------------------*/
/* Gets the inode number corresponding to the file name             */
/*------------------------------------------------------------------*/
int filename_to_inode_num(char *name, int check_root_dir) {
    // Check the directory cache
    for (int i = 0; i < max_dir_len; i++) {
        if (dir_cache[i].valid == 1 && strcmp(dir_cache[i].name, name) == 0) {
            return dir_cache[i].inode_num;
        }
    }
    if (check_root_dir == 0) {
        return -1;
    }
    // Check the root inode
    for (int i = 0; i < 12; i++) {
        if (root_inode->direct[i] != -1) { // If the direct block is a valid block #
            printf("direct block: %d\n", root_inode->direct[i]);
            // Read the directory block
            struct directory_block* block = (struct directory_block*) calloc(1, sizeof(struct block));
            read_blocks(data_block_start + root_inode->direct[i], 1, (void *) block);
            // Check if the filename is in the directory block
            for (int j = 0; j < num_files_per_dir_block; j++) {
                if (block->entries[j].valid == 1 && strcmp(block->entries[j].name, name) == 0) {
                    return block->entries[j].inode_num;
                }
            }
        }
    }
    return -1;
}


/*------------------------------------------------------------------*/
/* Gets the inode entry corresponding to the inode number           */
/*------------------------------------------------------------------*/
struct inode get_inode(int inode_number) {
    // Get the block number
    int block_number = ((int) inode_number / num_of_inodes_per_block) + 1;
    struct inode_block* block = (struct inode_block*) calloc(1, sizeof(struct block));
    
    // Read the block
    read_blocks(block_number, 1, (void *) block);
    
    // Get the inode entry number
    int inode_entry_number = inode_number % num_of_inodes_per_block;
    
    // Return the inode entry
    return block->inodes[inode_entry_number];
}

/*------------------------------------------------------------------*/
/* Updates the inode entry corresponding to the inode number        */
/*------------------------------------------------------------------*/
int update_inode(int inode_number, struct inode inode) {
    // Get the block number
    int block_number = ((int) inode_number / num_of_inodes_per_block) + 1;
    struct inode_block* block = (struct inode_block*) calloc(1, sizeof(struct block));
    
    // Read the block
    read_blocks(block_number, 1, (void *) block);
    
    // Get the inode entry number
    int inode_entry_number = inode_number % num_of_inodes_per_block;
    
    // Update the inode entry
    block->inodes[inode_entry_number] = inode;
    
    // Write the block
    write_blocks(block_number, 1, (void *) block);
    return 0;
}


/*------------------------------------------------------------------*/
/* Gets the next free inode number                                  */
/*------------------------------------------------------------------*/
int get_free_inode_num() {
    // Check if there is space in the inode table
    for (int i = 1; i < num_of_inodes; i++) {
        int found = 0;
        for (int j = 0; j < max_dir_len; j++) {
            if (dir_cache[j].valid == 1 && dir_cache[j].inode_num == i) {
                found = 1;
                break;
            }
        }
        if (found == 0) {
            return i;
        }
    }
    return -1;
}


/*------------------------------------------------------------------*/
/* Initializes the root inode                                       */
/*------------------------------------------------------------------*/
int init_root_inode(int fresh) {
    // Get the root inode
    root_inode = (struct inode*) calloc(1, sizeof(struct inode));
    *root_inode = get_inode(root_inode_num);
    if (fresh) {
        // Set the size to 0
        root_inode->size = 0;
        
        // Set all direct blocks to -1
        for (int i = 0; i < 12; i++) {
            root_inode->direct[i] = -1;
        }
        
        // Set indirect block to -1
        root_inode->indirect = -1;
        
        // Write the root inode to disk
        update_inode(root_inode_num, *root_inode);
    } else {
        // Remove the inode pages to the free bitmap
        remove_inode_blocks_from_fbm(*root_inode);
    }
    return 0;
}

/*------------------------------------------------------------------*/
/* Checks if the inode link is a valid data block, and if not,      */
/* gets a free block and adds it to the inode                       */
/*------------------------------------------------------------------*/
int check_inode_link(struct inode inode, int link_index) {
    // Check if the link is a valid indirect block
    if (link_index > 11 && link_index < 12+max_ints_in_block) {
        if (inode.indirect == -1) { // If the indirect block is not a valid block #
            // Get a free data block
            int block_num = get_next_free_block();
            if (block_num == -1) {
                printf("No space left on disk\n");
                return -1;
            }
            // Link the data block to the inode
            inode.indirect = block_num;

            // Make all links -1
            struct indirect_block* indirect_block = (struct indirect_block*) calloc(1, sizeof(struct block));
            for (int i = 0; i < max_ints_in_block; i++) {
                indirect_block->entries[i] = -1;
            }
            // Write the indirect block to disk
            write_blocks(data_block_start + block_num, 1, (void *) indirect_block);
        }
        return inode.indirect;
    } else if (link_index >= 12+max_ints_in_block) {
        printf("Maximum file size reached\n");
        return -1;
    }

    // Check if the link is a valid data block
    if (inode.direct[link_index] == -1) {
        // Get a free data block
        int block_num = get_next_free_block();
        if (block_num == -1) {
            printf("No space left on disk\n");
            return -1;
        }
        // Link the data block to the inode
        inode.direct[link_index] = block_num;
        // update_inode(root_inode_num, inode);
    }
    return inode.direct[link_index];
}


/*------------------------------------------------------------------*/
/* Creates an inode                                                 */
/*------------------------------------------------------------------*/
void create_inode(int inode_number, char *name) {
    struct inode inode = get_inode(inode_number);
    inode.size = 0;
    for (int i = 0; i < 12; i++) {
        inode.direct[i] = -1;
    }
    inode.indirect = -1;
    update_inode(inode_number, inode);

    // Add the file to a directory block entry
    for (int i = 0; i < 12; i++) {
        if (root_inode->direct[i] != -1) { // If the direct block is a valid block #
            // Read the directory block
            struct directory_block* dir_block = (struct directory_block*) calloc(1, sizeof(struct block));
            read_blocks(data_block_start + root_inode->direct[i], 1, (void *) dir_block);
            // Check if there is space in the directory block
            for (int j = 0; j < num_files_per_dir_block; j++) {
                if (dir_block->entries[j].valid == 0) {
                    dir_block->entries[j].valid = 1;
                    strcpy(dir_block->entries[j].name, name);
                    dir_block->entries[j].inode_num = inode_number;
                    write_blocks(data_block_start + root_inode->direct[i], 1, (void *) dir_block);
                    return;
                }
            }
        } else {
            // Get a free data block
            int block_num = get_next_free_block();
            if (block_num == -1) {
                printf("No space left on disk\n");
                return;
            }
            // Link the data block to the root inode
            root_inode->direct[i] = block_num;
            update_inode(root_inode_num, *root_inode);
            // Create the directory block
            struct directory_block* dir_block = (struct directory_block*) calloc(1, sizeof(struct block));
            // Add the file to the directory block
            dir_block->entries[0].valid = 1;
            strcpy(dir_block->entries[0].name, name);
            dir_block->entries[0].inode_num = inode_number;
            // Write the directory block to disk
            write_blocks(data_block_start + block_num, 1, (void *) dir_block);
            return;
        }
    }
    return;
}

/*------------------------------------------------------------------*/
/* Removes an inode from the directory table                        */
/*------------------------------------------------------------------*/
void remove_inode(int inode_number) {
    // Add the linked inode blocks from the free bitmap
    struct inode inode = get_inode(inode_number);
    add_inode_blocks_to_fbm(inode);

    // Remove the file from the directory table
    for (int i = 0; i < 12; i++) {
        if (root_inode->direct[i] != -1) { // If the direct block is a valid block #
            // Read the directory block
            struct directory_block* dir_block = (struct directory_block*) calloc(1, sizeof(struct block));
            read_blocks(data_block_start + root_inode->direct[i], 1, (void *) dir_block);
            // Check if there is space in the directory block
            for (int j = 0; j < num_files_per_dir_block; j++) {
                if (dir_block->entries[j].valid == 1 && dir_block->entries[j].inode_num == inode_number) {
                    dir_block->entries[j].valid = 0;
                    write_blocks(data_block_start + root_inode->direct[i], 1, (void *) dir_block);
                    return;
                }
            }
        }
    }
    return;
}


/*------------------------------------------------------------------*/
/* Adds the inode pages to the free bitmap                          */
/*------------------------------------------------------------------*/
int add_inode_blocks_to_fbm(struct inode inode) {
    // Check direct blocks
    for (int i = 0; i < 12; i++) {
        if (inode.direct[i] != -1) {
            bitmap->bits[inode.direct[i]] = 1;
        }
    }
    if (inode.indirect != -1) {
        struct indirect_block* indirect_block = (struct indirect_block*) calloc(1, sizeof(struct block));
        read_blocks(data_block_start + inode.indirect, 1, (void *) indirect_block);
        for (int i = 0; i < max_ints_in_block; i++) {
            if (indirect_block->entries[i] != -1) {
                bitmap->bits[indirect_block->entries[i]] = 1;
            }
        }
        bitmap->bits[inode.indirect] = 1;
    }

    write_blocks(max_blocks-1, 1, (void *) bitmap);
    return 0;
}

/*------------------------------------------------------------------*/
/* Removes the inode pages from the free bitmap                     */
/*------------------------------------------------------------------*/
int remove_inode_blocks_from_fbm(struct inode inode) {
    // Check direct blocks
    for (int i = 0; i < 12; i++) {
        if (inode.direct[i] != -1) {
            bitmap->bits[inode.direct[i]] = 0;
        }
    }
    if (inode.indirect != -1) {
        struct indirect_block* indirect_block = (struct indirect_block*) calloc(1, sizeof(struct block));
        read_blocks(data_block_start + inode.indirect, 1, (void *) indirect_block);
        for (int i = 0; i < max_ints_in_block; i++) {
            if (indirect_block->entries[i] != -1) {
                bitmap->bits[indirect_block->entries[i]] = 0;
            }
        }
    }
    write_blocks(max_blocks-1, 1, (void *) bitmap);
    return 0;
}

/*------------------------------------------------------------------*/
/* Gets the next free data block using free bitmap                  */
/*------------------------------------------------------------------*/
int get_next_free_block() {
    for (int i = 0; i < num_of_data_blocks; i++) {
        if (bitmap->bits[i] == 1) {
            bitmap->bits[i] = 0;
            write_blocks(max_blocks-1, 1, (void *) bitmap);
            return i;
        }
    }
    return -1;
}

/*------------------------------------------------------------------*/
/* Initializes the free bitmap                                      */
/*------------------------------------------------------------------*/
int init_bitmap(int fresh) {
    bitmap = (struct free_bitmap*) calloc(1, sizeof(struct block));
    if (fresh){
        for (int i = 0; i < num_of_data_blocks; i++) {
            bitmap->bits[i] = 1;
        }
        write_blocks(max_blocks-1, 1, (void *) bitmap);
    } else {
        read_blocks(max_blocks-1, 1, (void *) bitmap);
    }
    return 0;
}

/*------------------------------------------------------------------*/
/* Writes data into a data block                                    */
/*------------------------------------------------------------------*/
int write_into_data_block(int block_number, int offset, const char *buffer, int size) {
    // Read the block
    struct block* block = (struct block*) calloc(1, sizeof(struct block));
    read_blocks(data_block_start + block_number, 1, (void *) block);
    // Write the data into the block
    memcpy(block->data + offset, buffer, size);
    // Write the block to disk
    write_blocks(data_block_start + block_number, 1, (void *) block);
    return 0;
}

/*------------------------------------------------------------------*/
/* Writes data into an indirect data block                          */
/*------------------------------------------------------------------*/
int write_into_indirect_data_block(int ind_block_num, int block_index, int offset, const char *buffer, int size) {
    // Read the indirect block
    struct indirect_block* ind_block = (struct indirect_block*) calloc(1, sizeof(struct block));
    read_blocks(data_block_start + ind_block_num, 1, (void *) ind_block);
    // Check if the 'bloc_index' data block is a valid block #
    if (ind_block->entries[block_index] == -1) {
        // Get a free data block
        int block_num = get_next_free_block();
        if (block_num == -1) {
            printf("No space left on disk\n");
            return -1;
        }
        // Link the data block to the indirect block
        ind_block->entries[block_index] = block_num;
        // Write the indirect block to disk
        write_blocks(data_block_start + ind_block_num, 1, (void *) ind_block);
    }

    // Write the data into the data block
    write_into_data_block(ind_block->entries[block_index], offset, buffer, size);
    return 0;
}

/*------------------------------------------------------------------*/
/* Reads data from a data block                                     */
/*------------------------------------------------------------------*/
int read_from_data_block(int block_number, int offset, char *buffer, int size) {
    // Read the block
    struct block* block = (struct block*) calloc(1, sizeof(struct block));
    read_blocks(data_block_start + block_number, 1, (void *) block);
    // Read the data from the block
    memcpy(buffer, block->data + offset, size);
    return 0;
}

/*------------------------------------------------------------------*/
/* Reads data from an indirect data block                           */
/*------------------------------------------------------------------*/
int read_from_indirect_data_block(int ind_block_num, int block_index, int offset, char *buffer, int size) {
    // Read the indirect block
    struct indirect_block* ind_block = (struct indirect_block*) calloc(1, sizeof(struct block));
    read_blocks(data_block_start + ind_block_num, 1, (void *) ind_block);
    // Check if the 'bloc_index' data block is a valid block #
    if (ind_block->entries[block_index] == -1) {
        printf("Invalid block number\n");
        return -1;
    }
    // Read the data from the data block
    read_from_data_block(ind_block->entries[block_index], offset, buffer, size);
    return 0;
}