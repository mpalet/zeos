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

#if 1
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
struct task_struct *task1;

/* last used PID */
int lastPID;

/* current task ticks */
int current_task_ticks;


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
	unsigned long * stack_base;

	//restaurem la pila de task_idle pel proxim canvi de context
	/*stack_base = (unsigned long *) (idle_task + KERNEL_STACK_SIZE -1);
	*stack_base = cpu_idle;
	stack_base--;
	*stack_base = 0;
	idle_task->kernel_esp = (unsigned long) stack_base;*/

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
	idle_task->quantum = RR_DEFAULT_QUANTUM;
	
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

	idle_task->kernel_esp = (unsigned long) stack_base;
}

void init_task1(void)
{
	struct list_head *e;
	union task_union *u;
	//struct task_struct *task1;
	/*	
	Get an available task_union from the freequeue to 
	contain the init_process
	*/
	e = list_first(&freequeue);
	task1 = list_entry(e, struct task_struct, list);
	list_del(e);
	task1->PID = 1;
	task1->quantum = RR_DEFAULT_QUANTUM;

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

	/*4) Update the TSS to make it point to the new_task system stack.*/
	u = (union task_union *) task1;
	tss.esp0 = (unsigned long)&u->stack[KERNEL_STACK_SIZE];



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

	current_task_ticks = 0;

	lastPID = 1;
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
	__asm__ __volatile__ (
  	"pushl %esi\n\t"
	"pushl %edi\n\t"
	"pushl %ebx\n\t"
	);

  inner_task_switch(new);

  __asm__ __volatile__ (
  	"popl %ebx\n\t"
	"popl %edi\n\t"
	"popl %esi\n\t"
	);

}

void inner_task_switch(union task_union *new)
{
	struct task_struct * current_task_struct;
	page_table_entry * dir_new, * dir_current;

	current_task_struct = current();
	dir_new = get_DIR(&new->task);
	dir_current = get_DIR(current_task_struct);

	// 1) Update the TSS to make it point to the new_task system stack.
	tss.esp0 = (unsigned long) &new->stack[KERNEL_STACK_SIZE];
	

	// 	2) Change the user address space by updating the current page directory: use the set_cr3
	// funtion to set the cr3 register to point to the page directory of the new_task.
	if (dir_new != dir_current) set_cr3(dir_new);

/*	3) Store the current value of the EBP register in the PCB. EBP has the address of the current
	system stack where the inner_task_switch routine begins (the dynamic link).
	4) Change the current system stack by setting ESP register to point to the stored value in the
	new PCB.
	5) Restore the EBP register from the stack.
	6) Return to the routine that called this one using the instruction RET (usually task_switch,
	but. . . ).
*/

	unsigned long *kernel_esp = &(current_task_struct->kernel_esp);
	unsigned long new_kernel_esp = new->task.kernel_esp;

	__asm__ __volatile__(
  	"movl %%ebp, (%0)\n\t" 	//3
  	"movl %1, %%esp\n\t"	//4
  	"popl %%ebp\n\t"		//5
  	"ret"					//6
	:  
	: "r" (kernel_esp), "r" (new_kernel_esp)
  	);
}


/* get new PID value */
int get_new_PID() {
	lastPID++;
	return lastPID;
}


int get_quantum (struct task_struct *t) {
	return t->quantum;
}

void set_quantum (struct task_struct *t, int new_quantum) {
	t->quantum = new_quantum;
}


/*Function to update the relevant information to take scheduling decisions. In the case of the
round robin policy it should update the number of ticks that the process has executed since
it got assigned the cpu.*/
void update_sched_data_rr (void) {
	current_task_ticks++;
}


/*returns: 1 if it is necessary to change the current process and 0
otherwise*/
int needs_sched_rr (void) {
	//proc supera el quanum
	if (current_task_ticks >= get_quantum(current())) {
		// la llista ready es buida. reset quantum i continuem executant
		if (list_empty(&readyqueue)) {
			current_task_ticks = 0;
			return 0;
		}
		//la llista ready no es buida, cal canviar el proces en execucio
		else {
			return 1;
		}
	}

	return 0;
}

/*Function to update the state of a process. If the current state of the process is not running,
then this function deletes the process from its current queue. If the new state of the process is
not running, then this function inserts the process into a suitable queue (for example, the free
queue or the ready queue). The parameters of this function are the task_struct of the process
and the queue according to the new state of the process. If the new state of the process is
running, then the queue parameter shoud be NULL.*/
void update_process_state_rr (struct task_struct *t, struct list_head *dst_queue) {
	
	//passem proces a ready >> proces estava running
	if (dst_queue == &readyqueue) {
		list_add_tail(&(t->list), dst_queue);
	}
	else if (dst_queue == NULL) {
		list_del(&t->list);
	}
}


/* round robin scheduler routine*/
void scheduler(void) {

	update_sched_data_rr();

	if (needs_sched_rr()) {

		//send curent process to ready queue
		update_process_state_rr(current(), &readyqueue);
		sched_next_rr();
	}
}


void sched_next_rr(void) {
	struct list_head * e;
	struct task_struct * next;

	if (!list_empty(&readyqueue)) {
		//get next ready process
		e = list_first(&readyqueue);
  		next = list_entry(e, struct task_struct, list);
	}
	else {
		next = idle_task;
	}

  	update_process_state_rr(next, NULL);

	//reset current task tick counter and switch task
	current_task_ticks = 0;
	task_switch(TASK_UNION(next));
}