// alarm.cc
//	Routines to use a hardware timer device to provide a
//	software alarm clock.  For now, we just provide time-slicing.
//
//	Not completely implemented.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "alarm.h"
#include "main.h"
#include "thread.h"
#include <vector>
#include "scheduler.h"
using namespace std;


//----------------------------------------------------------------------
// Alarm::Alarm
//      Initialize a software alarm clock.  Start up a timer device
//
//      "doRandom" -- if true, arrange for the hardware interrupts to 
//		occur at random, instead of fixed, intervals.
//----------------------------------------------------------------------

Alarm::Alarm(bool doRandom)
{
    timer = new Timer(doRandom, this);
}


//My self-defined function : void make_thread_to_sleep(int time_argument_, Thread * thr_)
void sleeping_space_for_threads::make_thread_to_sleep(int time_argument_, Thread * thr_) {
    IntStatus indicating_status;
    indicating_status = (*((*kernel).interrupt)).getLevel();
    ASSERT(indicating_status == IntOff);
    int total_time = now_time + time_argument_;
    sleeping_space s = sleeping_space(total_time, thr_);
    vector_containing_sleeping_spaces.push_back(s);
    (*thr_).Sleep(0);
}


//My self-defined function : int check_thread()
int sleeping_space_for_threads::check_thread() {
    now_time ++;
    int sleep_enough = 0;
    for (int j = 0; j < vector_containing_sleeping_spaces.size(); j++) {
        if (vector_containing_sleeping_spaces[j].time_to_wake_up <= now_time) { //this means that this thread already slept enough!
            sleep_enough = 1;
            Thread * thread_that_is_going_to_wake_up = vector_containing_sleeping_spaces[j].thread_that_is_sleeping;
            (*((*kernel).scheduler)).ReadyToRun(thread_that_is_going_to_wake_up);
            vector_containing_sleeping_spaces.erase(vector_containing_sleeping_spaces.begin()+j);            
        }
    }
    return sleep_enough;
}


//Implementation of function: void WaitUntil(int x)
void Alarm::WaitUntil(int x) {
    IntStatus original_status;
    Thread * current_thread;
    original_status = (*((*kernel).interrupt)).SetLevel(IntOff);
    current_thread = (*kernel).currentThread;


    (*current_thread).set_start_time((*((*kernel).stats)).userTicks);


    int time_spent_executing_user_code = (*((*kernel).stats)).userTicks; //by userTicks' definition.
    int thread_start_time = (*current_thread).start_time_of_the_thread;
    int thread_predicted_burst_time = (*current_thread).predicted_burst_time_of_the_thread;
    int time_sum = time_spent_executing_user_code - thread_start_time + thread_predicted_burst_time;
    (*current_thread).set_predicted_burst_time(time_sum);


    a_sleeping_space_for_threads.make_thread_to_sleep(x, current_thread);


    (*((*kernel).interrupt)).SetLevel(original_status);
} 


//----------------------------------------------------------------------
// Alarm::CallBack
//	Software interrupt handler for the timer device. The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as 
//	if the interrupted thread called Yield at the point it is 
//	was interrupted.
//
//	For now, just provide time-slicing.  Only need to time slice 
//      if we're currently running something (in other words, not idle).
//	Also, to keep from looping forever, we check if there's
//	nothing on the ready list, and there are no other pending
//	interrupts.  In this case, we can safely halt.
//----------------------------------------------------------------------

void Alarm::CallBack() 
{
    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();
    bool indicator = (a_sleeping_space_for_threads.check_thread() == 0);

    if ( (indicator) && (a_sleeping_space_for_threads.vector_containing_sleeping_spaces.size() == 0) && (status == IdleMode) ) {	// is it time to quit?
        if (!interrupt->AnyFutureInterrupts()) {
	    timer->Disable();	// turn off the timer
	}
    } else {			// there's someone to preempt
        SchedulerType type_ =  (*((*kernel).scheduler)).get_scheduler_type();
        if (type_ == Priority) {
            interrupt->YieldOnReturn();
        }
    }
}

