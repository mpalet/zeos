/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <codes.h>

#define LECTURA 0
#define ESCRIPTURA 1

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -ENOSYS; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int sys_fork()
{
  int PID=-1;

  // creates the child process
  
  return PID;
}

void sys_exit()
{  
}

/*
fd: file descriptor. In this delivery it must always be 1.
buffer: pointer to the bytes.
size: number of bytes.
return: Negative number in case of error (specifying the kind of error) and the number of bytes written if OK.
*/

#define BUFFERSIZE 1024
unsigned char dest[BUFFERSIZE];

int sys_write(int fd, char * buffer, int size) {
  int res;
  
  res = check_fd(fd, ESCRIPTURA);
  if (res < 0)
    return res;
  
  if (buffer == NULL)
    return -EBADBUF;

  if (size < 0)
    return -ENEGSIZE;
  
  
  int i,ret;
  
  for (i=0,ret=0; i<size; i +=BUFFERSIZE) {
    int len = min(size-i,BUFFERSIZE);
    int callret;

    if (copy_from_user(&buffer[i],dest,len))
      return -EUSERMEMCOPY;
    
    callret = sys_write_console(&buffer[i],len);

    if (callret < 0)
      return -1; //TODO: definir user code

    ret += callret;
  }

  return ret;
}

