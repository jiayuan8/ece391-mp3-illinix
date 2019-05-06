#include "scheduling.h"
static uint8_t pending_interrupt = 0x3F;

/* void init_pit(uint32_t frequency)
 * --------------------------------------------------------------------------------------
 * Descriptions:    This function will initialize pit
 * Inputs:          uint32_t frequency :    The frequency you want
 * Outputs:         None
 * Side Effects:    None
 */
void init_pit(uint32_t frequency) {
    uint32_t divisor = PIT_CONST/frequency;
    outb(PIT_MODE_3, PIT_REG);
    outb(PIT_MASK & divisor, CHANNEL_0);
    outb(divisor >> 8, CHANNEL_0);
    enable_irq(PIT_IRQ);
}

/* void pit_handler()
 * --------------------------------------------------------------------------------------
 * Descriptions:    When do the scheduling, it will
 *                      1. check if there are other terminals that are running
 *                      2. get the pid of next task from the process array
 *                      3. map virtual address 128 MB to coresponding physical address
 *                      4. map the video mem address of last task to its buffer
 *                      5. adjust tss, save old esp/ebp and restore new esp/ebp
 * Inputs:          None
 * Outputs:         None
 * Side Effects:    Preform context switch, may slow down the system
 */
void pit_handler() {

    // Variables that needed
    uint8_t i;                      // counter
    uint8_t next_terminal;          // id of next terminal
    uint8_t next_process;           // pid of next process
    pcb_t * old_pcb;                // pointer to the old pcb
    pcb_t * new_pcb;                // pointer to the new pcb
    send_eoi(PIT_IRQ);

    // enter critical section
    cli();
    //printf("current terminal: %d || running terminal: %d\n", current_terminal, running_terminal);
    // check if other terminal is running, otherwise return
    if(process_terminal_cnt[1] != 0 || process_terminal_cnt[2] != 0){

        // look for next task
        next_terminal = running_terminal;

        for(i = 0; i < MAX_TERMINAL_NUM; i ++){
            next_terminal = (running_terminal + 1 + i) % 3;
            if(is_running(next_terminal) == 1){
                break;
            }
        }
        next_process = process_terminal[next_terminal][process_terminal_cnt[next_terminal] - 1];
        if(next_process > 5){
            sti();
            return;
        }
        //printf("%d\n", next_process);
        // map the new task to its corresponding physical address
        map_program(next_process);

        // get new and old pcb
        old_pcb = get_curr_pcb();
        running_terminal = next_terminal;
        new_pcb = get_pcb_by_index(next_process);
        pending_interrupt = new_pcb->pending_signal;

        // adjust tss
        tss.ss0 = KERNEL_DS;
        tss.esp0 = EIGHT_MB_SIZE - EIGHT_KB_SIZE * (next_process) - ESP_OFFSET;

        // map the old process's videomem to its buffer
        map_user_video_to_buffer(next_terminal);
    
        //printf("%x\n", (uint32_t)(saved_addr));
        // save old process's esp and ebp, restore new process's esp and ebp
        asm volatile(
            "movl %%esp, %%eax;"
            "movl %%ebp, %%ebx;"
            : "=a" (old_pcb->schedule_esp), "=b" (old_pcb->schedule_ebp)
        );

        asm volatile(
            "movl %%eax, %%esp;"
            "movl %%ebx, %%ebp;"
            : /* no outputs */
            : "a" (new_pcb->schedule_esp), "b" (new_pcb->schedule_ebp)
        );

        if(pending_interrupt != 0x3F){
            send_signal(pending_interrupt);
        }

    }else{
        if(process_terminal_cnt[0] == 0){
            sti();

            return;
        }
        new_pcb = get_curr_pcb();
        if(new_pcb->pending_signal != 0x3F){
            send_signal(new_pcb->pending_signal);
            new_pcb->pending_signal = 0x3F;
        }

    }
    sti();

    return;
}
