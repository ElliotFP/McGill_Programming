#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "disk_emu.h"
#include "sfs_api.h"
#include "sfs_inode.h"
#include "constants.h"

int data_block_start;
int num_of_data_blocks;
int root_inode_num;

/* ------- */
/* mksfs() */
/* ------- */
/* This function creates a new file system or loads an existing one */
void mksfs(int fresh)
{
    printf("\n");
    // Initialize variables calculated at runtime
    data_block_start = (int)ceil(1 + (num_of_inodes / num_of_inodes_per_block));
    num_of_data_blocks = max_blocks - data_block_start - 1; // -1 for free bitmap block

    if (fresh)
    {
        printf("Creating new file system\n");
        init_fresh_disk("sfs_disk.disk", block_size_, max_blocks);
    }
    else
    {
        printf("Loading existing file system\n");
        init_disk("sfs_disk.disk", block_size_, max_blocks);
    }

    // Initialize all the datastructures
    init_bitmap(fresh);
    init_superblock(fresh);
    init_root_inode(fresh);
    init_dir_cache();
    init_fdt();

    printf("mksfs finished\n");
}

/*------------------------------------------------------------------*/
/* Get the name of the next file in the directory                   */
/*------------------------------------------------------------------*/
int sfs_getnextfilename(char *fname)
{
    // Copy the name of the next file in the directory into fname.

    // Return 0 if there are no more files in the directory, otherwise return 1.
    return 0;
}

/*------------------------------------------------------------------*/
/* Get the size of the given file                                   */
/*------------------------------------------------------------------*/
int sfs_getfilesize(const char *path)
{
    // Return the size of the given file.
    int inode_num = filename_to_inode_num(path, 1);
    if (inode_num == -1)
    {
        printf("File not found\n");
        return -1;
    }
    struct inode inode = get_inode(inode_num);
    return inode.size;
}

/*------------------------------------------------------------------*/
/* Open the given file                                              */
/*------------------------------------------------------------------*/
int sfs_fopen(char *name)
{
    // Check filename length
    if (strlen(name) > max_fname_size)
    {
        printf("Filename too long\n");
        return -1;
    }
    // Check if the file exists
    int fdt_num = -1;
    int inode_num = filename_to_inode_num(name, 1);
    if (inode_num == -1)
    {
        printf("File not found\n");
        // Check if there is space in the directory
        int dir_num = get_free_dir_cache_entry();
        if (dir_num == -1)
        {
            // If there is no space, exit
            printf("No space left in directory\n");
            return -1;
        }

        // Check if there is enough space in the fdt
        fdt_num = get_free_fdt_entry();
        if (fdt_num == -1)
        {
            // If there is no space, exit
            printf("No space left in File Descriptor Table\n");
            return -1;
        }

        // Get the inode number
        inode_num = get_free_inode_num();
        // Create the inode for the file
        create_inode(inode_num, name);

        // Add the file to the directory cache
        dir_cache[dir_num].valid = 1;
        strcpy(dir_cache[dir_num].name, name);
        dir_cache[dir_num].inode_num = inode_num;
        // Add the file to the file descriptor table
        fdt[fdt_num].valid = 1;
        fdt[fdt_num].fd = fdt_num;
        fdt[fdt_num].inode = inode_num;
        fdt[fdt_num].rw_ptr = 0;
    }
    else
    {
        fdt_num = add_to_fdt(inode_num);
    }
    // Return the file descriptor.
    return fdt_num;
}

/*------------------------------------------------------------------*/
/* Close the given file                                             */
/*------------------------------------------------------------------*/
int sfs_fclose(int fileID)
{
    // Check if the file is open
    if (fdt[fileID].valid == 0)
    {
        printf("File not open\n");
        return -1;
    }
    // Remove the file from the file descriptor table
    fdt[fileID].valid = 0;
    return 0;
}

/*------------------------------------------------------------------*/
/* Write characters from buf into the file associated with fileID   */
/*------------------------------------------------------------------*/
int sfs_fwrite(int fileID, const char *buf, int length)
{
    int initial_length = length;
    // Get the file's inode and rw_pointer position
    int inode_num = fdt[fileID].inode;
    struct inode inode = get_inode(inode_num);
    int rw_ptr = fdt[fileID].rw_ptr;

    // Write loop
    while (length > 0)
    {
        // Get the block number and offset of the rw_ptr
        int inode_link_index = rw_ptr / block_size_;
        int block_offset = rw_ptr % block_size_;

        // Get the number of bytes to write to block
        int write_length;
        if (block_size_ - block_offset < length)
        {
            // If the remaining space in the block is less than the length of the buffer, write to the remaining space
            write_length = block_size_ - block_offset;
        }
        else
        {
            // If the remaining space in the block is greater than the length of the buffer, write the rest of the buffer
            write_length = length;
        }

        // Check if inode link is a valid data block
        if (inode_link_index > 11)
        {
            inode.indirect = check_inode_link(inode, inode_link_index);
            if (inode.indirect == -1)
            { // No space left on disk
                break;
            }
            // Write the data
            write_into_indirect_data_block(inode.indirect, inode_link_index - 12, block_offset, buf, write_length);
        }
        else
        {
            inode.direct[inode_link_index] = check_inode_link(inode, inode_link_index);
            if (inode.direct[inode_link_index] == -1)
            { // No space left on disk
                break;
            }
            // Write the data
            write_into_data_block(inode.direct[inode_link_index], block_offset, buf, write_length);
        }
        length -= write_length;
        rw_ptr += write_length;
        buf += write_length;
        if (rw_ptr > inode.size)
            inode.size = rw_ptr;

        update_inode(inode_num, inode);
    }
    // Perform FDT updates
    fdt[fileID].rw_ptr = rw_ptr;
    // Return the number of bytes written.
    return initial_length - length;
}

/*------------------------------------------------------------------*/
/* Read characters from the file associated with fileID into buf    */
/*------------------------------------------------------------------*/
int sfs_fread(int fileID, char *buf, int length)
{
    int initial_length = length;
    // Get the file's inode and rw_pointer position
    int inode_num = fdt[fileID].inode;
    struct inode inode = get_inode(inode_num);
    int rw_ptr = fdt[fileID].rw_ptr;
    int file_size = inode.size;

    // Read loop
    while (length > 0)
    {
        // Get the block number and offset of the rw_ptr
        int inode_link_index = rw_ptr / block_size_;
        int block_offset = rw_ptr % block_size_;

        // Get the number of bytes to read from block
        int read_length = length;
        // If the remaining space in the block is less than the length of the buffer, write to the remaining space
        if (file_size - rw_ptr < read_length)
            read_length = file_size - rw_ptr;
        // If the remaining space in the block is greater than the length of the buffer, write the rest of the buffer
        if (block_size_ - block_offset < read_length)
            read_length = block_size_ - block_offset;
        if (read_length <= 0)
            break;
        // Check if inode link is a valid data block
        if (inode_link_index > 11)
        {
            read_from_indirect_data_block(inode.indirect, inode_link_index - 12, block_offset, buf, read_length);
        }
        else
        {
            read_from_data_block(inode.direct[inode_link_index], block_offset, buf, read_length);
        }
        length -= read_length;
        rw_ptr += read_length;
        buf += read_length;
    }
    fdt[fileID].rw_ptr = rw_ptr;

    return initial_length - length;
}

/*------------------------------------------------------------------*/
/* Set the current position of the file pointer associated with     */
/* fileID to loc                                                    */
/*------------------------------------------------------------------*/
int sfs_fseek(int fileID, int loc)
{
    // Get the file's rw_pointer position
    if (loc < 0)
    {
        printf("Invalid location\n");
        return -1;
    }
    else if (fileID < 0 || fileID >= max_open_files)
    {
        printf("Invalid location\n");
        return -1;
    }
    if (fdt[fileID].valid == 0)
    {
        printf("File not open\n");
        return -1;
    }
    fdt[fileID].rw_ptr = loc;
    return 0;
}

/*------------------------------------------------------------------*/
/* Remove the file with the given name from the file system         */
/*------------------------------------------------------------------*/
int sfs_remove(char *file)
{
    // Get the inode number of the file
    for (int i = 0; i < max_dir_len; i++)
    {
        if (strcmp(dir_cache[i].name, file) == 0)
        {
            int inode_num = dir_cache[i].inode_num;
            // Remove the file from the directory cache
            dir_cache[i].valid = 0;
            // Remove the file from the file descriptor table
            for (int j = 0; j < max_open_files; j++)
            {
                if (fdt[j].inode == inode_num)
                {
                    fdt[j].valid = 0;
                }
            }
            // Remove the file from the inode table
            remove_inode(inode_num);

            // Return 0 on success, otherwise return -1
            return 0;
        }
    }
    printf("File not found\n");
    return -1;
}