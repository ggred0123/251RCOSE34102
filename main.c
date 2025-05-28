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
    printf("Enter the number of processes: ");
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
/*
    Process processes[5] = {
    // P1
    {
        .pid = 1,
        .arrival_time = 0,
        .cpu_burst_time = 8,
        .remaining_cpu_burst_time = 8,
        .time_entered_ready = 0,
        .io_burst_time = 1,
        .remaining_io_burst_time = 1,
        .current_io_index = 0,
        .cpu_time_used = 0,
        .io_count = 2,
        .io_trigger = {3, 5, 0},
        .io_burst_times = {1, 3, 0},
        .status = NEW,
        .waiting_time = 0,
        .turnaround_time = 0,
        .completion_time = 0,
        .priority = 2
    },

    // P2
    {
        .pid = 2,
        .arrival_time = 1,
        .cpu_burst_time = 4,
        .remaining_cpu_burst_time = 4,
        .time_entered_ready = 1,
        .io_burst_time = 0,
        .remaining_io_burst_time = 0,
        .current_io_index = 0,
        .cpu_time_used = 0,
        .io_count = 0,
        .io_trigger = {0, 0, 0},
        .io_burst_times = {0, 0, 0},
        .status = NEW,
        .waiting_time = 0,
        .turnaround_time = 0,
        .completion_time = 0,
        .priority = 1
    },

    // P3
    {
        .pid = 3,
        .arrival_time = 2,
        .cpu_burst_time = 12,
        .remaining_cpu_burst_time = 12,
        .time_entered_ready = 2,
        .io_burst_time = 2,
        .remaining_io_burst_time = 2,
        .current_io_index = 0,
        .cpu_time_used = 0,
        .io_count = 1,
        .io_trigger = {5, 0, 0},
        .io_burst_times = {2, 0, 0},
        .status = NEW,
        .waiting_time = 0,
        .turnaround_time = 0,
        .completion_time = 0,
        .priority = 4
    },

    // P4
    {
        .pid = 4,
        .arrival_time = 4,
        .cpu_burst_time = 6,
        .remaining_cpu_burst_time = 6,
        .time_entered_ready = 4,
        .io_burst_time = 0,
        .remaining_io_burst_time = 0,
        .current_io_index = 0,
        .cpu_time_used = 0,
        .io_count = 0,
        .io_trigger = {0, 0, 0},
        .io_burst_times = {0, 0, 0},
        .status = NEW,
        .waiting_time = 0,
        .turnaround_time = 0,
        .completion_time = 0,
        .priority = 3
    },

    // P5
    {
        .pid = 5,
        .arrival_time = 6,
        .cpu_burst_time = 3,
        .remaining_cpu_burst_time = 3,
        .time_entered_ready = 6,
        .io_burst_time = 1,
        .remaining_io_burst_time = 1,
        .current_io_index = 0,
        .cpu_time_used = 0,
        .io_count = 2,
        .io_trigger = {1, 2, 0},
        .io_burst_times = {1, 3, 0},
        .status = NEW,
        .waiting_time = 0,
        .turnaround_time = 0,
        .completion_time = 0,
        .priority = 2
    }
};

    Process *jobqueue[5];

    for (int i = 0; i < 5; i++) {
        jobqueue[i] = &processes[i];
    }
    int processNumber = 5;*/


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
    printf("15 - MultiLevelFeedbackQueue\n");
    printf("16 - NonpreemptiveAgingPriority\n");
    printf("17 - PreemptiveAgingPriority\n");
    printf("18 - NonPreemptiveStride\n");
    printf("19 - PreemptiveAgingStride\n");
    printf("20 - Run All and Compare Performance\n");






    int selectedAlgorithm = 0;



    scanf("%d", &selectedAlgorithm);
    if (selectedAlgorithm==20) {
        printAllResults(jobqueue, processNumber); // 모든 알고리즘 실행 및 결과 분석
    }
    else {
        runScheduler(jobqueue, processNumber, selectedAlgorithm);// 스케줄러 run

        printResults(jobqueue, processNumber, selectedAlgorithm);// 결과 출력
    }
    return 0;
}





