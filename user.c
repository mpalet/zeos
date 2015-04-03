#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
  int a;
  char* text = "\nUser code: hola que tal?\n";
  write(1,text,strlen(text));

  char* text2 = "             0";

  while(1) {  
    write(1,"PID: ",strlen("PID: "));
      a=getpid();
      itoa(a,text2);
      write(1,text2,strlen(text2));
      write(1,"         \n",strlen("         \n"));

    a=gettime();
    itoa(a,text2);
    write(1,text2,strlen(text2));
    write(1,"         \n",strlen("         \n"));

    if (a%1000 == 99) {
      write(1,"       fork: ",strlen("       fork: "));
      a=fork();
      if (a < 0)
        a = errno;

      itoa(a,text2);
      write(1,text2,strlen(text2));
      write(1,"         \n",strlen("         \n"));
    }
  }
}
