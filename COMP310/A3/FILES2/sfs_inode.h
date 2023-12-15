struct inode {
    int size;
    int direct[12];
    int indirect;
};

struct indirect_block {
    int entries[256];
};

struct inode_block {
    struct inode inodes[18];
};

struct superblock {
    int magic;
    int block_size;
    int num_blocks;
    int num_inodes;
    int root_inode;
};

struct block {
    char data[1024]; // Size of a block
};

struct free_bitmap {
    char bits[994]; // Number of data blocks
};

struct inode* root_inode;
struct free_bitmap* bitmap;

int init_superblock(int);

int filename_to_inode_num(char *name, int check_root_dir);
struct inode get_inode(int inode_number);
int update_inode(int inode_number, struct inode inode);
int get_free_inode_num();
int init_root_inode(int fresh);
int check_inode_link(struct inode inode, int link_index);
void create_inode(int inode_number, char* name);
void remove_inode(int inode_number);

int add_inode_blocks_to_fbm(struct inode inode);
int remove_inode_blocks_from_fbm(struct inode inode);
int get_next_free_block();
int init_bitmap(int fresh);

int write_into_data_block(int block_number, int offset, const char *buffer, int size);
int write_into_indirect_data_block(int ind_block_num, int block_index, int offset, const char *buffer, int size);

int read_from_data_block(int block_number, int offset, char *buffer, int size);
int read_from_indirect_data_block(int ind_block_num, int block_index, int offset, char *buffer, int size);