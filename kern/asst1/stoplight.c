/* 
 * stoplight.c
 *
 * 31-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: You can use any synchronization primitives available to solve
 * the stoplight problem in this file.
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
#include <curthread.h>


/*
 *
 * Constants
 *
 */

/*
 * Number of cars created.
 */

#define NCARS 20

// variables

int carCount;
char *greenLight; // holds direction facing green light

// semaphores

static struct semaphore *tsem; 	// hang main thread until children finish

// locks

struct lock *enterQueue; 	// keeps cars in order after they approach intersection
struct lock *countLock;		// lock for carCount
struct lock *nwLock;		// locks for each quarter of
struct lock *swLock;		// the intersection
struct lock *neLock;
struct lock *seLock;

// cv's

struct cv *clearCond;		// signaled when a car
				// leaving intersection
				// was the only one and 
				// so carCount = 0

 
/*
 *
 * Function Definitions
 *
 */

static const char *directions[] = { "N", "E", "S", "W" };

static const char *msgs[] = {
        "approaching:",
        "region1:    ",
        "region2:    ",
        "region3:    ",
        "leaving:    "
};

/* use these constants for the first parameter of message */
enum { APPROACHING, REGION1, REGION2, REGION3, LEAVING };

static void
message(int msg_nr, int carnumber, int cardirection, int destdirection)
{
        kprintf("%s car = %2d, direction = %s, destination = %s\n",
                msgs[msg_nr], carnumber,
                directions[cardirection], directions[destdirection]);
}
 
/*
 * gostraight()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement passing straight through the
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
gostraight(unsigned long cardirection,
           unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */
	
        (void) carnumber;

	int destdirection;


// ********************* North
	if (cardirection == 0) // N
	{
		destdirection = 2; // S
		
		message(APPROACHING, carnumber, cardirection, destdirection);
	
		// enter queue for the intersection

		lock_acquire (enterQueue); 
		lock_acquire (countLock);

		

		// wait if the intersection has cars in it and it's not your light
		while (carCount != 0 && strcmp(greenLight,directions[cardirection])!=0)
			cv_wait(clearCond, countLock);

		// in the intersection
		carCount++;

		// if no other car is here, change the light to what we want		
		if (carCount == 1)
			greenLight = directions[cardirection];

		lock_release (countLock);


		lock_acquire (nwLock);
		lock_release (enterQueue);	

		// in region 1 

		message(REGION1, carnumber, cardirection, destdirection);

		lock_acquire (swLock);
		

		// in region 2
	
		message(REGION2, carnumber, cardirection, destdirection);
		
		lock_release (nwLock);

		// out of intersection

		message (LEAVING, carnumber, cardirection, destdirection);

		lock_release (swLock);

		// decrement the car count	
		lock_acquire (countLock);
		carCount--;

		// last car out wake up car waiting for a green light
		if (carCount == 0) 
			cv_signal(clearCond, countLock);
		lock_release (countLock);

	} // end of if North

// ********************* East
	
	else if (cardirection == 1) // E
	{
		destdirection = 3; // W
		
		message(APPROACHING, carnumber, cardirection, destdirection);
	
		// enter queue for the intersection

		lock_acquire (enterQueue); 
		lock_acquire (countLock);

		// wait if the intersection has cars in it and it's not your light
		while (carCount != 0 && strcmp(greenLight,directions[cardirection])!=0)
			cv_wait(clearCond, countLock);

		// in the intersection
		carCount++;

		// if no other car is here, change the light to what we want		
		if (carCount == 1)
			greenLight = directions[cardirection];

		lock_release (countLock);


		lock_acquire (neLock);
		lock_release (enterQueue);	

		// in region 1 

		message(REGION1, carnumber, cardirection, destdirection);

		lock_acquire (nwLock);
		

		// in region 2
	
		message(REGION2, carnumber, cardirection, destdirection);
		
		lock_release (neLock);

		// out of intersection

		message (LEAVING, carnumber, cardirection, destdirection);

		lock_release (nwLock);

		// decrement the car count	
		lock_acquire (countLock);
		carCount--;

		// last car out wake up car waiting for a green light
		if (carCount == 0) 
			cv_signal(clearCond, countLock);
		lock_release (countLock);
	} // end of if East

// ********************* South

	else if (cardirection == 2) // S
	{
		destdirection = 0; // N
		
		message(APPROACHING, carnumber, cardirection, destdirection);
	
		// enter queue for the intersection

		lock_acquire (enterQueue); 
		lock_acquire (countLock);

		// wait if the intersection has cars in it and it's not your light
		while (carCount != 0 && strcmp(greenLight,directions[cardirection])!=0)
			cv_wait(clearCond, countLock);

		// in the intersection
		carCount++;

		// if no other car is here, change the light to what we want		
		if (carCount == 1)
			greenLight = directions[cardirection];

		lock_release (countLock);


		lock_acquire (seLock);
		lock_release (enterQueue);	

		// in region 1 

		message(REGION1, carnumber, cardirection, destdirection);

		lock_acquire (neLock);
		

		// in region 2
	
		message(REGION2, carnumber, cardirection, destdirection);
		
		lock_release (seLock);

		// out of intersection

		message (LEAVING, carnumber, cardirection, destdirection);

		lock_release (neLock);

		// decrement the car count	
		lock_acquire (countLock);
		carCount--;

		// last car out wake up car waiting for a green light
		if (carCount == 0) 
			cv_signal(clearCond, countLock);
		lock_release (countLock);
	} // end of if South

// ********************* West

	// for car approaching from West
	else if (cardirection == 3) // W
	{
		destdirection = 1; // E
		
		message(APPROACHING, carnumber, cardirection, destdirection);
	
		// enter queue for the intersection

		lock_acquire (enterQueue); 
		lock_acquire (countLock);

		// wait if the intersection has cars in it and it's not your light
		while (carCount != 0 && strcmp(greenLight,directions[cardirection])!=0)
			cv_wait(clearCond, countLock);

		// in the intersection
		carCount++;

		// if no other car is here, change the light to what we want		
		if (carCount == 1)
			greenLight = directions[cardirection];

		lock_release (countLock);


		lock_acquire (swLock);
		lock_release (enterQueue);	

		// in region 1 

		message(REGION1, carnumber, cardirection, destdirection);

		lock_acquire (seLock);


		// in region 2
	
		message(REGION2, carnumber, cardirection, destdirection);

		lock_release (swLock);

		// out of intersection

		message (LEAVING, carnumber, cardirection, destdirection);

		lock_release (seLock);

		// decrement the car count	
		lock_acquire (countLock);
		carCount--;

		// last car out wake up car waiting for a green light
		if (carCount == 0) 
			cv_signal(clearCond, countLock);
		lock_release (countLock);
	} // end of if West

}


/*
 * turnleft()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a left turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnleft(unsigned long cardirection,
         unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) cardirection;
        (void) carnumber;
	
	int destdirection;


// ********************* North
	if (cardirection == 0) // N
	{
		destdirection = 1; // E
		
		message(APPROACHING, carnumber, cardirection, destdirection);
	
		// enter queue for the intersection

		lock_acquire (enterQueue); 
		lock_acquire (countLock);

		// wait if the intersection has cars in it and it's not your light
		while (carCount != 0 && strcmp(greenLight,directions[cardirection])!=0)
			cv_wait(clearCond, countLock);

		// in the intersection
		carCount++;

		// if no other car is here, change the light to what we want		
		if (carCount == 1)
			greenLight = directions[cardirection];

		lock_release (countLock);


		lock_acquire (nwLock);
		lock_release (enterQueue);	

		// in region 1 

		message(REGION1, carnumber, cardirection, destdirection);

		lock_acquire (swLock);


		// in region 2
	
		message(REGION2, carnumber, cardirection, destdirection);

		lock_release (nwLock);	
		lock_acquire (seLock);


		// in region 3

		message (REGION3, carnumber, cardirection, destdirection);
		
		lock_release (swLock);

		// out of intersection

		message (LEAVING, carnumber, cardirection, destdirection);
		
		lock_release (seLock);


		

		// decrement the car count	
		lock_acquire (countLock);
		carCount--;

		// last car out wake up car waiting for a green light
		if (carCount == 0) 
			cv_signal(clearCond, countLock);
		lock_release (countLock);

	} // end of if North

// ********************* East
	
	else if (cardirection == 1) // E
	{
		destdirection = 2; // S
		
		message(APPROACHING, carnumber, cardirection, destdirection);
	
		// enter queue for the intersection

		lock_acquire (enterQueue); 
		lock_acquire (countLock);

		// wait if the intersection has cars in it and it's not your light
		while (carCount != 0 && strcmp(greenLight,directions[cardirection])!=0)
			cv_wait(clearCond, countLock);

		// in the intersection
		carCount++;

		// if no other car is here, change the light to what we want		
		if (carCount == 1)
			greenLight = directions[cardirection];

		lock_release (countLock);


		lock_acquire (neLock);
		lock_release (enterQueue);	

		// in region 1 

		message(REGION1, carnumber, cardirection, destdirection);

		lock_acquire (nwLock);


		// in region 2
	
		message(REGION2, carnumber, cardirection, destdirection);
		
		lock_release (neLock);

		lock_acquire (swLock);


		// in region 3

		message (REGION3, carnumber, cardirection, destdirection);
		
		lock_release (nwLock);

		// exiting intersection
		
		message (LEAVING, carnumber, cardirection, destdirection);
		
		lock_release (swLock);

		// decrement the car count	
		lock_acquire (countLock);
		carCount--;

		// last car out wake up car waiting for a green light
		if (carCount == 0) 
			cv_signal(clearCond, countLock);
		lock_release (countLock);
	} // end of if East

// ********************* South

	else if (cardirection == 2) // S
	{
		destdirection = 3; // W
		
		message(APPROACHING, carnumber, cardirection, destdirection);
	
		// enter queue for the intersection

		lock_acquire (enterQueue); 
		lock_acquire (countLock);

		// wait if the intersection has cars in it and it's not your light
		while (carCount != 0 && strcmp(greenLight,directions[cardirection])!=0)
			cv_wait(clearCond, countLock);

		// in the intersection
		carCount++;

		// if no other car is here, change the light to what we want		
		if (carCount == 1)
			greenLight = directions[cardirection];

		lock_release (countLock);


		lock_acquire (seLock);
		lock_release (enterQueue);	

		// in region 1 

		message(REGION1, carnumber, cardirection, destdirection);

		lock_acquire (neLock);


		// in region 2
	
		message(REGION2, carnumber, cardirection, destdirection);

		lock_release (seLock);
		lock_acquire (nwLock);

		
		// in region 3

		message (REGION3, carnumber, cardirection, destdirection);
		
		lock_release (neLock);

		// exiting intersection
		
		message (LEAVING, carnumber, cardirection, destdirection);

		lock_release (nwLock);

		// decrement the car count	
		lock_acquire (countLock);
		carCount--;

		// last car out wake up car waiting for a green light
		if (carCount == 0) 
			cv_signal(clearCond, countLock);
		lock_release (countLock);
	} // end of if South

// ********************* West

	// for car approaching from West
	else if (cardirection == 3) // W
	{
		destdirection = 0; // N
		
		message(APPROACHING, carnumber, cardirection, destdirection);
	
		// enter queue for the intersection

		lock_acquire (enterQueue); 
		lock_acquire (countLock);

		// wait if the intersection has cars in it and it's not your light
		while (carCount != 0 && strcmp(greenLight,directions[cardirection])!=0)
			cv_wait(clearCond, countLock);

		// in the intersection
		carCount++;

		// if no other car is here, change the light to what we want		
		if (carCount == 1)
			greenLight = directions[cardirection];

		lock_release (countLock);


		lock_acquire (swLock);
		lock_release (enterQueue);	

		// in region 1 

		message(REGION1, carnumber, cardirection, destdirection);

		lock_acquire (seLock);


		// in region 2
	
		message(REGION2, carnumber, cardirection, destdirection);
		
		lock_release (swLock);
		lock_acquire (neLock);


		// in region 3

		message (REGION3, carnumber, cardirection, destdirection);
		
		lock_release (seLock);
		
		// exiting intersection
		
		message (LEAVING, carnumber, cardirection, destdirection);

		lock_release (neLock);

		// decrement the car count	
		lock_acquire (countLock);
		carCount--;

		// last car out wake up car waiting for a green light
		if (carCount == 0) 
			cv_signal(clearCond, countLock);
		lock_release (countLock);
	} // end of if West

}


/*
 * turnright()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a right turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnright(unsigned long cardirection,
          unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) cardirection;
        (void) carnumber;
	
	
	int destdirection;


// ********************* North
	if (cardirection == 0) // N
	{
		destdirection = 3; // W
		
		message(APPROACHING, carnumber, cardirection, destdirection);
	
		// enter queue for the intersection

		lock_acquire (enterQueue); 
		lock_acquire (countLock);

		// wait if the intersection has cars in it and it's not your light
		while (carCount != 0 && strcmp(greenLight,directions[cardirection])!=0)
			cv_wait(clearCond, countLock);

		// in the intersection
		carCount++;

		// if no other car is here, change the light to what we want		
		if (carCount == 1)
			greenLight = directions[cardirection];

		lock_release (countLock);


		lock_acquire (nwLock);
		lock_release (enterQueue);	

		// in region 1 

		message(REGION1, carnumber, cardirection, destdirection);

		// out of intersection

		message (LEAVING, carnumber, cardirection, destdirection);

		lock_release (nwLock);

		// decrement the car count	
		lock_acquire (countLock);
		carCount--;

		// last car out wake up car waiting for a green light
		if (carCount == 0) 
			cv_signal(clearCond, countLock);
		lock_release (countLock);

	} // end of if North

// ********************* East
	
	else if (cardirection == 1) // E
	{
		destdirection = 0; // N
		
		message(APPROACHING, carnumber, cardirection, destdirection);
	
		// enter queue for the intersection

		lock_acquire (enterQueue); 
		lock_acquire (countLock);

		// wait if the intersection has cars in it and it's not your light
		while (carCount != 0 && strcmp(greenLight,directions[cardirection])!=0)
			cv_wait(clearCond, countLock);

		// in the intersection
		carCount++;

		// if no other car is here, change the light to what we want		
		if (carCount == 1)
			greenLight = directions[cardirection];

		lock_release (countLock);


		lock_acquire (neLock);
		lock_release (enterQueue);	

		// in region 1 

		message(REGION1, carnumber, cardirection, destdirection);

		// out of intersection

		message (LEAVING, carnumber, cardirection, destdirection);

		lock_release (neLock);

		// decrement the car count	
		lock_acquire (countLock);
		carCount--;

		// last car out wake up car waiting for a green light
		if (carCount == 0) 
			cv_signal(clearCond, countLock);
		lock_release (countLock);
	} // end of if East

// ********************* South

	else if (cardirection == 2) // S
	{
		destdirection = 1; // E
		
		message(APPROACHING, carnumber, cardirection, destdirection);
	
		// enter queue for the intersection

		lock_acquire (enterQueue); 
		lock_acquire (countLock);

		// wait if the intersection has cars in it and it's not your light
		while (carCount != 0 && strcmp(greenLight,directions[cardirection])!=0)
			cv_wait(clearCond, countLock);

		// in the intersection
		carCount++;

		// if no other car is here, change the light to what we want		
		if (carCount == 1)
			greenLight = directions[cardirection];

		lock_release (countLock);


		lock_acquire (seLock);
		lock_release (enterQueue);	

		// in region 1 

		message(REGION1, carnumber, cardirection, destdirection);

		// out of intersection

		message (LEAVING, carnumber, cardirection, destdirection);

		lock_release (seLock);

		// decrement the car count	
		lock_acquire (countLock);
		carCount--;

		// last car out wake up car waiting for a green light
		if (carCount == 0) 
			cv_signal(clearCond, countLock);
		lock_release (countLock);
	} // end of if South

// ********************* West

	// for car approaching from West
	else if (cardirection == 3) // W
	{
		destdirection = 2; // S
		
		message(APPROACHING, carnumber, cardirection, destdirection);
	
		// enter queue for the intersection

		lock_acquire (enterQueue); 
		lock_acquire (countLock);

		// wait if the intersection has cars in it and it's not your light
		while (carCount != 0 && strcmp(greenLight,directions[cardirection])!=0)
			cv_wait(clearCond, countLock);

		// in the intersection
		carCount++;

		// if no other car is here, change the light to what we want		
		if (carCount == 1)
			greenLight = directions[cardirection];

		lock_release (countLock);


		lock_acquire (swLock);
		lock_release (enterQueue);	

		// in region 1 

		message(REGION1, carnumber, cardirection, destdirection);

		// out of intersection

		message (LEAVING, carnumber, cardirection, destdirection);

		lock_release (swLock);

		// decrement the car count	
		lock_acquire (countLock);
		carCount--;

		// last car out wake up car waiting for a green light
		if (carCount == 0) 
			cv_signal(clearCond, countLock);
		lock_release (countLock);
	} // end of if West
}


/*
 * approachintersection()
 *
 * Arguments: 
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Change this function as necessary to implement your solution. These
 *      threads are created by createcars().  Each one must choose a direction
 *      randomly, approach the intersection, choose a turn randomly, and then
 *      complete that turn.  The code to choose a direction randomly is
 *      provided, the rest is left to you to implement.  Making a turn
 *      or going straight should be done by calling one of the functions
 *      above.
 */
 
static
void
approachintersection(void * unusedpointer,
                     unsigned long carnumber)
{
        int cardirection;
	int turndirection;

        /*
         * Avoid unused variable and function warnings.
         */

        (void) unusedpointer;
        (void) carnumber;
	(void) gostraight;
	(void) turnleft;
	(void) turnright;

        /*
         * cardirection is set randomly.
         */

	// debug message
	//kprintf ("Car #%d is running on %p.\n",carnumber,curthread);	

        cardirection = random() % 4;

	// randomly set turn
        turndirection = random() % 3;
	
	if (turndirection == 0)
		gostraight(cardirection, carnumber);
	
	else if (turndirection == 1)
		turnright(cardirection, carnumber);
	
	else if (turndirection == 2)
		turnleft(cardirection, carnumber);

	// tell the main thread we are done
	
	V(tsem);	
}


/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.  You are
 *      free to modiy this code as necessary for your solution.
 */

int
createcars(int nargs,
           char ** args)
{
        int index, error;
	char name[50];

        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;

	// initialize vars

	tsem = sem_create("tsem", 0);
	enterQueue = lock_create("enterQueue");
	countLock = lock_create("countLock");
	nwLock = lock_create("nwLock");
	neLock = lock_create("neLock");
	swLock = lock_create("swLock");
	seLock = lock_create("seLock");
	
	clearCond = cv_create("clearCond");

	
	carCount = 0;
	greenLight = directions[0]; // default set to North

        /*
         * Start NCARS approachintersection() threads.
         */

        for (index = 0; index < NCARS; index++) {
		
		
                error = thread_fork("approachintersection thread",
                                    NULL,
                                    index,
                                    approachintersection,
                                    NULL
                                    );

                /*
                 * panic() on error.
                 */

                if (error) {
                        
                        panic("approachintersection: thread_fork failed: %s\n",
                              strerror(error)
                              );
                }
        }

	// wait for all the car threads to finish
	
	for (index=0; index<NCARS; index++) {
		P(tsem);
	}

	// cleaning like a good citizen
	sem_destroy(tsem);
	lock_destroy(enterQueue);
	lock_destroy(countLock);
	lock_destroy(nwLock);
	lock_destroy(neLock);
	lock_destroy(seLock);
	lock_destroy(swLock);
	cv_destroy(clearCond);
        
	return 0;
}
