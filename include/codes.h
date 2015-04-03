/*
 * codes.h - Definició de codis d'error i constants
 */

#ifndef __CODES_H__
#define __CODES_H__


//ERROR CODES
#define ENOSYS 38 //NO SYSTEM CALL
#define EBADBUF 50 //invalid buffer
#define ENEGSIZE 60 //negative size
#define EUSERMEMCOPY 70 //error accessing user memory
#define ENOFREEPROC 80 //error no free processes
#define ENOFRAMEAVAIL 81 //error no physical frames available




//SYSCALL CODES
#define SYSCALL_EXIT 1
#define SYSCALL_FORK 2
#define SYSCALL_WRITE 4
#define SYSCALL_GETTIME 10
#define SYSCALL_GETPID 20

#endif  /* __CODES_H__ */
