#include <types.h>
#include <kern/errno.h>
#include <array.h>
#include <curthread.h>
#include <lib.h>
#include <kern/limits.h>
#include <synch.h>
#include <kern/unistd.h>
#include <thread.h>
#include <syscall.h>
#include <thread.h>





void sys__exit(int code){

	if (curthread != NULL){
		// Set exit status to 1 and pass the exit code
		curthread-> exit_status = 1;
		curthread-> exit_code = code; 
	
		V(curthread->waitpid_sem);
		thread_exit();	
	}
	
}



int
sys_waitpid(int _pid, int *status, int options, int *errno){

	/**** Parameter checking ****/
	if (_pid < 1 || _pid > PROCESS_MAX-1){
		*errno = EINVAL;
		return -1;
	} 

	if (status == NULL){
		*errno = EFAULT;
		return -1;
	}
	
	if (options){
		*errno = EINVAL;
		return -1;
	}

	
	// Check if the pid given is a child of curthread	
	int found = 0;
	int result;  
	int i;
	struct thread *child_thread;
	
	// The assignment will either assign to clone or actual child
	child_thread = array_getguy(process_table , _pid);
	
	// Invalid PID	
	if (child_thread == NULL){
		*errno = EINVAL;
		return -1;
	}	

	// If is process's child then found 
	if ( child_thread-> ppid == curthread-> pid){
		
		found = 1;
		// If the child exited already then return with its exit code
		if ( child_thread -> exit_status == 1){	
			int exitcode = child_thread->exit_code;
			result = copyout((void*)&exitcode, status, sizeof(int));
			return _pid;
		}
	} 

	// PID was never his child
	if (found == 0){
		*errno = EINVAL;
		return -1;
	}

	
	// A child died and Ved this
	P(child_thread->waitpid_sem);
		
	// Assign the clone as child_thread
	child_thread = array_getguy(process_table, _pid);


	int exitcode = child_thread->exit_code;
	result = copyout((void*)&exitcode, status, sizeof(int));
	if (result){
		*errno = EFAULT;
		return -1;
	}

	// now needs to remove child_thread off curthread's childpid_array & ptable	
	lock_acquire(ptable_lock);	
	array_setnull(process_table, _pid);
	lock_release(ptable_lock);

	// Free the clone & its waitpid_sem (thread_destroy)
	sem_destroy(child_thread->waitpid_sem);
	kfree(child_thread->t_name);
	kfree(child_thread);


	return _pid;
}

