#include "evaluateresults.h"
#include <stdio.h>

void calculateMetrics(Process *process)
{
    if (process->status == TERMINATED) {
        process->turnaround_time = process->completion_time - process->arrival_time;
        process->waiting_time    = process->turnaround_time - process->cpu_burst_time;
        if (process->waiting_time < 0)
            process->waiting_time = 0;

    } else {
        // 종료되지 않은 프로세스가 이 함수를 불렀다는 것 자체가 오류 상황
        process->turnaround_time = -1;
        process->waiting_time    = -1;
    }
}

void printResults(Process processes[], int processCount,int algorithm)
{
    int totalWaitingTime    = 0;
    int totalTurnaroundTime = 0;
    const char* algorithmNames[] = {
        "",         // 인덱스 0은 사용 안 함 (선택적)
        "FCFS",     // 인덱스 1 (ALG_FCFS)
        "SJF",      // 인덱스 2 (ALG_SJF)
        "RR"        // 인덱스 3 (ALG_RR)
        // ... 다른 알고리즘 이름 추가 ...
    };
    printf("\n--- Scheduling Results for %s ---\n", algorithmNames[algorithm]);
    printf("PID\tArrivalTime\tCPUBurstTime\tCompletionTime\tWaitingTime\tTurnaroundTime\n");

    for (int i = 0; i < processCount; i++) {
        calculateMetrics(&processes[i]);

        printf("%d\t%d\t%d\t%d\t\t%d\t%d\t\n",
               processes[i].pid,
               processes[i].arrival_time,
               processes[i].cpu_burst_time,
               processes[i].completion_time,
               processes[i].waiting_time,
               processes[i].turnaround_time);
        if (processes[i].status == TERMINATED) {
            totalWaitingTime += processes[i].waiting_time;
            totalTurnaroundTime += processes[i].turnaround_time;
        }
    }

    if (processCount > 0) {
        printf("\nAverage Waiting Time: %.2f\n", (double)totalWaitingTime / processCount);
        printf("Average Turnaround Time: %.2f\n", (double)totalTurnaroundTime / processCount);
    }

    printf("---------------------------------------\n");
}