#include "terminal.h"

volatile uint8_t terminal_running[MAX_TERMINAL_NUM] = {0,0,0};
/* int32_t terminal_open(const uint8_t* filename)
 * Inputs: filename -- pointer to the name of the file
 * Return Value: 0 on success
 * Function: open the terminal
 */
extern int32_t terminal_open(const uint8_t* filename){
    return 0;
}

/* int32_t terminal_close(int32_t fd)
 * Inputs: fd -- file discriptor
 * Return Value: 0 on success
 * Function: close the terminal
 */
extern int32_t terminal_close(int32_t fd){
    return 0;
}

/* int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes)
 * Inputs: fd -- file discriptor
 *         buf -- buffer that stores the input string
 *         nbytes -- the number of bytes to be read
 * Return Value: number of bytes read
 * Function: read the input from the user input
 */
extern int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes){
    int32_t i = 0;
    pcb_t* pcb;
    pcb = get_curr_pcb();
    // wait for enter signal
    while(1){
        if(enter_state != 0 && get_terminal_id(pcb->pid) == current_terminal){
            break;
        }
    }
    uint8_t tid = get_terminal_id(pcb->pid);
    printf("current tid: %d\n", tid);
    cli();
    enter_state = 0;
    sti();

    // copy content to buffer
    uint8_t* buf_cast_ptr = (uint8_t *)buf;
    for(i = 0; (i < (int32_t)key_arr_ptr) && (i < nbytes); i ++ ){
        buf_cast_ptr[i] = key_arr[i];
    }
    buf_cast_ptr[i] = '\0';
    // clear key array
    key_arr_clear();

    return i;
}

/* int32_t terminal_write(int32_t fd, void* buf, int32_t nbytes)
 * Inputs: fd -- file discriptor
 *         buf -- buffer that stores the input string
 *         nbytes -- the number of bytes to be write
 * Return Value: number of bytes written
 * Function: write the user input to terminal
 */
extern int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes){
    int32_t i = 0;

    cli();

    pcb_t* pcb = get_curr_pcb();
    uint8_t tid = get_terminal_id(pcb->pid);
    uint8_t* buf_cast_ptr = (uint8_t *)buf;

    if(tid == current_terminal){
        for(i = 0; i < nbytes; i++){
            putc(buf_cast_ptr[i]);
            set_column_stack();
        }
    }else{
        if(tid >= MAX_TERMINAL_NUM){
            return -1;
        }
        for(i = 0; i < nbytes; i++){
            putc_terminal(buf_cast_ptr[i], tid);
        }
        terminal_info[tid].column_stack_old = terminal_info[tid].screen_pos_x;
    }

    sti();

    return i;
}

/* void init_terminal()
 * ------------------------------------------------------------------------------------------------------
 * Descriptions: This function is responsible for initializing terminal
 *               It will:
 *                  1. initialize 3 keyboard arrays
 *                  2. initialize the array which stores information of whether a terminal is running
 *                  3. initialize terminal_info
 *                  4. initialize the background color and text color
 *                  5. execute shell for the first terminal
 * Inputs: none
 * Return Value: none
 * Function: initialize the terminal
 */
void init_terminal(){
    int32_t i, j;
    clear();
    key_arr = key_arr_all[0];
    for(i = 0; i < (KEY_ARR_SIZE + 1) * 3; i ++){
        key_arr[i] = '\0';
    }

    for (i = 0; i < MAX_TERMINAL_NUM; i++) {
        for (j = 0; j < MAX_TASK; j++) {
            process_terminal[i][j] = 0;
        }
        process_terminal_cnt[i] = 0;
    }

    current_terminal = 0;
    running_terminal = 0;

    for(i = 0; i < MAX_TERMINAL_NUM; i ++ ){
        terminal_info[i].screen_pos_x = 0;
        terminal_info[i].screen_pos_y = 0;
        terminal_info[i].key_arr_ptr_old = 0;
        terminal_info[i].key_arr_overflow_old = 0;
        terminal_info[i].row_stack_old = 0;
        terminal_info[i].column_stack_old = 0;
        terminal_info[i].key_arr_old[0] = '\0';
        current_terminal = i;
        if(i == 0){
            terminal_info[i].screen_buffer = (uint8_t*)TERMINAL_BUFFER_1;
        }else if(i == 1){
            terminal_info[i].screen_buffer = (uint8_t*)TERMINAL_BUFFER_2;
        }else{
            terminal_info[i].screen_buffer = (uint8_t*)TERMINAL_BUFFER_3;
        }
        clear();
        copy_from_screen_buffer(terminal_info[i].screen_buffer);
    }
    current_terminal = 0;
    clear();
    copy_from_screen_buffer(terminal_info[0].screen_buffer);
    sti();
    terminal_running[0] = 1;
    execute((uint8_t*)"shell");

    return;
}

/* int32_t switch_to_terminal(uint8_t terminal_id)
 * --------------------------------------------------------------------------------------
 * Descriptions:    This function is responsible for terminal switching
 *                  if the terminal we want to switch to is not running
 *                      it will do a context switch and execute shell program
 *                  if the terminal we want to switch is running
 *                      it will simply switch the terminal
 * Inputs:          uint8_t terminal_id :   the terminal's id, which can be 0 or 1 or 2
 * Outputs:         (int32_t)   -1:     invalid terminal id
 *                              0       function returns successfully
 * Side Effects: Modefies the structure of terminal info
 */
extern int32_t switch_to_terminal(uint8_t terminal_id){
    cli();
    pcb_t * old_pcb;
    int32_t new_pid;

    // check if the terminal id is valid
    if(terminal_id >= MAX_TERMINAL_NUM){
        return -1;
    }

    // check if we are switching to other terminals
    if(terminal_id == current_terminal){
        return 0;
    }

    // if the terminal we are switching to is not running
    if(terminal_running[terminal_id] == 0){
        if ((new_pid = get_pid()) == -1) {
            printf("process number reaches maximum! Can't switch to new terminal!\n391OS> ");
            return -1;
        }
        terminal_running[terminal_id] = 1;
        save_terminal_info(current_terminal);
        current_terminal = terminal_id;
        //map_user_video();
        clear();
        restore_terminal_info(current_terminal);
        old_pcb = get_curr_pcb();
        //save the last esp and ebp

        asm volatile(
            "movl %%esp, %%eax;"
            "movl %%ebp, %%ebx;"
            : "=a" (old_pcb->schedule_esp), "=b" (old_pcb->schedule_ebp)
        );
        //printf("========>%x\n", old_pcb->pid);
        //printf("========>%x\n", old_pcb->schedule_esp);
        //printf("========>%x\n", old_pcb->schedule_ebp);

        running_terminal = terminal_id;
        sti();

        // execute the new terminal!
        execute((uint8_t*)"shell");
        return 0;
    }


    save_terminal_info(current_terminal);
    current_terminal = terminal_id;
    //map_user_video();
    clear();

    restore_terminal_info(terminal_id);
    sti();
    return 0;
}

/* int32_t save_terminal_info(uint8_t terminal_id)
 * --------------------------------------------------------------------------------------
 * Descriptions:    This funtion will save all the parameters of a terminal to
 *                  the stored terminal struct
 * Inputs:          uint8_t terminal_id :   the terminal's id, which can be 0 or 1 or 2
 * Outputs:         (int32_t)   -1:     invalid terminal id
 *                              0       function returns successfully
 * Side Effects: Modefies the structure of terminal info
 */
extern int32_t save_terminal_info(uint8_t terminal_id){
    // Invalid terminal ID
    if(terminal_id >= MAX_TERMINAL_NUM){
        return -1;
    }

    // save relavent info to the struct
    terminal_info[terminal_id].screen_pos_x = get_screen_x();
    terminal_info[terminal_id].screen_pos_y = get_screen_y();
    terminal_info[terminal_id].key_arr_ptr_old = key_arr_ptr;
    terminal_info[terminal_id].key_arr_overflow_old = key_arr_overflow;
    terminal_info[terminal_id].row_stack_old = get_row_stack();
    terminal_info[terminal_id].column_stack_old = get_column_stack();
    strncpy((int8_t*)terminal_info[terminal_id].key_arr_old, (int8_t*)key_arr, KEY_ARR_SIZE_OLD);
    copy_from_screen_buffer(terminal_info[terminal_id].screen_buffer);

    return 0;
}

/* int32_t restore_terminal_info(uint8_t terminal_id)
 * --------------------------------------------------------------------------------------
 * Descriptions:    This funtion will restore all the parameters of a terminal from
 *                  the stored terminal struct
 * Inputs:          uint8_t terminal_id :   the terminal's id, which can be 0 or 1 or 2
 * Outputs:         (int32_t)   -1:     invalid terminal id
 *                              0       function returns successfully
 * Side Effects: Modifies all the parameters related to screen!!!
 */
extern int32_t restore_terminal_info(uint8_t terminal_id){
    // Invalid terminal ID
    if(terminal_id > MAX_TERMINAL_NUM){
        return -1;
    }

    // restore relavent info from the struct
    set_cursor_position(terminal_info[terminal_id].screen_pos_x, terminal_info[terminal_id].screen_pos_y);
    key_arr_ptr = terminal_info[terminal_id].key_arr_ptr_old;
    key_arr_overflow = terminal_info[terminal_id].key_arr_overflow_old;
    set_row_stack_in(terminal_info[terminal_id].row_stack_old);
    set_column_stack_in(terminal_info[terminal_id].column_stack_old);
    strncpy((int8_t*)key_arr, (int8_t*)terminal_info[terminal_id].key_arr_old, KEY_ARR_SIZE_OLD);
    //clear();
    copy_to_screen_buffer(terminal_info[terminal_id].screen_buffer);

    return 0;
}

/* uint8_t is_running(uint8_t terminal_id)
 * --------------------------------------------------------------------------------------
 * Descriptions:    This function will reveice a terminal id, it will return whether
 *                  the terminal is currently running
 * Inputs:          uint8_t terminal_id :   the terminal's id, which can be 0 or 1 or 2
 * Outputs:         (uint8_t)   0x3F:   invalid terminal id
 *                              0x01    terminal is running
 *                              0x00    terminal is not running
 * Side Effects: None
 */
uint8_t is_running(uint8_t terminal_id){
    if(terminal_id > MAX_TERMINAL_NUM){
        return 0x3F;
    }
    return terminal_running[terminal_id] == 1;
}
