#include <conf.h>
#include <kernel.h>
#include <proc.h>
#define NDEBUG
#include <assert.h>
#include <lock.h>
#include <stdio.h>
#include <sem.h>


void test_sem();
void test_lock();


prch(char c)
{
	int i, count=0;

	while(count++ < 100) {
		kprintf("%c", c);
		for (i = 0; i< 1000000; i++);
	}
}



void reader1 (char *msg, int sem)
{
	int i, count=0;
        kprintf ("\n  %s: to acquire semaphore\n", msg);
        wait(sem);
        //sleep (10);
        kprintf ("\n  %s: acquired lock. Then sleep 5 seconds.\n", msg);
		//sleep(5);
		

		while(count++ < 50) {
			kprintf("R");
			for (i = 0; i< 1000000; i++);
		}
        kprintf ("\n  %s: to release lock\n", msg);
        signal(sem);
}

void writer1 (char *msg, int sem)
{
	int i, count=0;
        kprintf ("\n  %s: to acquire lock\n", msg);
        wait(sem);
        kprintf ("\n  %s: acquired lock, sleep 5s\n", msg);
        //sleep (5);
		while(count++ < 50) {
			kprintf("W");
			for (i = 0; i< 1000000; i++);
		}
        kprintf ("\n  %s: to release lock\n", msg);
        signal(sem);
}




void reader2 (char *msg, int lck)
{
	int i, count=0;
        kprintf ("\n  %s: to acquire lock.\n", msg);
        lock(lck, READ, 20);
		//sleep (5);
        kprintf ("\n  %s: acquired lock. Then sleep 5 seconds.\n", msg);
		//sleep(5);
		while(count++ < 50) {
			kprintf("R");
			for (i = 0; i< 1000000; i++);
		}
        kprintf ("\n  %s: to release lock\n", msg);
        releaseall(1, lck);
}

void writer2 (char *msg, int lck)
{
	int i, count=0;
       kprintf ("\n  %s: to acquire lock.\n", msg);
        lock(lck, WRITE, 20);
		//sleep (5);
        kprintf ("\n  %s: acquired lock. Then sleep 5 seconds.\n", msg);
		//sleep(5);
		while(count++ < 50) {
			kprintf("W");
			for (i = 0; i< 1000000; i++);
		}
        kprintf ("\n  %s: to release lock\n", msg);
        releaseall(1, lck);
        
}



void task1()
{
	kprintf("\nTest for Semaphore for priority inversion");
	test_sem();
	kprintf("\n\nTest for Locks for priority inversion");
	test_lock();
}

void test_sem()
{
	int sem;
	
	sem = screate(1);
	int     pr1, pr2, pr3;
    int     rd1,wr1;

    rd1 = create(reader1, 2000, 20, "reader1", 2, "Reader1_sem", sem);
    wr1 = create(writer1, 2000, 30, "writer1", 2, "Writer1_sem", sem);
    pr1 = create(prch,2000,25,"proc A",1,'A');
	pr2 = create(prch,2000,25,"proc B",1,'B');
	pr3 = create(prch,2000,25,"proc C",1,'C');
	
	resume(rd1);
	resume(wr1);
	resume(pr1);
	resume(pr2);
	resume(pr3);
	
	sleep(20);
	kprintf("\nSemaphore priority inversion completed");
    
}



void test_lock()
{
	int lck;
	
	lck = lcreate(1);
	int     pr1, pr2, pr3;
    int     rd2,wr2;

    rd2 = create(reader2, 2000, 20, "reader1", 2, "Reader1_lock", lck);
    wr2 = create(writer2, 2000, 30, "writer1", 2, "Writer1_lock", lck);
    pr1 = create(prch,2000,25,"proc A",1,'A');
	pr2 = create(prch,2000,25,"proc B",1,'B');
	pr3 = create(prch,2000,25,"proc C",1,'C');
	
	resume(rd2);
	resume(wr2);
	resume(pr1);
	resume(pr2);
	resume(pr3);
	
	sleep(20);
	kprintf("\nLocks priority inversion completed");
    
}