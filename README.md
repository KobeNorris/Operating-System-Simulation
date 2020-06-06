# **Operating System Simulation** (COMP2007 UNUK)

## **Project Description**

The goal of this coursework is to make use of operating system APIs (specifically, the POSIX API in Linux) and simple concurrency directives to solve a number of synchronisation problems that occur on scheduling systems that are similar to the ones that you may find in fairly simple operating systems. 

----

## **Task Overview**

1. Task 1: Static Process Scheduling 

2. Task 2: Unbounded Buffer with binary semaphores (a.k.a. producer/consumer problem) 

3. Task 3: Bounded Buffer with binary semaphores

4. Task 4: A Simplified Windows Scheduler

5. Task 5: Pre-emptive FCFS and Priority Boosting

***Clear marking criteria will be provided in [here](./ProjectExplaination/coursework2019-2020-Part2-v1.0.0.pdf).***

----

## **Task 1**: Static Process Scheduling

The goal of this task is to implement two basic process scheduling algorithms: First Come First Served (FCFS) and Round Robin in a static environment. That is, all jobs are known at the start (in practice, in a dynamic environment, jobs would show up over time, which is what we will be simulating in later tasks). You are expected to calculate response and turnaround times for each of the processes, as well as averages for all jobs. 

Both algorithms should be implemented in separate source files (task1a.c for FCFS, task1b.c for RR) and use the linked list provided as underlying data structure. In both cases, your implementation should contain a function that generates a pre-defined NUMBER_OF_JOBS (this constant is defined in coursework.h) and stores them in a linked list (using a FIFO approach in both cases). This linked list simulates the ready queue in a real operating system. Your implementation of FCFS and RR will remove jobs from the ready queues in the order in which they should run. Note that you will be required to use multiple linked lists in the second part of this coursework, which is why we provided a generic linked list implementation using “pointers to pointers”.

----

## **Task 2**: Unbounded Buffer with binary semaphores (a.k.a. producer/consumer problem) 

You are asked to implement an ``imaginary unbounded buffer’’ with a single producer and a single consumer. The producer adds jobs to the buffer, the consumer removes jobs from the buffer (provided that elements are available). Different implementations are possible, but we are asking you to use only binary semaphores in your implementation. That is, the value for the semaphores should never exceed 1. This corresponds to the first “implementation” discussed in the lecture slides (in which a semaphore is used to delay the consumer by putting it to sleep if no elements are available in the buffer). 

----

## **Task 3**: Bounded Buffer with binary semaphores

You are asked to implement a FCFS bounded buffer (represented as linked list) using a binary semaphore (the value cannot exceed 1) to make the producer go to sleep, and a full “counting semaphore” that represents the number of “jobs” in the bounded buffer (note that you may use additional semaphores or mutexes to synchronise critical sections, but not to implement the “sleep mechanisms”). The buffer can contain at most MAX_BUFFER_SIZE (defined in coursework.h) elements, and each element contains one character (a ‘*’ in this case). You are asked to implement a single producer and a single consumer. The producer generates NUMBER_OF_JOBS (defined in coursework.h)  ‘*’ characters, adds them to the end of the buffer, and goes to sleep when the buffer is full. The consumer removes ‘*’ characters from the start of the buffer (provided that "jobs” are available). Each time the producer (consumer) adds (removes) an element, the number of elements currently in the buffer is shown on the screen as a line of stars (see output sample provided on Moodle). Different implementations and synchronisation approaches are possible, however, we are asking you to implement the sleep mechanism for the producer using a binary semaphore (or mutex if you believe this would be a better choice), and use a counting semaphore to represent the number of full buffers in your implementation.

----

## **Task 4**: A Simplified Windows Scheduler

In task 1, it was assumed that all jobs are available at the start. This is usually not the case in real world systems, nor can it be assumed that an infinite number of processes can simultaneously co-exist in an operating system (which typically has an upper limit on the number of processes that can exist at any one time, determined by the size of the process table). 

The goal of this task to implement the process scheduling algorithms from task 1 (FCFS and RR) using multiple bounded buffers (each one representing a different queue at a different priority level) to approximate a Windows Process Scheduling System. To do so, you are asked to extend the code from task 3 to use **multiple consumers with a single producer**. This time, you are allowed to use multiple counting semaphores if you wish to do so. The code must integrate your implementation of FCFS and RR from task 1 and every consumer should have a unique “consumer id”. 

The producer thread is responsible for generating jobs and adding them to the appropriate queues based on their priority (process->iPriority). The consumer thread(s) iterate through the queues in order of priority and remove the first available job from the queues (with 0 representing the highest priority, 31 the lowest priority for MAX_PRIORITY set to 32). The consumer(s) also simulate the jobs “running” on the CPU, using the runJob() function which, depending on the priority level, calls runNonPreemptiveJob()for a FCFS queue (if priority < MAX_PRIORITY / 2) and the runPreemptiveJob()for the RR queues (if MAX_PRIORITY / 2 <= priority). All necessary functions are provided in the coursework.c file. 

The final version of your code should use 32 bounded buffers (for MAX_PRIORITY set to 32). The bounded buffers must be implemented as linked lists (one for each priority level) and the maximum number of elements across all bounded buffers should not exceed MAX_BUFFER_SIZE. Hint: you could use arrays of the size MAX_PRIORITY to keep track of the heads and tails of the different linked lists.

----

## **Task 5**: Pre-emptive FCFS and Priority Boosting

You are asked to extend the code from task 4: (1) to allow the producer thread to pre-empt a FCFS job from running if a higher priority job was generated, and (2) to boost the priority of RR jobs (i.e. implement dynamic priority boosting).

Priority boosting can be implemented as a separate thread that iterates through the different RR priority levels (i.e. 16 to 31). The thread checks the first job in every priority level, and if the job has not run for a predetermined amount of time - called the boost interval, e.g. 100ms, the priority of the job is boosted to the maximum level for RR jobs (level 16 for MAX_PRIORITY set to 32), independent of what the priority of the original job is. The boost interval is determined by the value of the  BOOST_INTERVAL constant defined in the coursework.h file. Note that, to keep things simple, the priority booster loops through the different priority levels and only checks the first job in the respective queue.