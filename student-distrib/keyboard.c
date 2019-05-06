#include "keyboard.h"
#include "lib.h"
#include "syscall.h"
#include "terminal.h"

// variables defined
volatile uint8_t caps_lock_enabled = 0;
volatile uint8_t shift_state = 0;
volatile uint8_t ctrl_state = 0;
volatile uint8_t alt_state = 0;

// reference of the scan code: https://wiki.osdev.org/PS/2_Keyboard
// some keys are not used currently and are marked as '\0'
char key_scancode[TOTAL_STATE][SCANCODE_NUM] = {
    {
    '\0','\0',                                          // 1-2 the 0th index contains nothing, and the first is esc
    '1','2','3','4','5','6','7','8','9','0','-','=',    // 3-14 keyboard 0 to 9, minus and equal sign
    '\0',                                               // 15 backspace
    '\t',                                               // 16 tab
    'q','w','e','r','t','y','u','i','o','p','[',']',    // 17-28 third line on keyboard
    '\n',                                               // 29 enter
    '\0',                                               // 30 crtl
    'a','s','d','f','g','h','j','k','l',';','\'','`',   // 31-42 fourth line on keyboard
    '\0', '\\',                                         // 43-44 left shift and slash
    'z','x','c','v','b','n','m',',','.','/', '\0', '\0',     // 45-55 fifth line on keyboard
    '\0', ' ', '\0'                                    //  This line is for PtScr, Alt, Spc, CpsLk
    },
    // Case that only shift is pressed
    {
    '\0','\0',                                          // the 0th index contains nothing, and the first is esc
    '!','@','#','$','%','^','&','*','(',')','_','+',    // keyboard 0 to 9, minus and equal sign
    '\0',                                               // backspace
    '\t',                                               // tab
    'Q','W','E','R','T','Y','U','I','O','P','{','}',    // third line on keyboard
    '\n',                                               // enter
    '\0',                                               // crtl
    'A','S','D','F','G','H','J','K','L',':','\"','~',   // fourth line on keyboard
    '\0', '|',                                          // left shift and slash
    'Z','X','C','V','B','N','M','<','>','?', '\0', '\0',      // fifth line on keyboard
    '\0', ' ', '\0'                                    // This line is for PtScr, Alt, Spc, CpsLk
    },
    // Case that only capslock is enabled;
    {
    '\0','\0',                                          // the 0th index contains nothing, and the first is esc
    '1','2','3','4','5','6','7','8','9','0','-','=',    // keyboard 0 to 9, minus and equal sign
    '\0',                                               // backspace
    '\t',                                               // tab
    'Q','W','E','R','T','Y','U','I','O','P','[',']',    // third line on keyboard
    '\n',                                               // enter
    '\0',                                               // crtl
    'A','S','D','F','G','H','J','K','L',';','\'','`',   // fourth line on keyboard
    '\0', '\\',                                         // left shift and slash
    'Z','X','C','V','B','N','M',',','.','/', '\0', '\0',     // fifth line on keyboard
    '\0', ' ', '\0'                                    // This line is for PtScr, Alt, Spc, CpsLk
    },
    // Case that capslock is enabled and shift in pressed
    {
    '\0','\0',                                          // the 0th index contains nothing, and the first is esc
    '!','@','#','$','%','^','&','*','(',')','_','+',    // keyboard 0 to 9, minus and equal sign
    '\0',                                               // backspace
    '\t',                                               // tab
    'q','w','e','r','t','y','u','i','o','p','{','}',    // third line on keyboard
    '\n',                                               // enter
    '\0',                                               // crtl
    'a','s','d','f','g','h','j','k','l',':','\"','~',   // fourth line on keyboard
    '\0', '|',                                          // left shift and slash
    'z','x','c','v','b','n','m','<','>','?', '\0', '\0',     // fifth line on keyboard
    '\0', ' ', '\0'                                    // This line is for PtScr, Alt, Spc, CpsLk
    }
};

/* void init_keyboard();
 * Inputs: none
 * Return Value: none
 * Function: initialize the keyboard by enabling the corresponding irq
 */
void init_keyboard() {
    enable_irq(KEYBOARD_IRQ);
    key_arr_ptr = 0;
    enter_state = 0;
    key_arr_overflow = 0;
    set_cursor_position(0,0);
}

/* void keyboard_handler();
 * Inputs: none
 * Return Value: none
 * Function: read the keyboard port and display the corresponding
 *           character onto the screen
 */
void keyboard_handler() {
    //clear();
    // get the scan code from the keyboard

    unsigned flages = 0;
    // enter critical section
    cli_and_save(flages);

    unsigned scancode = 0;

    // wait for port to send signal
    while(1){
        scancode = inb(KEY_DATA_PORT);
        if(scancode > 0){
            break;
        }
    }

    // execute the scancode
    switch(scancode){
        case L_SHIFT_CODE:
            shift_state = 1;
            break;
        case R_SHIFT_CODE:
            shift_state = 1;
            break;
        case L_SHIFT_REL_CODE:
            shift_state = 0;
            break;
        case R_SHIFT_REL_CODE:
            shift_state = 0;
            break;
        case CPSLK_CODE:
            if(caps_lock_enabled){
                caps_lock_enabled = 0;
            }else{
                caps_lock_enabled = 1;
            }
            break;
        case L_CTRL_CODE:
            ctrl_state = 1;
            break;
        case L_CTRL_REL_CODE:
            ctrl_state = 0;
            break;

        case ENTER_CODE:
            //input_handler(scancode);
            key_arr_push(key_scancode[NOTHING_PRESS][scancode]);
            enter_state = 1;
            enter_handler();
            break;

        case BKSP_CODE:
            backspace_handler();
            break;
        case TAB_CODE:
            break;
        case ALT_CODE:
            alt_state = 1;
            break;
        case ALT_REL_CODE:
            alt_state = 0;
            break;

        case ESC_CODE:
            break;

        case NUM_STAR_CODE:
            break;

        default:
            input_handler(scancode);

    }
    // send eoi to pic
    send_eoi(KEYBOARD_IRQ);

    // leave critical section
    restore_flags(flages);

}


/* void input_handler(unsigned s_code);
 * Inputs: s_code: scancode
 * Return Value: none
 * Function: read the scancode and display the corresponding
 *           character onto the screen
 */

void input_handler(unsigned s_code){
    unsigned char current_state;

    if(ctrl_state == 1 && s_code == SCANCODE_L){
        // ctrl-l clears the screen
        clear();
        //key_arr_clear();
        set_cursor_position(0,0);
        printf("391OS> ");
        set_column_stack();
        return;
    }

    if (ctrl_state == 1 && s_code == SCANCODE_C) {
        // ctrl-c halts the current program
        send_eoi(KEYBOARD_IRQ);
        stop();
        cli();
        // printf("%d\n", current_terminal);
        // wait for scheduling to execute the current terminal and then halt
        pcb_t * pcb = get_pcb_by_index(process_terminal[current_terminal][process_terminal_cnt[current_terminal] - 1]);
        pcb->pending_signal = INTERRUPT;
        sti();
        return;
    }

    // 0x3E is the scancode of F4 press
    if((s_code >= 0x3E)){
        return;
    }

    // Alt + Fx: switch to xth terminal
    if(alt_state == 1 && s_code == F1_CODE){
        send_eoi(KEYBOARD_IRQ);
        sti();
        switch_to_terminal(0);
        return;
    }
    if(alt_state == 1 && s_code == F2_CODE){
        send_eoi(KEYBOARD_IRQ);
        sti();
        switch_to_terminal(1);
        return;
    }
    if(alt_state == 1 && s_code == F3_CODE){
        send_eoi(KEYBOARD_IRQ);
        sti();
        switch_to_terminal(2);
        return;
    }

    if((s_code >= 0x3C)){
        return;
    }

    if(shift_state == 0 && caps_lock_enabled == 0){
        current_state = NOTHING_PRESS;
    }else if(shift_state == 1 && caps_lock_enabled == 0){
        current_state = SHIFT_PRESS;
    }else if(shift_state == 0 && caps_lock_enabled == 1){
        current_state = CAPS_PRESS;
    }else if(shift_state == 1 && caps_lock_enabled == 1){
        current_state = CAP_SHI_PRESS;
    }
    //printf("%x", s_code);
    putc(key_scancode[current_state][s_code]);
    key_arr_push(key_scancode[current_state][s_code]);
    //puts("test!!\n");
    return;

}
