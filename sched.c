/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */

#if 0
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif

/* Task Scheduling queues */
struct list_head freequeue;
struct list_head readyqueue;
extern struct list_head blocked;

/* idle task */
struct task_struct * idle_task;


/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory"); //activa interrupcions

	while(1)
	{
	;
	}
}

void init_idle (void)
{
	struct list_head * e;
	unsigned long * stack_base;

	/*	
	Get an available task_union from the freequeue to 
	contain the idle_process
	*/
	e = list_first(&freequeue);
	idle_task = list_entry(e, struct task_struct, list);
	list_del(e);
	idle_task->PID = 0;
	
	/*
	Initialize field dir_pages_baseAaddr with a new directory to store
	the process address space
	*/
	allocate_DIR(idle_task); 

	/*
	Initialize an execution context for the procees to restore it when it
	gets assigned the cpu and executes cpu_idle.
	*/
	stack_base = (unsigned long *) (idle_task + KERNEL_STACK_SIZE -1);

	*stack_base = cpu_idle;
	stack_base--;
	*stack_base = 0;

	idle_task->kernel_esp = stack_base;
}

void init_task1(void)
{
	struct list_head *e;
	struct task_struct *task1;
	/*	
	Get an available task_union from the freequeue to 
	contain the init_process
	*/
	e = list_first(&freequeue);
	task1 = list_entry(e, struct task_struct, list);
	list_del(e);
	task1->PID = 1;

	/*
	Initialize field dir_pages_baseAddr with a new directory to store
	the process address space
	*/
	allocate_DIR(task1);

	/*
	Complete the initialization of its address space, by using the function set_user_pages (see file
	mm.c). This function allocates physical pages to hold the user address space (both code and
	data pages) and adds to the page table the logical-to-physical translation for these pages.
	Remind that the region that supports the kernel address space is already configure for all
	the possible processes by the function init_mm.
	*/
	set_user_pages(task1);

	/*Set its page directory as the current page directory in the system, by using the set_cr3
	function (see file mm.c).*/
	set_cr3(get_DIR(task1));
}


//init scheduling
void init_sched(){
	int i;
	
	/* freequeue initialitzation: 
	   Add all tasks to the free queue
	*/
	INIT_LIST_HEAD(&freequeue);

	for (i=0; i< NR_TASKS; ++i)
		list_add(&(task[i].task.list), &freequeue);

	/* readyqueue initialization:
	   queue is empty
	*/
	INIT_LIST_HEAD(&readyqueue);
}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

/*
task_switch: canvi de tasca
new: pointer to the task_union of the process that will be executed
*/
void task_switch(union task_union *new)
{
	int esi, edi, ebx;

	//save ESI, EDI, EBX
	__asm__ __volatile__(
  	"movl %%esi, %0\n\t"
  	"movl %%edi, %1\n\t"
  	"movl %%ebx, %2"
	: "=g" (esi), "=g" (edi), "=g" (ebx)
  	);

	inner_task_switch(new);

	//restore ESI, EDI, EBX
	__asm__ __volatile__ (
  	"movl %0, %%esi\n\t"
  	"movl %1, %%edi\n\t"
  	"movl %2, %%ebx"
	:
	: "g" (esi), "g" (edi), "g" (ebx)
  	);

}

void inner_task_switch(union task_union *new)
{
	struct task_struct * current_task_struct, * new_task_struct;
	page_table_entry * dir_new, * dir_current;

	current_task_struct = current();
	new_task_struct = new->task;
	dir_new = get_DIR((struct task_struct *) new);
	dir_current = get_DIR(current_task_struct);

	// 1) Update the TSS to make it point to the new_task system stack.
	tss.esp0 = (unsigned long)&new->stack[KERNEL_STACK_SIZE];
	

	// 	2) Change the user address space by updating the current page directory: use the set_cr3
	// funtion to set the cr3 register to point to the page directory of the new_task.
	if (dir_new != dir_current) set_cr3(dir_new);

/*	3) Store the current value of the EBP register in the PCB. EBP has the address of the current
	system stack where the inner_task_switch routine begins (the dynamic link).*/
	__asm__ __volatile__(
  	"movl %%ebp, %0\n\t" 	//3
  	"movl %1, %%esp\n\t"		//4
  	"popl %%ebp\n\t"
  	"ret"
	: "=g" (current_task_struct->kernel_esp)
	: "g" (new_task_struct->kernel_esp)
  	);
}
