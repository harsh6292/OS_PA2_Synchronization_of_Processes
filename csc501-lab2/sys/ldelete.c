#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int ldelete(int ldes)
{
	STATWORD ps;
	
	struct lentry *lptr;
	int pid, lockID;
	
	disable(ps);
	lockID = getLockID(ldes);
	
	if( isbadlock(lockID) || (lptr=&locktab[lockID])->lstate == LOCKFREE )
	{
		restore(ps);
		return(SYSERR);
	}
	
	//lptr = &locktab[ldes];
	lptr->lstate = LOCKFREE;
	lptr->locked = 0;
	lptr->acquiredby[currpid]=-1;
	lptr->lockType = DELETED;
	lptr->lprio=-1;
	
	
	if( nonempty(lptr->lhead) )
	{
		while( (pid=getlast(lptr->lhead)) != EMPTY )
		{
			proctab[pid].waitPriority = -1;
			proctab[pid].lockID = -1;
			proctab[pid].procLockType[lockID] = DELETED;
			proctab[pid].pwaitret = DELETED;
			
			ready(pid, RESCHNO);
		}
		resched();
	}
	
	restore(ps);
	return(OK);
}
