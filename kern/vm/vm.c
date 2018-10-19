#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <addrspace.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <kern/types.h>
#include <synch.h>

// toggle debug prints
//#define VM_DEBUG 1


// TODO: using dumbvm still; replace with proper
// TODO: test tlb replacement to see if it works

// dumb
#define DUMBVM_STACKPAGES    12

// debugging function
// print tlb entries
void printtlb(void)
{
	u_int32_t ehi, elo, i;
	char vld[10];	
	for (i=0; i<NUM_TLB; i++)
	{

		TLB_Read(&ehi, &elo, i);
		if( (elo & TLBLO_VALID)==0)
		{
			strcpy(vld,"invalid");
		}
		else
		{
			strcpy(vld,"valid");
		}
		kprintf ("%2d: %8x %8x %9s\n",i,ehi,elo,vld);

	}

}
void
vm_bootstrap(void)
{
	nextEvict = 0;
	coremap_lock = lock_create("coremap lock");
}


// simple tlb replacement policy
static int robinEvict(void)
{
	if (nextEvict==NUM_TLB)
	{
		nextEvict = 0;
	}	

	return nextEvict++;

}
// tells us which tlb entry to evict next
// returns and int from 0 to 63
static int evict(void)
{
	return robinEvict();
}



// Not used
/*
static
paddr_t
getppages(unsigned long npages)
{
	int spl;
	paddr_t addr;
	spl = splhigh();

	addr = ram_stealmem(npages);

	splx(spl);
	return addr;
}
*/



//TODO: need to place in main.c 
// ppage_start should be different after implement swapping
// should consider how swap implementation change ppage range & start
void
coremap_bootstrap(void){

	// firstaddr & lastaddr to calculate page # we get
	// corebase -> first actual paddr for free pages
	paddr_t firstaddr, lastaddr;
	int ppagenumber, i;

	lastaddr = mips_ramsize();
	firstaddr = ram_stealmem(0);

	// since we need 1 page size for coremap itself
	ppagenumber = (lastaddr - firstaddr - PAGE_SIZE)/ PAGE_SIZE;
	coremap_size = ppagenumber;

	// Initalize coremap by using ram_stealmem since kmalloc isnt available yet
	// Find the actual start of the physical memory
	coremap = PADDR_TO_KVADDR(ram_stealmem(1));
	corebase = ram_stealmem(0);
		

	// Initialize page state and paddr for each page
	for(i = 0 ; i < ppagenumber; i++){
		coremap[i].pstate = PFREE;
		coremap[i].paddr = i * PAGE_SIZE;
	}

}



/* 
 * Main Function. Used to fetch A single free ppage for USER from the coremap 
 * This should be called in for loop if you want to allocate more than 1 page
 * Also sets pid and vaddr in the coremap_entry for mapping
 */
// TODO: change accordingly after swapping, add pid to support multi processes

paddr_t
get_ppage(vaddr_t vaddr){
	
	//assert(_pid > 0);	
	assert(vaddr % PAGE_FRAME == 0);

	int spl, i ;
	paddr_t paddr;
	spl= splhigh();

	// Need to be removed after implement swapping
	int found = 0;
	
	// Loop through the coremap and finds a free page
	for( i=0 ; i < coremap_size ; i++){
		
		/* If you find a free page, set its pid and vaddr 
		 * to map this page.
		 */

		if(coremap[i].pstate == PFREE){
			found = 1;
			//coremap[i].pid = _pid;
			coremap[i].block_size = 1;
			coremap[i].vaddr = vaddr;
			coremap[i].pstate = PCLEAN;
			
			paddr = coremap[i].paddr;
			splx(spl);
			return paddr;	
		}
	}
	
	// if there is no free page, return 0 
	// TODO: this should not be allowed if we implement swap
	if (!found){
		splx(spl);
		return 0;
	}	
	return 0;
}

/* 
 * Main function to free a user page when calling as_destroy
 * Should only be called in as_destroy
 */
void
free_ppage(vaddr_t va, u_int32_t pageframenumber){
	
	int spl;
	spl= splhigh();
	
	// Make sure that the page state is user ( either clean or dirty)
	// TODO: may need to do something 
	assert(coremap[pageframenumber].pstate != PKERNEL || coremap[pageframenumber].pstate != PFREE);
	assert(coremap[pageframenumber].vaddr == va);

	coremap[pageframenumber].pstate = PFREE;
	//coremap[pageframenumber].pid = 0;
	coremap[pageframenumber].vaddr = 0;
	
	splx(spl);	
}

/* given paddr return the pointer to specific coremap_entry */
struct coremap_entry*
get_coremapentry( paddr_t paddr){

	/* make sure paddr is page aligned */
	paddr &= paddr;
	
	assert(paddr % PAGE_SIZE == 0);

	struct coremap_entry* cmptr;
	int coremap_index = paddr / PAGE_SIZE;
	
	cmptr = coremap[coremap_index];

	return cmptr;
}


// TODO: change this when doing swapping
/* Allocate/free some kernel-space virtual pages, only called by kmalloc */
// Need to implement synchronization here

vaddr_t
alloc_kpages(int npages)
{
	int spl;
	int count, index, offset;
	paddr_t paddr;

	spl = splhigh();

	// need 1 or n contiguous pages free
	int i;
	for (i = 0 ; i < coremap_size ; i++){
		if (coremap[i].pstate == PFREE){
			count ++;

			for ( offset = 1 ; offset < npages; offset ++){
				// if another free page, increment count
				if (coremap[i+offset].pstate == PFREE){
					count++;
				}

				// Restart count again
				else{
					count = 0;
					break;
				}
			}
		}
	
		// number of pages required found
		if (count == npages){
			index = i;
			break;
		}
	}

	// No free n pages found, return 0 for error
	if (count < npages){
		//lock_release(coremap_lock);
		splx(spl);		
		return 0;
	}

	// Only first page can contain chunk size
	coremap[index].block_size = npages;

	// Set the rest
	for (i = 0 ; i< npages; i++){
		coremap[index+i].pstate = PKERNEL;
		coremap[index+i].vaddr = PADDR_TO_KVADDR((i+index)*PAGE_SIZE);
		if (i != 0)		
			coremap[index+i].block_size = 0;
	}
	
	paddr = coremap[index].paddr;
	//lock_release(coremap_lock);
	
	splx(spl);
	return PADDR_TO_KVADDR(paddr);
}



/* Kernel free pages function, only called by kfree */
void
free_kpages(vaddr_t addr)
{
	int pageIndex, npages, i;
	int spl = splhigh();

	// Convert vaddr to paddr
	paddr_t paddr = KVADDR_TO_PADDR(addr);

	// Check if page aligned and compute the physical page index
	assert((paddr % PAGE_SIZE) == 0);
	pageIndex = paddr/ PAGE_SIZE;

	// Only the first ppage of the block has block_size info
	assert(coremap[pageIndex].block_size != 0);

	//lock_acquire(coremap_lock);

	// Since all these pages are kernel pages, it is already to be contiguous
	// TODO: maybe add the PID?
	for (i = 0 ; i < npages; i++){

		coremap[i+pageIndex].vaddr = 0x0;
		coremap[i+pageIndex].block_size = 0;
		coremap[i+pageIndex].pstate = PFREE;
	}

	//lock_release(coremap_lock);
	splx(spl);
}


// TODO: dumb
int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
	u_int32_t ehi, elo;
	struct addrspace *as;
	int spl;

	spl = splhigh();
	
	// getting the virtual page number
	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		panic("dumbvm: got VM_FAULT_READONLY\n");
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		splx(spl);
		return EINVAL;
	}

	as = curthread->t_vmspace;
	if (as == NULL) {
		/*
		 * No address space set up. This is probably a kernel
		 * fault early in boot. Return EFAULT so as to panic
		 * instead of getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	assert(as->as_vbase1 != 0);
	assert(as->as_pbase1 != 0);
	assert(as->as_npages1 != 0);
	assert(as->as_vbase2 != 0);
	assert(as->as_pbase2 != 0);
	assert(as->as_npages2 != 0);
	assert(as->as_stackpbase != 0);
	assert((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	assert((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	assert((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	assert((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	assert((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	// find out which region of virtual address space
	// the fault address is and convert it to physical address
	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		paddr = (faultaddress - vbase1) + as->as_pbase1;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		paddr = (faultaddress - vbase2) + as->as_pbase2;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		paddr = (faultaddress - stackbase) + as->as_stackpbase;
	}
	else {
		splx(spl);
		return EFAULT;
	}

	/* make sure it's page-aligned */
	assert((paddr & PAGE_FRAME)==paddr);

	// put requested page into TLB
	// TODO: simple: make better
	
	// pick the TLB entry to evict
	i = evict();	// for now does a roundrobin replace
	assert (i >=0);
	assert (i < NUM_TLB);

	// enter the missing page entry
	ehi = faultaddress;		// this is actually the virtual page number
	elo = paddr | TLBLO_DIRTY | TLBLO_VALID;	// DUMB: dirty bit always on?
	DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);

// for debugging	
#ifdef VM_DEBUG
	kprintf ("Current Thread: %x",curthread);
	kprintf ("Writing to TLB%2d: %8x %8x\n",i,ehi,elo);

#endif

	TLB_Write(ehi, elo, i);
	
// for debugging	
#ifdef VM_DEBUG
	TLB_Read(&ehi, &elo, i);
	kprintf ("Read from TLB: %2d: %8x %8x\n",i,ehi,elo);
	printtlb();
#endif
	splx(spl);

	return 0;
/* DUMB
	// check for an invalid (i.e. empty) tlb slot
	// if can't find one than just self-destruct 
	for (i=0; i<NUM_TLB; i++) {
		TLB_Read(&ehi, &elo, i);
		// check if valid
		if (elo & TLBLO_VALID) {
			continue;
		}

		// found an empty slot in TLB
		// enter the missing page entry
		ehi = faultaddress;	// this is actually the page number
		// DUMB: dirty bit always on?
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		TLB_Write(ehi, elo, i);
		splx(spl);
		return 0;
	}

	kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	splx(spl);
	return EFAULT;
*/	

}

