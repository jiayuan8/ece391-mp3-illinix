#include "paging.h"

/* flush the tlb */
static void flush_tlb();

/* int init_page()
 *
 * Descriptions: Initializing paging for OS
 * Inputs: None
 * Outputs: None
 * Side Effects: Changing cr3, cr4, cr0 registers
 */

void init_page()
{
    int i = 0;

    uint32_t base_add = 0x00000000;

    // initialize all the entry for paging directory and paging table to 0
    for (i = 0; i < DIR_SIZE; i++)
    {
        page_directory[i] = 0;
        // set all the read/ write bit to 1
        page_directory[i] |= R_W_MASK;
    }
    for (i = 0; i < TABLE_SIZE; i++)
    {
        page_table[i] = 0;
        // set all the read/ write bit to 1
        page_table[i] |= R_W_MASK;
        // set bit 31 to 12 to its original address
        page_table[i] |= base_add;
        base_add += FOUR_KB_SIZE;
    }

    map_kernel();
    initialize_page_table();
    map_video();

    asm volatile(

        // load the physical base address of the page directory
        "movl $page_directory, %%eax        ;"
        "movl %%eax, %%cr3                  ;"

        // enable paging size extention, which allows 4MB pages exist
        "movl %%cr4, %%eax                  ;"
        "orl  $0x00000010, %%eax            ;"
        "movl %%eax, %%cr4                  ;"

        // enable paging, set the bit 31 in cr0 register to 1
        "movl %%cr0, %%eax                  ;"
        "orl  $0x80000001, %%eax            ;"
        "movl %%eax, %%cr0                  ;"
        :
        :
        : "eax");
}

/* int map_kernel()
 *
 * Descriptions: Build a 4MB page for kernel, set the second
 *              entrance of page_directory to the address of kernel
 * Inputs: None
 * Outputs: None
 * Side Effects: Changing page_directory
 */
void map_kernel()
{
    uint32_t ker_entrance = 0;

    // presents the page
    ker_entrance |= PRESENT_MASK;
    // declare that it is 4MB page
    ker_entrance |= PS_MASK;

    ker_entrance |= ((uint32_t)(KERNEL_OFFSET)&FOUR_LB_PB_MASK);
    page_directory[1] = ker_entrance;
}

/* int map_video()
 *
 * Descriptions: set the specific entrance for page_table to let video
 *              memory maps to its page
 * Inputs: None
 * Outputs: None
 * Side Effects: Changing page_table
 */
void map_video()
{
    uint32_t vid_entrance = 0;

    vid_entrance |= PRESENT_MASK;
    vid_entrance |= VIDEO_OFFSET;
    vid_entrance |= R_W_MASK;

    page_table[VIDEO_OFFSET >> FOUR_KB_OFFSET] = vid_entrance;

    // map the buffer that stores the screen content of first terminal
    vid_entrance = 0;
    vid_entrance |= PRESENT_MASK;
    vid_entrance |= TERMINAL_BUFFER_1;
    vid_entrance |= R_W_MASK;
    page_table[TERMINAL_BUFFER_1 >> FOUR_KB_OFFSET] = vid_entrance;

    // map the buffer that stores the screen content of the second terminal
    vid_entrance = 0;
    vid_entrance |= PRESENT_MASK;
    vid_entrance |= TERMINAL_BUFFER_2;
    vid_entrance |= R_W_MASK;
    page_table[TERMINAL_BUFFER_2 >> FOUR_KB_OFFSET] = vid_entrance;

    // map the buffer that stores the screen content of the thrid terminal
    vid_entrance = 0;
    vid_entrance |= PRESENT_MASK;
    vid_entrance |= TERMINAL_BUFFER_3;
    vid_entrance |= R_W_MASK;
    page_table[TERMINAL_BUFFER_3 >> FOUR_KB_OFFSET] = vid_entrance;

    flush_tlb();
}

/* void map_user_video()
 *
 * Descriptions: Map the virtual address (128MB + 80MB) t0 video memory for user program
 * Inputs: None
 * Outputs: None
 * Side Effects: Changing page directory, Changing page_table_vidmem
 */

void map_user_video(){
    uint32_t vid_entrance = 0;
    uint32_t pde_index = (USER_VIDEO >> FOUR_MB_OFFSET);

    // present this pde
    vid_entrance |= (uint32_t)PRESENT_MASK;
    // map this VM to corresponding page table
    vid_entrance |= (uint32_t)(&page_table_vidmem[0]);
    // allow read and write
    vid_entrance |= (uint32_t)R_W_MASK;
    // specify that this is user program
    vid_entrance |= (uint32_t)U_S_MASK;
    // map to page table
    page_directory[pde_index] = vid_entrance;

    vid_entrance = 0;
    // specify that this pte is for user program
    vid_entrance |= U_S_MASK;
    // enable read and write
    vid_entrance |= R_W_MASK;
    // present this page
    vid_entrance |= PRESENT_MASK;
    // map the virtual address to physical address 
    vid_entrance |= VIDEO_OFFSET;

    page_table_vidmem[0] = vid_entrance;

    flush_tlb();

    return;
}

/* void map_user_video_to_buffer(uint8_t terminal_id)
 * --------------------------------------------------------------------------------------
 * Descriptions: This function is a helper funtion for scheduling.
 *              For the processes that does not belong to current terminal, it will map the virtual address 
 *                  208MB to the corresponding buffer that stores the terminal's screen
 *              For the processes that belongs to the current buffer, it will map the virtual address 
 *                  208MB to video buffer
 *              Finally, the function will flush tlb
 * Inputs:      uint8_t terminal_id :   the terminal's id, which can be 0 or 1 or 2
 * Outputs:     None
 * Side Effects: Changing page directory, Changing page_table_vidmem
 */
void map_user_video_to_buffer(uint8_t terminal_id){
    if(terminal_id == current_terminal){
        map_user_video();
        return;
    }else{
        map_terminal_video(terminal_id);
        return;
    }
}

/* int initialize_page_table()
 *
 * Descriptions: set the specific entrance for page_directory to let entrance
 *              point to page table
 * Inputs: None
 * Outputs: None
 * Side Effects: Changing page_directory
 */
void initialize_page_table()
{
    uint32_t init_entrance = 0;

    // set the directory to present
    init_entrance |= PRESENT_MASK;

    // set bit 31-22 to address of page table
    init_entrance |= ((uint32_t)(page_table)&FOUR_LB_PB_MASK);

    page_directory[0] = init_entrance;
}

/* void map_program(uint32_t pid)
 *
 * Descriptions: map the paging for the program
 * Inputs: uint32_t pid -- the process id of the program to be mapped
 * Outputs: None
 * Side Effects: Changing page_directory
 */
void map_program(uint32_t pid) {
    uint32_t program_entrance = 0;

    // presents the page
    program_entrance |= PRESENT_MASK;
    // declare that it is 4MB page
    program_entrance |= PS_MASK;
    // declare that it is a user program
    program_entrance |= U_S_MASK;
    // specify this page is readable and writable
    program_entrance |= R_W_MASK;

    program_entrance |= (uint32_t)((pid * FOUR_MB_SIZE + EIGHT_MB_SIZE));
    page_directory[PROGRAM_VIRTUAL] = program_entrance;

    flush_tlb();
}

/* void map_terminal_video(uint8_t terminal_id)
 * --------------------------------------------------------------------------------------
 * Descriptions: This function is a helper funtion for scheduling.
 *              For the processes that does not belong to current terminal, it will map the virtual address 
 *                  208MB to the corresponding buffer that stores the terminal's screen
 * 
 *                  ------------------------------------------------------
 *                  | VIDEO (4kb) | the buffer that stores video mem     |
 *                  ------------------------------------------------------
 *                  | VIDEO + 4kb | the buffer stores terminal 1's scr   |
 *                  ------------------------------------------------------
 *                  | VIDEO + 8kb | the buffer stores terminal 2's scr   |
 *                  ------------------------------------------------------
 *                  | VIDEO + 12kb| the buffer stores terminal 3's scr   |
 *                  ------------------------------------------------------
 * 
 *              Finally, the function will flush tlb
 * Inputs:      uint8_t terminal_id :   the terminal's id, which can be 0 or 1 or 2
 * Outputs:     None
 * Side Effects: Changing page directory, Changing page_table_vidmem
 */
void map_terminal_video(uint8_t terminal_id){
    uint32_t vid_entrance = 0;
    uint32_t pde_index = (USER_VIDEO >> FOUR_MB_OFFSET);

    // present this pde
    vid_entrance |= (uint32_t)PRESENT_MASK;
    // map this VM to corresponding page table
    vid_entrance |= (uint32_t)(&page_table_vidmem[0]);
    // allow read and write
    vid_entrance |= (uint32_t)R_W_MASK;
    // specify that this is user program
    vid_entrance |= (uint32_t)U_S_MASK;
    // map to page table
    page_directory[pde_index] = vid_entrance;

    vid_entrance = 0;
    // specify that this pte is for user program
    vid_entrance |= U_S_MASK;
    // enable read and write
    vid_entrance |= R_W_MASK;
    // present this page
    vid_entrance |= PRESENT_MASK;
    // map the virtual address to physical address 
    vid_entrance |= (VIDEO_OFFSET + (terminal_id + 1) * FOUR_KB_SIZE);

    page_table_vidmem[0] = vid_entrance;

    flush_tlb();

    return;
}

/* void flush_tlb()
 *
 * Descriptions: flush the tlb
 * Inputs: None
 * Outputs: None
 * Side Effects: flush the tlb
 */
static void flush_tlb() {
    asm volatile(
        "movl %%cr3, %%eax;"
        "movl %%eax, %%cr3;"
        :
        :
        : "eax"
    );
}
