// alarm.h 
//	Data structures for a software alarm clock.
//
//	We make use of a hardware timer device, that generates
//	an interrupt every X time ticks (on real systems, X is
//	usually between 0.25 - 10 milliseconds).
//
//	From this, we provide the ability for a thread to be
//	woken up after a delay; we also provide time-slicing.
//
//	NOTE: this abstraction is not completely implemented.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ALARM_H
#define ALARM_H

#include "copyright.h"
#include "utility.h"
#include "callback.h"
#include "timer.h"
#include "thread.h"
#include <vector>
using namespace std;

// Define a class, trying to put threads to sleep.
class sleeping_space_for_threads {
public:
  sleeping_space_for_threads():now_time(0) {};

  int now_time;

  class sleeping_space {
  public:
    int time_to_wake_up;
    Thread * thread_that_is_sleeping;
    sleeping_space(int time_argument, Thread * thr):
      thread_that_is_sleeping(thr), time_to_wake_up(time_argument) {};
  };

  void make_thread_to_sleep(int time_argument_, Thread * thr_);

  int check_thread();

  vector<sleeping_space> vector_containing_sleeping_spaces;

};



// The following class defines a software alarm clock. 
class Alarm : public CallBackObj {
  public:
    Alarm(bool doRandomYield);	// Initialize the timer, and callback 
				// to "toCall" every time slice.
    ~Alarm() { delete timer; }
    
    void WaitUntil(int x);	// suspend execution until time > now + x

    sleeping_space_for_threads a_sleeping_space_for_threads;

  private:
    Timer *timer;		// the hardware timer device

    void CallBack();		// called when the hardware
				// timer generates an interrupt
};

#endif // ALARM_H
