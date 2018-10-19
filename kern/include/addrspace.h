#ifndef _ADDRSPACE_H_
#define _ADDRSPACE_H_

#include <vm.h>
//#include "opt-dumbvm.h"


struct vnode;

/*
 * Address space - data structure associated with the virtual memory
 * space of a process.
 *
 * You write this.
 */

#define PDE_MAX 1024
#define PTE_MAX 1024


/* Macros for normal PTE */
#define IS_VALID(x)   ( 0x00008000 & x )  // Check if PTE is valid
#define IS_DIRTY(x)   ( 0x00002000 & x )  // Check if PTE is dirty
#define IS_REF(x)     ( 0x00004000 & x )  // Check if PTE is referenced
#define GET_PFN(x)    ( 0x000003FF & x )  // Get PFN of the PTE
#define IS_RDABLE(x)  ( 0x00000400 & x )  // Check if PTE is readable
#define IS_WTABLE(x)  ( 0x00000800 & x )  // Check if PTE is writable
#define IS_XTABLE(x)  ( 0x00001000 & x )  // Check if PTE is executable

#define SET_RDABLE(x)  ( 0x00000400 | x )  // Check if PTE is readable
#define SET_WTABLE(x)  ( 0x00000800 | x )  // Check if PTE is writable
#define SET_XTABLE(x)  ( 0x00001000 | x )  // Check if PTE is executable


/* Macros for ELF PTE */
#define IS_ELF(x)     ( 0x80000000 & x )  // Check if PTE is load_elf format
#define SET_ELF(x)    ( 0x80000000 | x )  // Set ELF bit to 1
#define IS_E_XBLE(x)  ( 0x40000000 & x )  // Check if PTE is load_elf is_executable
#define SET_E_XBLE(x) ( 0x40000000 | x )  // Set is_executable bit

#define GET_FOFFS(x)  ( 0x000003FF &x )   // Get the file offset for load_segment
#define SET_FOFFS(x,y) ( x | y)           // Set file offset, x is PTE and y is fileoffset
#define GET_FSIZE(x)  ( (0x000FFC00 &x)>>10 )  // Get the Filesize for load_segment
#define SET_FSIZE(x,y) ( x | (y<<10) )    // Set filesize, x is PTE and y is filesize
#define GET_MEMSZ(x)  ( (0x3FF00000 &x)>>20 )  // Get the memsize for load_segment
#define SET_MEMSZ(x)  ( x | (y<<20))      // Set memsize, x is PTE and y is memsz

/* Macros for vaddr */
#define GET_OFFS(x)   ( 0x00000FFF & x )  // Get the vaddr offset
#define GET_PTBL(x)   ( (0x003FF000 &x)>> 12 ) // Get the page table index for this vaddr
#define GET_PDIR(x)   ( (0xFFC00000 &x)>> 22 ) // Get the page directory index for this vaddr

struct page_table {
	int PTE[1024];
};

/*   Structure of the PTE:  Total 16 bits
 *
 *  |  1 bit  |  1 bit  |  1 bit  | 3 bits |    10 bits    |
 *     valid      ref      dirty    X/W/R         PFN
 *
 *
 *   Structure of the ELF PTE:
 *
 *  |   1 bit  |    1 bit   |   10 bits  |  10 bits  |  10 bits  |
 *   Elf bit     is_xtable    memsize      filesize    file offset
 */


struct addrspace {
#if OPT_DUMBVM
	vaddr_t as_vbase1;
	paddr_t as_pbase1;
	size_t as_npages1;
	vaddr_t as_vbase2;
	paddr_t as_pbase2;
	size_t as_npages2;
	paddr_t as_stackpbase;
#else

	// Pointer to page directory, an array of pointers pointing to
	// different page table, saves a lot of memory as it does not need to allocate 1024
	// page tables at once
	struct page_table *page_directory[1024];
	struct vnode *v;

	/* Data & code segments*/
	vaddr_t as_vbase1;
	size_t as_npages1;
	u_int32_t as_permission1;

	vaddr_t as_vbase2;
	size_t as_npages2;
	u_int32_t as_permission2;

	/* Heap & stack region */
	vaddr_t heapvbase;
	vaddr_t heapvtop;

	vaddr_t stackvbase;

#endif
};

/*
 * Functions in addrspace.c:
 *
 *    as_create - create a new empty address space. You need to make
 *                sure this gets called in all the right places. You
 *                may find you want to change the argument list. May
 *                return NULL on out-of-memory error.
 *
 *    as_copy   - create a new address space that is an exact copy of
 *                an old one. Probably calls as_create to get a new
 *                empty address space and fill it in, but that's up to
 *                you.
 *
 *    as_activate - make the specified address space the one currently
 *                "seen" by the processor. Argument might be NULL,
 *		  meaning "no particular address space".
 *
 *    as_destroy - dispose of an address space. You may need to change
 *                the way this works if implementing user-level threads.
 *
 *    as_define_region - set up a region of memory within the address
 *                space.
 *
 *    as_prepare_load - this is called before actually loading from an
 *                executable into the address space.
 *
 *    as_complete_load - this is called when loading from an executable
 *                is complete.
 *
 *    as_define_stack - set up the stack region in the address space.
 *                (Normally called *after* as_complete_load().) Hands
 *                back the initial stack pointer for the new process.
 */

struct addrspace *as_create(void);
int               as_copy(struct addrspace *src, struct addrspace **ret);
void              as_activate(struct addrspace *);
void              as_destroy(struct addrspace *);

int               as_define_region(struct addrspace *as,
				   vaddr_t vaddr, size_t sz,
				   int readable,
				   int writeable,
				   int executable);
int		  as_prepare_load(struct addrspace *as);
int		  as_complete_load(struct addrspace *as);
int               as_define_stack(struct addrspace *as, vaddr_t *initstackptr);

vaddr_t vaddr_join(u_int32_t pdir_index, u_int32_t ptable_index);
int as_get_permission(struct addrspace *as, vaddr_t vaddr);

/*
 * Functions in loadelf.c
 *    load_elf - load an ELF user program executable into the current
 *               address space. Returns the entry point (initial PC)
 *               in the space pointed to by ENTRYPOINT.
 */

int load_elf(struct vnode *v, vaddr_t *entrypoint);



#endif /* _ADDRSPACE_H_ */
