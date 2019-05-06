#include "syscall.h"

// specific file operation tables
file_operation_ptrs fail_funcs = {fail, fail, fail, fail};
file_operation_ptrs stdin_funcs = {terminal_open, terminal_read, fail, terminal_close};
file_operation_ptrs stdout_funcs = {terminal_open, fail, terminal_write, terminal_close};
file_operation_ptrs dir_funcs = {dir_open, dir_read, dir_write, dir_close};
file_operation_ptrs rtc_funcs = {rtc_open, rtc_read, rtc_write, rtc_close};
file_operation_ptrs file_funcs = {file_open, file_read, file_write, file_close};

// 0 indicates file discriptor not used, and 1 indicates file discriptor is in use
uint32_t process[MAX_TASK] = {0,0,0,0,0,0};


// return value for halt
uint32_t retval = 0;
/*
 * Function:  int32_t halt(uint8_t status)
 * --------------------
 * This function does four things: restore parent data, restore parent paging,
 * close any relavent FDs, and jump to execute return. This function should
 * never return to its caller.
 *
 *  Inputs:     uint8_t status: the status to be return by execute
 *
 *  Returns:    -1: failed
 *              status: success
 *
 *  Side effects: halt the program
 *
 */
int32_t halt(uint8_t status) {
    uint32_t i;             // loop counter
    pcb_t * pcb;            // pcb pointer
    uint32_t esp, ebp;      // store parent esp and ebp
cli();
    pcb = get_pcb_by_index(process_terminal[running_terminal][process_terminal_cnt[running_terminal] - 1]);

    //printf("--->Executing halt!\n");
    //printf("--->pid: %d\n", running_terminal);
    /* restore parent data */
    // restore parent esp and ebp
    esp = pcb->parent_esp;
    ebp = pcb->parent_ebp;

    //print("parentpid: %d\n", pcb->parent_pid);

    /* restore parent paging */
    map_program(pcb->parent_pid);
    // set tss parameters
    tss.ss0 = KERNEL_DS;
    tss.esp0 = EIGHT_MB_SIZE - pcb->parent_pid * EIGHT_KB_SIZE - ESP_OFFSET;
    // free up current pid in the pid array
    process[pcb->pid] = NOT_IN_USE;
    process_terminal[running_terminal][process_terminal_cnt[running_terminal] - 1] = NOT_IN_USE;
    process_terminal_cnt[running_terminal] -= 1;

    /* close any relavent fds */
    for (i = 0; i < MAX_FD; i++) {
        if (pcb->files[i].flags == IN_USE) {
            pcb->files[i].ptrs->close(i);
        }
        pcb->files[i].flags = NOT_IN_USE;
        pcb->files[i].file_pos = 0;
        pcb->files[i].inode = -1;
        pcb->files[i].ptrs = &fail_funcs;
    }

    // check if we are halting shell
    if (pcb->pid == 0 || process_terminal_cnt[running_terminal] == 0) {
        execute((uint8_t*)"shell");
    }

    retval = status;

    /* jump to execute return */
    // return status to execute
    asm volatile(
        "movl %0, %%esp;"
        "movl %1, %%ebp;"
        "jmp return_to_execute;"
        : /* no outputs */
        : "g" (esp), "g" (ebp)
        : "%eax"
    );

    // should never reach here
    return -1;
}

/*
 * Function:  int32_t execute(const uint8_t* command)
 * --------------------
 * This function does six things:
 *          1. parse arguments
 *          2. executable check
 *          3. set up program paging
 *          4. user-level program loader
 *          5. create pcb
 *          6. context switch
 *
 *  Inputs:     const uint8_t* command: the command to be executed
 *
 *  Returns:    -1: failed
 *              0: success
 *              256: exception occur
 *
 *  Side effects: execute the program
 *
 */
int32_t execute(const uint8_t* command) {
    uint32_t i;                         // loop counter
    uint32_t new_pid;                   // the pid of the executable
    uint8_t exe_name[F_TYPE_OFFSET];    // executable name
    uint8_t args_buf[ARG_MAX];          // buffer that stores the argument
    uint8_t magics[FOUR_BYTES];         // magic number buffer
    uint8_t real_magics[FOUR_BYTES] = {
        MAGIC_ONE, MAGIC_TWO, MAGIC_THREE, MAGIC_FOUR
    };
    uint32_t command_start = 0;         // variables to find the exe name and argument
    uint32_t command_length = 0;
    dentry_t dentry;
    uint32_t entry;                     // stores the entry point of the executable
    pcb_t * parent_pcb;                 // parent pcb
    pcb_t * new_pcb;                    // new executable's pcb
    //printf("hi");
cli();
    /**********************
     * 1. Parse Arguments *
     **********************/
    // find the first char that is not space
    while (command[command_start] == SPACE)
        command_start++;
    // find the end of the executable name
    while (command[command_start + command_length] != SPACE
            && command[command_start + command_length] != '\n'
            && command[command_start + command_length] != '\0')
        command_length++;
    // printf("%d\n", command_length);
    // copy the executable name to the exe_name buffer
    for (i = command_start; i < command_start + command_length; i++) {
        if(i > KEY_ARR_SIZE - 1){
            printf(">>>>>>>> keyboard array overflow <<<<<<<<<\n");
            return -1;
        }
        if(i - command_start > F_TYPE_OFFSET){
            printf(">>>>>>>> Commond is too long! <<<<<<<<<\n");
            return -1;
        }
        exe_name[i-command_start] = command[i];
    }
    //printf("hi\n");
    if (i-command_start >= F_TYPE_OFFSET)
        i = command_start + F_TYPE_OFFSET - 1;
    exe_name[i-command_start] = '\0';

    // copy the arguments into the argument buffer
    command_length += command_start;
    command_start = command_length;

    while (command[command_length] != '\n'
            && command[command_length] != '\0') {
        args_buf[command_length - command_start] = command[command_length];
        command_length++;
    }
    args_buf[command_length-command_start] = '\0';
    //printf("hi");

    /***********************
     * 2. Executable Check *
     ***********************/
    // check if the executable exists
    if (read_dentry_by_name((uint8_t*)exe_name, &dentry) == -1)
        return -1;

    // check if the four magic numbers are correct
    read_data(dentry.i_node, 0, magics, FOUR_BYTES);
    for (i = 0; i < FOUR_BYTES; i++) {
        if (magics[i] != real_magics[i])
            return -1;
    }

    // get the entry point of the executable
    read_data(dentry.i_node, ENTRY_POINT, magics, FOUR_BYTES);
    entry = *((uint32_t*)magics);


    /***************************
     * 3. Setup Program Paging *
     ***************************/
    // get available pid
    for (new_pid = 0; new_pid < MAX_TASK; new_pid++) {
        if (process[new_pid] == NOT_IN_USE) {
            process[new_pid] = IN_USE;
            break;
        }
    }
    // check if it reaches the maximum process
    if (new_pid >= MAX_TASK)
        return -1;
    // map program paging
    map_program(new_pid);


    /********************************
     * 4. User-level Progran Loader *
     ********************************/
    read_data(dentry.i_node, 0, (uint8_t*)(PROGRAM_OFFSET), FOUR_MB_SIZE);


    /*****************
     * 5. Create PCB *
     *****************/
    // check if it's the first user program loaded and set the parent pcb
    if (new_pid == 0)
        parent_pcb = NULL;
    else{
        if(process_terminal_cnt[current_terminal] != 0){
            parent_pcb = get_pcb_by_index(process_terminal[current_terminal][process_terminal_cnt[current_terminal] - 1]);
        }else{
            parent_pcb = get_pcb_by_index(0);
        }

    }

    process_terminal_cnt[current_terminal] += 1;

    // assign the new pcb
    new_pcb = get_pcb_by_index(new_pid);
    new_pcb->pid = new_pid;
    process_terminal[current_terminal][process_terminal_cnt[current_terminal] - 1] = new_pid;
    // set the parent pid parameter
    if (parent_pcb != NULL)
        new_pcb->parent_pid = parent_pcb->pid;
    else
        new_pcb->parent_pid = new_pid;

    // save argument in pcb
    strncpy((int8_t*)new_pcb->argument, (int8_t*)args_buf, ARG_MAX);

    // save parent esp and ebp
    asm volatile(
        "movl %%esp, %%eax;"
        "movl %%ebp, %%ebx;"
        : "=a" (new_pcb->parent_esp), "=b" (new_pcb->parent_ebp)
        :
        : "cc"
    );

    // fill up the first(stdin) and the second(stdout) entry
    // of the file discriptor table
    // stdin file
    new_pcb->files[0].inode = -1;                   // this field shouldn't be used
    new_pcb->files[0].file_pos = 0;
    new_pcb->files[0].flags = IN_USE;
    new_pcb->files[0].ptrs = &stdin_funcs;
    // stdout file
    new_pcb->files[1].inode = -1;                   // this field shouldn't be used
    new_pcb->files[1].file_pos = 0;
    new_pcb->files[1].flags = IN_USE;
    new_pcb->files[1].ptrs = &stdout_funcs;

    // set the rest of the file discriptors unused
    for (i = MIN_FD; i < MAX_FD; i++) {
        new_pcb->files[i].flags = NOT_IN_USE;
    }

    new_pcb->sighandler = signal_default;
    new_pcb->pending_signal = 0x3F;


    /*********************
     * 6. Context Switch *
     *********************/
    // set tss parameters
    tss.ss0 = KERNEL_DS;
    tss.esp0 = EIGHT_MB_SIZE - new_pid * EIGHT_KB_SIZE - ESP_OFFSET;

    // prepare for context switch
    asm volatile(
        "xorl %%eax, %%eax;"
        "movw %w0, %%ax;"
        "movw %%ax, %%ds;"
        "pushl %%eax;"          // push the user data segment information
        "pushl %1;"             // push user program esp
        "pushfl;"               // push eflags
        "popl %%eax;"           // manually enable the interrupt
        "orl $0x200, %%eax;"
        "pushl %%eax;"
        "pushl %2;"             // push code segment information
        "pushl %3;"             // push user program eip
        "iret;"
        "return_to_execute:;"
        : /* no outputs */
        : "g" (USER_DS), "g" (_128_MB_SIZE + FOUR_MB_SIZE - ESP_OFFSET), "g" (USER_CS), "g" (entry)
        : "eax"
    );

    // check if any exception occured
    sti();
    if (retval == EXCEPTION_MAGIC)
        return EXCEPTION_RET;
    return 0;
}

/*
 * Function:  read(int32_t fd, void* buf, int32_t nbytes)
 * --------------------
 * This function read the file to a buffer. Given a specific type of
 * the file to be read, corresponding read function will be called.
 *
 *  Inputs:     int32_t fd: file discriptor
 *              void* buf: buffer stores the contents to be read
 *              int32_t nbytes: number of bytes to be read
 *
 *  Returns:    -1: failed
 *              n: number of bytes successfully read
 *
 *  Side effects: read a file into a buffer
 *
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes) {
    pcb_t * pcb;                // pcb pointer

    // check if the file discriptor is within range
    if (fd < 0 || fd >= MAX_FD)
        return -1;
    // check valid inputs
    if (nbytes < 0)
        return -1;
    if (buf == NULL)
        return -1;

    pcb = get_curr_pcb();
    // check if file is opened
    if (pcb->files[fd].flags == NOT_IN_USE)
        return -1;

    return pcb->files[fd].ptrs->read(fd, buf, nbytes);
}

/*
 * Function:  write(int32_t fd, void* buf, int32_t nbytes)
 * --------------------
 * This function write the file to a buffer. Given a specific type of
 * the file to be written, corresponding write function will be called.
 *
 *  Inputs:     int32_t fd: file discriptor
 *              void* buf: buffer stores the contents to be written
 *              int32_t nbytes: number of bytes to be written
 *
 *  Returns:    -1: failed
 *              n: number of bytes successfully written
 *
 *  Side effects: write a file into a buffer
 *
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes) {
    pcb_t * pcb;                // pcb pointer

    // check if the file discriptor is within range
    if (fd < 0 || fd >= MAX_FD)
        return -1;
    // check valid inputs
    if (nbytes < 0)
        return -1;
    if (buf == NULL)
        return -1;

    pcb = get_curr_pcb();
    // check if file is opened
    if (pcb->files[fd].flags == NOT_IN_USE)
        return -1;

    return pcb->files[fd].ptrs->write(fd, buf, nbytes);
}

/*
 * Function:  open(const uint8_t* filename)
 * --------------------
 * This function opens a file and set appropriate parameters to the
 * pcb file discriptor
 *
 *  Inputs:     const uint8_t* filename: filename to be opened
 *
 *  Returns:    -1: failed
 *              i: file discriptor corresponding to the opened file
 *
 *  Side effects: change pcb parameters
 *
 */
int32_t open(const uint8_t* filename) {
    int i;                  // loop index
    pcb_t * pcb;            // pcb pointer
    dentry_t dentry;        // holds file information

    pcb = get_curr_pcb();

    // check valid inputs
    if (filename == NULL)
        return -1;

    //printf("filename: %s\n", filename);

    // check if the file exists
    if (read_dentry_by_name(filename, &dentry) == -1)
        return -1;

    // assign a free file discriptor to the file
    for (i = MIN_FD; i < MAX_FD; i++) {
        if (pcb->files[i].flags == NOT_IN_USE) {
            pcb->files[i].flags = IN_USE;
            pcb->files[i].file_pos = 0;
            pcb->files[i].inode = dentry.i_node;
            //printf("fit in pcb->files[%d]\n", i);
            break;
        }
        // add case that when pcb->file is all occupied, return -1
        if(i == MAX_FD - 1){
            //printf("fail to open because pcb is full\n");
            return -1;
        }
    }

    // check if there is no available fd
    if (i == MAX_FD)
        return -1;

    // set specific functions according to the file type
    switch (dentry.f_type) {
        case RTC_TYPE:
            pcb->files[i].ptrs = &rtc_funcs;
            break;

        case DIR_TYPE:
            pcb->files[i].ptrs = &dir_funcs;
            break;

        case REGULAR_FILE:
            pcb->files[i].ptrs = &file_funcs;
            break;

        default:
            printf("invalid file type!!!\n");
            return -1;
    }

    // open the file
    if (pcb->files[i].ptrs->open(filename) == -1)
        return -1;

    return i;
}

/*
 * Function:  close(int32_t fd)
 * --------------------
 * This function closes a file and set appropriate parameters to the
 * pcb file discriptor
 *
 *  Inputs:     int32_t fd: file discriptor to be closed
 *
 *  Returns:    -1: failed
 *
 *  Side effects: change pcb parameters
 *
 */
int32_t close(int32_t fd) {
    pcb_t * pcb;            // pcb pointer

    // check valid inputs
    if (fd < MIN_FD || fd >= MAX_FD)
        return -1;

    pcb = get_curr_pcb();
    // check if the file is opened before
    if (pcb->files[fd].flags == NOT_IN_USE)
        return -1;

    // set fd not in use for future uses
    pcb->files[fd].flags = NOT_IN_USE;

    return pcb->files[fd].ptrs->close(fd);
}

/*
 * Function:  getargs(uint8_t* buf, int32_t nbytes)
 * --------------------
 * This function returns the argument previously saved
 *
 *  Inputs:     uint8_t* buf: the buffer that stores the argument
 *              int32_t nbytes: number of bytes in the buffer
 *
 *  Returns:    -1: failed
 *              0: success
 *
 *  Side effects: return the argument to the caller
 *
 */
int32_t getargs(uint8_t* buf, int32_t nbytes) {
    pcb_t * pcb = get_curr_pcb();
    uint32_t offset = 0;
    // check if the argument is empty
    if (pcb->argument[0] == '\0')
        return -1;

    for(offset = 0; offset < ARG_MAX; offset ++){
        if(pcb->argument[offset] == ' ' && pcb->argument[offset + 1] != ' '){
            offset += 1;
            break;
        }
        if(offset == ARG_MAX - 2){
            return -1;
        }
    }

    // copy the string into the buffer
    strncpy((int8_t*)buf, (int8_t*)(&(pcb->argument[offset])), nbytes);
    //printf("%s\n%d", buf, strlen((int8_t*)buf));
    return 0;
}

/*
 * Function:  int32_t vidmap(uint8_t** screen_start)
 * --------------------
 * This function maps virtual address 128MB+80MB to video mem
 *
 *  Inputs:     uint8_t** screen_start: a pointer to the pointer that points to the start addr of video memory
 *
 *  Returns:    -1: failed
 *              0: success
 *
 *  Side effects: return the assigned virtual memory address for user
 *
 */
int32_t vidmap(uint8_t** screen_start) {
    if((uint32_t)(screen_start) < _128_MB_SIZE || (uint32_t)(screen_start) > _128_MB_SIZE + FOUR_MB_SIZE - ESP_OFFSET){
        return -1;
    }
    map_user_video();
    *screen_start = (uint8_t*)USER_VIDEO;
    return (int32_t)USER_VIDEO;
}

int32_t set_handler(int32_t signum, void* handler_address) {
    if(signum < 0 || signum > 4){
        return -1;
    }
    if((uint32_t)handler_address == 0){
        return 0;
    }else{
        pcb_t * pcb = get_curr_pcb();
        pcb->sighandler = handler_address;
        return 0;
    }
}

int32_t sigreturn(void) {
    return -1;
}

int32_t play(const uint8_t* filename){
    if((uint32_t)(filename) < _128_MB_SIZE || (uint32_t)(filename) > _128_MB_SIZE + FOUR_MB_SIZE - ESP_OFFSET){
        return -1;
    }
    play_music((int8_t*)filename);
    return 0;
}

/*
 * int32_t fail()
 * Inputs: none
 * Return Value: -1
 * Function: this function should never be called
 */
int32_t fail() {
    //printf("This function shouldn't be called!!! Unable to perform any operation.\n");
    return -1;
}

/*
 * int32_t get_pid()
 * Inputs: none
 * Return Value: available pid or -1 if process reaches maximum
 * Function: this function returns the next available pid
 */
extern int32_t get_pid() {
    int32_t new_pid;
    for (new_pid = 0; new_pid < MAX_TASK; new_pid++) {
        if (process[new_pid] == NOT_IN_USE) {
            return new_pid;
        }
    }
    return -1;
}

/*
 * int32_t get_terminal_id()
 * Inputs: process id
 * Return Value: the terminal that the pid is running on or 0x3F if
 *               the process is not running
 * Function: this function returns the terminal id
 */
uint8_t get_terminal_id(uint32_t pid){
    uint8_t i = 0;
    uint8_t j = 0;
    for(i = 0; i < MAX_TERMINAL_NUM; i ++){
        for(j = 0; j < MAX_TASK; j ++){
            if(process_terminal[i][j] == pid){
                return i;
            }
        }
    }
    return 0x3F;
}


void _kill(){
    halt(0);
    return;
}

void kill(){
    halt(0xF);
}

void send_signal(uint8_t signum){
    pcb_t* pcb = get_curr_pcb();
    // printf("hello %d\n", running_terminal);
    pcb->sighandler(signum);
    return;
}

void signal_default(uint8_t signum){
    // set_up_stack_frame();
    switch(signum){
        case DIV_ZERO:
            //printf("receive DIV_ZERO, terminating...\n");
            kill();
            break;

        case SEGFAULT:
            //printf("receive SEGFAULT, terminating...\n");
            kill();
            break;
        
        case INTERRUPT:
            //printf("receive INTERRUPT, terminating...\n");
            _kill();
            break;

        case ALARM:
            //printf("reveice ALARM, ignoring...\n");
            break;

        case USER1:
            //printf("receive USER1, ignoreing...\n");
            break;

    }

    return;

}
