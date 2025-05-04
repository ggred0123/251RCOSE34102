//
// Created by 김영민 on 25. 5. 4.
//

#include "evaluateresults.h"
#include "process.h"
#include "scheduler.h"
#include <stdio.h>

int main()
{
    Process processes[] = {
        // pid, arrivalTime, cpuBurstTime
        {1, 0, 7},
        {2, 2, 4},
        {3, 4, 1},
        {4, 5, 4}
    };

    int processCount = sizeof(processes) / sizeof(processes[0]);

    printf("Initial Processes (%d): \n", processCount);
    for (int i = 0; i < processCount; i++) {
        printf("Process Id %d, Arrival Time %d, CPU Burst Time %d: \n",
               processes[i].pid,
               processes[i].arrival_time,
               processes[i].cpu_burst_time);

    }

    printf("----------------------------------\n");

    printf("Select Algorithm\n");
    printf("1- FCFS\n");
    printf("2- SJF\n");
    printf("3- RR\n");
    int selectedAlgorithm = 0;

    scanf("%d", &selectedAlgorithm);

    runScheduler(processes, processCount, selectedAlgorithm);

    return 0;
}





