//
// Created by 김영민 on 25. 5. 4.
//

#include "scheduler.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

void runScheduler(Process *processes[], int processCount, int algorithm)
{
    resetProcesses(procceses, processCount);

    switch (algorithm) {
    case FCFS:
        printf("FCFS algorithm selected\n");
        runFCFS(processes, processCount);
        break;

    case SJF:
        printf("SJF algorithm selected\n");
        break;

    case RoundRobin:
        printf("RoundRobin algorithm selected\n");
        break;

    default:
        printf("Invalid algorithm selected\n");
        break;
    }
}

void resetProcesses(Process processes[], int processCount)
{
    for (int i = 0; i < processCount; i++) {
        // 남은 실행 시간을 원래 burst 시간으로 복원
        processes[i].remaining_cpu_burst_time = processes[i].cpu_burst_time;
        processes[i].remaining_io_burst_time  = processes[i].io_burst_time; // I/O 시간도 복원

        // 상태를 NEW로 초기화 (스케줄러가 arrivalTime에 맞춰 READY로 변경)
        processes[i].status = NEW;

        // 측정된 메트릭 값들을 초기화 (0 또는 -1 등 초기 상태 값으로)
        processes[i].completion_time = 0; // 또는 -1 (미완료 표시)
        processes[i].waiting_time    = 0;
        processes[i].turnaround_time = 0; // 또는 -1
    }
}

void runFCFS(Process processes[], int processCount)
{
    int      currentTime     = 0;
    int      terminatedCount = 0;
    Queue    readyQueue;
    Process *runningProcess = NULL;

    initializeQueue(&readyQueue);

    printf("FCFS Simulation STARTED-----------\n");

    while (terminatedCount < processCount) {
        // 도착 프로세스 확인 및 ready queue 삽입
        for (int i = 0; i < processCount; i++) {
            if (processes[i].status == NEW && processes[i].arrival_time <= currentTime) {
                processes[i].status = READY;
            }
        }
    }
    // cpu 할당 결정
    if (runningProcess == NULL && !isEmpty(&readyQueue)) {
        runningProcess         = dequeue(&readyQueue);
        runningProcess->status = RUNNING;
    }
    // cpu 할당 및 시간 흐름
    if (runningProcess != NULL) {
        runningProcess->remaining_cpu_burst_time--;
        // 프로세스 종료되는거 확인
        if (runningProcess->remaining_cpu_burst_time == 0) {
            runningProcess->status          = TERMINATED;
            runningProcess->completion_time = currentTime + 1;
            terminatedCount++;
            runningProcess = null;
        }
    }

    printf("FCFS Simulation COMPLETED-----------\n");
