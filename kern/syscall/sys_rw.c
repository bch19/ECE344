#include <types.h>
#include <kern/limits.h>
#include <kern/errno.h>
#include <lib.h>
#include <synch.h>
#include <kern/unistd.h>
#include <thread.h>


int
sys_write (int fd, const void *buf , size_t size , int *errno){

	/********************PARAMETER ERROR CHECKING  ****************************/
	
	// if FD is out of range or to stdin, then set errno
	if (fd != 1 && fd != 2 ){
		*errno = EBADF;
		return -1;
	}

	// if nbytes is 0 then there is nothing to write
	if (size == 0 ){ 
		*errno = EBADF;
		return -1; 
	}
	
	// if buf is NULL, then there is no buffer to read from
	if (buf == NULL){
		*errno = EFAULT;
		return -1;
	}
	
	
	int test_result;
	char* testbuffer = (char*)kmalloc(sizeof(char)*size);
	test_result = copyin(buf, testbuffer,size);

	if (test_result){
		*errno = EFAULT;
		kfree(testbuffer);
		return -1;
	}
	kfree(testbuffer);
	/*************************************************************************/
	

	lock_acquire(writeLock);
	int result;
	char* kbuffer = (char*)kmalloc(sizeof(char)*size);
	char c;
	unsigned i;
	result = copyin( buf, kbuffer , size);
	// buffer to read in
	void *temp = kmalloc (sizeof(char)*size);	

	// copy in from userspace
	copyin(buf, temp, size);


	for ( i = 0 ; i< size ; i++){
		c = kbuffer[i];
		putch(c);
	}	
	 
	lock_release(writeLock);

	kfree(kbuffer);
	return size;
}


int
sys_read (int fd, void *buf , size_t size , int *errno){
	
	/********************PARAMETER ERROR CHECKING  ****************************/

	// if FD is out of range or to stdin, then set errno
	if (fd != 0 ){
		*errno = EBADF;
		return -1;
	}

	// Assume all size = 1
	if (size == 0 ){ 
		*errno = EBADF;
		return -1; 
	}
	
	// Check for NULL address
	if (buf == NULL){
		*errno = EFAULT;
		return -1;
	}
	

	// Check for valid address
	int test_result;
	char* testbuffer = (char*)kmalloc(sizeof(char)*size);
	test_result = copyin(buf, testbuffer,size);

	if (test_result){
		*errno = EFAULT;
		kfree(testbuffer);
		return -1;
	}
	kfree(testbuffer);
	/*************************************************************************/
	//lock_acquire(writeLock);
	int result;
	char* kbuffer = (char*)kmalloc(sizeof(char)*size);
	unsigned i;
	
	
	for ( i = 0 ; i< size ; i++){
		kbuffer [i] = getch();
	}	
	

	result = copyout( kbuffer, buf , size);	
	//lock_release(writeLock);
	kfree(kbuffer);
	return size;



}




