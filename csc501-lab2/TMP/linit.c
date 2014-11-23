#include <kernel.h>
#include <lock.h>
#include <proc.h>

void linit()
{
	int i,j;
	struct  lentry  *lptr;
	
	nextlock = NLOCKS-1;
	nextLDes = 1;
	
	for (i=0 ; i<NLOCKS ; i++)
	{
		(lptr = &locktab[i])->lstate = LOCKFREE;
		lptr->locked=0;
		lptr->lDes = -1;
		for(j=0; j<NPROC; j++)
		{
			lptr->acquiredby[j]=0;
		}
		
		lptr->lockType = DELETED;
		lptr->effectOfPriorityInheritance=0;
		lptr->lprio=-1;
		lptr->ltail = 1 + (lptr->lhead = newqueue());
	}
}
