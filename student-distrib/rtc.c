#include "rtc.h"
#include "idt.h"
#include "lib.h"

//index-port/data-port (0x70/0x71) of the RTC
#define RTC_PORT 0x70
#define CMOS_PORT 0x71

// reference: https://wiki.osdev.org/RTC    be careful that outportb and outb is reversed in parameter position
#define RTC_REG_A 0x8A
#define RTC_REG_B 0x8B
#define RTC_REG_C 0x8C
#define RTC_REG_D 0x8D
#define SIXTH_BIT 0x40
#define BASE_FREQ _1024HZ
#define INITIAL_FREQ_IDX BASE_FREQ/_64HZ

//static int freq_arr[11] = {_0HZ, _2HZ, _4HZ, _8HZ, _16HZ, _32HZ, _64HZ, _128HZ, _256HZ, _512HZ, _1024HZ}; // array for set freq
// volatile int itr_occur = 0;
volatile int itr_occur_table[MAX_TERMINAL_NUM] = {0,0,0};
volatile int counter_table[MAX_TERMINAL_NUM] = {0,0,0};
volatile int constant_arr[MAX_TERMINAL_NUM] = {1024,1024,1024};

/*
 * Function:  rtc_init
 * --------------------
 * initialize RTC
 *
 *  Inputs: none
 *
 *  Returns: none
 *
 *  Side effects: turn on PIE at control register B
 *
 */
void rtc_init()
{
  // outb(RTC_REG_A, RTC_PORT);
  // unsigned char A = inb(CMOS_PORT);
  /*  A pattern of 010 is the only combination of bits that turn the oscillator on and allow the RTC
  *  to keep time.     --DS12885 P15
  */
  // not implemented at this stage
  outb(RTC_REG_B, RTC_PORT); //disable NMI, avoid nmi_exception
  unsigned char B = inb(CMOS_PORT);
  outb(RTC_REG_B, RTC_PORT);
  outb(SIXTH_BIT | B, CMOS_PORT); //turn on PIE
  enable_irq(RTC_IRQ);
}

/*
 * Function:  rtc_set_freq
 * --------------------
 *  set the interrupt set in Hz, without lost track of time.
 *
 *  Inputs: freq  -- the frequency to be inputed, in 4 bytes
 *
 *  Returns: 0 -- success
 *          -1 -- failure
 *
 *  Side effects: turn on PIE at control register B
 *
 */
int rtc_set_freq(int freq)
{
  // set frequency
  // disable_irq(RTC_IRQ);
  cli();
  int fffffff = freq;
  if (freq < 0 || freq > 1024 || freq % 2)
    return -1;

  while (freq > 1)
  {
    if (freq % 2) // not power of 2
      return -1;

    /* turn freq into idx */
    freq /= 2;
  }

  constant_arr[running_terminal] = 1024 / fffffff;
  sti();
  //enable_irq(RTC_IRQ);
  return 0;
}

/*
 * Function:  rtc_handler
 * --------------------
 *  handle interrupts
 *
 *  Inputs: none
 *
 *  Returns: none
 *
 *  Side effects: none
 *
 */
void rtc_handler()
{
  send_eoi(RTC_IRQ);
  // itr_occur = 1;
  int i = 0;
  cli();
  for (i = 0; i < MAX_TERMINAL_NUM; i++) {
    /* change interrupt table */
    counter_table[i] += 1;
    if(counter_table[i] >= constant_arr[i]){
      counter_table[i] = 0;
      itr_occur_table[i] = 1;
    }
  }
  //printf("%d %d\n", itr_occur_table[0], constant_arr[0]);
  outb(RTC_REG_C, RTC_PORT);
  // test_interrupts();
  inb(CMOS_PORT);
  sti();
}

/*
 * Function:  rtc_read
 * --------------------
 *  RTC read() should block until the next interrupt, return 0
 *
 *  Inputs: none
 *
 *  Returns: 0 --success
 *          -1 --failure
 *
 *  Side effects: none
 *
 */
int rtc_read(int32_t fd, void *buf, int32_t nbytes)
{
  // while (!itr_occur);
  while(!itr_occur_table[running_terminal]){
  }
  // itr_occur = 0;
  itr_occur_table[running_terminal] = 0;
  return 0;
}

/*
  * Function:  rtc_write
  * --------------------
  *  RTC write() must be able to change frequency, return 0 or -1
  *
  *  Inputs: none
  *
  *  Returns: 0 --success
  *          -1 --failure
  *
  *  Side effects: none
  *
  */
int rtc_write(int32_t fd, const void *buf, int32_t nbytes)
{
  if(nbytes != 4 || (int32_t*)buf == NULL){
    return -1;
  }

  int freq = *((int *)buf);
  return (rtc_set_freq(freq));
}

/*
   * Function:  rtc_open
   * --------------------
   *  RTC open() initializes RTC frequency to 2HZ, return 0
   *
   *  Inputs: none
   *
   *  Returns: 0 --success
   *          -1 --failure
   *
   *  Side effects: none
   *
   */
int rtc_open(const uint8_t *filename)
{
  rtc_set_freq(2);
  return 0;
}

/*
    * Function:  rtc_close
    * --------------------
    *  RTC close() probably does nothing, unless you virtualize RTC, return 0
    *
    *  Inputs: none
    *
    *  Returns: 0 --successwrite
    *          -1 --failure
    *
    *  Side effects: none
    *
    */
int rtc_close(int32_t fd)
{
  return 0;
}
