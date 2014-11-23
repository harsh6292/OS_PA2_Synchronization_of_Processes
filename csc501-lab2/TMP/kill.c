/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <lock.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
void relLockOnKill(pid, lockID);
 
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev, i, callResched, lockID;
	callResched = RESCHNO;
	
	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	for( i=0; i<NLOCKS; i++)
	{
		if( locktab[i].acquiredby[pid] == 1)		//this process has acquired some lock and holding it, so release it and resched
		{
			//kprintf("\n Kill is holding lock %s", proctab[pid].pname);
			callResched = RESCHYES;
			relLock(pid, i, 0);
		}
	}
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;
					//if this proc is in wait queue of any lock i, remove it ... a proc can be in waiting q of any one lock only
					/* for( i=0; i<NLOCKS; i++)
					{
						
						if( (proctab[pid].procLockType[i]!=DELETED) && (locktab[i].acquiredby[pid]!=1) )
						{
							kprintf("Killed Process %d waiting on lock %d", pid, i);
							locktab[j].effectOfPriorityInheritance = 1;
							rampUpPriority(j, proctab[i].pprio);
						}
					} */
					
					
					lockID = getLockID(pptr->lockID);
					if( !isbadlock(lockID) || locktab[lockID].lstate != LOCKFREE )
					{
						//dequeue(pid);
						//kprintf("\nKill proc getting called in PRWAIT, releasing wait lock %d..%d", lockID, pid);
						proctab[pid].pprio = -1;
						proctab[pid].mainPrio = -1;
						relLock(pid, lockID, 1);
						//dequeue(pid);
					}
					

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	
	if(callResched)
	{
		//kprintf("\n About to call resched");
		//indicates that this proc was holding lock and is deleted
		//in between so now it should release lock and call resched
		resched();
	}
	
	restore(ps);
	return(OK);
}


/*
void relLockOnKill(pid, lockID)
{
	struct lentry *lptr;
	struct pentry *pptr;
	int temp;
	
	//pptr=&proctab[pid];
	lptr=&locktab[lockID];
	
	dequeue(pid);
	
	//lptr->lstate = LOCKUSED;
	lptr->locked = 0;					//indicates someone has acquired lockID
	lptr->acquiredby[pid] = -1;			//stores the PID of process which has acquired lockID
	lptr->lockType = DELETED;	
		
	pptr->procLockType[lKey] = DELETED;	//indicates type of locking (WRITE or READ) for a process
	pptr->waitPriority = -1;		//sets wait priority of process
	pptr->lockID = -1;	
	
	//currProcPprio = pptr->pprio;
	//pptr->pprio = pptr->mainPrio;
	proctab[currpid].pprio = -1;//proctab[currpid].mainPrio;
	kprintf("\nKilled Process %d ", pid);
			//locktab[j].effectOfPriorityInheritance = 1;
			//for this lock i, find the maximum "scheduling priority" of waiting processes
	temp = lptr->ltail;
	while(q[temp].qprev != lptr->lhead)
	{
		if( maxProcPrio < proctab[q[temp].qprev].pprio )
		{
			maxProcPrio = proctab[q[temp].qprev].pprio;
		}
				
		temp = q[temp].qprev;
	}
	kprintf("\n Max scheduling priority of all waiting process on lock %d is %d", i, maxProcPrio);
	rampUpPriority(i, maxProcPrio);
			
			
	kprintf("\n Release finished for proc %d with priority %d", pid, proctab[pid].pprio);
	
}
*/