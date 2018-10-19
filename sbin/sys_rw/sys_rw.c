#include <types.h>
#include <thread.h>
#include <uio.h>
#include <syscall.h>
#include <vfs.h>
#include <vnode.h>
#include <copyinout.h>
#include <kern/iovec.h>
#include <unistd.h>
#include <errno.h> 



//TODO: Go through the code again and check which part can we add more error condition to make it more robust


int 
write (int fd, void *buf , size_t buflen) {

	
	/********************PARAMETER ERROR CHECKING  ****************************/

	// if FD is out of range then return error
	if (fd > (int)curtthread -> array_getnum(fd_table) || fd < 0 ){
		errno = EBADF;
		return -1;
	}

	// if the FD is NULL or if the FD permission is read only
	if ( curthread -> array_getguy(fd_table , fd) == NULL || curthread -> array_getguy (fd_table , fd ) -> permission == 		O_RDONLY){
		errno = EBADF;
		return -1;
	}

	// if nbytes is 0 then there is nothing to write
	if (nbytes == 0 ){ 
		errno = EBADF;
		return -1; 
	}
	
	// if buf is NULL, then there is no buffer to write to
	if (buf == NULL){
		errno = EFAULT;
		return -1;
	}


	/*************** SET UP UIO **************************************************/
	struct uio u;

	int bytesWrite;   // for return purpose, if successful -> number of bytes wrote , if fail -> -1
	int result;	  // for vop_write error checking
		
	u.uio_iovec.iov_ubase = (userptr_t) buf;
	u.uio_iovec.iovlen = nbytes;
	u.uio_offset = curthread -> array_getguy(fd_table , fd) -> offset;
	u.uio_resid = nbytes;
	u.segflg = UIO_USERISPACE;
	u.uio_rw = UIO_Write;
	u.uio_space = curthread -> t_vmspace;


	/*****************USE VOP_READ ********************************/
	
	result = vop_write ( curthread -> array_getguy( fd_table , fd ) -> v , &u);

	if ( result ){
		// the result will be the error condition
		errno = result;
		return -1;
	} 


	/********************* CHANGING OFFSET ****************************/
	
	// for return purpose	
	bytesWrite = (int) uio.uio_offset - (int) curthread -> array_getguy(fd_table , fd) -> offset ;
	
	// update the offset in the file table to the offset in the UIO
	curthread -> array_getguy( fd_table , fd) -> offset = uio.uio_offset ;
		
	
	return bytesWrite ;
}





int 
read (int fd, const void *buf , size_t nbytes) {

	
	/********************PARAMETER ERROR CHECKING  ****************************/

	// if FD is out of range then return error
	if (fd > (int)curtthread -> array_getnum(fd_table) || fd < 0 ){
		errno = EBADF;
		return -1;
	}

	// if the FD is NULL or if the FD permission is write only
	if ( curthread -> array_getguy(fd_table , fd) == NULL || curthread -> array_getguy (fd_table , fd ) -> permission == 		O_WRONLY){
		errno = EBADF;
		return -1;
	}

	// if nbytes is 0 then there is nothing to read
	if (nbytes == 0 ){ 
		errno = EBADF;
		return -1; 
	}
	
	// if buf is NULL, then there is no buffer to read from
	if (buf == NULL){
		errno = EFAULT;
		return -1;
	}


	/*************** SET UP UIO **************************************************/
	struct uio u;

	int bytesRead;   // for return purpose, if successful -> number of bytes read , if fail -> -1
	int result;	  // for vop_read error checking
		
	u.uio_iovec.iov_ubase = (userptr_t) buf;
	u.uio_iovec.iovlen = nbytes;
	u.uio_offset = curthread -> array_getguy(fd_table , fd) -> offset;
	u.uio_resid = nbytes;
	u.segflg = UIO_USERISPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = curthread -> t_vmspace;


	/*****************USE VOP_WRITE ********************************/
	
	result = vop_read ( curthread -> array_getguy( fd_table , fd ) -> v , &u);

	if (result ){
		// the result will be the error condition ((((NOT SURE))))
		//errno = result;
		return result;
	} 


	/********************* CHANGING OFFSET ****************************/
	
	// for return purpose	
	bytesRead = (int) uio.uio_offset - (int) curthread -> array_getguy(fd_table , fd) -> offset ;
	
	// update the offset in the file table to the offset in the UIO
	curthread -> array_getguy( fd_table , fd) -> offset = uio.uio_offset ;
		
	
	return bytesRead ;
}









































	









