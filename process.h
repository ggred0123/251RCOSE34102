#include <stdio.h>
#include <stdlib.h>
#ifndef PROCESS_H
#define PROCESS_H

typedef enum { NEW, READY, RUNNING, WAITING, TERMINATED } Status;

typedef struct Process
{
    int    pid; // processId
    int    arrival_time;
    int    cpu_burst_time;           // cpu burst time
    int    remaining_cpu_burst_time; // 남아있는 cpu burst
    int    io_burst_time;
    int    remaining_io_burst_time;
    Status status;
    int    waiting_time;
    int    turnaround_time;
    int    completion_time;
} Process;

#endif // PROCESS_H
