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

//SYSCALL CODES
#define SYSCALL_WRITE 4
#define SYSCALL_GETTIME 10

#endif  /* __CODES_H__ */
