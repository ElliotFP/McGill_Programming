
#define magic_ 888            // magic number, lucky number in chinese
#define block_size_ 1024      // 1KB
#define max_blocks 1024       // 1MB
#define max_ints_in_block 256 // 1024 / 4 = 256

#define max_dir_len 255            // maximum number of directory entries in a directory
#define num_of_inodes 256          // total number of inodes
#define num_of_inodes_per_block 18 // 1024 / (14 x 4) = 18.28

#define max_fname_size 20  // Maximum filename size
#define max_open_files 200 // Maximum number of open files

#define files_per_dir_block block_size_ / 24 // 16 characters + 2 int = 24 bytes