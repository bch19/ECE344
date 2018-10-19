#include <types.h>
#include <lib.h>
#include <hello.h>


void hello(void){
	DEBUG(DB_EXEC, "Executing funtion hello.\n");  
	kprintf("Hello World\n");

	return ;
}

