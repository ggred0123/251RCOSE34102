#include "evaluateresults.h"
#include <stdio.h>

void calculateMetrics(Process* process)
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

void printResults(Process* processes[], int processCount,int algorithm)
{
    int totalWaitingTime    = 0;
    int totalTurnaroundTime = 0;
    const char* algorithmNames[] = {
        "",         // 인덱스 0은 사용 안 함 (선택적)
        "FCFS",     // 인덱스 1 (ALG_FCFS)
        "SJF",      // 인덱스 2 (ALG_SJF)
        "RR",        // 인덱스 3 (ALG_RR)
        "Priority",
        "PreemptiveSJF",
        "PreemptivePriority",
        "RoundRobinWithPriority",
        "LongestIOFirst",
        "PreemptiveLongestIOFirst",
        "MultiLevelQueue",
        "LOTTERY",
        "LongestIOShortestCPU",
        "PreemptiveLongestIOShortestCPU",
        "HRRN",



    };
    printf("\n--- Scheduling Results for %s ---\n", algorithmNames[algorithm]);
    printf("PID\tArrival\tCPU_Burst\tIO_Count\tIO_Burst\tPriority\tStatus\tCompletion\tWaiting\tTurnaround\tRemaining_CPU\tCPU_Used\n");

    for (int i = 0; i < processCount; i++) {
        calculateMetrics(processes[i]);

        // 프로세스 상태를 문자열로 변환
        const char* status_str;
        switch(processes[i]->status) {
            case NEW: status_str = "NEW"; break;
            case READY: status_str = "READY"; break;
            case RUNNING: status_str = "RUNNING"; break;
            case WAITING: status_str = "WAITING"; break;
            case TERMINATED: status_str = "TERMINATED"; break;
            default: status_str = "UNKNOWN"; break;
        }
        
        printf("%d\t%d\t%d\t%d\t%d\t%d\t%s\t%d\t\t%d\t%d\t\t%d\t\t%d\n",
               processes[i]->pid,
               processes[i]->arrival_time,
               processes[i]->cpu_burst_time,
               processes[i]->io_count,
               processes[i]->io_burst_time,
               processes[i]->priority,
               status_str,
               processes[i]->completion_time,
               processes[i]->waiting_time,
               processes[i]->turnaround_time,
               processes[i]->remaining_cpu_burst_time,
               processes[i]->cpu_time_used);
        if (processes[i]->status == TERMINATED) {
            totalWaitingTime += processes[i]->waiting_time;
            totalTurnaroundTime += processes[i]->turnaround_time;
        }
    }

    if (processCount > 0) {
        printf("\nAverage Waiting Time: %.2f\n", (double)totalWaitingTime / processCount);
        printf("Average Turnaround Time: %.2f\n", (double)totalTurnaroundTime / processCount);
    }

    printf("---------------------------------------\n");
}