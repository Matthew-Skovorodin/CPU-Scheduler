# CPU-Scheduler
This program implements a CPU scheduling simulation with four scheduling algorithms: FCFS, SJF, PR, and RR.

## Command-line Operation
The program takes the name of the scheduling algorithm, related parameters (if any), and an input file name from command line.
Here is how your program should be invoked:
./exec -alg [FCFS|SJF|PR|RR] [-quantum [integer(ms)]] -input [filename]

## Input File Format
The input file provides information about when simulated processes arrive in the system, along with their priority and CPU and I/O burst lengths. The file is formatted such that each line starts with one of the following keywords: proc, sleep, or stop.

## Input Parsing
One thread is responsible for reading the input file. This corresponds to user activity causing new processes to come into existence. Any time this thread sees a proc line, it should create a new ‘process’ and put it in the ready queue. The ‘process’ is not a real process, it just represents the characteristics of the process to simulate. When the thread encounters a sleep X it should go to sleep for X milliseconds, before going back to reading the file. This thread quits once it reads stop.

## CPU Thread
This thread is analogous to the kernel’s CPU scheduler. Its job is to check the ready queue for processes and, if there are any, pick one from the queue according to the designated scheduling algorithm and reserve CPU time for the designated burst (or quantum) time. This means that the thread will sleep for an appropriate number of milliseconds. Once the thread wakes back up it either:
1. Moves the process to the I/O queue
2. Returns it to the ready queue (if using Round Robin and the quantum expired)
3. Terminates the process if it completed its last CPU burst Then, schedule another process from the ready queue.

## I/O Thread
This thread simulates the I/O subsystem. If there is anything waiting in the I/O queue, select a process in FIFO order. Sleep for the given I/O burst time (to simulate waiting on an I/O device) and then put the process back in the ready queue. Repeat until there are no more jobs.

## main() Thread
The main thread (i.e. the one in which main runs) is responsible for setting everything up and then letting the other threads run. It should wait until the file reading thread is done, and then wait for the ready and I/O queues to empty out. Once that happens, it can print the performance evaluation results and terminate the program.
