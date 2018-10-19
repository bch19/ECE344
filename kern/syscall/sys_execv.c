#include <types.h>
#include <kern/unistd.h>
#include <kern/limits.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <syscall.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>


int
sys_execv (char *program, char **args){

	int i;	//for loop indexer	
	int result;	// use this for errors	
	int argsize;	// keep track of the total size of args we've coppied
	int totalsize; // do.
	char _program[PATH_MAX]; // our copy of the program string
	userptr_t user_argptrs[NARGS_MAX+1]; // pointers to each arg string in user's stack
	char *kernel_argv[NARGS_MAX+2]; //copy of user's argv on kernel 
	char buf[SARGS_MAX]; // buffer to temporarily hold each arg we copyin
	int argc;	// number of args user sent
	struct vnode *v; // to load elf
	vaddr_t entrypoint, stackptr; // needed to pass into md_usermode

	// the error checkings
	
	// null pointers
	if (program == NULL || args == NULL)
	{
		return	EFAULT;
	}

	
	
	// copyin program name
	result = copyinstr(program, _program, PATH_MAX, NULL);
	if(result) 		
		return result;
	
	//check for empty
	if (strlen(_program)==0)
		return EINVAL;

	// copyin pointers in user's argv
	for(i=0 ; i<NARGS_MAX ; i++)
	{
		//copyin the addr
		result = copyin(&args[i], &user_argptrs[i], sizeof(userptr_t) );
		if(result) 
			return result;

		// check if we've reached the end
		if(user_argptrs[i]==0x0)
			break;
	}	

	// in case user's args hit the max, we'll stop it 
	// and mark the end with null ourselves
	user_argptrs[NARGS_MAX]=0x0;

	// we know how many args now
	argc=i;
	
	// copyin each arg string from user's argv
	totalsize=0;
	for(i=0 ; i<argc ; i++)
	{
		result = copyinstr(user_argptrs[i], buf, SARGS_MAX, NULL);
		if(result)
			return result;
		
		//size check
		argsize = strlen(buf)+1;	
		totalsize+=argsize;
		if(totalsize>SARGS_MAX)
			return E2BIG;

		//put into kernel_argv
		kernel_argv[i]=kmalloc(argsize);
		strcpy (kernel_argv[i], buf);
	}
		
	/* print test 
	//TODO: comment out
	kprintf("program: %s\n",_program);
	kprintf("user's arg addr: %x\n",user_argptrs[0]);
	kprintf("argc: %d\n",argc);
	kprintf("user_argptrs[argc]: %x\n",user_argptrs[argc]);
	kprintf("kernel_argv[0]: %s\n",kernel_argv[0]);
	kprintf("kernel_argv[1]: %s\n",kernel_argv[1]);
	kprintf("totalsize: %d\n",totalsize);
	*/	
	// load new program onto address space
	result = vfs_open(_program, O_RDONLY, &v);
	if (result) {
		return result;
	}
	
	// clear old addrspace
	struct addrspace *as = curthread->t_vmspace;
/*	
	kfree(curthread->t_vmspace);
	curthread->t_vmspace = as_create();
*/
	as->as_vbase1 = 0;
	//as->as_pbase1 = 0;
	as->as_npages1 = 0;
	as->as_permission1 = 0x0;
	as->as_vbase2 = 0;
	//as->as_pbase2 = 0;
	as->as_npages2 = 0;
	as->as_permission2 = 0x0;
	as->heapvbase = 0;
	as->heapvtop = 0;
	as->stackvbase = 0;

	// reset address space entrypoint and stackptr
	as_activate(curthread->t_vmspace);

	result = load_elf(v, &entrypoint);
	if (result) {
		vfs_close(v);
		return result;
	}
	vfs_close(v);
	
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		return result;
	}
	
	// load actual arg strings onto stack

	int user_argv [argc+1]; //stackaddress for each arg we put on stack;
	totalsize=0;
	for(i=0; i<argc; i++)
	{
		// shift sp up by size of string
		argsize = strlen(kernel_argv[i])+1;	// +1 for \0
		stackptr -= argsize;

		// copy onto stack
		result = copyoutstr(kernel_argv[i], stackptr, argsize, NULL);
		if( result)
		{
			return result;
		}

		// save the stack address of the string we just passed
		// and put into the user's version of argv (points to the copies of args on user stack)
		user_argv[i] = stackptr;	

		// count number of bytes transfered
		totalsize+=argsize;
		if (totalsize > SARGS_MAX)
		{	
			return E2BIG;	
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
