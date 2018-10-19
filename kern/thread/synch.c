/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *namearg, int initial_count)
{
	struct semaphore *sem;

	assert(initial_count >= 0);

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
	splx(spl);

	/*
	 * Note: while someone could theoretically start sleeping on
	 * the semaphore after the above test but before we free it,
	 * if they're going to do that, they can just as easily wait
	 * a bit and start sleeping on the semaphore after it's been
	 * freed. Consequently, there's not a whole lot of point in 
	 * including the kfrees in the splhigh block, so we don't.
	 */

	kfree(sem->name);
	kfree(sem);
}

void 
P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void
V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}

// like V but uses a FIFO queue
void
V1(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeone(sem);
	splx(spl);
}
////////////////////////////////////////////////////////////
//
// Lock.

const int IS_HELD = 1;
const int  NO_HELD = 0;

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name);
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}
	
	// add stuff here as needed
	// initialize lock to be "free"
	lock->held = NO_HELD;
	lock->holding_thread = NULL;


	// initialize lock queue
	//q_create(32);  //locktest only have 32 threads, can expand using preallocate

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	// add stuff here as needed
	
	kfree(lock->name);
	kfree(lock->holding_thread);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	// Write this

	assert(lock != NULL);
	
	// disable interrupts
	int s = splhigh();
	
	while (lock->held == IS_HELD)
	{	
		thread_sleep (lock);
	}	

	lock->held = IS_HELD;
	lock->holding_thread = curthread;

	
	
	// re-enable interrupts
	splx( s );

}

void
lock_release(struct lock *lock)
{
	// Write this
	//(void) lock;

	assert(lock != NULL);
	
	// disable interrupts
	int s = splhigh();


	lock->held = NO_HELD;	
	lock->holding_thread = NULL;

	// wake up waiting threads 
	thread_wakeone (lock);

	// re-enable interrupts if it was enabled before we changed it; else it stays the same
	splx( s );
}

int
lock_do_i_hold(struct lock *lock)
{
	// Write this
	if (lock->holding_thread == NULL)
		return 0;
	if (lock->holding_thread == curthread)
		return 1;
	
	return 0;
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}
	
	// add stuff here as needed
	
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	// add stuff here as needed
	
	kfree(cv->name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	// Write this
	// (void)cv;    // suppress warning until code gets written
	// (void)lock;  // suppress warning until code gets written
	
	assert(lock != NULL);
	assert(cv != NULL);
	
	// disable interrupts
	int s = splhigh();


	// release lock
	lock_release (lock);

	
	// go to sleep
	thread_sleep (cv);
	
	// acquire lock
	lock_acquire (lock);

	// re-enable interrupts
	splx (s);
	
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	assert(lock != NULL);
	assert(cv != NULL);
	
	// disable interrupts
	int s = splhigh();
	
	//lock_release(cv);

	// wake up one thread from cv
	thread_wakeone (cv);

	// re-enable interrupts
	splx (s);
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	// Write this
	//(void)cv;    // suppress warning until code gets written
	//(void)lock;  // suppress warning until code gets written

	//disable interrupt
	int s = splhigh();

	//lock_release(lock);

	// wake all threads in cv
	thread_wakeup(cv);


	// re-enable interrupt
	splx(s);

}
