#include <proc.h>

#ifndef _LOCK_H_
#define _LOCK_H_


/* 
#ifndef NLOCKS
#define NLOCKS	50
#endif */


#ifndef DELETED
#define DELETED		101
#endif

#define READ		102
#define WRITE		103

#define LOCKFREE	'\01'
#define LOCKUSED	'\02'

struct lentry
{
	char lstate;
	int  locked;
	int  lDes;
	int  lockType;
	int  acquiredby[NPROC];
	
	int  effectOfPriorityInheritance;
	int  lprio;
	int  lhead;
	int  ltail;
};

extern struct lentry locktab[];
extern int nextlock;
extern int nextLDes;
extern void linit();
extern int lcreate();
extern int getLockID(int ldes);
extern void relLock(int pid, int lKey, int callFromWhere);
extern void rampUpPriority(int lock, int currProcPrio);
extern int lock(int ldes, int type, int priority);
extern int releaseall(int numLocks, int args, ...);
extern unsigned long ctr1000;

#define isbadlock(l) (l<0 || l>=NLOCKS)

#endif


