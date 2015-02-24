/*
 * entry.S - Entry point to system mode from user mode
 */

#include <asm.h>
#include <segment.h>


/**************************************************/
/**** Save & Restore ******************************/
/**                                              **/
/** When we change to privilege level 0 (kernel) **/
/** (through an interrupt, a system call, an     **/
/** exception ...) we must save the state of the **/
/** currently running task (save).               **/
/**                                              **/
/** Stack layout in 'systemCall':                **/
/**                                              **/
/**   0(%esp) - %ebx    \                        **/
/**   4(%esp) - %ecx     |                       **/
/**   8(%esp) - %edx     |                       **/
/**   C(%esp) - %esi     | Register saved        **/
/**  10(%esp) - %edi     |  by 'save'            **/
/**  14(%esp) - %ebp     |                       **/
/**  18(%esp) - %eax     |                       **/
/**  1C(%esp) - %ds      |                       **/
/**  20(%esp) - %es      |                       **/
/**  24(%esp) - %fs      |                       **/
/**  28(%esp) - %gs     /                        **/
/**  2C(%esp) - %eip    \                        **/
/**  30(%esp) - %cs      |                       **/
/**  34(%esp) - %eflags  |  Return context saved **/
/**  38(%esp) - %oldesp  |   by the processor.   **/
/**  3C(%esp) - %oldss  /                        **/
/**                                              **/
/**************************************************/

#define SAVE_ALL \
      pushl %gs; \
      pushl %fs; \
      pushl %es; \
      pushl %ds; \
      pushl %eax; \
      pushl %ebp; \
      pushl %edi; \
      pushl %esi; \
      pushl %edx; \
      pushl %ecx; \
      pushl %ebx; \
      movl $__KERNEL_DS, %edx;    \
      movl %edx, %ds;           \
      movl %edx, %es

#define RESTORE_ALL \
      popl %ebx; \
      popl %ecx; \
      popl %edx; \
      popl %esi; \
      popl %edi; \
      popl %ebp; \
      popl %eax; \
      popl %ds; \
      popl %es; \
      popl %fs; \
      popl %gs

#define EOI      \
  movb $0x20, %al ; \
  outb %al, $0x20 ;

//ERROR CODES
#define ENOSYS 38

//INTERRUPTION HANDLERS

//keyboard
ENTRY(keyboard_handler)
  SAVE_ALL                           // Save the current context
  EOI
  call keyboard_rsi
  RESTORE_ALL                        // Restore the context
  iret

//SYSTEM CALLS
ENTRY (sys_call_table)
        .long sys_ni_syscall    // 0
        .long sys_ni_syscall    // 1
        .long sys_ni_syscall    // 2
        .long sys_ni_syscall    // 3
        .long sys_write         // 4
.globl MAX_SYSCALL
MAX_SYSCALL = (. - sys_call_table)/4

//system_call_handler
ENTRY(system_call_handler)
  SAVE_ALL                 // Save the current context
  cmpl $0, %eax            // Is syscall number negative?
  jl err                   // If it is, jump to return an error
  cmpl $MAX_SYSCALL, %eax  // Is syscall greater than MAX_SYSCALL (4)?
  jg err                    // If it is, jump to return an error
  call *sys_call_table(, %eax, 0x04) // Call the corresponding service routine
  jmp fin                            // Finish
err:
  movl $-ENOSYS, %eax                // Move to eax the ENOSYS error
fin:
  movl %eax, 0x18(%esp)              // Change the eax value in the stack
  RESTORE_ALL                        // Restore the context
  iret

