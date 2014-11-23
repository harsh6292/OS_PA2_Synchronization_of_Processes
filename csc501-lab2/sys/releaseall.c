#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <q.h>
#include <stdio.h>

void relLock(int pid, int lKey, int callFromWhere);

void updatePriorityOnRelease(int pid);

int releaseall(int numLocks, int args, ...)
{
	int nargs, i, getNextLdes;
	STATWORD ps;
	//unsigned long *a;			//pointer to list of args
	//unsigned long *saddr;		//stack address
	int lockKey;
	int returnState = OK;

	
	disable(ps);
	
	//a = (unsigned long *)(&args) + (nargs-1);
	
	for (i=0; i<numLocks; i++)
	{	
		getNextLdes = *(&args + i);
		lockKey = getLockID(getNextLdes);
		
		//kprintf("\n In fun to release.. args[%d]...proc %s.....lock des %d......lock key..%d", i, proctab[currpid].pname, getNextLdes, lockKey);
		
		if( isbadlock(lockKey) || locktab[lockKey].lstate == LOCKFREE || locktab[lockKey].acquiredby[currpid]!=1)
		{
			//restore(ps);
			//kprintf("\n bad lock");
			returnState = SYSERR;
		}
		else
		{
			/* proctab[currpid].pprio = proctab[currpid].mainPrio;
			updatePriorityOnRelease(currpid); */
			relLock(currpid, lockKey, 0);
		}
	
	}
	//kprintf("\n Release finished");
	resched();
	//kprintf("\n Release finished");
	restore(ps);
	return(returnState);
}


void relLock(int pid, int lKey, int callFromWhere)
{
	struct lentry *lptr;
	struct pentry *pptr;
	int nextPID, i, temp, temp2;
	int writerFound = 0;
	int readerFound = 0;
	int writer = 0;
	int currProcPprio = 0;
	unsigned long absoluteTimeDiff = 0;
	
	pptr=&proctab[pid];
	lptr=&locktab[lKey];
	
	//lptr->lstate = LOCKUSED;
	
	if(callFromWhere == 1)
	{
		proctab[pid].pprio = proctab[pid].mainPrio;
		updatePriorityOnRelease(pid);
			
		lptr->locked = 0;					//indicates someone has acquired lockID
		lptr->acquiredby[pid] = -1;			//stores the PID of process which has acquired lockID
		lptr->lockType = DELETED;	
			
		pptr->procLockType[lKey] = DELETED;	//indicates type of locking (WRITE or READ) for a process
		pptr->waitPriority = -1;		//sets wait priority of process
		pptr->lockID = -1;	
			
		currProcPprio = pptr->pprio;
		//pptr->pprio = pptr->mainPrio;
			
		proctab[pid].pprio = proctab[pid].mainPrio;
		updatePriorityOnRelease(pid);
	}
			
	//kprintf("\n Release finished for proc %d with priority %d", pid, proctab[pid].pprio);
	
	
	
	//if wait list is non-empty, then release some process else no process to release, else nothing is to be done.
	else if(callFromWhere == 0)
	{
		//proctab[pid].pprio = proctab[pid].mainPrio;
		//updatePriorityOnRelease(pid);
			
		lptr->locked = 0;					//indicates someone has acquired lockID
		lptr->acquiredby[pid] = -1;			//stores the PID of process which has acquired lockID
		lptr->lockType = DELETED;	
			
		pptr->procLockType[lKey] = DELETED;	//indicates type of locking (WRITE or READ) for a process
		pptr->waitPriority = -1;		//sets wait priority of process
		pptr->lockID = -1;	
			
		currProcPprio = pptr->pprio;
		//pptr->pprio = pptr->mainPrio;
			
		proctab[pid].pprio = proctab[pid].mainPrio;
		updatePriorityOnRelease(pid);
	
		if( nonempty(lptr->lhead) )
		{
			/* kprintf("\n non emptry");
			temp = lptr->ltail;
			while( q[temp].qprev!= lptr->lhead)
			{
				kprintf("\t %d, %s\n", q[temp].qprev, proctab[q[temp].qprev].pname);
				temp = q[temp].qprev;
			}
			
			//for(i=0 ;i<NLOCKS; i++)
			{
				for(temp=0;temp<NPROC;temp++)
				{
					if(locktab[lKey].acquiredby[temp]==1)
					{
						kprintf("Process %d has acquired lock %s", temp, proctab[temp].pname);
					}
					else
						kprintf("None ");
				}
			}
			 */
			
			
			//nextPID = getlast(lptr->ltail);
			
			
			//if the next in line is writer then check if some "other" readers have already acquired or not. if yes then this writer
			//is not scheduled to run unitl all readers have finished
			if( proctab[q[lptr->ltail].qprev].procLockType[lKey] == WRITE )
			{
				for( i=0; i<NPROC; i++)
				{
					if( (proctab[i].procLockType[lKey] == READ) && (proctab[i].waitPriority >= proctab[q[lptr->ltail].qprev].waitPriority) && locktab[lKey].acquiredby[i]==1)
					{
						//kprintf("\n READER found!!!");
						readerFound = 1;
						break;
					}
				}
				
				//if no reader found holding lock, then this writer can be given lock
				if( readerFound != 1 )
				{
					//kprintf("\n NO READER FOUND!!!");
					nextPID =  getlast(lptr->ltail);
					lptr->lstate = LOCKUSED;
					lptr->locked = 1;					//indicates someone has acquired lockID
					lptr->acquiredby[nextPID] = 1;			//stores the PID of process which has acquired lockID
					lptr->lockType = WRITE;	
					proctab[nextPID].timeInWaiting = ctr1000 - proctab[nextPID].timeInWaiting;
					//kprintf("\nProcess %d->%s spend %d time in waiting", nextPID, proctab[nextPID].pname, proctab[nextPID].timeInWaiting);
					
					//proctab[nextPID].procLockType[lockID] = type;	//indicates type of locking (WRITE or READ) for a process
					//proctab[nextPID].waitPriority = priority;		//sets wait priority of process
					proctab[nextPID].lockID = -1;
					ready(nextPID, RESCHNO);
				}
				
			}
			//else if the list has readers in first position, then find the highest priority waiting writer in queue
			else if (proctab[q[lptr->ltail].qprev].procLockType[lKey] == READ)
			{
				//kprintf("\nReader avlbl");
				temp = lptr->ltail;
				while( (q[temp].qprev!=lptr->lhead) )		//(proctab[q[temp].qprev].procLockType[lKey] != WRITE) || 
				{
					//kprintf("\n values: %d", q[temp].qprev);
					if( proctab[q[temp].qprev].procLockType[lKey] == WRITE )
					{
						writerFound=1;
						writer = q[temp].qprev;
						break;
					}
					temp = q[temp].qprev;
				}
				//kprintf("\n temp values: %d....%d", temp, q[temp].qprev);
				
				//if writer found in queue
				if(writerFound == 1)
				{
					//kprintf("\n writer found");
					
					
					//implement here time concept
					//check if writer found has same wait priority as that of first reader in queue. If yes then writer is scheduled to run
					if( proctab[q[lptr->ltail].qprev].waitPriority == proctab[writer].waitPriority )				//compare wait priority of reader and writer
					{
						//kprintf("\n Wait prio of r & w equal");
						absoluteTimeDiff = (proctab[writer].timeInWaiting - proctab[q[lptr->ltail].qprev].timeInWaiting);
						if( absoluteTimeDiff < 0)
						{
							absoluteTimeDiff = -1*absoluteTimeDiff;
						}
						//kprintf("\n absolute time diff < 1000: %d, %d, %d...ctr %d", proctab[writer].timeInWaiting, proctab[q[lptr->ltail].qprev].timeInWaiting, absoluteTimeDiff, ctr1000);
						if( absoluteTimeDiff < 1000 )
						{
							
							nextPID =  dequeue(writer);//getlast(lptr->ltail);
							lptr->lstate = LOCKUSED;
							lptr->locked = 1;					//indicates someone has acquired lockID
							lptr->acquiredby[nextPID] = 1;			//stores the PID of process which has acquired lockID
							lptr->lockType = WRITE;	
							proctab[nextPID].timeInWaiting = ctr1000 - proctab[nextPID].timeInWaiting;
							//kprintf("\nProcess %d->%s spend %d time in waiting", nextPID, proctab[nextPID].pname, proctab[nextPID].timeInWaiting);
							//proctab[nextPID].procLockType[lockID] = type;	//indicates type of locking (WRITE or READ) for a process
							//proctab[nextPID].waitPriority = priority;		//sets wait priority of process
							proctab[nextPID].lockID = -1;
							ready(nextPID, RESCHNO);
						}
						//else if absoluteTimeDiff > 1000, then release all readers upto first writer found
						else
						{
							temp2 = lptr->ltail;
							//kprintf("\n Wait prio not equal...tail = %d.....tail prev..%d......tail prev prev...%d ...temp prev..%d", temp2, q[temp2].qprev, q[q[temp2].qprev].qprev, q[temp].qprev);
							while( q[lptr->ltail].qprev != writer )
							{
								
								nextPID =  getlast(lptr->ltail);//dequeue(q[temp2].qprev);//
								//kprintf("\n andar %d...%s", nextPID, proctab[nextPID].pname);
								//kprintf("\n andar %d...%s", nextPID, proctab[nextPID].pname);
								lptr->lstate = LOCKUSED;
								lptr->locked = 1;					//indicates someone has acquired lockID
								lptr->acquiredby[nextPID] = 1;			//stores the PID of process which has acquired lockID
								lptr->lockType = READ;	
								proctab[nextPID].timeInWaiting = ctr1000 - proctab[nextPID].timeInWaiting;
								//kprintf("\nProcess %d->%s spend %d time in waiting", nextPID, proctab[nextPID].pname, proctab[nextPID].timeInWaiting);
								//proctab[nextPID].procLockType[lockID] = type;	//indicates type of locking (WRITE or READ) for a process
								//proctab[nextPID].waitPriority = priority;		//sets wait priority of process
								proctab[nextPID].lockID = -1;
								ready(nextPID, RESCHNO);
								
								//temp = q[temp].qprev;
							}
						}
					}
					//else if writer wait priority is less than the first reader, then release all readers upto first writer found
					else
					{
						
						temp2 = lptr->ltail;
						//kprintf("\n Wait prio not equal...tail = %d.....tail prev..%d......tail prev prev...%d ...temp prev..%d", temp2, q[temp2].qprev, q[q[temp2].qprev].qprev, q[temp].qprev);
						while( q[lptr->ltail].qprev != writer )
						{
							
							nextPID =  getlast(lptr->ltail);//dequeue(q[temp2].qprev);//
							//kprintf("\n andar %d...%s", nextPID, proctab[nextPID].pname);
							//kprintf("\n andar %d...%s", nextPID, proctab[nextPID].pname);
							lptr->lstate = LOCKUSED;
							lptr->locked = 1;					//indicates someone has acquired lockID
							lptr->acquiredby[nextPID] = 1;			//stores the PID of process which has acquired lockID
							lptr->lockType = READ;	
							proctab[nextPID].timeInWaiting = ctr1000 - proctab[nextPID].timeInWaiting;
							//kprintf("\nProcess %d->%s spend %d time in waiting", nextPID, proctab[nextPID].pname, proctab[nextPID].timeInWaiting);
							//proctab[nextPID].procLockType[lockID] = type;	//indicates type of locking (WRITE or READ) for a process
							//proctab[nextPID].waitPriority = priority;		//sets wait priority of process
							proctab[nextPID].lockID = -1;
							ready(nextPID, RESCHNO);
							
							//temp = q[temp].qprev;
						} 
					}
				}
				//else if no writer found in queue, then release all readers in the queue
				else if(writerFound!=1)
				{
					//kprintf("\n no writer so reader execute");
					temp = lptr->ltail;
					while( q[temp].qprev != lptr->lhead )
					{
						
						nextPID =  dequeue(q[temp].qprev);//getlast(lptr->ltail);
						//kprintf("\n andar %d...%s", nextPID, proctab[nextPID].pname);
						lptr->lstate = LOCKUSED;
						lptr->locked = 1;					//indicates someone has acquired lockID
						lptr->acquiredby[nextPID] = 1;			//stores the PID of process which has acquired lockID
						lptr->lockType = READ;	
						proctab[nextPID].timeInWaiting = ctr1000 - proctab[nextPID].timeInWaiting;
						//kprintf("\nProcess %d->%s spend %d time in waiting", nextPID, proctab[nextPID].pname, proctab[nextPID].timeInWaiting);
						//proctab[nextPID].procLockType[lockID] = type;	//indicates type of locking (WRITE or READ) for a process
						//proctab[nextPID].waitPriority = priority;		//sets wait priority of process
						proctab[nextPID].lockID = -1;
						ready(nextPID, RESCHNO);
						
						//temp = q[temp].qprev;
					}
					//kprintf("\n bAHAR");
				}
				
			}		
		
		//lock(locktab[lKey].lDes, proctab[nextPID].procLockType[lKey],proctab[nextPID].waitPriority);
		}
	}
}



void updatePriorityOnRelease(int pid)
{
	int i, temp, maxProcPrio;
	maxProcPrio = 0;
	//kprintf("\nIn release of process %d %d",pid, proctab[pid].lockID);
	//check process is in wait queue of which all locks
	//for( i=0; i<NLOCKS; i++)
	i = getLockID(proctab[pid].lockID);
	if(i!=SYSERR)
	{
		//if( (proctab[pid].procLockType[i]!=DELETED) && (locktab[i].acquiredby[pid]!=1) )
		{
			//kprintf("\nProcess %d is in wait queue of lock %d", pid, i);
			//locktab[j].effectOfPriorityInheritance = 1;
			//for this lock i, find the maximum "scheduling priority" of waiting processes
			temp = locktab[i].ltail;
			while(q[temp].qprev != locktab[i].lhead)
			{
				if( maxProcPrio < proctab[q[temp].qprev].pprio )
				{
					maxProcPrio = proctab[q[temp].qprev].pprio;
				}
				
				temp = q[temp].qprev;
			}
			//kprintf("\n Max scheduling priority of all waiting process on lock %d is %d", i, maxProcPrio);
			rampUpPriority(i, maxProcPrio);
			//for this lock i, find all the holders for this
			/* for( j=0; j<NPROC; j++)
			{
				if( (proctab[j].procLockType[i]!=DELETED) && (locktab[i].acquiredBy[j]==1) )
				{
					
				}
			} */
		}
	}
	
}
