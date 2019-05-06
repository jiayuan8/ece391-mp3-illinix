#ifndef _PAGING_H
#define _PAGING_H

#include "lib.h"

#define DIR_SIZE 1024
#define TABLE_SIZE 1024

#define VIDEO_OFFSET        0x000B8000
#define KERNEL_OFFSET       0x00400000

#define PRESENT_MASK        0x00000001
#define R_W_MASK            0x00000002
#define U_S_MASK            0x00000004
#define PWT_MASK            0x00000008
#define PCD_MASK            0x00000010
#define A_MASK              0x00000020
#define D_MASK              0x00000040
#define PS_MASK             0x00000080
#define G_MASK              0x00000100
#define AVAIL_MASK          0x00000E00

#define AVAIL_OFFSET        0xA

#define FOUR_MB_OFFSET      22
#define FOUR_MB_PB_MASK     0xFFC00000
#define FOUR_KB_OFFSET      12
#define FOUR_LB_PB_MASK     0xFFFFF000
#define TABLE_MASK          0x003FF000

#define PROGRAM_VIRTUAL     _128_MB_SIZE / FOUR_MB_SIZE
#define PROGRAM_OFFSET      0x08048000
#define ESP_OFFSET          4

#define USER_VIDEO          (_128_MB_SIZE + (EIGHT_MB_SIZE * 10))

// buffer for page directory
uint32_t page_directory[DIR_SIZE] __attribute__((aligned(FOUR_KB_SIZE)));
// only the first 4KB memory needs page table
uint32_t page_table[TABLE_SIZE] __attribute__((aligned(FOUR_KB_SIZE)));
// The page table for user video mem
uint32_t page_table_vidmem[TABLE_SIZE] __attribute__((aligned(FOUR_KB_SIZE)));

/* Initializing paging for OS */
extern void init_page();
/* Build a 4MB page for kernel, set the second entrance of page_directory to the address of kernel */
extern void map_kernel();
/* set the specific entrance for page_table to let video memory maps to its page */
extern void initialize_page_table();
/* set the specific entrance for page_directory to let entrance point to page table */
extern void map_video();
/* setup the paging for specific user program */
extern void map_program(uint32_t pid);
/* map the virtual addr of user to video mem */
extern void map_user_video();
/* helper funtion for scheduling, especially for fish */
extern void map_user_video_to_buffer(uint8_t terminal_id);
/* helper funtion for scheduling, especially for fish */
extern void map_terminal_video(uint8_t terminal_id);

#endif
