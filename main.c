//
// Created by 김영민 on 25. 5. 4.
//

#include "evaluateresults.h"
#include "process.h"
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
    int processNumber = 0;
    scanf("%d",&processNumber);

    if (processNumber>100) {
        perror(" too many process!");
        return -1;
    }
    Process *jobqueue[processNumber];

    for (int i = 0; i < processNumber; i++) {
        jobqueue[i] = createProcess(i + 1);  // PID는 1부터 시작
    }


    printf("Initial Processes (%d): \n", processNumber);
    for (int i = 0; i < processNumber; i++) {
        printf("Process Id %d, Arrival Time %d, CPU Burst Time %d. Priority: %d\n",
               jobqueue[i]->pid,
               jobqueue[i]->arrival_time,
               jobqueue[i]->cpu_burst_time,
               jobqueue[i]->priority);


    }

    printf("----------------------------------\n");

    printf("Select Algorithm\n");
    printf("1- FCFS\n");
    printf("2- SJF\n");
    printf("3- RR\n");
    printf("4- Priority\n");
    printf("5- PreemtiveSJF\n");
    printf("6- PreemtivePriority\n");
    printf("7- RoundRobinWithPriority\n");
    printf("8 - LongestIOFirst\n");
    printf("9 - PreemptiveLongestIOFirst\n");
    printf("10 - MultiLevelQueue\n");
    printf("11 - Priority-based Lottery Scheduling\n");
    printf("12 - LongestIOShortestCPU Scheduling\n");
    printf("13 - PreemptiveLongestIOShortestCPU\n");
    printf("14 - HRRN\n");










    int selectedAlgorithm = 0;

    scanf("%d", &selectedAlgorithm);

    runScheduler(jobqueue, processNumber, selectedAlgorithm);// 스케줄러 run



    printResults(jobqueue, processNumber, selectedAlgorithm);// 결과 출력
    return 0;
}





