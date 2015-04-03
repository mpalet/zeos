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


int ret_from_fork() {
  return 0;
}

int sys_fork2(void)
{
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;
  
  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -1;

  lhcurrent=list_first(&freequeue);
  
  list_del(lhcurrent);
  
  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);
  
  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  /* new pages dir */
  allocate_DIR((struct task_struct*)uchild);
  
  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    new_ph_pag=alloc_frame();
    if (new_ph_pag!=-1) /* One page allocated */
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA_P0+pag, new_ph_pag);
    }
    else /* No more free pages left. Deallocate everything */
    {
      /* Deallocate allocated pages. Up to pag. */
      for (i=0; i<pag; i++)
      {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA_P0+i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA_P0+i);
      }
      /* Deallocate task_struct */
      list_add_tail(lhcurrent, &freequeue);
      
      /* Return error */
      return -1; 
    }
  }

  /* Copy parent's SYSTEM and CODE to child. */
  page_table_entry *parent_PT = get_PT(current());
  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag=0; pag<NUM_PAG_CODE; pag++)
  {
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE_P0+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE_P0+pag));
  }
  /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
  for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
  {
    /* Map one child page to parent's address space. */
    set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
    copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
  }
  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));
  
  uchild->task.PID=get_new_PID();
  //uchild->task.state=ST_READY;
  
  int register_ebp;   /* frame pointer */
  /* Map Parent's ebp to child's stack */
  __asm__ __volatile__ (
    "movl %%ebp, %0\n\t"
      : "=g" (register_ebp)
      : );
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->task.kernel_esp=register_ebp + sizeof(DWord);
  
  DWord temp_ebp=*(DWord*)register_ebp;
  /* Prepare child stack for context switch */
  uchild->task.kernel_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.kernel_esp)=(DWord)&ret_from_fork;
  uchild->task.kernel_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.kernel_esp)=temp_ebp;

  /* Set stats to 0 */
  //init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  //uchild->task.state=ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);
  
  return uchild->task.PID;
}

int sys_fork()
{
  struct list_head * e;
  struct task_struct * child, * parent;

  /*  Get a free task_struct for the process. If there is no space for a new process, an error
  will be returned.*/
  if (list_empty(&freequeue))
    return -ENOFREEPROC; //there's no space for a new process

  e = list_first(&freequeue);
  child = list_entry(e, struct task_struct, list);
  list_del(e);

  /*Inherit system data: copy the parent’s task_union to the child. Determine whether it is
  necessary to modify the page table of tablhe parent to access the child’s system data. The
  copy_data function can be used to copy.*/
  parent = current();
  copy_data(parent,child,sizeof(union task_union));

  /*Initialize field dir_pages_baseAddr with a new directory to store the process address
  space using the allocate_DIR routine.*/
  allocate_DIR(child);

  /*Search physical pages in which to map logical pages for data+stack of the child process
  (using the alloc_frames function). If there is no enough free pages, an error will be
  return.
  B) Page table entries for the user data+stack have to point to new allocated pages
  which hold this region*/ 
  unsigned int pag, frame;
  page_table_entry * child_PT =  get_PT(child);

  /*DATA+STACK pages initialize*/
  for (pag=0;pag<NUM_PAG_DATA;pag++){
    frame=alloc_frame();

    if (frame == -1)
      return -ENOFRAMEAVAIL; //there's no enough free pages TODO: restaurar l'estat abans de la crida a fork

    set_ss_pag(child_PT, PAG_LOG_INIT_DATA_P0+pag, frame);
  }


  /*A) Page table entries for the system code and data and for the user code can be a
  copy of the page table entries of the parent process (they will be shared)*/

   /* Copy parent's SYSTEM and CODE to child. */
  page_table_entry *parent_PT = get_PT(current());
  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(child_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag=0; pag<NUM_PAG_CODE; pag++)
  {
    set_ss_pag(child_PT, PAG_LOG_INIT_CODE_P0+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE_P0+pag));
  }



  /*Copy the user data+stack pages from the parent process to the child process. The
  child’s physical pages cannot be directly accessed because they are not mapped in
  the parent’s page table. In addition, they cannot be mapped directly because the
  logical parent process pages are the same. They must therefore be mapped in new
  entries of the page table temporally (only for the copy). Thus, both pages can be
  accessed simultaneously as follows:
  A) Use temporal free entries on the page table of the parent. Use the set_ss_pag and
  del_ss_pag functions.
  B) Copy data+stack pages.
  C) Free temporal entries in the page table and flush the TLB to really disable the
  parent process to access the child pages.*/
  
  
  unsigned int origpage, destpage;
  for (pag=0;pag<NUM_PAG_DATA;pag++){
    origpage = PAG_LOG_INIT_DATA_P0+pag;
    destpage = origpage+NUM_PAG_DATA  ;

    frame = get_frame(child_PT, origpage);
    //temporal free page alloc on parent
    set_ss_pag(parent_PT, destpage, frame); 
    
    //Copy data+stack pages.
    copy_data((void *)(origpage<<12), (void *)(destpage<<12), PAGE_SIZE);

    //temporal page dealloc on parent
    del_ss_pag(parent_PT, destpage);
  }

  /*flush the TLB to really disable the
  parent process to access the child pages.*/
  set_cr3(get_DIR(parent));

  /*Assign a new PID to the process. The PID must be different from its position in the
  task_array table.*/
  child->PID = get_new_PID();

  /*Initialize the fields of the task_struct that are not common to the child.
  i) Think about the register or registers that will not be common in the returning of the
  child process and modify its content in the system stack so that each one receive its
  values when the context is restored.*/

  /*Prepare the child stack with a content that emulates the result of a call to task_switch.
  This process will be executed at some point by issuing a call to task_switch. Therefore
  the child stack must have the same content as the task_switch expects to find, so it will
  be able to restore its context in a known position. The stack of this new process must
  be forged so it can be restored at some point in the future by a task_switch. In fact this
  new process has to a) restore its hardware context and b) continue the execution of the
  user process, so you must create a routine ret_from_fork which does exactly this. And
  use it as the restore point like in the idle process initialization 4.4.*/

  unsigned long parent_ebp, child_ebp, *child_esp;

  __asm__ __volatile__ (
    "movl %%ebp, %0"
  : "=r" (parent_ebp)
  );  

  //calculate child ebp
  child_ebp = (parent_ebp & 0x00000fff) | ((int)child & 0xfffff000);

  child_esp = child_ebp;
  //prepare child dynamic link
  *child_esp = ret_from_fork; //return address
  child_esp--;
  *child_esp = 0; //dummy ebp

  child->kernel_esp = child_esp;


  /*Insert the new process into the ready list: readyqueue. This list will contain all processes
  that are ready to execute but there is no processor to run them.  */
  list_add_tail(&(child->list), &readyqueue);

  /*j) Return the pid of the child process.*/
  return child->PID;
}

void sys_exit()
{
  /*  Free the data structures and resources of this process (physical memory, task_struct,
  and so). It uses the free_frame function to free physical pages.
  Use the scheduler interface to select a new process to be executed and make a context
  switch.
  */
  int pag;
  page_table_entry *proc_PT = get_PT(current());

  //free process data frames
  for (pag=PAG_LOG_INIT_DATA_P0;pag<PAG_LOG_INIT_DATA_P0+NUM_PAG_DATA;pag++){
    free_frame(get_frame(proc_PT,pag));
    del_ss_pag(proc_PT, pag);
  }
  list_add_tail(&(current()->list), &freequeue);
  sched_next_rr();
}

/*
fd: file descriptor. In this delivery it must always be 1.
buffer: pointer to the bytes.
size: number of bytes.
return: Negative number in case of error (specifying the kind of error) and the number of bytes written if OK.
*/

#define BUFFERSIZE 1024
unsigned char dest[BUFFERSIZE];

//GET TIME
int sys_gettime() {
  return get_zeos_clock();
}

//SYSTEM WRITE
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