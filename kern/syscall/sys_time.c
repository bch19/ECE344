#include <types.h>
#include <kern/errno.h>
#include <kern/errno.h>
#include <clock.h>
#include <lib.h>
#include <syscall.h>

// returns error code or success
// returns EFAULT if seconds and nanoseconds ptrs are not in user space
// actually copyout checks EFAULTS for us
// secondsKrn is used to return seconds by value from the syscall
int sys_time(time_t *secondsKrn,time_t *seconds, unsigned long *nanoseconds)
{
	int result=0;	
	time_t secs, nsecs;

	// get the time
	gettime(&secs, &nsecs);

	// debugging
	//kprintf ("Current time is: %d s or %lu ns\n",secs, nsecs);

	// error check
	if (seconds==NULL)
	{
		//do nothing
	}	
	// copyout
	else
	{
		result=copyout(&secs, seconds, sizeof(secs));
		if (result)
		{
			return result;
		}
	}

	// error check
	if (nanoseconds==NULL)
	{
		//do nothing
	}	
	// copyout
	else
	{
		result=copyout(&nsecs, nanoseconds, sizeof(nsecs));
		if (result)
		{
			return result;
		}
	}

	// seconds also returned from the syscall	
	*secondsKrn = secs;

	return result;
}
