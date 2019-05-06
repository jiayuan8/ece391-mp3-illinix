#include "filesystem.h"

/*             filesystem initializer                */


/*
 * Function:  void filesystem_init(uint32_t starting_addr)
 * --------------------
 * This function will initialize and get all needed configurations of file system
 *
 *  Inputs:     uint32_t starting_addr: the kernel need to pass the starting address of file dir to thie function
 *
 *  Returns: none
 *
 *  Side effects: none
 *
 */
void filesystem_init(uint32_t starting_addr){

    // start address of file system is passed in kernel.c
    fs_addr = starting_addr;

    // get number of directory entries, number of inodes and number of data blocks
    num_dentries = *((uint32_t*)(fs_addr));
    num_inodes = *((uint32_t*)(fs_addr + BYTE_OFFSET));
    num_d_blocks = *((uint32_t*)(fs_addr + (BYTE_OFFSET << 1)));

    // get the address of first directory entries, first inodes and first data blocks 
    dentry_addr = fs_addr + ENTRY_OFFSET;
    inode_addr = fs_addr + BLOCKSIZE;
    d_block_addr = inode_addr + num_inodes * BLOCKSIZE;

    // set the counter for dir_read() to 0
    dir_location = 0;

    return;
}

/*             file system routines                */

/*
 * Function:  read_dentry_by_name(const uint8_t *fname, dentry_t *dentry)
 * --------------------
 * This function will receive a filename and a pointer to a dentry
 * The function will store the information of the directory entry with the 
 * given file name to the given dentry
 *
 *  Inputs:     const uint8_t *fname: a pointer to a array storing filename
 *              dentry_t *dentry: a pointer to dentry struct that will store the desired information
 *
 *  Returns:    -1: failed
 *              0: success
 *
 *  Side effects: change the data where dentry is pointing to
 *
 */
int32_t read_dentry_by_name(const uint8_t *fname, dentry_t *dentry){
    
    uint32_t cmp_res;           // the comparing result for filenames
    uint32_t cmp_res_len;
    uint32_t i = 0;             // loop counter
    uint32_t temp_addr = dentry_addr;       
    uint32_t fnlength = strlen((int8_t*)fname);

    // if length of filename is larger than 32, return -1
    if(fnlength > F_TYPE_OFFSET){
        return -1;
    }

    // otherwise, compare that name to all directory's filename
    for(i = 0; i < num_dentries; i ++){
        cmp_res = strncmp((int8_t*)fname, (int8_t*)temp_addr, fnlength);

        // check the length of filename
        if(fnlength >= 32 && strlen((int8_t*)temp_addr) >= 32){
            cmp_res_len = 1;
        }else{
            cmp_res_len = (strlen((int8_t*)fname) == strlen((int8_t*)temp_addr));
        }
        // if got the same name. copy the wanted information to buffer
        if(cmp_res == 0 && cmp_res_len == 1){
            memcpy((void*)dentry, (void*)temp_addr, D_ENT_COPY_SIZE);
            return 0;
        }
        // let the temp_addr points to next directory entry
        temp_addr += ENTRY_OFFSET;
    }   

    // if nothing found, return -1
    return -1;
}

/*
 * Function:  read_dentry_by_name(const uint8_t *fname, dentry_t *dentry)
 * --------------------
 * This function will receive a index of directory entry and a pointer to a dentry struct
 * The function will store the information of the desired directory entry with the 
 * given file name to the given dentry
 *
 *  Inputs:     uint32_t index: the index of the desired directory entry
 *              dentry_t *dentry: a pointer to dentry struct that will store the desired information
 *
 *  Returns:    -1: if failed or encounter invalid index
 *              0: success
 *
 *  Side effects: change the data where dentry is pointing to
 *
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t *dentry){
    
    // if the index if invalid, return -1
    if(index >= num_dentries){
        return -1;
    }

    // otherwise, copy the corresponding dentry to buffer
    memcpy((void *)dentry, (void *)(dentry_addr +  index* ENTRY_OFFSET),
               D_ENT_COPY_SIZE);
    
    return 0;
}

/*
 * Function:  read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
 * --------------------
 * This function will receive a index of an index node, an offset,
 * a pointer to a buffer and required length of desired data
 * The function will store the desired data we want to have into buf
 *
 *  Inputs:     uint32_t inode: the index of index node
 *              uint32_t offset: in bytes, indecating how many bytes from the start of the file
 *                              does the caller want to start copy data
 *              uint8_t* buf: a pointer to a buffer that will store the data copied from file
 *              uint32_t length: in bytes, indecating how many bytes of data does the caller want
 *                              to get starting from offset
 *
 *  Returns:    >0: number of bytes copied
 *              0: end of file has been reached
 *              -1: invalid offset or invalid index of index node
 *
 *  Side effects: change the data where buf is pointing to
 *
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    // TODO
    uint32_t cur_inode_addr;        // address of the inode
    uint32_t file_size;             // total file size
    uint32_t real_length;           // the size to be copied
    uint32_t d_block_idx_ptr;       // address
    uint32_t d_block_idx_offset;    // the index offset of datablock's index entry in inode
    uint32_t d_block_byte_offset;   
    uint32_t d_block_real_index;    // the real index of corresponding data block
    uint32_t bytes_to_copy;         // actual bytes to copy
    uint8_t* temp_buf;              // a pointer to buf, but will change if the file is in multiple datablocks
    
    temp_buf = buf;

    if(inode >= num_inodes - 1){
        return -1;              // invalid inode
    }

    cur_inode_addr = inode_addr + BLOCKSIZE * inode;
    file_size = *((uint32_t*)(cur_inode_addr));
    
    if(offset >= file_size){
        return 0;              // invalid filesize
    }

    real_length = (file_size - offset) < length ? (file_size - offset) : length;

    d_block_idx_ptr = cur_inode_addr + UINT32_OFFSET;
    d_block_idx_offset = offset / BLOCKSIZE;
    d_block_byte_offset = offset % BLOCKSIZE;
    d_block_real_index = *(uint32_t*)(d_block_idx_ptr + d_block_idx_offset * UINT32_OFFSET);
    
    if(d_block_real_index > num_d_blocks){
        return -1;
    }

    // handle the first block
    if(BLOCKSIZE - d_block_byte_offset >= real_length){

        memcpy((void*)buf, (void*)(d_block_addr +d_block_real_index * BLOCKSIZE + d_block_byte_offset), real_length);
        return real_length;

    }else{

        memcpy((void*)buf, (void*)(d_block_addr +d_block_real_index * BLOCKSIZE + d_block_byte_offset), BLOCKSIZE - d_block_byte_offset);
        temp_buf += (BLOCKSIZE - d_block_byte_offset);

    }

    // see how many bytes left to be copied
    bytes_to_copy = real_length - (BLOCKSIZE - d_block_byte_offset);

    // handle rest of block
    while(bytes_to_copy){

        // if bytes left more then a block
        if(bytes_to_copy > BLOCKSIZE){

            bytes_to_copy -= BLOCKSIZE;
            d_block_idx_offset += 1;
            d_block_real_index = *(uint32_t*)(d_block_idx_ptr + d_block_idx_offset * UINT32_OFFSET);

            if(d_block_real_index > num_d_blocks){
                return -1;
            }
            
            memcpy((void*)temp_buf, (void*)(d_block_addr + d_block_real_index * BLOCKSIZE), BLOCKSIZE);

            temp_buf += BLOCKSIZE;

        }else{

            // handle the last block
            d_block_idx_offset += 1;
            d_block_real_index = *(uint32_t*)(d_block_idx_ptr + d_block_idx_offset * UINT32_OFFSET);
            memcpy((void*)temp_buf, (void*)(d_block_addr +d_block_real_index * BLOCKSIZE), bytes_to_copy);
            temp_buf += bytes_to_copy;
            //temp_buf[0] = '\0';
            bytes_to_copy = 0;
            break;
        }
    }

    return real_length;
}

/*            driver for file system directory              */

/*
 * Function:  dir_open(const uint8_t *filename)
 * --------------------
 * Open the file directory, for mp3.2, just return 0
 * Follow the syscal routine
 *
 *  Inputs:     const uint8_t *filename: not used right now
 *
 *  Returns:    0
 *
 *  Side effects: none
 *
 */
int32_t dir_open(const uint8_t *filename){
    dir_location = 0;
    return 0;
}

/*
 * Function:  dir_close(const uint8_t *filename)
 * --------------------
 * Close the file directory, for mp3.2, just return 0
 * Follow the syscal routine
 *
 *  Inputs:     int32_t fd: not used right now
 *
 *  Returns:    0
 *
 *  Side effects: none
 *
 */
int32_t dir_close(int32_t fd){
    return 0;
}

/*
 * Function:  dir_read(int32_t fd, void *buf, int32_t nbytes)
 * --------------------
 *  read all the files in directory, and store all names in buffer
 *
 *  Inputs:     int32_t fd: not used right now
 *
 *  Returns:    0 if success, -1 if fail
 *
 *  Side effects: none
 *
 */
int32_t dir_read(int32_t fd, void *buf, int32_t nbytes){
    // TODO
    dentry_t dentry;            // struct to store file information
    int8_t* fname_ptr;          // pointer to a filename
    uint32_t fname_len;         // filenames's length
    uint32_t i;                 // loop counter

    if(read_dentry_by_index(dir_location, &dentry) == 0){
        
        fname_ptr = (int8_t*)(&(dentry.f_name[0]));
        fname_len = strlen(fname_ptr);
        // clear the buffer
        for(i = 0; i < F_TYPE_OFFSET; i ++){
            ((int8_t*)buf)[i] = '\0';
        }
        // adjust the maximum length of filename
        if(nbytes < fname_len){
            fname_len = nbytes;
        }
        strncpy((int8_t*)buf, fname_ptr, fname_len);

        // set the dir_location fo next file
        dir_location += 1;

        return fname_len;

    }else{
        // if end of directory, return 0
        dir_location = 0;
        return 0;
    }


    return 0;
}

/*
 * Function:  dir_write(int32_t fd, const void *buf, int32_t nbytes)
 * --------------------
 *  for checkpoint 3.2, just return -1
 *
 *  Inputs:     int32_t fd, const void *buf, int32_t nbytes useless!!!!!!
 *
 *  Returns:    -1 if fail
 *
 *  Side effects: none
 *
 */
int32_t dir_write(int32_t fd, const void *buf, int32_t nbytes){

    return -1;
}


/*            driver for file system                       */

/*
 * Function:  file_open(const uint8_t *filename)
 * --------------------
 *  Initialize the related variables for file driver
 *  check if the given filename is valid
 *
 *  Inputs:     const uint8_t *filename: the filename passed by caller
 *
 *  Returns:    0 if the filename is valid, -1 if not
 *
 *  Side effects: none
 *
 */
int32_t file_open(const uint8_t *filename){
    return 0;
}

/*
 * Function:  int32_t file_close(int32_t fd)
 * --------------------
 *  Close the file, for cp3.2, just return 0
 *
 *  Inputs:     int32_t fd: useless
 *
 *  Returns:    0
 *
 *  Side effects: none
 *
 */
int32_t file_close(int32_t fd){
    
    return 0;
}

/*
 * Function:  file_read(int32_t fd, void *buf, int32_t nbytes)
 * --------------------
 *  if the file has been successfully opened, this function will be called
 *
 *  Inputs:     int32_t fd: useless
                const void *buf: the buffer that stores data read 
                int32_t nbytes: number of bytes to read
 *
 *  Returns:    -1 if fail
 *              0 if end of file is reached
 *              >0 which is the # of byte read
 *
 *  Side effects: change the content of buffer
 *
 */
int32_t file_read(int32_t fd, void *buf, int32_t nbytes){
    //TODO
    int32_t res;
    uint32_t inode_idx;
    pcb_t * pcb;            // pcb pointer
    uint32_t offset;

    pcb = get_curr_pcb();
    
    inode_idx = pcb->files[fd].inode;
    offset = pcb->files[fd].file_pos;

    res = read_data(inode_idx, offset, (uint8_t*)buf, (uint32_t)nbytes);

    pcb->files[fd].file_pos += res;

    //printf("file content:\n%s\n", (int8_t*)buf);

    return (int32_t)res;

}

/*
 * Function:  file_write(int32_t fd, const void *buf, int32_t nbytes)
 * --------------------
 *  for checkpoint 3.2, just return -1
 *
 *  Inputs:     int32_t fd, const void *buf, int32_t nbytes useless!!!!!!
 *
 *  Returns:    -1 if fail
 *
 *  Side effects: none
 *
 */
int32_t file_write(int32_t fd, const void *buf, int32_t nbytes){
    return -1;
}
