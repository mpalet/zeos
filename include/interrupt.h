/*
 * interrupt.h - Definició de les diferents rutines de tractament d'exepcions
 */

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <types.h>

#define IDT_ENTRIES 256

//CONSTANTS
#define KEYBOARD_PORT 0X60

//IDT ENTRIES
#define IDTENTRY_KEYBOARD 33
#define IDTENTRY_CLOCK 32
#define IDTENTRY_SYSTEM_CALL 0x80

extern Gate idt[IDT_ENTRIES];
extern Register idtR;

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL);
void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL);

void setIdt();

void keyboard_rsi();
void clock_rsi();

void init_clock();
int get_zeos_clock();

#endif  /* __INTERRUPT_H__ */
