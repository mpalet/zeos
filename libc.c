/*
 * libc.c 
 */

#include <libc.h>
#include <types.h>
#include <codes.h>

int errno;

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}


//error handling
void perror() {
  switch(errno) {
    case -ENOSYS:
      write(1,"Invalid system call\n",18);
    break;
    case -EBADBUF:
      write(1,"Bad buffer         \n",18);
    break;
    case -ENEGSIZE:
      write(1,"Negative size      \n",18);
    break;
    case -EUSERMEMCOPY:
      write(1,"Error accessing user mem\n",20);
    break;
    default:
      write(1,"Invalid error code      \n",20);
  }
}


//SYS CALL HANDLERS ROUTINES
int generate_sys_trap(int syscall_number, int arg1, int arg2, int arg3) {
 int res;
  __asm__ volatile(
    "int $0x80"        /* make the request to the OS */
    : "=a" (res),      /* return result in eax ("a") */
      "+b" (arg1),     /* pass arg1 in ebx ("b") */
      "+c" (arg2),     /* pass arg2 in ecx ("c") */
      "+d" (arg3)      /* pass arg3 in edx ("d") */
    : "a"  (syscall_number)       /* pass system call number in eax ("a") */
  );
  
  return res;
}

int write (int fd, char * buffer, int size) {
  int res = generate_sys_trap(SYSCALL_WRITE, fd, (int)buffer, size);
  
  if (res >= 0)
    return res;
  else {
    errno = res;
    return -1;
  }
}

int gettime() {
  int res = generate_sys_trap(SYSCALL_GETTIME, 0, 0, 0);
  
  if (res >= 0)
    return res;
  else {
    errno = res;
    return -1;
  }
}

