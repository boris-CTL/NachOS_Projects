// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"
using namespace std;

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------


//To compare different threads by various means, I need some "different" comparing functions.
//First, a comparing function is designed for FCFS scheduling.
int comparing_function_FCFS(Thread * thr_1, Thread * thr_2) {
    return 1;
}

//Second, a comparing function is designed for Priority scheduling.
int comparing_function_Priority(Thread * thr_1, Thread * thr_2) {
    bool bigger_ = ((*thr_1).priority_of_the_thread > (*thr_2).priority_of_the_thread);
    bool equal_ = ((*thr_1).priority_of_the_thread == (*thr_2).priority_of_the_thread);
    bool smaller_ = ((*thr_1).priority_of_the_thread < (*thr_2).priority_of_the_thread);
    if (bigger_) {
        return 1;
    }
    else if (equal_) {
        return 0;
    }
    else {
        return -1;
    }
}

//Third, a comparing function is designed for SJF scheduling.
int comparing_function_SJF(Thread * thr_1, Thread * thr_2) {
    bool bigger_ = ((*thr_1).predicted_burst_time_of_the_thread > (*thr_2).predicted_burst_time_of_the_thread);
    bool equal_ = ((*thr_1).predicted_burst_time_of_the_thread == (*thr_2).predicted_burst_time_of_the_thread);
    bool smaller_ = ((*thr_1).predicted_burst_time_of_the_thread < (*thr_2).predicted_burst_time_of_the_thread);
    if (bigger_) {
        return 1;
    }
    else if (equal_) {
        return 0;
    }
    else {
        return -1;
    }
}


Scheduler::Scheduler()
{
    Scheduler(FCFS);
} 

//
Scheduler::Scheduler(SchedulerType scheduling_type) {
    schedulerType = scheduling_type;
    if (schedulerType == SJF) {
        readyList = new SortedList<Thread *>(comparing_function_SJF);
        cout << "readyList is assigned as SortedList<Thread *>(comparing_function_SJF) (pointer-wise)." <<endl;
    }
    else if (schedulerType == Priority) {
        readyList = new SortedList<Thread *>(comparing_function_Priority);
        cout << "readyList is assigned as SortedList<Thread *>(comparing_function_Priority) (pointer-wise)." <<endl;
    }
    else if(schedulerType == FCFS) {
        readyList = new SortedList<Thread *>(comparing_function_FCFS);
        cout << "readyList is assigned as SortedList<Thread *>(comparing_function_FCFS) (pointer-wise)." <<endl;
    }
    toBeDestroyed = NULL;
}



//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());

    thread->setStatus(READY);
    readyList->Append(thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (readyList->IsEmpty()) {
	return NULL;
    } else {
    	return readyList->RemoveFront();
    }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
 
//	cout << "Current Thread" <<oldThread->getName() << "    Next Thread"<<nextThread->getName()<<endl;
   
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
#ifdef USER_PROGRAM
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
#endif
}


//My self-defined function: void set_scheduler_type(SchedulerType scheduler_type_)
void Scheduler::set_scheduler_type(SchedulerType scheduler_type_){
    schedulerType = scheduler_type_;
}


//My self-defined function: SchedulerType get_scheduler_type()
SchedulerType Scheduler::get_scheduler_type() {
    return schedulerType;
}






//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyList->Apply(ThreadPrint);
}
