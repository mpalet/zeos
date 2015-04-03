#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
  int pid, t, f;
  char* text = "\nUser code: hola que tal?\n";
  write(1,text,strlen(text));

  char* text2 = "             0";

  while(1) {  
    write(1,"PID: ",strlen("PID: "));
      pid=getpid();
      itoa(pid,text2);
      write(1,text2,strlen(text2));
      write(1,"         \n",strlen("         \n"));

    t=gettime();
    itoa(t,text2);
    write(1,text2,strlen(text2));
    write(1,"         \n",strlen("         \n"));

    if (t%1000 == 0) {
      write(1,"       fork: ",strlen("       fork: "));
      f=fork();
      if (f < 0)
        f = errno;

      itoa(f,text2);
      write(1,text2,strlen(text2));
      write(1,"         \n",strlen("         \n"));
    }

     if (t>10000-(pid*500))
      exit();
  }
}
