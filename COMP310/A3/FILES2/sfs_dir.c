#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sfs_api.h"
#include "sfs_inode.h"
#include "sfs_dir.h"
#include "disk_emu.h"

/*------------------------------------------------------------------*/
/* Get the next free entry in the directory cache                   */
/*------------------------------------------------------------------*/
int get_free_dir_cache_entry() {
    for (int i = 0; i < max_dir_len; i++) {
        if (dir_cache[i].valid == 0) {
            return i;
        }
    }
    return -1;
}

/*------------------------------------------------------------------*/
/* Initializes the directory cache                                  */
/*------------------------------------------------------------------*/
int init_dir_cache() {
    // Get root inode
    struct inode root_inode = get_inode(root_inode_num);

    // Check all direct blocks
    for (int i = 0; i < 12; i++){
        if (root_inode.direct[i] != -1) { // If the direct block is a valid block #
            // Read the directory block
            struct directory_block* block = (struct directory_block*) malloc(sizeof(struct block));
            read_blocks(data_block_start + root_inode.direct[i], 1, (void *) block);

            for (int j = 0; j < num_files_per_dir_block; j++) {
                if (block->entries[j].valid == 1) {
                    // Add file to directory cache
                    int free_entry_index = get_free_dir_cache_entry();
                    dir_cache[free_entry_index] = block->entries[j];
                }
            }
        }
    }
    return 0;
}

/*------------------------------------------------------------------*/
/* Initializes the file descriptor table                            */
/*------------------------------------------------------------------*/
int init_fdt() {
    for (int i = 0; i < max_number_of_open_files; i++) {
        fdt[i].valid = 0;
    }
    return 0;
}

/*------------------------------------------------------------------*/
/* Get the next free entry in the file descriptor table             */
/*------------------------------------------------------------------*/
int get_free_fdt_entry() {
    for (int i = 0; i < max_number_of_open_files; i++) {
        if (fdt[i].valid == 0) {
            return i;
        }
    }
    return -1;
}

/*------------------------------------------------------------------*/
/* Add enrty to fdt using file inode                                */
/*------------------------------------------------------------------*/
int add_to_fdt(int inode_num) {
    printf("Adding file to fdt\n");
    printf("inode_num: %d\n", inode_num);
    struct inode inode = get_inode(inode_num);
    // Check if the inode is already in fdt
    for (int i = 0; i < max_number_of_open_files; i++) {
        if (fdt[i].valid == 1 && fdt[i].inode == inode_num) {
            printf("File already open\n");
            return -1;
        }
    }
    // Check if there is enough space in the fdt
    int fdt_num = get_free_fdt_entry();
    if (fdt_num == -1) {
        // If there is no space, exit
        printf("No space left in File Descriptor Table\n");
        return -1;
    }

    // Add the file to the file descriptor table
    fdt[fdt_num].valid = 1;
    fdt[fdt_num].fd = fdt_num;
    fdt[fdt_num].inode = inode_num;
    fdt[fdt_num].rw_ptr = inode.size;
    return fdt_num;
}