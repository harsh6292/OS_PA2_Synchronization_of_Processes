/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
SYSCALL chprio(int pid, int newprio)
{
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
	    (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	//pptr->pprio = newprio;
	pptr->mainPrio = newprio;
	updatePriorityOnRelease(pid);
	if(pptr -> pstate == PRREADY) {
		dequeue(pid);
		insert(pid, rdyhead, pptr -> pprio);
	}
	restore(ps);
	return(newprio);
}
