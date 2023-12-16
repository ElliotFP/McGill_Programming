#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sfs_api.h"
#include "structures.h"
#include "disk_emu.h"
#include "constants.h"

int init_superblock(int fresh) // Initializes the superblock
{
    struct superblock *sblock = (struct superblock *)calloc(1, sizeof(struct block));
    printf("Initializing superblock\n");

    // Handle superblock creation
    if (fresh)
    {
        // set superblock fields
        sblock->magic = magic_;             // 888
        sblock->block_size = block_size_;   // 1024
        sblock->num_blocks = max_blocks;    // 1024
        sblock->num_inodes = num_of_inodes; // 256
        sblock->root_inode = 0;             // inode number of root directory

        // Write superblock to disk
        write_blocks(0, 1, (void *)sblock);
    }

    // Handle data from superblock
    read_blocks(0, 1, sblock);
    root_inode_num = sblock->root_inode;
    return 0;
}

int i_fname_to_num(char *name, int check_root_dir) // check_root_dir = 1 if we want to check the root directory
{
    // Check the directory cache
    for (int i = 0; i < max_dir_len; i++)
    {
        if (dir_cache[i].valid == 1 && strcmp(dir_cache[i].name, name) == 0)
        {
            return dir_cache[i].inode_num;
        }
    }
    if (check_root_dir == 0)
    {
        return -1;
    }
    // Check the root inode
    for (int i = 0; i < 12; i++)
    {
        if (root_inode->direct[i] != -1)
        { // If the direct block is a valid block #
            printf("direct block: %d\n", root_inode->direct[i]);
            // Read the directory block
            struct directory_block *block = (struct directory_block *)calloc(1, sizeof(struct block));
            read_blocks(db_start + root_inode->direct[i], 1, (void *)block);
            // Check if the filename is in the directory block
            for (int j = 0; j < files_per_dir_block; j++)
            {
                if (block->entries[j].valid == 1 && strcmp(block->entries[j].name, name) == 0)
                {
                    return block->entries[j].inode_num;
                }
            }
        }
    }
    return -1;
}

struct inode i_get_inode(int inode_number) // Returns the inode with the given inode number
{
    // Get the block number
    int block_number = ((int)inode_number / num_of_inodes_per_block) + 1;
    struct inode_block *block = (struct inode_block *)calloc(1, sizeof(struct block));

    // Read the block
    read_blocks(block_number, 1, (void *)block);

    // Get the inode entry number
    int inode_entry_number = inode_number % num_of_inodes_per_block;

    // Return the inode entry
    return block->inodes[inode_entry_number];
}

int i_update_inode(int inode_number, struct inode inode) // Updates the inode with the given inode number
{
    // Get the block number
    int block_number = ((int)inode_number / num_of_inodes_per_block) + 1;
    struct inode_block *block = (struct inode_block *)calloc(1, sizeof(struct block));

    // Read the block
    read_blocks(block_number, 1, (void *)block);

    // Get the inode entry number
    int inode_entry_number = inode_number % num_of_inodes_per_block;

    // Update the inode entry
    block->inodes[inode_entry_number] = inode;

    // Write the block
    write_blocks(block_number, 1, (void *)block);
    return 0;
}

int i_get_free_inode() // Returns the inode number of the next free inode
{
    // Check if there is space in the inode table
    for (int i = 1; i < num_of_inodes; i++)
    {
        int found = 0;
        for (int j = 0; j < max_dir_len; j++)
        {
            if (dir_cache[j].valid == 1 && dir_cache[j].inode_num == i)
            {
                found = 1;
                break;
            }
        }
        if (found == 0)
        {
            return i;
        }
    }
    return -1;
}

int i_init_root_inode(int fresh) // Initializes the root inode
{
    printf("Initializing root inode\n");
    // Get the root inode
    root_inode = (struct inode *)calloc(1, sizeof(struct inode));
    *root_inode = i_get_inode(root_inode_num);
    if (fresh)
    {
        // Set the size to 0
        root_inode->size = 0;

        // Set all direct blocks to -1
        for (int i = 0; i < 12; i++)
        {
            root_inode->direct[i] = -1;
        }

        // Set indirect block to -1
        root_inode->indirect = -1;

        // Write the root inode to disk
        i_update_inode(root_inode_num, *root_inode);
    }
    else
    {
        // Remove the inode pages to the free bitmap
        i_remove_inode_blocks_from_fbm(*root_inode);
    }
    return 0;
}

int i_check_link(struct inode inode, int link_index)
{
    // Check if the link is a valid indirect block
    if (link_index > 11 && link_index < 12 + max_ints_in_block)
    {
        if (inode.indirect == -1)
        { // If the indirect block is not a valid block #
            // Get a free data block
            int block_num = get_next_free_block();
            if (block_num == -1)
            {
                printf("No space left on disk\n");
                return -1;
            }
            // Link the data block to the inode
            inode.indirect = block_num;

            // Make all links -1
            struct indirect_block *indirect_block = (struct indirect_block *)calloc(1, sizeof(struct block));
            for (int i = 0; i < max_ints_in_block; i++)
            {
                indirect_block->entries[i] = -1;
            }
            // Write the indirect block to disk
            write_blocks(db_start + block_num, 1, (void *)indirect_block);
        }
        return inode.indirect;
    }
    else if (link_index >= 12 + max_ints_in_block)
    {
        printf("Maximum file size reached\n");
        return -1;
    }

    // Check if the link is a valid data block
    if (inode.direct[link_index] == -1)
    {
        // Get a free data block
        int block_num = get_next_free_block();
        if (block_num == -1)
        {
            printf("No space left on disk\n");
            return -1;
        }
        // Link the data block to the inode
        inode.direct[link_index] = block_num;
        // i_update_inode(root_inode_num, inode);
    }
    return inode.direct[link_index];
}

void i_create_inode(int inode_number, char *name) // Creates an inode with the given inode number and name
{
    struct inode inode = i_get_inode(inode_number);
    inode.size = 0;
    for (int i = 0; i < 12; i++)
    {
        inode.direct[i] = -1;
    }
    inode.indirect = -1;
    i_update_inode(inode_number, inode);

    // Add the file to a directory block entry
    for (int i = 0; i < 12; i++)
    {
        if (root_inode->direct[i] != -1)
        { // If the direct block is a valid block #
            // Read the directory block
            struct directory_block *dir_block = (struct directory_block *)calloc(1, sizeof(struct block));
            read_blocks(db_start + root_inode->direct[i], 1, (void *)dir_block);
            // Check if there is space in the directory block
            for (int j = 0; j < files_per_dir_block; j++)
            {
                if (dir_block->entries[j].valid == 0)
                {
                    dir_block->entries[j].valid = 1;
                    strcpy(dir_block->entries[j].name, name);
                    dir_block->entries[j].inode_num = inode_number;
                    write_blocks(db_start + root_inode->direct[i], 1, (void *)dir_block);
                    return;
                }
            }
        }
        else
        {
            // Get a free data block
            int block_num = get_next_free_block();
            if (block_num == -1)
            {
                printf("No space left on disk\n");
                return;
            }
            // Link the data block to the root inode
            root_inode->direct[i] = block_num;
            i_update_inode(root_inode_num, *root_inode);
            // Create the directory block
            struct directory_block *dir_block = (struct directory_block *)calloc(1, sizeof(struct block));
            // Add the file to the directory block
            dir_block->entries[0].valid = 1;
            strcpy(dir_block->entries[0].name, name);
            dir_block->entries[0].inode_num = inode_number;
            // Write the directory block to disk
            write_blocks(db_start + block_num, 1, (void *)dir_block);
            return;
        }
    }
    return;
}

void i_remove_inode(int inode_number) // Removes the inode with the given inode number
{
    // Add the linked inode blocks from the free bitmap
    struct inode inode = i_get_inode(inode_number);
    i_add_blocks_to_fbm(inode);

    // Remove the file from the directory table
    for (int i = 0; i < 12; i++)
    {
        if (root_inode->direct[i] != -1)
        { // If the direct block is a valid block #
            // Read the directory block
            struct directory_block *dir_block = (struct directory_block *)calloc(1, sizeof(struct block));
            read_blocks(db_start + root_inode->direct[i], 1, (void *)dir_block);
            // Check if there is space in the directory block
            for (int j = 0; j < files_per_dir_block; j++)
            {
                if (dir_block->entries[j].valid == 1 && dir_block->entries[j].inode_num == inode_number)
                {
                    dir_block->entries[j].valid = 0;
                    write_blocks(db_start + root_inode->direct[i], 1, (void *)dir_block);
                    return;
                }
            }
        }
    }
    return;
}

int i_add_blocks_to_fbm(struct inode inode) // Adds the blocks of the given inode to the free bitmap
{
    // Check direct blocks
    for (int i = 0; i < 12; i++)
    {
        if (inode.direct[i] != -1)
        {
            bitmap->bits[inode.direct[i]] = 1;
        }
    }
    if (inode.indirect != -1)
    {
        struct indirect_block *indirect_block = (struct indirect_block *)calloc(1, sizeof(struct block));
        read_blocks(db_start + inode.indirect, 1, (void *)indirect_block);
        for (int i = 0; i < max_ints_in_block; i++)
        {
            if (indirect_block->entries[i] != -1)
            {
                bitmap->bits[indirect_block->entries[i]] = 1;
            }
        }
        bitmap->bits[inode.indirect] = 1;
    }

    write_blocks(max_blocks - 1, 1, (void *)bitmap);
    return 0;
}

int i_remove_inode_blocks_from_fbm(struct inode inode) // Removes the blocks of the given inode from the free bitmap
{
    // Check direct blocks
    for (int i = 0; i < 12; i++)
    {
        if (inode.direct[i] != -1)
        {
            bitmap->bits[inode.direct[i]] = 0;
        }
    }
    if (inode.indirect != -1)
    {
        struct indirect_block *indirect_block = (struct indirect_block *)calloc(1, sizeof(struct block));
        read_blocks(db_start + inode.indirect, 1, (void *)indirect_block);
        for (int i = 0; i < max_ints_in_block; i++)
        {
            if (indirect_block->entries[i] != -1)
            {
                bitmap->bits[indirect_block->entries[i]] = 0;
            }
        }
    }
    write_blocks(max_blocks - 1, 1, (void *)bitmap);
    return 0;
}

int get_next_free_block() // Returns the index of the next free block
{
    for (int i = 0; i < num_db; i++)
    {
        if (bitmap->bits[i] == 1)
        {
            bitmap->bits[i] = 0;
            write_blocks(max_blocks - 1, 1, (void *)bitmap);
            return i;
        }
    }
    return -1;
}

int init_bitmap(int fresh) // Initializes the free bitmap
{
    printf("Initializing bitmap\n");
    bitmap = (struct free_bitmap *)calloc(1, sizeof(struct block));
    if (fresh)
    {
        for (int i = 0; i < num_db; i++)
        {
            bitmap->bits[i] = 1;
        }
        write_blocks(max_blocks - 1, 1, (void *)bitmap);
    }
    else
    {
        read_blocks(max_blocks - 1, 1, (void *)bitmap);
    }
    return 0;
}

int write_to_block(int block_number, int offset, const char *buffer, int size) // write data into a data block
{
    // Read the block
    struct block *block = (struct block *)calloc(1, sizeof(struct block));
    read_blocks(db_start + block_number, 1, (void *)block);
    memcpy(block->data + offset, buffer, size); // Copy the data from the buffer to the block
    // Write the block to disk
    write_blocks(db_start + block_number, 1, (void *)block);
    return 0;
}

int write_to_indirect(int ind_block_num, int block_index, int offset, const char *buffer, int size) // write data into an indirect data block
{
    // Read the indirect block
    struct indirect_block *ind_block = (struct indirect_block *)calloc(1, sizeof(struct block));
    read_blocks(db_start + ind_block_num, 1, (void *)ind_block);
    // Check if the 'bloc_index' data block is a valid block #
    if (ind_block->entries[block_index] == -1)
    {
        // Get a free data block
        int block_num = get_next_free_block();
        if (block_num == -1)
        {
            printf("No space left on disk\n");
            return -1;
        }
        // Link the data block to the indirect block
        ind_block->entries[block_index] = block_num;
        // Write the indirect block to disk
        write_blocks(db_start + ind_block_num, 1, (void *)ind_block);
    }

    // Write the data into the data block
    write_to_block(ind_block->entries[block_index], offset, buffer, size);
    return 0;
}

int read_from_block(int block_number, int offset, char *buffer, int size) // read data from a data block
{
    // Read the block
    struct block *block = (struct block *)calloc(1, sizeof(struct block));
    read_blocks(db_start + block_number, 1, (void *)block);
    memcpy(buffer, block->data + offset, size); // Copy the data from the block to the buffer
    return 0;
}

int read_from_indirect(int ind_block_num, int block_index, int offset, char *buffer, int size) // read data from an indirect data block
{
    // Read the indirect block
    struct indirect_block *ind_block = (struct indirect_block *)calloc(1, sizeof(struct block));
    read_blocks(db_start + ind_block_num, 1, (void *)ind_block);
    // Check if the 'bloc_index' data block is a valid block #
    if (ind_block->entries[block_index] == -1)
    {
        printf("Invalid block number\n");
        return -1;
    }
    // Read the data from the data block
    read_from_block(ind_block->entries[block_index], offset, buffer, size);
    return 0;
}

int d_get_free_entry() // Returns the index of the next free directory cache entry
{
    for (int i = 0; i < max_dir_len; i++)
    {
        if (dir_cache[i].valid == 0)
        {
            return i;
        }
    }
    return -1;
}

int init_dir_cache() // Initializes the directory cache
{
    // Get root inode
    struct inode root_inode = i_get_inode(root_inode_num);

    // Check all direct blocks
    for (int i = 0; i < 12; i++)
    {
        if (root_inode.direct[i] != -1)
        { // If the direct block is a valid block #
            // Read the directory block
            struct directory_block *block = (struct directory_block *)malloc(sizeof(struct block));
            read_blocks(db_start + root_inode.direct[i], 1, (void *)block);

            for (int j = 0; j < files_per_dir_block; j++)
            {
                if (block->entries[j].valid == 1)
                {
                    // Add file to directory cache
                    int free_entry_index = d_get_free_entry();
                    dir_cache[free_entry_index] = block->entries[j];
                }
            }
        }
    }
    return 0;
}

int init_fdt() // Initializes the file descriptor table
{
    for (int i = 0; i < max_open_files; i++)
    {
        fdt[i].valid = 0;
    }
    return 0;
}

int f_get_free_entry() // Returns the index of the next free file descriptor table entry
{
    for (int i = 0; i < max_open_files; i++)
    {
        if (fdt[i].valid == 0)
        {
            return i;
        }
    }
    return -1;
}

int f_add(int inode_num) // Adds the file with the given inode number to the file descriptor table
{
    printf("Adding file to fdt\n");
    printf("inode_num: %d\n", inode_num);
    struct inode inode = i_get_inode(inode_num);
    // Check if the inode is already in fdt
    for (int i = 0; i < max_open_files; i++)
    {
        if (fdt[i].valid == 1 && fdt[i].inode == inode_num)
        {
            printf("File already open\n");
            return -1;
        }
    }
    // Check if there is enough space in the fdt
    int fdt_num = f_get_free_entry();
    if (fdt_num == -1)
    {
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
