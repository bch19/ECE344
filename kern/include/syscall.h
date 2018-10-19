#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <types.h>
#include <synch.h>

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */


int sys_reboot(int code);

int sys_read(int filehandle, void *buf, size_t size , int *errno);

int sys_write(int filehandle, const void *buf, size_t size , int *errno);

int sys_fork(struct trapframe *tf, int *errno);

int sys_getpid(void);


int sys_execv (char *program, char **args);

void sys__exit(int code);

int sys_waitpid(int pid, int *status, int options, int *errno);

int sys_time(time_t *secondsKrn, time_t* seconds, unsigned long *nanoseconds);


#endif /* _SYSCALL_H_ */
