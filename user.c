#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  char* text = "\nUser code: hola que tal?\n";
  write(1,text,strlen(text));

  char* text2 = "             0";

  while(1) {
    int a=gettime();
    itoa(a,text2);
    write(1,text2,strlen(text2));
    write(1,"\n",strlen("\n"));
 }
}
