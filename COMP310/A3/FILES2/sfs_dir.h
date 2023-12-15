struct directory_entry {
    int valid;
    char name[16];
    int inode_num;
};

struct directory_block {
    struct directory_entry entries[42];
};

struct fdt_entry {
    int valid;
    int fd;
    int inode;
    int rw_ptr;
};

struct fdt_entry fdt[300];

struct directory_entry dir_cache[500];

int get_free_dir_cache_entry();
int init_dir_cache();

int init_fdt();
int get_free_fdt_entry();
int add_to_fdt(int inode_num);

// int get_inode_num_from_fdt(int fd);
// int get_rw_ptr_from_fdt(int fd);