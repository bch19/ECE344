#ifndef _KERN_LIMITS_H_
#define _KERN_LIMITS_H_

/* Longest filename (without directory) not including null terminator */
#define NAME_MAX  255

/* Longest full path name */
#define PATH_MAX   1024

// max number of open files per thread
#define OPEN_MAX 16
#define PROCESS_MAX 64
#define CHILD_MAX 16
#define PID_MIN 2

// for running programs
#define NARGS_MAX 15
#define SARGS_MAX 1024


#endif /* _KERN_LIMITS_H_ */
