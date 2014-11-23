#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <q.h>
#include <stdio.h>

int getLockID(int ldes);
void rampUpPriority(int lock, int currProcPrio);

int lock(int ldes, int type, int priority)		//priority here = process wait priority
{
	STATWORD ps;
	struct lentry *lptr;
	struct	pentry	*pptr;
	int lockNotGivenToRead = 0;
	int temp = -1;
	int lockID;
	
	disable(ps);
	pptr = &proctab[currpid];
	
	lockID = getLockID(ldes);
	
	//kprintf("\n IN lock: %d...lock des..%d", lockID, ldes);
	if( isbadlock(lockID) || (lptr=&locktab[lockID])->lstate == LOCKFREE || type == DELETED)
	{
		restore(ps);
		return(SYSERR);
	}
	
	
	if( lptr->locked == 0 )
	{
		lptr->lstate = LOCKUSED;
		lptr->locked = 1;					//indicates someone has acquired lockID
		lptr->acquiredby[currpid] = 1;			//stores the PID of process which has acquired lockID
		lptr->lockType = type;	
		lptr->lprio = pptr->pprio;
		
		
		pptr->procLockType[lockID] = type;	//indicates type of locking (WRITE or READ) for a process
		pptr->waitPriority = priority;		//sets wait priority of process
		pptr->lockID = -1;					//process not in wait queue so its lock id is -1;
		
		
		restore(ps);
		return(OK);
	}
	
	else if( lptr->locked != 0 )
	{
	
		if(lptr->lockType == WRITE)
		{
			pptr->pstate = PRWAIT;
			pptr->lockID = ldes;
			pptr->procLockType[lockID] = type;	//indicates type of locking (WRITE or READ) for a process
			pptr->waitPriority = priority;		//sets wait priority of process
			pptr->timeInWaiting = ctr1000;
			rampUpPriority(lockID, pptr->pprio);
			
			insert(currpid, lptr->lhead, priority);
			
			//pptr->pwaitret = OK;
			resched();
		
			restore(ps);
			return(OK);
		}
		
		else if(lptr->lockType == READ )
		{
		
			if( type == READ )
			{
				//check if incoming process is reader and has wait priority larger than any waiting writer process wait priority
				temp = lptr->lhead;
				while( q[temp].qnext != lptr->ltail )
				{ 
					if( (proctab[q[temp].qnext].procLockType[lockID] == WRITE) && (priority < proctab[q[temp].qnext].waitPriority) )
					{
						lockNotGivenToRead = 1;		//1 value indicates the lock should not be given because a writer with larger wait prio than this read is waiting
					}
					
					temp = q[temp].qnext;
				}
				
				if(lockNotGivenToRead == 1)
				{
					pptr->pstate = PRWAIT;
					pptr->lockID = ldes;
					pptr->procLockType[lockID] = type;	//indicates type of locking (WRITE or READ) for a process
					pptr->waitPriority = priority;		//sets wait priority of process

					rampUpPriority(lockID, pptr->pprio);
					pptr->timeInWaiting = ctr1000;
					insert(currpid, lptr->lhead, priority);
					
					//pptr->pwaitret = OK;
					resched();
				
					restore(ps);
					return(OK);
				}
				else if(lockNotGivenToRead != 1)				//READER can be given lockID as no writer with wait prio greater than currpid wait prio is there
				{
					
					if( lptr->lprio < pptr->pprio )
					{
						lptr->lprio = pptr->pprio;
					}
					
					lptr->acquiredby[currpid] = 1;
					pptr->procLockType[lockID] = type;			//indicates type of locking (WRITE or READ) for a process
					pptr->waitPriority = priority;
					pptr->lockID = -1;
					
					restore(ps);
					return(OK);
				}
				
			}
			else if( type == WRITE )
			{
				pptr->pstate = PRWAIT;
				pptr->lockID = ldes;
				pptr->procLockType[lockID] = type;	//indicates type of locking (WRITE or READ) for a process
				pptr->waitPriority = priority;		//sets wait priority of process

				rampUpPriority(lockID, pptr->pprio);
				pptr->timeInWaiting = ctr1000;
				insert(currpid, lptr->lhead, priority);
				
				//pptr->pwaitret = OK;
				resched();
			
				restore(ps);
				return(OK);
			}
		}
	}
	
	
	
	restore(ps);
	return(OK);
}


int getLockID(int ldes)
{
	int i;
	if(ldes>0)
	{
		for( i=0; i<NLOCKS; i++)
		{
			if( locktab[i].lDes == ldes )
			{
				return(i);
			}
		}
	}
	
	return(SYSERR);
}



void rampUpPriority(int lock, int currProcPrio)
{
	int i, j;
	//kprintf("\n In ramp up.. %d", currProcPrio);
	for( i=0; i<NPROC; i++)
	{
	
		//Some Processes (1 or more) are holding the lock 
		if( (proctab[i].procLockType[lock]!=DELETED) && (locktab[lock].acquiredby[i]==1) )
		{
			//kprintf("\n Proc %d holding lock %d ", i, lock);
			// make pprio = main prio
			if( proctab[i].mainPrio < currProcPrio )
			{
				//kprintf("\nProc %d has priority less than currProcPrio %d", i, currProcPrio);
				//proctab[i].mainPrio = proctab[i].pprio;		//change their priority because they have less than currently waiting proc
				proctab[i].pprio = currProcPrio;
				//kprintf("\nChanged Priority: %d", proctab[i].pprio);
				//check all locks this proc is waiting to acquire/or is in wait queue
				/* for( j=0; j<NLOCKS; j++)
				{
					
					if( (proctab[i].procLockType[j]!=DELETED) && (locktab[j].acquiredby[i]!=1) )
					{
						kprintf("Process %d waiting on lock %d", i, j);
						locktab[j].effectOfPriorityInheritance = 1;
						rampUpPriority(j, proctab[i].pprio);
					}
				} */
				
				updatePriorityOnRelease(i);
				
				
			}
			/* else
			{
				if(proctab[i].pprio <= proctab[i].mainPrio)
			} */
		}
		//else if(
	}
}