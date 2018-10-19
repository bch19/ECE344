/*
 * catsem.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use SEMAPHORES to solve the cat syncronization problem in 
 * this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>


/*
 * 
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2

static struct semaphore *tsem; 	// hang main thread until children finish
struct semaphore *mainQueue; 	// lets thread that approaches bowl first go first, even if it has to wait for cat to leave
struct semaphore *bowlQueue; 	// locks for the two bowls
struct semaphore *catMutex;	// lock for catCount
struct semaphore *mouseMutex;	// lock for mouseCount
struct semaphore *bowlAccess; 	// stop data race when picking which of the bowls to go to
volatile int catCount;		// how many cats at the bowls (0-2)
volatile int mouseCount;	// how many mice at the bowls (0-2)
volatile int bowlOne;		// 0 means bowlOne is unoccupied
volatile int bowlTwo;		// do. for bowlTwo


/*
 * 
 * Function Definitions
 * 
 */

/* who should be "cat" or "mouse" */
static void
sem_eat(const char *who, int num, int bowl, int iteration)
{
        kprintf("%s: %d starts eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
        clocksleep(1);
        kprintf("%s: %d ends eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
}

/*
 * catsem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
catsem(void * unusedpointer, 
       unsigned long catnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;
        (void) catnumber;

	// variables
	int bowl; 	// bowl I'm eating from
	int iteration = 0;	// which round of eating I'm on
	
	while (iteration < 4)
	{	
		P(mainQueue);	// getting into main line; lets cat wait in line while a mouse is eating and not get starved
		P(bowlQueue);	// get one of two bowl spots
		P(catMutex);	// block access to catCount
		catCount++;
		if (catCount == 1)
		{
			P(mouseMutex);	// no mouse while a cat is in
		}
		V(catMutex);
		V1(mainQueue);

		// critical region of bowlQueue
	
		// critical region within critical region to pick the bowl
		P(bowlAccess);
		if (bowlOne == 0)
		{
			bowlOne = 1;
			bowl = 1;
		}

		else
		{
			bowlTwo = 1;
			bowl = 2;
		}
		V(bowlAccess);
		
		sem_eat("cat", catnumber,bowl, iteration);
		
		
		// release bowl		
		P(bowlAccess);
		if (bowl == 1)
		{
			bowlOne = 0;			
		}

		else
		{
			bowlTwo = 0;
		}
		V(bowlAccess);
		
		// decrement cat count
		P(catMutex);
		catCount--;
		if (catCount == 0)   // you are the last cat eating
		{				  
			V(mouseMutex);  // let the mouse eat
		}	
		V(catMutex);

		iteration ++;
		// done critical region

		V1(bowlQueue);		
	

	} // end of while

	V(tsem); // let the main thread know we're done
}
        

/*
 * mousesem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
mousesem(void * unusedpointer, 
         unsigned long mousenumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;
        (void) mousenumber;

	// variableS
	int bowl; 	// bowl I'm eating from
	int iteration = 0;	// which round of eating
	
	while (iteration < 4)
	{	
		P(mainQueue);	// getting into main line; lets mouse wait in line for will cat finishes eating and not get starved
		P(bowlQueue);	// get one of two bowl spots
		P(mouseMutex);	// block access to catCount
		mouseCount++;
		if (mouseCount == 1)
		{
			P(catMutex);	// no cat while a mouse is in
		}
		V(mouseMutex);
		V1(mainQueue);

		// critical region of bowlQueue
	
		// critical region within critical region to pick the bowl
		P(bowlAccess);
		if (bowlOne == 0)
		{
			bowlOne = 1;
			bowl = 1;
		}

		else
		{
			bowlTwo = 1;
			bowl = 2;
		}
		V(bowlAccess);
		
		sem_eat("mouse", mousenumber,bowl, iteration);
		
		
		// release bowl		
		P(bowlAccess);
		if (bowl == 1)
		{
			bowlOne = 0;			
		}

		else
		{
			bowlTwo = 0;
		}
		V(bowlAccess);
		
		// decrement mouse count
		P(mouseMutex);
		mouseCount--;
		if (mouseCount == 0){  		// you are the last cat eating
			V(catMutex);	// let the mouse eat
		}
		V(mouseMutex);

		iteration ++;
		// done critical region

		V1(bowlQueue);		
	

	} // end of while
	
	V(tsem); // let the main thread know we're done
}


/*
 * catmousesem()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catsem() and mousesem() threads.  Change this 
 *      code as necessary for your solution.
 */

int
catmousesem(int nargs,
            char ** args)
{
        int index, error;
   
        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;
   
	// initialize variables
	catCount = 0;
	mouseCount = 0;
	bowlOne = 0;
	bowlTwo = 0;

	// initialize sems
	tsem = sem_create("tsem", 0);
	mainQueue = sem_create("mainQueue", 1);
	bowlQueue = sem_create("bowlQueue", 2);
	catMutex = sem_create("catMutex", 1);
	mouseMutex = sem_create("mouseMutex", 1);
	bowlAccess = sem_create("bowlAccess", 1);


        /*
         * Start NCATS catsem() threads.
         */

	

	

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catsem Thread", 
                                    NULL, 
                                    index, 
                                    catsem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catsem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
        
        /*
         * Start NMICE mousesem() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mousesem Thread", 
                                    NULL, 
                                    index, 
                                    mousesem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mousesem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

	// wait for all cats and all mice to finish
	
	for (index=0; index<NCATS; index++) {
		P(tsem);
	}
	
	for (index=0; index<NMICE; index++) {
		P(tsem);
	}

	// cleaning up like a good citizen
	
	sem_destroy(tsem);
	sem_destroy(mainQueue);
	sem_destroy(bowlQueue);
	sem_destroy(catMutex);
	sem_destroy(mouseMutex);
	sem_destroy(bowlAccess);

       
       	return 0;
}


/*
 * End of catsem.c
 */
