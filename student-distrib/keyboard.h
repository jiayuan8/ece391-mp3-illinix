#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "lib.h"
#include "i8259.h"
#include "sb16.h"

#define SCANCODE_NUM        0x40
#define KEY_DATA_PORT       0x60
#define KEY_COMMAND_PORT    0x64
#define KEYBOARD_IRQ        0x01


// reference of the scan code: https://wiki.osdev.org/PS/2_Keyboard
#define L_SHIFT_CODE        0x2A
#define R_SHIFT_CODE        0x36
#define TAB_CODE            0x0F

// Code for terminal switch
#define ALT_CODE            0x38
#define ALT_REL_CODE        0xB8
#define F1_CODE             0x3B
#define F2_CODE             0x3C
#define F3_CODE             0x3D

#define L_SHIFT_REL_CODE    0xAA
#define R_SHIFT_REL_CODE    0xB6
#define L_CTRL_CODE         0x1D
#define L_CTRL_REL_CODE     0x9D
#define ESC_CODE            0x01
#define NUM_STAR_CODE       0x37


#define CPSLK_CODE          0x3A
#define BKSP_CODE           0x0E
#define TAB_CODE            0x0F
#define ENTER_CODE          0x1C

#define TOTAL_STATE         4
#define NOTHING_PRESS       0           // State that capslock and shift are not pressed
#define SHIFT_PRESS         1           // State that only shift is pressed
#define CAPS_PRESS          2           // State that only capslock is enabled
#define CAP_SHI_PRESS       3           // State that shift is pressed when capslock is enabled

#define NOT_PRESSED         0
#define PRESSED             1

#define KEY_ARR_SIZE        128

#define SCANCODE_L          0x26
#define SCANCODE_C          0x2E

volatile uint8_t enter_state;

extern void init_keyboard();
extern void keyboard_handler();
extern void input_handler(unsigned s_code);

#endif
