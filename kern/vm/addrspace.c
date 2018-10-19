#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <kern/types.h>



vaddr_t
vaddr_join(u_int32_t pdir_index, u_int32_t ptable_index){

	vaddr_t vaddr;

	pdir_index = pdir_index << 22;  //shift by 22 bits
	ptable_index = ptable_index << 12; // shift by 12 bits

	vaddr = pdir_index | ptable_index;

	assert(vaddr % PAGE_SIZE == 0);

	return vaddr;
}



struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}


	as->as_vbase1 = 0;
	as->as_npages1 = 0;
	as->as_permission1 = 0x0;

	as->as_vbase2 = 0;
	as->as_npages2 = 0;
	as->as_permission2 = 0x0;

	as->heapvbase = 0;
	as->heapvtop = 0;

	as->stackvbase = 0;

	return as;
}



// Since we call as_copy in sys_fork rather than md_forkentry, cant use curthread-> pid
// to pass and set the page. so in sys_fork we pass the new pid in as_copy
// can be left for now
int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	assert(old != NULL);


	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	/* Copy the data & code segment info */
	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_permission2 = old->as_permission2;

	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;
	new->as_permission2 = old->as_permission2;

	/* Copy stack & heap addresses */
	new->heapvbase = old->heapvbase;
	new->heapvtop = old->heapvtop;
	new->stackvbase = old->stackvbase;


	/* Copy everything in old as's page directory & table */
	u_int32_t i, j;

	// Loop through page directory, if ptr isnt null, kmalloc a page table for new
	for (i = 0 ; i < PDE_MAX ; i++){

		// If the old page directory pointer points to a page table
		if (old-> page_directory[i] != NULL){

			// allocate a page table for new page directory
			struct page_table *ptable = kmalloc(sizeof(struct page_table));
			assert(ptable != NULL);
			new->page_directory[i]= ptable;

			// Loop through old ptable, if a pte exists, allocate a page for new as
			// and copy everything from old page -> new page
			for (j = 0 ; j < PTE_MAX; j++){

				if (old-> page_directory[i]-> PTE[j] != 0){

					/*
					 *  1. first allocate a user page
					 *  2. find out the PFN of the page & copy other bits
					 *     to form a PTE for new as. Insert in its ptable
					 *  3. in order to move the old page content to new,
					 *     find out the oldpage's & newpage's pa, convert
					 *     to kvaddr in order to use memmove
					 */
					// TODO: what about the old's pages not in phy memory?
					int new_PTE, new_PFN;
					vaddr_t new_va, old_va;
					paddr_t new_pa, old_pa;

					// Find the vaddr for this PTE
					// & allocate a page for new
					new_va = vaddr_join(i , j);
					new_pa = get_ppage(new_va);
					assert(new_pa != 0 );

					// Update the page table entry in new
					new_PTE = old->page_directory[i]->PTE[j];
					new_PTE = new_PTE & 0xFFFFFC00; // mask the PFN in old
					new_PFN = new_pa / PAGE_SIZE; // find the PFN for new
					new_PTE = new_PTE | new_PFN;  // create the new PTE
					new->page_directory[i]->PTE[j] = new_PTE; // insert

					// Find the paddr of the old's page
					old_pa = (old->page_directory[i]->PTE[j]) & PFN_MASK;
					old_pa = old_pa * PAGE_SIZE;

					// Convert them to kernel vaddr to use memmove
					// for new_va, we already set the PTE, now need to 						// convert to kernel vaddr for memmove
					new_va = PADDR_TO_KVADDR(new_pa);
					old_va = PADDR_TO_KVADDR(old_pa);

					memmove((void*)new_va, (const void*)old_va, PAGE_SIZE);

				}
				// If no PTE exists, then set new's PTE = 0x0
				else{
					new->page_directory[i]->PTE[j] = 0;
				}
			}
		}
		// If no page table exists, set the new page directory pointer to NULL
		else{
			new->page_directory[i] = NULL;
		}

	}

	*ret = new;
	return 0;
}



// TODO: need to change for swapping
void
as_destroy(struct addrspace *as)
{
	u_int32_t i, j;

	// Loop through the page directory
	for ( i=0; i < PDE_MAX ; i++){

		// If there exists a page table
		if ( as->page_directory[i] != NULL){
			// Loop through that page table
			for ( j = 0 ; j < PTE_MAX; j++){

				// If the PTE exists
				if(as->page_directory[i]->PTE[j] != 0){

					// get the vaddr of that PTE
					vaddr_t vaddr = vaddr_join(i,j);
					// get the PFN in that PTE
					u_int32_t pfn = (as->page_directory[i]->PTE[j]) & PFN_MASK;

					// free the pages on the coremap
					// TODO: what if that vaddr is not mapped to any ppage?
					free_ppage(vaddr, pfn);
				}
			}
			// Free the page table after freeing PTE
			kfree(as->page_directory[i]);
		}
	}

	kfree(as);
}


// Just use dumbvm version
void
as_activate(struct addrspace *as)
{
	int i, spl;

	(void)as;

	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}



/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */


int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages;

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	/* Number of pages we need to allocate for this region */
	npages = sz / PAGE_SIZE;


	/* Set up address space either code / data depending on permission */
	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
		as->as_permission1 = readable | writeable | executable;

		// make sure addr is page aligned
		assert(as->as_vbase1 % PAGE_SIZE == 0);
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
		as->as_permission2 = readable | writeable | executable;

		// make sure addr is page aligned
		assert(as->as_vbase2 % PAGE_SIZE == 0);
	}

	/* Set up heap only after data and code region is set up */
	if (as->heapvbase == 0 && as->as_vbase1 != 0 && as->as_vbase2 != 0 ){

		vaddr_t data_base;
		size_t data_npages;

		/* Decide which region 1/2 has higher address */
		if ( as->as_vbase1 > as->as_vbase2 ){
			data_base = as->as_vbase1;
			data_npages = as->as_npages1;
		}
		else{
			data_base = as->as_vbase2;
			data_npages = as->as_npages2;
		}

		/* heap should start right after data segment end */
		as->heapvbase = (data_npages * PAGE_SIZE) + data_base;	
		as->heapvtop = as->heapvbase;

		/* Make sure heap is page aligned */
		assert(as->heapvbase % PAGE_SIZE == 0);
		// TODO: should we make sure that it is larger or equal to end of data seg?
	}

	return 0;
}

// We dont really need this since we implement demand paging
/*
int
as_prepare_load(struct addrspace *as)
{
	assert(as->as_pbase1 == 0);
	assert(as->as_pbase2 == 0);
	assert(as->as_stackpbase == 0);

	as->as_pbase1 = getppages(as->as_npages1);
	if (as->as_pbase1 == 0) {
		return ENOMEM;
	}

	as->as_pbase2 = getppages(as->as_npages2);
	if (as->as_pbase2 == 0) {
		return ENOMEM;
	}

	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
	if (as->as_stackpbase == 0) {
		return ENOMEM;
	}

	return 0;
}
*/

// dont really need this since we implement demand paging
int
as_complete_load(struct addrspace *as)
{
	(void)as;
	return 0;
}




int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	assert(as->stackvbase != 0);

	/* Max stack page is 256, but we can only allocate one page space for now */
	as->stackvbase = USERSTACK - PAGE_SIZE;
	as->stackvbase &= PAGE_SIZE;

	/* Make sure stack base is page aligned */
	assert(as->stackvbase % PAGE_SIZE == 0);


	/* User stack pointer */
	*stackptr = USERSTACK;
	return 0;
}


int
as_get_permission(struct addrspace *as, vaddr_t vaddr){

	vaddr_t region1top = as->as_vbase1 + (as->as_npages1*PAGE_SIZE);
	vaddr_t region2top = as->as_vbase2 + (as->as_npages2*PAGE_SIZE);

	// If vaddr in region 1, return region 1 permission
	if ( vaddr < region1top && vaddr >= as->as_vbase1){
		return as->as_permission1;
	}

	// else region 2 permission
	else if (vaddr < region2top && vaddr >= as->as_vbase2){
		return as->as_permission2;
	}

	// must be heap or stack (R/W) (0b110)
	else
		return 0x6;
}


int *
find_pte (pdir_index,
