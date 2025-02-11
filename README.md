# Virtual Memory Management and CPU Optimization in NachOS

## Overview

This project consists of three distinct sub-projects focusing on thread management, CPU optimization, and virtual memory management using the NachOS operating system. Each sub-project addresses a different aspect of operating system functionality.


## Project 1: Thread Management

### Description

This sub-project focuses on fixing potential issues related to multithreading in NachOS. The primary objective is to ensure that multiple user programs can run simultaneously without interfering with each other's memory.

### Key Features

- **Issue Addressed**: Incorrect memory sharing between threads.
- **Solution**:
  - Introduced a Boolean array to track physical page usage in `addrspace.h`.
  - Modified memory allocation strategy to ensure unique physical pages for each thread in `addrspace.cc`.
    - Rewrote the memory allocation logic in `AddrSpace::Load(char *fileName)` to ensure that each thread receives a unique physical page.
  - Updated program entry point calculation and memory deallocation in `addrspace.cc`.
    - Adjusted the method of determining the program’s physical entry point by correctly mapping virtual pages to physical pages. 
    - Used `divRoundDown(noffH.code.virtualAddr, PageSize)` to compute the correct physical memory location. 

- **Results**: Correct execution of multithreaded programs without memory interference.


## Project 2: CPU Optimization

### Description

This project implements a `Sleep()` system call using MIPS assembly and multiple CPU scheduling algorithms in NachOS. The goal is to enhance the OS's ability to manage thread execution and scheduling efficiently.

### Key Features

- **System Call - Sleep()**:
  - Added `Sleep()` system call to pause thread execution in `userprog/syscall.h` using MIPS.
  - Modified assembly code and exception handling to support `Sleep()`.
    - Updated `test/start.s` by mimicking the `PrintInt` assembly code to include `SC_Sleep`.
    - Modified `userprog/exception.cc` to handle `SC_Sleep` and invoke `WaitUntil()`.
  - Implemented an alarm mechanism to manage sleeping threads.
    - Created a `vector` container in `threads/alarm.h` to store sleeping threads.
    - Implemented `WaitUntil()` in `alarm.cc` to pause threads based on the provided sleep duration.
    - Used `CallBack()` to check if threads should wake up and remove them from the sleeping list.

- **CPU Scheduling**:
  - Implemented First-Come-First-Served (FCFS), Shortest-Job-First (SJF), and Priority Scheduling.
  - Added thread attributes for burst time and priority to `thread.h`.
    - Defined scheduler-specific comparison functions in `scheduler.cc`.
  - Modified scheduler to support dynamic scheduling policies.
    - Implemented preemptive priority scheduling by integrating `CallBack()` in `alarm.cc`.


## Project 3: Virtual Memory Management

### Description

This project extends NachOS to implement virtual memory management using paging, demand paging, and a page replacement algorithm. The goal is to enable the concurrent execution of memory-intensive programs within limited physical memory constraints.

### Key Features

- **Virtual Memory**:
  - Introduced a disk-based swap space (`backing_store`) using `SynchDisk`.
  - Implemented page table entries to track virtual-to-physical memory mappings.
  - Allocated physical frames dynamically and swapped pages in and out as needed.
    - Used valid-invalid bits for page presence checks.
- **Page Replacement Algorithm (LFU)**:
  - Implemented Least Frequently Used (LFU) algorithm for page replacement.
  - Managed page faults and swapping efficiently.
    - Implemented in `Machine::Translate()`

- **Results**: Successful execution of memory-intensive programs with efficient memory management.


## Conclusion

These projects collectively enhance the NachOS operating system by addressing critical aspects of thread management, system calls, CPU scheduling, and virtual memory management. Each project provides valuable insights into OS-level programming and debugging.

