#ifndef SCHEDULING_H
#define SCHEDULING_H

#include "i8259.h"
#include "types.h"
#include "terminal.h"
#include "paging.h"
#include "lib.h"

#define PIT_IRQ     0
#define PIT_CONST   1193182
#define PIT_REG     0x43
#define PIT_MODE_3  0x36
#define CHANNEL_0   0x40
#define PIT_MASK


extern void init_pit();
extern void pit_handler();

#endif
