#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

LOCAL int newLockDes();


/*
	Create a lock and returning its id
*/

int lcreate()
{
	STATWORD ps;
	int lock;
	
	disable(ps);
	
	if( (lock=newLockDes())==SYSERR )
	{
		restore(ps);
		return(SYSERR);
	}
	//kprintf("\n lock granted : %d......lock des...%d", lock, locktab[lock].lDes);
	restore(ps);
	return(locktab[lock].lDes);
}


LOCAL int newLockDes()
{
	int lock, i;
	
	for(i=0; i<NLOCKS; i++)
	{
		lock=nextlock--;
		if(nextlock < 0)
			nextlock = NLOCKS - 1;
		if(locktab[lock].lstate==LOCKFREE)
		{
			locktab[lock].lstate = LOCKUSED;
			locktab[lock].lDes = nextLDes++;
			return(lock);
		}
	}
	return(SYSERR);
}
