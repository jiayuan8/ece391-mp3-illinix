#include "types.h"
#include "lib.h"
#include "syscall.h"

#ifndef _FILESYSTEM
#define _FILESYSTEM

#define BLOCKSIZE   4096            // size of datablocl
#define NUM_DENTRY  63              // maximum number of file

#define ENTRY_OFFSET    64          // the offset for directory from start address of filesystem
#define BYTE_OFFSET     4           // address offset for 4 byte

#define F_TYPE_OFFSET   32          // size of the buffer that stores filename
#define INODE_OFFSET    36          // offset for pointer to inode
#define UINT32_OFFSET   4
#define D_ENT_COPY_SIZE 40          // size of (filename + filesize + filetype)



uint32_t fs_addr;           // starting address of filesystem

uint32_t dentry_addr;       // first index node's address
uint32_t inode_addr;
uint32_t d_block_addr;      // first data block's address

uint32_t num_dentries;      // number of dirctory entries
uint32_t num_inodes;        // number of inodes
uint32_t num_d_blocks;      // number of data blocks

uint32_t dir_location;      // counter for dir_read
uint8_t opened_filename[F_TYPE_OFFSET];     // the buffer that stores filename for new opened file
uint32_t opened_file_size;                  // stores filesize for new opened file
dentry_t opened_dentry;                     // the dentry_t struct that stores the info of new opened file

uint8_t cur_fname[F_TYPE_OFFSET];
uint32_t cur_fn_len;

// filesystem initialization function
void filesystem_init(uint32_t starting_addr);

// routines in Appendix A
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

// driver functions for directories
int32_t dir_open(const uint8_t *filename);
int32_t dir_close(int32_t fd);
int32_t dir_read(int32_t fd, void *buf, int32_t nbytes);
int32_t dir_write(int32_t fd, const void *buf, int32_t nbytes);

// driver function for files
int32_t file_open(const uint8_t *filename);
int32_t file_close(int32_t fd);
int32_t file_read(int32_t fd, void *buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void *buf, int32_t nbytes);

#endif
