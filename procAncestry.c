#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/list.h>
#include "ancestor.h"

unsigned long **sys_call_table;

asmlinkage long (*ref_sys_cs3013_syscall2)(void);

asmlinkage long new_sys_procAncestry(unsigned short *target_pid, struct ancestry *response) {
  struct task_struct *task = current; // Get the task_struct of the current proccess
  struct task_struct *temp_task; // Pointer for list_for_each_entry macro
  struct task_struct *sib_task; // Pointer for list_for_each_entry macro on siblings
  struct task_struct *kid_task; // Pointer for list_for_each_entry macro on siblings
  struct ancestry temp_response; // Ancestry struct to fill out in kernel
  unsigned short pid; // PID value in kernel
  int i; // loop integer
  // Take value given by user and initialize it in kernel space
  if(copy_from_user(&pid,target_pid,sizeof(unsigned short)) < 0)return -EFAULT;

  printk(KERN_INFO "looking for task with pid %d", pid);
  list_for_each_entry(temp_task, &task->tasks, tasks){ // Iterate through list
    if ((unsigned short)temp_task->pid == *target_pid) { // Find specified pid
      printk(KERN_INFO "target pid %d found!", pid);
      break; // Break the loop
    }
  }
  if ((unsigned short)temp_task->pid != pid) {
    printk(KERN_INFO "Could not find pid %d currently running", pid);
    return -1; //error if could not find specified pid
  }
  // Loop through child list
  i = 0;
  list_for_each_entry(kid_task, &temp_task->children, sibling){
    temp_response.children[i] = kid_task->pid;
    printk(KERN_INFO "child pid: %d", temp_response.children[i]);
    i++;
  }
  // loop through sibling list
  i = 0;
  list_for_each_entry(sib_task, &temp_task->sibling, sibling){
    printk(KERN_INFO "sibling pid: %d", sib_task->pid);
	  temp_response.sibling[i] = sib_task->pid;
	  i++;
  }
  // Go up ancestry tree
  i = 0;
  while ((int)temp_task->pid > 1) {
	  temp_task = temp_task->real_parent; //reasign temp_task with parent task_struct
	  printk(KERN_INFO "%d'th parent pid: %d", i, (int)temp_task->pid);
	  temp_response.ancestors[i] = temp_task->pid;
	  i++;
  }
  if(copy_to_user(response, &temp_response,sizeof(struct ancestry)) < 0) return -EFAULT;
  return 0;
}

static unsigned long **find_sys_call_table(void) {
  unsigned long int offset = PAGE_OFFSET;
  unsigned long **sct;

  while (offset < ULLONG_MAX) {
    sct = (unsigned long **)offset;

    if (sct[__NR_close] == (unsigned long *) sys_close) {
      printk(KERN_INFO "Interceptor: Found syscall table at address: 0x%02lX",
	     (unsigned long) sct);
      return sct;
    }

    offset += sizeof(void *);
  }

  return NULL;
}

static void disable_page_protection(void) {
  /*
    Control Register 0 (cr0) governs how the CPU operates.

    Bit #16, if set, prevents the CPU from writing to memory marked as
    read only. Well, our system call table meets that description.
    But, we can simply turn off this bit in cr0 to allow us to make
    changes. We read in the current value of the register (32 or 64
    bits wide), and AND that with a value where all bits are 0 except
    the 16th bit (using a negation operation), causing the write_cr0
    value to have the 16th bit cleared (with all other bits staying
    the same. We will thus be able to write to the protected memory.

    It's good to be the kernel!
  */
  write_cr0 (read_cr0 () & (~ 0x10000));
}

static void enable_page_protection(void) {
  /*
   See the above description for cr0. Here, we use an OR to set the
   16th bit to re-enable write protection on the CPU.
  */
  write_cr0 (read_cr0 () | 0x10000);
}

static int __init interceptor_start(void) {
  /* Find the system call table */
  if(!(sys_call_table = find_sys_call_table())) {
    /* Well, that didn't work.
       Cancel the module loading step. */
    return -1;
  }

  /* Store a copy of all the existing functions */
  ref_sys_cs3013_syscall2 = (void *)sys_call_table[__NR_cs3013_syscall2];

  /* Replace the existing system calls */
  disable_page_protection();

  sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)new_sys_procAncestry;

  enable_page_protection();

  /* And indicate the load was successful */
  printk(KERN_INFO "Loaded interceptor!");

  return 0;
}

static void __exit interceptor_end(void) {
  /* If we don't know what the syscall table is, don't bother. */
  if(!sys_call_table)
    return;

  /* Revert all system calls to what they were before we began. */
  disable_page_protection();
  sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)ref_sys_cs3013_syscall2;
  enable_page_protection();

  printk(KERN_INFO "Unloaded interceptor!");
}

MODULE_LICENSE("GPL");
module_init(interceptor_start);
module_exit(interceptor_end);
