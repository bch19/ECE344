#include <types.h>
#include <kern/errno.h>
#include <array.h>
#include <machine/pcb.h>
#include <curthread.h>
#include <addrspace.h>
#include <lib.h>
#include <kern/limits.h>
#include <synch.h>
#include <kern/unistd.h>
#include <thread.h>
#include <syscall.h>
#include <machine/trapframe.h>





int
sys_getpid(void){

	/****** PROCESS CHECKING *****/
	if (curthread == NULL){
		return -1;
	}	

	return curthread-> pid;
}

// Not a syscall
int
fetch_pid(void){
	
	/* Find any empty process_table slot for PID fetching */
	int i ;
	struct thread* emptyslot;
	
	
	for ( i = PID_MIN ; i < PROCESS_MAX ; i++ ){
		emptyslot = array_getguy(process_table, i);
		if (!emptyslot){
			return i ;	
		}	
	}
		
	/* Cant find any free slot in process table */
	return -1;
}



int
sys_fork(struct trapframe *tf, int *errno){

	// Parameter checking 	
	if (tf == NULL){
		*errno = EFAULT;
		return -1;
	}

	// Create vars for error checking and thread to pass to thread_fork
	int result;
	struct thread *child_thread;


	// Fetching pid: atomic 
	
	lock_acquire(ptable_lock);
	int pidresult = fetch_pid();
	if (pidresult == -1){
		// process table is null
		*errno = ENOMEM;
		lock_release(ptable_lock);	
		return -1;
	}	
	
	array_setguy(process_table , pidresult , curthread);
	lock_release(ptable_lock);	

	// End fetching pid

	// Saves a copy of tf and addrspace for later
	// trapframe rn is in heap, copy to child's kernel stack in mdforkentry
	struct trapframe *child_tf = kmalloc( sizeof(struct trapframe));
	
	// Check if valid pointer and if copy is successful
	if (child_tf == NULL || tf == NULL){
		goto error;
	}
	*child_tf = *tf;
	if (child_tf == NULL){
		goto error;
	}

	struct addrspace *child_addrspace;
	result = as_copy (curthread->t_vmspace, &child_addrspace);
	if (result){
		goto error;
	}


	// Acquire lock for atomicity
	lock_acquire(process_lock);	

	
	// Actual forking of the thread, make sure to free the child_tf in mdforkentry
	result = thread_fork(curthread->t_name, child_tf, 0 , md_forkentry, &child_thread);
	if (result || child_thread == NULL){
		*errno = EAGAIN;
		kfree(child_tf);
		lock_release(process_lock);
		array_setnull(process_table, pidresult);
		return -1;
	}
	

	child_thread->pid = pidresult;
	child_thread->ppid = curthread-> pid;
	child_thread->t_vmspace = child_addrspace;
	

	lock_acquire(ptable_lock);
	array_setguy(process_table , pidresult , child_thread);
	lock_release(ptable_lock);


	// Create a childpid array for first time forker
	if (curthread->childpid_array == NULL){
		curthread->childpid_array = array_create();
	}
	
	// If childpid array is still null some ting wong
	assert(curthread->childpid_array != NULL);
	array_add(curthread->childpid_array, child_thread);


	// Other procs can run now
	lock_release(process_lock);

	return pidresult;

error:
	*errno = ENOMEM;
	kfree(child_tf);
	array_setnull(process_table, pidresult);
	return -1;


}


