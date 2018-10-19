#ifndef _VM_H_
#define _VM_H_

#include <machine/vm.h>

/*
 * VM system-related definitions.
 *
 * You'll probably want to add stuff here.
 */
// TODO: remove dumb parts

/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/

/* 1 MB for stack & heap */
#define STACK_MAXPAGE 256
#define HEAP_MAXPAGE 256


/* Enum for page status in coremap */
typedef enum{
	/* For coremap */
	PFREE,    // page is free, can be allocated	
	PKERNEL,  // page is allocated for kernel, don't mess with it

	/* User page */	 
	PDIRTY,   // page is dirty, need to be flushed when replaced
	PCLEAN    // page is clean, can be free without flushing
}p_state;



// Writes the the specified page of the process to the swap file.
// Info about the actual physical address of the page and the
// offset to place it in the file is taken from curthread
void savepage(int vpage);

// Reads the the specified page of the process from the swap file
// and loads it into the physical page specified.
// Info about the actual physical address of the vpage and the
// offset in the file where it is found is taken from curthread
void loadpage(int vpage, int ppage);

// holds index of tlb entry to replace 
int nextEvict;

// for debugging
// print hexdump of tlb
void printtlb(void);


/*
 *  COREMAP RELATED STUFF
 */

/* Coremap entry */
struct coremap_entry {

	pid_t pid;  //owner of the page
	vaddr_t vaddr;  // vaddr mapped to this ppage
	paddr_t paddr;  // paddr of this page
	unsigned block_size; // size of chunk, need for free_kpages (not used by user alloc)
	p_state pstate;   // page status, look at the enum on top
};

//TODO: should be initialized before vmbootstrap to avoid mem leak?
// Figure out how much entry should be in this after rambootstrap
// # of entry is figured out in runtime
/* Table that stores all the info of the ppage */
struct coremap_entry *coremap;


/* lock for coremap */
struct lock* coremap_lock;

/* Global for coremap */
paddr_t corebase;    // start of the free physical memory we can use
int coremap_size;    // # of coremap_entry in coremap



// Not used anymore after coremap bootstrap
//paddr_t getppage(unsigned long npagess);

/* Find the coremap entry given paddr */
struct coremap_entry* get_coremapentry(paddr_t paddr);

// Allocate a single free page for user, assign pid to the page
// TODO: Doesnt support multi-processes now pid ignroed 
paddr_t get_ppage(vaddr_t vaddr);

/* Free the page for user */
void free_ppage(vaddr_t va, u_int32_t pageframenumber);

// initialize coremap
void coremap_bootstrap(void);

/* Initialization function */
void vm_bootstrap(void);

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress);

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(int npages);
void free_kpages(vaddr_t addr);

#endif /* _VM_H_ */
