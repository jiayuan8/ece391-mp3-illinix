/* rtc.h
 * Real Time CLock for kernel, Not sure if it is MC146818 but the regsiters are the same.
 *
 *  control regsiters A
 *  BIT 7   BIT 6   BIT 5   BIT 4   BIT 3   BIT 2   BIT 1   BIT 0
 *  UIP     DV2     DV1     DV0     RS3     RS2     RS1     RS0
 *
 *  control regsiters B
 *  BIT 7   BIT 6   BIT 5   BIT 4   BIT 3   BIT 2   BIT 1   BIT 0
 *  SET     PIE     AIE     UIE     SQWE    DM      24/12   DSE
 *
 *  current designated worker:   Xiaoyi Shen      xiaoyis2@illinois.edu
 */
#include "types.h"


#ifndef RTC_H
#define RTC_H

/* initialize RTC */
extern void rtc_init();

/* set the interrupt frequency to freq, by a power of 2 no larger than 1024 */
int rtc_set_freq(int freq);

/* interrupt handler for RTC, called in int_linkage.S*/
extern void rtc_handler();
//RTC read() should block until the next interrupt, return 0
extern int rtc_read(int32_t fd, void *buf, int32_t nbytes);
//RTC write() must be able to change frequency, return 0 or -1
extern int rtc_write(int32_t fd, const void *buf, int32_t nbytes);
//RTC open() initializes RTC frequency to 2HZ, return 0
extern int rtc_open(const uint8_t *filename);
// RTC close() probably does nothing, unless you virtualize RTC, return 0
extern int rtc_close(int32_t fd);

#define RTC_IRQ 8
#define _0HZ 0x00
#define _2HZ 0x0F
#define _4HZ 0x0E
#define _8HZ 0x0D
#define _16HZ 0x0C
#define _32HZ 0x0B
#define _64HZ 0x0A
#define _128HZ 0x09
#define _256HZ 0x08
#define _512HZ 0x07
#define _1024HZ 0x06 //default frequency

#endif
