#include <types.h>
#include <kern/errno.h>
#include <kern/errno.h>
#include <clock.h>
#include <lib.h>
#include <syscall.h>
#include <addrspace.h>
#include <vm.h>


void*
sys_sbrk(intptr_t amount, int* errno)
{

	struct addrspace *as = curthread->t_vmspace;


	/* Amount has to be page-aligned */
	if((amount%PAGE_SIZE) != 0){
		*errno = EINVAL;
		return (void*) -1;
	}

	/* Amount + heapvtop will go below heapvbase */
	if ((amount + (as->heapvtop)) < as->heapvbase){
		*errno = EINVAL;
		return (void*) -1;
	}

	/* Amount + heapvtop will go over max heap size */
	if ((amount+ as->heapvtop) > (as->heapvbase + HEAP_MAXPAGE* PAGE_SIZE)){
		*errno = EINVAL;
		return (void*)-1;
	}


	int i, pdir_index, ptable_index, curt_pte;
	vaddr_t old_heapvtop = as->heapvtop;

	int page_required = amount/ PAGE_SIZE;

	/* Update pages_required amount of PTE */
	for(i = 0 ; i < pages_required; i++){

		/* Increment heap_vaddr by PAGE_SIZE */
		vaddr_t heap_vaddr = as->heapvtop + i* PAGE_SIZE;
		pdir_index = GET_PDIR(heap_vaddr);
		ptable_index = GET_PTBL(heap_vaddr);

		/* page table of the vaddr hasnt been initialized yet */
		if (as->page_directory[pdir_index] == NULL){
			struct page_table *ptable = kmalloc(sizeof(struct page_table));
			as->page_directory[pdir_index] = ptable;
		}

		/* Set up PTE, valid bit should be off, protection bits should be R/W */
		curt_pte = 0x0;
		curt_pte = SET_RDABLE(curt_pte);
		curt_pte = SET_WTABLE(curt_pte);

		as->page_directory[pdir_index]->PTE[ptable_index] = curt_pte;
	}

	as->heapvtop += pages_required * PAGE_SIZE;


	return (void*)old_heapvtop;
}
