#include "types.h"
#include "lib.h"
#include "syscall.h"
#include "filesystem.h"

#ifndef _DB16_H
#define _DB16_H

// Magic number for several ports
#define DSP_Reset               0x6
#define DSP_Read                0xA
#define DSP_Write               0xC
#define DSP_Read_Buffer_Status  0xE
#define DSP_16_interrupt_ack    0xF
#define SB16_IOBase             0x220
#define SB16_Version            0x405
#define SB16_IRQ                0x5
#define SB16_DMA                0x1
#define SB16_DMA_16             0x5
#define SB16_ISA_IRQ            0x5
#define SB_OUTPUT_RATE          0x41
#define SB_INPUT_RATE           0x42

// I/O port addresses for the control registers
#define DMA_1_MASK              0x0A
#define DMA_1_MODE              0x0B
#define DMA_1_CLEAR_PTR         0x0C
#define DMA_2_MASK              0xD4
#define DMA_2_MODE              0xD6
#define DMA_2_CLEAR_PTR         0xD8
#define DMA_1_BASE              0x00
#define DMA_2_BASE              0xC0

// Write single mask bit assignments
#define DMA_BIT_2               (1 << 2)

#define RIFF                    0x46464952
#define WAVE                    0x45564157
#define FMT                     0x20746D66
#define DATA                    0x61746164
#define FACT                    0x74636166

#define Pause_8_bit             0xD0
#define Continue_8_bit          0xD4
#define Pause_16_bit            0xD5
#define Continue_16_bit         0xD6
#define Exit_auto_8_bit         0xDA
#define Exit_auto_16_bit        0xD9
#define Start_8_bit             0xC0
#define Start_16_bit            0xB0


// DB16 Routines
int8_t Reset_DSP();
int8_t Write_DSP(uint8_t data);
uint8_t Read_From_DSP();
int8_t Transfer_Sound_DMA(uint8_t channel, uint8_t mode, uint32_t addr, uint32_t size);
int8_t Set_Sample_Rate(uint16_t frequency);
void sb16_handler();
int8_t play_music(int8_t* filename);
void start_play(uint32_t block_size);
void stop();

#endif
