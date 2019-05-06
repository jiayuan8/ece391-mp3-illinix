#include "sb16.h"

#define BLOCK_SIZE     (32*1024)
#define BUFFER_SIZE    (2 * BLOCK_SIZE)

//Allocate a buffer that does not cross a 64k physical page boundary
static int8_t DMA_Buffer[BUFFER_SIZE] __attribute__((aligned(32768))) = {};
static int8_t *block1 = DMA_Buffer;
static int8_t *block2 = &(DMA_Buffer[BLOCK_SIZE]);
static int8_t *cur_block = DMA_Buffer;
static int8_t audio_filename[33];
static uint32_t current_offset;
static int8_t is_playing = 0;
static uint32_t audio_file_inode = 0;

// I/O port addresses for lower page registers, channel 4 is unused
static int8_t page_ports[8] = {0x87, 0x83, 0x81, 0x82, 0x00, 0x8B, 0x89, 0x8A};

// 
int32_t _fd;

void DSP_outb(uint8_t data, uint8_t port_offset){
    outb(data, SB16_IOBase + port_offset);
}

uint8_t DSP_inb(uint8_t port_offset){
    return inb(SB16_IOBase + port_offset);
}

/*
 * int8_t Reset_DSP()
 * -----------------------------------------------------------------------
 * Description: Write a 1 to the reset port (2x6)
 *              Wait for 3 microseconds
 *              Write a 0 to the reset port (2x6)
 *              Poll the read-buffer status port (2xE) until bit 7 is set
 *              Poll the read data port (2xA) until you receive an AA
 * Input:       None
 * Output:      int8_t  0: if success
 * SideEffect:  None
 * 
 */
int8_t Reset_DSP(){
    uint32_t _3ms = 1000 * 0.03;
    uint8_t i = 0;                  // counter
    uint8_t read_val = 0;
    DSP_outb(1, DSP_Reset);
    while(i < _3ms){
        i ++;
    }
    DSP_outb(0, DSP_Reset);

    while(1){
        read_val = DSP_inb(DSP_Read_Buffer_Status);
        if(read_val & (1 << 7)){
            break;
        }
    }

    while(1){
        read_val = DSP_inb(DSP_Read);
        if(read_val & 0xAA){
            break;
        }
    }

    return 0;
}

/*
 * int8_t Write_DSP(uint8_t data)
 * -----------------------------------------------------------------------
 * Description: Read the write-buffer status port (2xC) until bit 7 is cleared
 *              Write the value to the write port (2xC)
 * Input:       uint8_t data:   The data which needed to be write to the port
 * Output:      int8_t  0: if success
 * SideEffect:  None
 * 
 */

int8_t Write_DSP(uint8_t data){
    volatile uint8_t read_val = 0;
    while(1){
        read_val = DSP_inb(DSP_Write);
        if(!(read_val & (1 << 7))){
            break;
        }
    }

    DSP_outb(data, DSP_Write);

    return 0;
}

/*
 * int8_t Read_From_DSP()
 * -----------------------------------------------------------------------
 * Description: Read the read-buffer status port (2xE) until bit 7 is set
 *              Read the value from the read port (2xA)
 * Input:       None
 * Output:      uint8_t  value from the read port
 * SideEffect:  None
 * 
 */
uint8_t Read_From_DSP(){
    uint8_t read_val = 0;
    while(1){
        read_val = DSP_inb(DSP_Read_Buffer_Status);
        if(!(read_val & (1 << 7))){
            break;
        }
    }

    return DSP_inb(DSP_Read);
}

/*
 * int8_t Transfer_Sound_DMA(uint8_t channel, uint8_t mode, uint32_t addr, uint32_t size)
 * -----------------------------------------------------------------------
 * Description: Read the read-buffer status port (2xE) until bit 7 is set
 *              Read the value from the read port (2xA)
 * Input:       None
 * Output:      uint8_t  0:     if success
 * SideEffect:  None
 * 
 */

int8_t Transfer_Sound_DMA(uint8_t channel, uint8_t mode, uint32_t addr, uint32_t size){
    uint32_t count = size;

    // 1.Disable the sound card DMA channel by setting the appropriate mask bit 
    if(channel <= 3){
        outb(channel | DMA_BIT_2, DMA_1_MASK);
    }else{
        outb((channel % 4) | DMA_BIT_2, DMA_2_MASK);
    }

    // 2.Clear the byte pointer flip-flop 
    if(channel <= 3){
        outb(0, DMA_1_CLEAR_PTR);
    }else{
        outb(0, DMA_2_CLEAR_PTR);
    }

    // 3.Write the DMA mode for the transfer 
    if(channel <= 3){
        outb(mode | channel, DMA_1_MODE);
    }else{
        outb(mode | (channel % 4), DMA_2_MODE);
    }

    // 4.Write the offset of the buffer, 
    //   low byte followed by high byte. 
    //   For sixteen bit data, the offset 
    //   should be in words from the start of a 128kbyte page. 
    //   The easiest method for computing 16-bit parameters is to 
    //   divide the linear address by two before calculating offset

    //      4.1.Set paging first
    if(channel <= 3){
        outb(addr >> 16, page_ports[channel]);
    }else{
        outb((addr >> 16) & 0xFE, page_ports[channel]);
    }

    //      4.2.After that , executing step 4
    if(channel <= 3){
        outb(addr & 0xFF, DMA_1_BASE + (channel << 1));
        outb((addr >> 8) & 0xFF, DMA_1_BASE + (channel << 1));
    }else{
        outb((addr / 2) & 0xFF, DMA_2_BASE + (channel << 2));
        outb(((addr / 2) >> 8) & 0xFF, DMA_2_BASE + (channel << 2));
    }

    // 5.Write the transfer length, low byte followed by high 
    //  byte. For an 8-bit transfer, write the number of bytes-1. 
    //  For a 16-bit transfer, write the number of words-1.
    count --;
    if(channel <= 3){
        outb(count & 0xFF, DMA_1_BASE + (channel << 1) + 1);
        outb((count >> 8) & 0xFF, DMA_1_BASE + (channel << 1) + 1);
    }else{
        outb((count / 2) & 0xFF, DMA_2_BASE + (channel << 2) + 2);
        outb(((count / 2) >> 8) & 0xFF, DMA_2_BASE + (channel << 2) + 2);
    }

    // 6.Enable the sound card DMA channel by clearing the appropriate mask bit
    if(channel <= 3){
        outb(channel, DMA_1_MASK);
    }else{
        outb((channel % 4), DMA_2_MASK);
    }
    return 0;
}

// Write the command (41h for output rate, 42h for input rate)
// Write the high byte of the sampling rate (56h for 22050 hz)
// Write the low byte of the sampling rate (22h for 22050 hz)
int8_t Set_Sample_Rate(uint16_t frequency){
    Write_DSP(SB_OUTPUT_RATE);
    Write_DSP((uint8_t)((frequency >> 8) & 0xFF));
    Write_DSP((uint8_t)(frequency & 0xFF));
    return 0;
}


int8_t play_music(int8_t* filename){
    if(is_playing == 1){
        return 0;
    }
    dentry_t audio_dentry;
    if (read_dentry_by_name((uint8_t*)filename, &audio_dentry) == -1){
        printf("No Such File");
        return -1;
    }else{
        audio_file_inode = audio_dentry.i_node;
    }
    uint8_t magic[4];
    read_data(audio_file_inode, 0, magic, 4);
    if(*((uint32_t*)magic) != RIFF){
        printf("file cannot be played! %s\n", magic);
        return 0;
    }

    Reset_DSP();
    strncpy(audio_filename, filename, strlen(filename));

    int32_t fd = open((uint8_t*)filename);
    _fd = fd;
    uint32_t bytes_read = read_data(audio_file_inode,current_offset, (uint8_t*)DMA_Buffer, BLOCK_SIZE*2);
    Transfer_Sound_DMA(1, 0x48 | 0x10, (uint32_t)(&DMA_Buffer[0]), sizeof(DMA_Buffer));
    Set_Sample_Rate(8000);
    start_play(BLOCK_SIZE);
    is_playing = 1;
    current_offset += bytes_read;

    return 0;
}

void start_play(uint32_t block_size){
    uint16_t blksize = block_size;
    uint8_t commond = 0;
    uint8_t mode = 0;
    commond |= Start_8_bit;
    commond |= DSP_Reset;
    mode |= 0x00;
    Write_DSP(commond);
    Write_DSP(mode);
    blksize --;
    Write_DSP((uint8_t)(blksize & 0xFF));
    Write_DSP((uint8_t)((blksize >> 8) & 0xFF));
    //printf("helloworld");
    return;
}

void stop(){
    uint8_t i = 0;
    Write_DSP(Pause_8_bit);
    is_playing = 0;
    audio_file_inode = 0;
    current_offset = 0;
    cur_block = block1;
    for(i = 0; i < 33; i ++){
        audio_filename[i] = 0;
    }
}

void sb16_handler(){
    //printf("interrupt works\n");
    DSP_inb(0xE);
    send_eoi(5);
    cli();
    //printf("Offset: %d\n", current_offset);
    uint32_t bytes_read = read_data(audio_file_inode, current_offset, (uint8_t*)cur_block, BLOCK_SIZE);
    current_offset += bytes_read;
    //printf("Bytes: %d\n", bytes_read);
    //printf("Offset: %d\n", current_offset);
    sti();

    if(bytes_read == BLOCK_SIZE){
        uint16_t blksize = (uint16_t) bytes_read;
        if(cur_block == block1){
            cur_block = block2;
        }else{
            cur_block = block1;
        }
        start_play(blksize);
    }else if(bytes_read == 0){
        stop();
    }
    //printf("interrupt works");
    return;
}

