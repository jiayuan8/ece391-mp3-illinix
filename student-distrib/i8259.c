/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

#define IRQ_NUMBER      15
#define MASTER_BOUND    0x08

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void) {
    // mask all irq during initialization
    master_mask = 0xFF;
    slave_mask = 0xFF;

    // mask all the irq on both master and slave
    outb(master_mask, MASTER_8259_DATA);
    outb(slave_mask, SLAVE_8259_DATA);

    // send the four instructions to the master PIC
    outb(ICW1, MASTER_8259_PORT);
    outb(ICW2_MASTER, MASTER_8259_DATA);
    outb(ICW3_MASTER, MASTER_8259_DATA);
    outb(ICW4, MASTER_8259_DATA);

    // send the four instructions to the slave PIC
    outb(ICW1, SLAVE_8259_PORT);
    outb(ICW2_SLAVE, SLAVE_8259_DATA);
    outb(ICW3_SLAVE, SLAVE_8259_DATA);
    outb(ICW4, SLAVE_8259_DATA);

    // mask all interrupts
    outb(master_mask, MASTER_8259_DATA);
    outb(slave_mask, SLAVE_8259_DATA);
}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
    // check if the irq_num is out of range
    if (irq_num < 0 || irq_num > IRQ_NUMBER)
        return;

    // if the irq is on the master PIC
    if (irq_num < MASTER_BOUND) {
        // set the corresponding bit as unmasked
        master_mask &= ~(1 << irq_num);
        outb(master_mask, MASTER_8259_DATA);
    }
    // if the irq is on the slave PIC
    else {
        // set the corresponding bit as unmasked
        slave_mask &= ~(1 << (irq_num - MASTER_BOUND));
        outb(slave_mask, SLAVE_8259_DATA);
    }
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
    // check if the irq_num is out of range
    if (irq_num < 0 || irq_num > IRQ_NUMBER)
        return;

    // if the irq is on the master PIC
    if (irq_num < MASTER_BOUND) {
        // set the corresponding bit as masked
        master_mask |= 1 << irq_num;
        outb(master_mask, MASTER_8259_DATA);
    }
    // if the irq if on the slave PIC
    else {
        // set the corresponding bit as masked
        slave_mask |= 1 << (irq_num - MASTER_BOUND);
        outb(slave_mask, SLAVE_8259_DATA);
    }
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
    // check if the irq_num is out of range
    if (irq_num < 0 || irq_num > IRQ_NUMBER)
        return;

    // if the irq is on the master PIC
    if (irq_num < MASTER_BOUND) {
        outb(EOI | irq_num, MASTER_8259_PORT);
    }
    // if the irq if on the slave PIC
    else {
        outb(EOI | (irq_num - MASTER_BOUND), SLAVE_8259_PORT);
        outb(EOI | ICW3_SLAVE, MASTER_8259_PORT);     // rectified with the help from Benny
    }
}
