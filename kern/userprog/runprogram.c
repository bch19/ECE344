/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>

//TODO: add error for arg overflow
/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char **argv, int argc)
{
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		return result;
	}

	// load actual arg strings onto stack

	int i;
	int strsize;
	int user_argv [argc+1]; //stackaddress for each arg we put on stack;
	int totalargsize=0;
	for(i=0; i<argc; i++)
	{
		// shift sp up by size of string
		strsize = strlen(argv[i])+1;	// +1 for \0
		stackptr -= strsize;

		// copy onto stack
		result = copyoutstr(argv[i], stackptr, strsize, NULL);
		if( result)
		{
			return result;
		}

		// save the stack address of the string we just passed
		// and put into the user's version of argv (points to the copies of args on user stack)
		user_argv[i] = stackptr;	

		// count number of bytes transfered
		totalargsize+=strsize;
		if (totalargsize > SARGS_MAX)
		{	//TODO put an error here
			kprintf("Oh noo.");
		}
	}

	//align stack
	stackptr-=stackptr%4;
	
	// load the pointers to the args onto stack

	//shift up sp	
	stackptr-=4*(argc+1);
	int tempsp=stackptr; 	// use this since we'll store first array element 
				// at the lowest space reserved and move to higher space for each following element
	int user_argv_head = stackptr; // store this to pass to md_usermode

	for(i=0; i<argc; i++)
	{
		// copy onto stack
		result = copyout(&user_argv[i], tempsp, sizeof(int));
		if( result)
		{
			return result;
		}

		// move down to the next byte of the allocated space
		tempsp+=4;

	}
	int zero = 0;		
	// put a NULL at the end of user's argv
	result = copyout(&zero, tempsp, sizeof(int));
	if( result)
	{
		return result;
	}
	

	/* Warp to user mode. */
	md_usermode( argc, user_argv_head,
		    stackptr, entrypoint);
	
	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
}

