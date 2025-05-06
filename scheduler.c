//
// Created by 김영민 on 25. 5. 4.
//

#include "scheduler.h"
#include "ganttchart.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

void runFCFS(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // 간트 차트 초기화 - 시작은 IDLE 상태
    int previousPID = -1; // IDLE 상태의 PID
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    printf("FCFS Simulation STARTED-----------\n");

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount) {
        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                if (!enqueue(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);
                }
                printf("Time %d: Process %d ARRIVED\n", currentTime, processes[i]->pid);
            }
        }

        // I/O 진행 및 완료 처리
        IO_Operation(&readyQueue, &waitQueue);

        // CPU 할당
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue);
            if (runningProcess) {
                runningProcess->status = RUNNING;
                printf("Time %d: Process %d RUNNING\n", currentTime, runningProcess->pid);
                
                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (previousPID == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                    
                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                }
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);
                
                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;
                
                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;
                
                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;
                
                printf("Time %d: Process %d requests I/O (Duration %d)\n", 
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);
                
                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;
                
                // I/O 상태 시작 - 다음 상태는 IDLE
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;
                
                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }
                
                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
        }
        
        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목)
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            // 유효한 로그만 유지
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    printf("FCFS Simulation COMPLETED at time %d-----------\n", currentTime);
    printf("Valid gantt chart logs: %d\n", validLogs);
    
    drawGanttChart(ganttLogArray, validLogs);
}



void runNonPreemptiveSJF(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // 간트 차트 초기화 - 시작은 IDLE 상태
    int previousPID = -1; // IDLE 상태의 PID
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    printf("Non-Preemptive SJF Simulation STARTED-----------\n");

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount) {
        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                // 일반 enqueue 대신 SJF용 enqueue 사용 - 이 부분이 FCFS와 다른 점
                if (!enqueue_for_sjf(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);
                }
                printf("Time %d: Process %d ARRIVED (Burst Time: %d)\n",
                       currentTime, processes[i]->pid, processes[i]->remaining_cpu_burst_time);
            }
        }

        // I/O 진행 및 완료 처리
        IO_Operation(&readyQueue, &waitQueue);

        // CPU 할당 - 일반 FCFS와 동일하게 큐의 첫 번째 프로세스를 선택
        // (enqueue_for_sjf에 의해 이미 짧은 작업 순으로 정렬되어 있음)
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue);

            if (runningProcess) {
                runningProcess->status = RUNNING;
                printf("Time %d: Process %d RUNNING (Remaining Burst: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->remaining_cpu_burst_time);

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (previousPID == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                }
            }
        }

        // CPU 실행 - FCFS와 동일한 로직
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // I/O 상태 시작 - 다음 상태는 IDLE
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                // I/O 대기 큐에 넣을 때도 일반 enqueue 사용 (waitQueue는 FCFS 방식으로 처리)
                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목)
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            // 유효한 로그만 유지
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    printf("Non-Preemptive SJF Simulation COMPLETED at time %d-----------\n", currentTime);
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(ganttLogArray, validLogs);
}


void runRoundRobin(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;
    int timeQuantum; // 라운드 로빈의 time quantum 값
    int quantumCounter = 0; // 현재 프로세스의 quantum 사용량 카운터

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // Time Quantum 입력 받기
    printf("Enter time quantum for Round Robin: ");
    scanf("%d", &timeQuantum);
    if (timeQuantum <= 0) {
        printf("Invalid time quantum. Setting to default value 1.\n");
        timeQuantum = 1;
    }

    // 간트 차트 초기화 - 시작은 IDLE 상태
    int previousPID = -1; // IDLE 상태의 PID
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    printf("Round Robin Simulation (Quantum=%d) STARTED-----------\n", timeQuantum);

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount) {
        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                if (!enqueue(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);
                }
                printf("Time %d: Process %d ARRIVED\n", currentTime, processes[i]->pid);
            }
        }

        // I/O 진행 및 완료 처리
        IO_Operation(&readyQueue, &waitQueue);

        // 현재 실행 중인 프로세스가 Time Quantum을 모두 사용했으면 preemption
        if (runningProcess != NULL && quantumCounter >= timeQuantum) {
            printf("Time %d: Process %d PREEMPTED (Quantum expired)\n", currentTime, runningProcess->pid);

            // 현재 프로세스 로그 종료
            ganttLogArray[ganttEntryCount-1].endTime = currentTime;

            // 프로세스를 다시 Ready Queue에 넣음 (선점)
            runningProcess->status = READY;
            if (!enqueue(&readyQueue, runningProcess)) {
                fprintf(stderr, "[Error] Ready queue full during preemption at time %d!\n", currentTime);
            }

            runningProcess = NULL;
            quantumCounter = 0;

            // 다음 상태는 IDLE로 설정 (다음 루프에서 프로세스가 할당되면 변경됨)
            ganttLogArray[ganttEntryCount].startTime = currentTime;
            ganttLogArray[ganttEntryCount].pid = -1; // IDLE
            ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
            ganttEntryCount++;
            previousPID = -1;
        }

        // CPU 할당 (이전 프로세스가 종료되었거나 Time Quantum을 모두 사용했을 때)
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue);
            quantumCounter = 0; // 새 프로세스가 할당되면 quantum 카운터 초기화

            if (runningProcess) {
                runningProcess->status = RUNNING;
                printf("Time %d: Process %d RUNNING (Remaining Burst: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->remaining_cpu_burst_time);

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (previousPID == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                } else if (previousPID != runningProcess->pid) {
                    // 이전과 다른 프로세스라면 새 로그 항목 추가
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0;
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                }
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;
            quantumCounter++; // time quantum 사용량 증가

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                quantumCounter = 0;
                previousPID = -1; // IDLE 상태로 변경
            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // I/O 상태 시작 - 다음 상태는 IDLE
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }

                runningProcess = NULL;
                quantumCounter = 0;
                previousPID = -1; // IDLE 상태로 변경
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (가끔 이런게 생김)
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            // 유효한 로그만 유지
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    printf("Round Robin Simulation COMPLETED at time %d-----------\n", currentTime);
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(ganttLogArray, validLogs);
}


void runPriority(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // 간트 차트 초기화 - 시작은 IDLE 상태
    int previousPID = -1; // IDLE 상태의 PID
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    printf("Priority Scheduling Simulation STARTED-----------\n");
    printf("Note: Lower priority value means higher priority\n");

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount) {
        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                // 일반 enqueue 대신 우선순위 기반 enqueue 사용
                if (!enqueue_for_priority(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);
                }
                printf("Time %d: Process %d ARRIVED (Priority: %d)\n",
                       currentTime, processes[i]->pid, processes[i]->priority);
            }
        }

        // I/O 진행 및 완료 처리
        IO_Operation_Priority(&readyQueue, &waitQueue);

        // CPU 할당 - 일반 FCFS와 동일하게 큐의 첫 번째 프로세스를 선택
        // (enqueue_priority에 의해 이미 우선순위가 높은 순으로 정렬되어 있음)
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue);

            if (runningProcess) {
                runningProcess->status = RUNNING;
                printf("Time %d: Process %d RUNNING (Priority: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->priority);

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (previousPID == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                }
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // I/O 상태 시작 - 다음 상태는 IDLE
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                // I/O 대기 큐에 넣을 때는 일반 enqueue 사용 (waitQueue는 FCFS 방식으로 처리)
                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목)
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            // 유효한 로그만 유지
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    printf("Priority Scheduling Simulation COMPLETED at time %d-----------\n", currentTime);
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(ganttLogArray, validLogs);
}


void runPreemptiveSJF(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // 간트 차트 초기화 - 시작은 IDLE 상태
    int previousPID = -1; // IDLE 상태의 PID
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    printf("Preemptive SJF (SRTF) Simulation STARTED-----------\n");

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount) {
        bool processArrived = false;

        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                // enqueue_for_sjf 함수를 사용하여 남은 CPU 버스트 시간 순서로 정렬
                if (!enqueue_for_sjf(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);
                }
                printf("Time %d: Process %d ARRIVED (Remaining Burst: %d)\n",
                       currentTime, processes[i]->pid, processes[i]->remaining_cpu_burst_time);
                processArrived = true;
            }
        }

        // I/O 진행 및 완료 처리 - 제공된 IO_Operation_SJF 함수 사용
        int initialWaitCount = waitQueue.count;
        IO_Operation_SJF(&readyQueue, &waitQueue);
        bool ioCompleted = (initialWaitCount > waitQueue.count);

        // 선점 판단: 새 프로세스 도착하거나 I/O가 완료되면 현재 실행 중인 프로세스와 비교
        if ((processArrived || ioCompleted) && runningProcess != NULL) {
            // 레디 큐에 프로세스가 있고, 맨 앞 프로세스가 현재 실행 중인 프로세스보다 짧으면 선점
            if (!isEmpty(&readyQueue)) {
                // readyQueue의 맨 앞 프로세스 확인 (dequeue 없이)
                Process *shortestProcess = readyQueue.items[readyQueue.front];

                if (shortestProcess &&
                    shortestProcess->remaining_cpu_burst_time < runningProcess->remaining_cpu_burst_time) {
                    printf("Time %d: Process %d PREEMPTED by Process %d (Remaining: %d vs %d)\n",
                           currentTime, runningProcess->pid, shortestProcess->pid,
                           runningProcess->remaining_cpu_burst_time, shortestProcess->remaining_cpu_burst_time);

                    // RUNNING->READY 현재 프로세스를 READY 상태로 변경하고 레디 큐에 추가 (SJF 순서로)
                    runningProcess->status = READY;
                    if (!enqueue_for_sjf(&readyQueue, runningProcess)) {
                        fprintf(stderr, "[Error] Ready queue full during preemption at time %d!\n", currentTime);
                    }

                    // 간트 차트 업데이트: 현재 실행 중이던 프로세스의 로그 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 디큐
                    runningProcess = dequeue(&readyQueue);

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;

                    // 새 프로세스 실행
                    runningProcess->status = RUNNING;
                    previousPID = runningProcess->pid;

                    printf("Time %d: Process %d RUNNING (Remaining Burst: %d)\n",
                           currentTime, runningProcess->pid, runningProcess->remaining_cpu_burst_time);
                }
            }
        }
        // CPU가 비어있고 레디 큐에 프로세스가 있는 경우 CPU 할당
        else if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            // 레디 큐에서 프로세스 가져오기 (enqueue_for_sjf에 의해 이미 정렬되어 있음)
            runningProcess = dequeue(&readyQueue);

            if (runningProcess) {
                runningProcess->status = RUNNING;
                printf("Time %d: Process %d RUNNING (Remaining Burst: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->remaining_cpu_burst_time);

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (previousPID == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                }
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // I/O 상태 시작 - 다음 상태는 IDLE
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목)
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            // 유효한 로그만 유지
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    printf("Preemptive SJF (SRTF) Simulation COMPLETED at time %d-----------\n", currentTime);
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(ganttLogArray, validLogs);
}


void runPreemptivePriority(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // 간트 차트 초기화 - 시작은 IDLE 상태
    int previousPID = -1; // IDLE 상태의 PID
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    printf("Preemptive Priority Scheduling Simulation STARTED-----------\n");
    printf("Note: Lower priority value means higher priority\n");

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount) {
        bool processArrived = false;

        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                // enqueue_for_priority 함수를 사용하여 우선순위 순서로 정렬
                if (!enqueue_for_priority(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);
                }
                printf("Time %d: Process %d ARRIVED (Priority: %d)\n",
                       currentTime, processes[i]->pid, processes[i]->priority);
                processArrived = true;
            }
        }

        // I/O 진행 및 완료 처리 - 제공된 IO_Operation_Priority 함수 사용
        int initialWaitCount = waitQueue.count;
        IO_Operation_Priority(&readyQueue, &waitQueue);
        bool ioCompleted = (initialWaitCount > waitQueue.count);

        // 선점 판단: 새 프로세스 도착하거나 I/O가 완료되면 현재 실행 중인 프로세스와 비교
        if ((processArrived || ioCompleted) && runningProcess != NULL) {
            // 레디 큐에 프로세스가 있고, 맨 앞 프로세스가 현재 실행 중인 프로세스보다 우선순위가 높으면 선점
            if (!isEmpty(&readyQueue)) {
                // readyQueue의 맨 앞 프로세스 확인 (dequeue 없이)
                Process *highestPriorityProcess = readyQueue.items[readyQueue.front];

                if (highestPriorityProcess &&
                    highestPriorityProcess->priority < runningProcess->priority) {
                    printf("Time %d: Process %d PREEMPTED by Process %d (Priority: %d vs %d)\n",
                           currentTime, runningProcess->pid, highestPriorityProcess->pid,
                           runningProcess->priority, highestPriorityProcess->priority);

                    // 현재 프로세스를 READY 상태로 변경하고 레디 큐에 추가 (우선순위 순서로)
                    runningProcess->status = READY;
                    if (!enqueue_for_priority(&readyQueue, runningProcess)) {
                        fprintf(stderr, "[Error] Ready queue full during preemption at time %d!\n", currentTime);
                    }

                    // 간트 차트 업데이트: 현재 실행 중이던 프로세스의 로그 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 디큐
                    runningProcess = dequeue(&readyQueue);

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;

                    // 새 프로세스 실행
                    runningProcess->status = RUNNING;
                    previousPID = runningProcess->pid;

                    printf("Time %d: Process %d RUNNING (Priority: %d)\n",
                           currentTime, runningProcess->pid, runningProcess->priority);
                }
            }
        }
        // CPU가 비어있고 레디 큐에 프로세스가 있는 경우 CPU 할당
        else if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            // 레디 큐에서 프로세스 가져오기 (enqueue_for_priority에 의해 이미 정렬되어 있음)
            runningProcess = dequeue(&readyQueue);

            if (runningProcess) {
                runningProcess->status = RUNNING;
                printf("Time %d: Process %d RUNNING (Priority: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->priority);

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (previousPID == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                }
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // I/O 상태 시작 - 다음 상태는 IDLE
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목)
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            // 유효한 로그만 유지
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    printf("Preemptive Priority Simulation COMPLETED at time %d-----------\n", currentTime);
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(ganttLogArray, validLogs);
}

void runRoundRobinWithPriority(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;
    int timeQuantum; // 라운드 로빈의 time quantum 값
    int quantumCounter = 0; // 현재 프로세스의 quantum 사용량 카운터

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // Time Quantum 입력 받기
    printf("Enter time quantum for Round Robin with Priority: ");
    scanf("%d", &timeQuantum);
    if (timeQuantum <= 0) {
        printf("Invalid time quantum. Setting to default value 1.\n");
        timeQuantum = 1;
    }

    // 간트 차트 초기화 - 시작은 IDLE 상태
    int previousPID = -1; // IDLE 상태의 PID
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    printf("Round Robin with Priority Simulation (Quantum=%d) STARTED-----------\n", timeQuantum);
    printf("Note: Lower priority value means higher priority\n");

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount) {
        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                // 우선순위 기반으로 큐에 삽입
                if (!enqueue_for_priority(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);
                }
                printf("Time %d: Process %d ARRIVED (Priority: %d)\n",
                       currentTime, processes[i]->pid, processes[i]->priority);
            }
        }

        // I/O 진행 및 완료 처리
        IO_Operation_Priority(&readyQueue, &waitQueue);

        // 현재 실행 중인 프로세스가 Time Quantum을 모두 사용했거나,
        // 더 높은 우선순위의 프로세스가 레디 큐에 있는지 확인
        if (runningProcess != NULL) {
            bool preempt = false;

            // Time Quantum 만료 확인
            if (quantumCounter >= timeQuantum) {
                printf("Time %d: Process %d QUANTUM EXPIRED\n", currentTime, runningProcess->pid);
                preempt = true;
            }
            // 더 높은 우선순위 프로세스 확인 (같은 우선순위는 선점하지 않음)
            else if (!isEmpty(&readyQueue) &&
                     readyQueue.items[readyQueue.front]->priority < runningProcess->priority) {
                Process *highestPriorityProcess = readyQueue.items[readyQueue.front];
                printf("Time %d: Process %d PREEMPTED by higher priority Process %d (Priority: %d vs %d)\n",
                       currentTime, runningProcess->pid, highestPriorityProcess->pid,
                       runningProcess->priority, highestPriorityProcess->priority);
                preempt = true;
            }

            // 선점 처리
            if (preempt) {
                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                // 프로세스를 다시 Ready Queue에 넣음 (선점)
                runningProcess->status = READY;
                if (!enqueue_for_priority(&readyQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Ready queue full during preemption at time %d!\n", currentTime);
                }

                runningProcess = NULL;
                quantumCounter = 0;

                // 다음 상태는 IDLE로 설정 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;
                previousPID = -1;
            }
        }

        // CPU 할당 (이전 프로세스가 종료되었거나 선점되었을 때)
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue);
            quantumCounter = 0; // 새 프로세스가 할당되면 quantum 카운터 초기화

            if (runningProcess) {
                runningProcess->status = RUNNING;
                printf("Time %d: Process %d RUNNING (Priority: %d, Remaining Burst: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->priority,
                       runningProcess->remaining_cpu_burst_time);

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (previousPID == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                } else if (previousPID != runningProcess->pid) {
                    // 이전과 다른 프로세스라면 새 로그 항목 추가
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0;
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                }
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;
            quantumCounter++; // time quantum 사용량 증가

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                quantumCounter = 0;
                previousPID = -1; // IDLE 상태로 변경
            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // I/O 상태 시작 - 다음 상태는 IDLE
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }

                runningProcess = NULL;
                quantumCounter = 0;
                previousPID = -1; // IDLE 상태로 변경
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목)
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            // 유효한 로그만 유지
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    printf("Round Robin with Priority Simulation COMPLETED at time %d-----------\n", currentTime);
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(ganttLogArray, validLogs);
}

void runLongestIOFirst(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // 간트 차트 초기화 - 시작은 IDLE 상태
    int previousPID = -1; // IDLE 상태의 PID
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    printf("Longest IO First Scheduling Simulation STARTED-----------\n");

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount) {
        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;

                // 총 IO 시간 계산
                int total_io_time = 0;
                for (int j = 0; j < processes[i]->io_count; j++) {
                    total_io_time += processes[i]->io_burst_times[j];
                }

                // enqueue_for_lif 함수 사용
                if (!enqueue_for_lif(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);
                }
                printf("Time %d: Process %d ARRIVED (Total IO Time: %d)\n",
                       currentTime, processes[i]->pid, total_io_time);
            }
        }

        // I/O 진행 및 완료 처리
        IO_Operation_LIF(&readyQueue, &waitQueue);

        // CPU 할당 - 큐의 첫 번째 프로세스 선택 (이미 LIF 순서로 정렬됨)
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue);

            if (runningProcess) {
                runningProcess->status = RUNNING;

                // 총 IO 시간 계산
                int total_io_time = 0;
                for (int i = 0; i < runningProcess->io_count; i++) {
                    total_io_time += runningProcess->io_burst_times[i];
                }

                printf("Time %d: Process %d RUNNING (Total IO Time: %d)\n",
                       currentTime, runningProcess->pid, total_io_time);

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (previousPID == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                }
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // I/O 상태 시작 - 다음 상태는 IDLE
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목)
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            // 유효한 로그만 유지
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    printf("Longest IO First Simulation COMPLETED at time %d-----------\n", currentTime);
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(ganttLogArray, validLogs);
}


// 선점형 Longest IO First 알고리즘
void runPreemptiveLongestIOFirst(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // 간트 차트 초기화 - 시작은 IDLE 상태
    int previousPID = -1; // IDLE 상태의 PID
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    printf("Preemptive Longest IO First Scheduling Simulation STARTED-----------\n");

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount) {
        bool processArrived = false;

        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;

                // 총 IO 시간 계산
                int total_io_time = 0;
                for (int j = 0; j < processes[i]->io_count; j++) {
                    total_io_time += processes[i]->io_burst_times[j];
                }

                // enqueue_for_lif 함수 사용
                if (!enqueue_for_lif(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);
                }
                printf("Time %d: Process %d ARRIVED (Total IO Time: %d)\n",
                       currentTime, processes[i]->pid, total_io_time);
                processArrived = true;
            }
        }

        // I/O 진행 및 완료 처리
        int initialWaitCount = waitQueue.count;
        IO_Operation_LIF(&readyQueue, &waitQueue);
        bool ioCompleted = (initialWaitCount > waitQueue.count);

        // 선점 판단: 새 프로세스 도착하거나 I/O가 완료되면 현재 실행 중인 프로세스와 비교
        if ((processArrived || ioCompleted) && runningProcess != NULL) {
            // 레디 큐에 프로세스가 있는지 확인
            if (!isEmpty(&readyQueue)) {
                // readyQueue의 맨 앞 프로세스 확인 (dequeue 없이)
                Process *highestIOProcess = readyQueue.items[readyQueue.front];

                if (highestIOProcess) {
                    // 현재 실행 중인 프로세스의 총 IO 시간 계산
                    int running_total_io_time = 0;
                    for (int i = 0; i < runningProcess->io_count; i++) {
                        running_total_io_time += runningProcess->io_burst_times[i];
                    }

                    // 레디 큐 첫 번째 프로세스의 총 IO 시간 계산
                    int highest_total_io_time = 0;
                    for (int i = 0; i < highestIOProcess->io_count; i++) {
                        highest_total_io_time += highestIOProcess->io_burst_times[i];
                    }

                    // 레디 큐의 프로세스가 현재 실행 중인 프로세스보다 IO 시간이 길면 선점
                    if (highest_total_io_time > running_total_io_time) {
                        printf("Time %d: Process %d PREEMPTED by Process %d (IO Time: %d vs %d)\n",
                               currentTime, runningProcess->pid, highestIOProcess->pid,
                               running_total_io_time, highest_total_io_time);

                        // 현재 프로세스를 READY 상태로 변경하고 레디 큐에 추가
                        runningProcess->status = READY;
                        if (!enqueue_for_lif(&readyQueue, runningProcess)) {
                            fprintf(stderr, "[Error] Ready queue full during preemption at time %d!\n", currentTime);
                        }

                        // 간트 차트 업데이트: 현재 실행 중이던 프로세스의 로그 종료
                        ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                        // 새 프로세스 디큐
                        runningProcess = dequeue(&readyQueue);

                        // 새 프로세스 로그 시작
                        ganttLogArray[ganttEntryCount].startTime = currentTime;
                        ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                        ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                        ganttEntryCount++;

                        // 새 프로세스 실행
                        runningProcess->status = RUNNING;
                        previousPID = runningProcess->pid;

                        printf("Time %d: Process %d RUNNING (Total IO Time: %d)\n",
                               currentTime, runningProcess->pid, highest_total_io_time);
                    }
                }
            }
        }
        // CPU가 비어있고 레디 큐에 프로세스가 있는 경우 CPU 할당
        else if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            // 레디 큐에서 프로세스 가져오기 (enqueue_for_lif에 의해 이미 정렬되어 있음)
            runningProcess = dequeue(&readyQueue);

            if (runningProcess) {
                runningProcess->status = RUNNING;

                // 총 IO 시간 계산
                int total_io_time = 0;
                for (int i = 0; i < runningProcess->io_count; i++) {
                    total_io_time += runningProcess->io_burst_times[i];
                }

                printf("Time %d: Process %d RUNNING (Total IO Time: %d)\n",
                       currentTime, runningProcess->pid, total_io_time);

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (previousPID == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                }
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // I/O 상태 시작 - 다음 상태는 IDLE
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목)
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            // 유효한 로그만 유지
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    printf("Preemptive Longest IO First Simulation COMPLETED at time %d-----------\n", currentTime);
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(ganttLogArray, validLogs);
}


void runMultiLevelQueue(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;
    int timeQuantum; // 라운드 로빈의 time quantum 값 (foreground 큐에서 사용)
    int quantumCounter = 0; // 현재 프로세스의 quantum 사용량 카운터
    int currentQueue = -1; // 현재 서비스 중인 큐 (0: foreground, 1: background)

    // 두 개의 큐(foreground, background)를 위한 레디 큐와 대기 큐
    Queue foregroundReadyQueue;
    Queue backgroundReadyQueue;
    Queue foregroundWaitQueue;
    Queue backgroundWaitQueue;

    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // Time Quantum 입력 받기 (foreground 큐용)
    printf("Enter time quantum for foreground queue: ");
    scanf("%d", &timeQuantum);
    if (timeQuantum <= 0) {
        printf("Invalid time quantum. Setting to default value 2.\n");
        timeQuantum = 2;
    }

    // 간트 차트 초기화 - 시작은 IDLE 상태
    int previousPID = -1; // IDLE 상태의 PID
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&foregroundReadyQueue);
    initialize_queue(&backgroundReadyQueue);
    initialize_queue(&foregroundWaitQueue);
    initialize_queue(&backgroundWaitQueue);

    printf("Multi-Level Queue Scheduling Simulation STARTED-----------\n");
    printf("Foreground queue (priority <= %d): Round Robin (quantum=%d)\n", PRIORITY_DIVIDER/2, timeQuantum);
    printf("Background queue (priority > %d): FCFS\n", PRIORITY_DIVIDER/2);
    printf("Time allocation: 80%% foreground, 20%% background\n");

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount) {
        // 현재 시간에 도착한 프로세스를 우선순위에 따라 적절한 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;

                // 우선순위에 따라 프로세스를 적절한 큐에 배치
                if (processes[i]->priority <= PRIORITY_DIVIDER/2) { // 우선순위가 높은 프로세스 (값이 작을수록 우선순위 높음)
                    if (!enqueue(&foregroundReadyQueue, processes[i])) {
                        fprintf(stderr, "[Error] Foreground queue full at time %d!\n", currentTime);
                    }
                    printf("Time %d: Process %d ARRIVED (Priority: %d) -> Foreground Queue\n",
                           currentTime, processes[i]->pid, processes[i]->priority);
                } else { // 우선순위가 낮은 프로세스
                    if (!enqueue(&backgroundReadyQueue, processes[i])) {
                        fprintf(stderr, "[Error] Background queue full at time %d!\n", currentTime);
                    }
                    printf("Time %d: Process %d ARRIVED (Priority: %d) -> Background Queue\n",
                           currentTime, processes[i]->pid, processes[i]->priority);
                }
            }
        }

        // I/O 작업 처리
        // Foreground queue I/O 작업 처리
        int initialForeWaitCount = foregroundWaitQueue.count;
        for (int i = 0; i < initialForeWaitCount; i++) {
            Process *process = dequeue(&foregroundWaitQueue);
            if (process == NULL) continue;

            process->remaining_io_burst_time--;
            if (process->remaining_io_burst_time <= 0) {
                process->current_io_index++;
                process->status = READY;
                printf("Time %d: Process %d I/O completed, back to Foreground Queue\n",
                       currentTime, process->pid);

                if (!enqueue(&foregroundReadyQueue, process)) {
                    printf("Error enqueuing process after I/O completion\n");
                }
            } else {
                if (!enqueue(&foregroundWaitQueue, process)) {
                    printf("Error re-enqueuing process to waitQueue\n");
                }
            }
        }

        // Background queue I/O 작업 처리
        int initialBackWaitCount = backgroundWaitQueue.count;
        for (int i = 0; i < initialBackWaitCount; i++) {
            Process *process = dequeue(&backgroundWaitQueue);
            if (process == NULL) continue;

            process->remaining_io_burst_time--;
            if (process->remaining_io_burst_time <= 0) {
                process->current_io_index++;
                process->status = READY;
                printf("Time %d: Process %d I/O completed, back to Background Queue\n",
                       currentTime, process->pid);

                if (!enqueue(&backgroundReadyQueue, process)) {
                    printf("Error enqueuing process after I/O completion\n");
                }
            } else {
                if (!enqueue(&backgroundWaitQueue, process)) {
                    printf("Error re-enqueuing process to waitQueue\n");
                }
            }
        }

        // 큐 선택: 70%는 foreground, 30%는 background 큐에 할당
        int whichQueue;
        if (currentTime % 10 < 7) { // 70% 확률로 foreground 큐 선택 cpu 할당 비율 높게!
            whichQueue = 0; // foreground
        } else { // 20% 확률로 background 큐 선택
            whichQueue = 1; // background
        }

        // 현재 서비스 중인 큐가 다르고, 선택된 큐에 프로세스가 있으면 선점
        if (runningProcess != NULL && currentQueue != whichQueue) {
            if ((whichQueue == 0 && !isEmpty(&foregroundReadyQueue)) ||
                (whichQueue == 1 && !isEmpty(&backgroundReadyQueue))) {
                printf("Time %d: Queue switch from %s to %s\n",
                       currentTime,
                       currentQueue == 0 ? "Foreground" : "Background",
                       whichQueue == 0 ? "Foreground" : "Background");

                // 현재 프로세스를 원래 큐로 돌려보냄
                runningProcess->status = READY;
                if (currentQueue == 0) {
                    if (!enqueue(&foregroundReadyQueue, runningProcess)) {
                        fprintf(stderr, "[Error] Foreground queue full during queue switch!\n");
                    }
                } else {
                    if (!enqueue(&backgroundReadyQueue, runningProcess)) {
                        fprintf(stderr, "[Error] Background queue full during queue switch!\n");
                    }
                }

                // 간트 차트 업데이트
                ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                // 새 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                quantumCounter = 0;
                previousPID = -1;
            }
        }

        // CPU 할당 (이전 프로세스가 종료되었거나 선점되었을 때)
        if (runningProcess == NULL) {
            // 선택된 큐에서 프로세스를 가져옴
            if (whichQueue == 0 && !isEmpty(&foregroundReadyQueue)) {
                runningProcess = dequeue(&foregroundReadyQueue);
                currentQueue = 0;
                quantumCounter = 0; // foreground 큐는 Round Robin 사용
                printf("Time %d: Process %d RUNNING from Foreground Queue\n",
                       currentTime, runningProcess->pid);
            } else if (whichQueue == 1 && !isEmpty(&backgroundReadyQueue)) {
                runningProcess = dequeue(&backgroundReadyQueue);
                currentQueue = 1;
                printf("Time %d: Process %d RUNNING from Background Queue\n",
                       currentTime, runningProcess->pid);
            }
            // 선택된 큐가 비어있으면 다른 큐를 확인
            else if (whichQueue == 0 && !isEmpty(&backgroundReadyQueue)) {
                runningProcess = dequeue(&backgroundReadyQueue);
                currentQueue = 1;
                printf("Time %d: Process %d RUNNING from Background Queue (Foreground empty)\n",
                       currentTime, runningProcess->pid);
            } else if (whichQueue == 1 && !isEmpty(&foregroundReadyQueue)) {
                runningProcess = dequeue(&foregroundReadyQueue);
                currentQueue = 0;
                quantumCounter = 0; // foreground 큐는 Round Robin 사용
                printf("Time %d: Process %d RUNNING from Foreground Queue (Background empty)\n",
                       currentTime, runningProcess->pid);
            }

            // 프로세스 할당 성공 시 간트 차트 업데이트
            if (runningProcess) {
                runningProcess->status = RUNNING;

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (previousPID == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                }
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            if (currentQueue == 0) { // foreground 큐는 time quantum을 사용
                quantumCounter++;
            }

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                quantumCounter = 0;
                previousPID = -1; // IDLE 상태로 변경
            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // I/O 상태 시작 - 다음 상태는 IDLE
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                // 적절한 I/O 큐에 프로세스 추가
                if (currentQueue == 0) {
                    if (!enqueue(&foregroundWaitQueue, runningProcess)) {
                        fprintf(stderr, "[Error] Foreground wait queue full at time %d!\n", currentTime);
                    }
                } else {
                    if (!enqueue(&backgroundWaitQueue, runningProcess)) {
                        fprintf(stderr, "[Error] Background wait queue full at time %d!\n", currentTime);
                    }
                }

                runningProcess = NULL;
                quantumCounter = 0;
                previousPID = -1; // IDLE 상태로 변경
            }
            // foreground 큐의 time quantum 만료 확인 (background 큐는 FCFS이므로 확인 안함)
            else if (currentQueue == 0 && quantumCounter >= timeQuantum) {
                printf("Time %d: Process %d QUANTUM EXPIRED (Returned to Foreground Queue)\n",
                       currentTime, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                // 프로세스를 foreground 큐의 끝으로 보냄
                runningProcess->status = READY;
                if (!enqueue(&foregroundReadyQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Foreground queue full during quantum expiration!\n");
                }

                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                quantumCounter = 0;
                previousPID = -1; // IDLE 상태로 변경
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목)
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            // 유효한 로그만 유지
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    printf("Multi-Level Queue Scheduling Simulation COMPLETED at time %d-----------\n", currentTime);
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(ganttLogArray, validLogs);
}

// 프로세스에 할당할 티켓 수를 계산하는 함수
int calculateTickets(int priority) {
    // 우선순위가 낮은 값일수록 더 많은 티켓을 받음
    // 가장 높은 우선순위(1)는 최대 티켓 수를 받음
    return (PRIORITY_DIVIDER - priority + 1) * 2; // 계수를 조정하여 티켓 수 조절
}

Process* selectProcessByLottery(Queue* readyQueue) {
    if (isEmpty(readyQueue)) {
        return NULL;
    }

    int totalTickets = 0;
    // Process 포인터를 저장할 배열, 각 인덱스가 티켓 번호를 의미
    Process* ticketToProcessMap[MAX_TOTAL_TICKETS_POSSIBLE];

    // 1단계: 티켓 계산 및 매핑 (큐를 직접 순회)
    // readyQueue를 직접 순회하며 티켓 정보를 수집 (dequeue/enqueue 최소화)
    int current_idx = readyQueue->front;
    for (int i = 0; i < readyQueue->count; i++) {
        Process* proc = readyQueue->items[current_idx];
        if (!proc) { // 혹시 모를 NULL 포인터 방지
            current_idx = (current_idx + 1) % MAX_QUEUE_CAPACITY;
            continue;
        }

        int tickets = calculateTickets(proc->priority);
        for (int t = 0; t < tickets; t++) {
            if (totalTickets < MAX_TOTAL_TICKETS_POSSIBLE) {
                ticketToProcessMap[totalTickets++] = proc;
            } else {
                fprintf(stderr, "Error: Maximum number of tickets exceeded.\n");
                return NULL; // 티켓 배열 오버플로우
            }
        }
        current_idx = (current_idx + 1) % MAX_QUEUE_CAPACITY;
    }

    if (totalTickets == 0) {
        return NULL;
    }

    // 난수 생성기 시드 설정 (프로그램 시작 시 한 번만 하는 것이 더 좋을 수 있음)
    // srand(time(NULL)); // 매번 호출하면 동일 시간 내 호출 시 같은 패턴 발생 가능

    int winningTicketNumber = rand() % totalTickets;
    Process* selectedProcess = ticketToProcessMap[winningTicketNumber];

    // 2단계: 선택된 프로세스를 readyQueue에서 제거
    Queue tempQueue;
    initialize_queue(&tempQueue);
    Process * actuallySelectedAndRemovedProc = NULL;

    while (!isEmpty(readyQueue)) {
        Process *dequeuedProc = dequeue(readyQueue);
        if (dequeuedProc == selectedProcess && !actuallySelectedAndRemovedProc) {
            // 선택된 프로세스를 찾았고, 아직 "제거" 처리 전이면 이 프로세스를 반환 대상으로 설정
            actuallySelectedAndRemovedProc = dequeuedProc;
        } else {
            // 선택된 프로세스가 아니거나, 이미 선택된 프로세스 인스턴스를 찾은 후 동일 포인터의 다른 (가상의) 복사본이라면 임시 큐로 이동
            enqueue(&tempQueue, dequeuedProc);
        }
    }

    // 임시 큐의 내용을 다시 readyQueue로 복원 (선택된 프로세스는 제외됨)
    while (!isEmpty(&tempQueue)) {
        enqueue(readyQueue, dequeue(&tempQueue));
    }

    return actuallySelectedAndRemovedProc; // "제거"된 프로세스 반환
}

void runLotteryScheduling(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // 간트 차트 초기화 - 시작은 IDLE 상태
    int previousPID = -1; // IDLE 상태의 PID
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    printf("Priority-based Lottery Scheduling Simulation STARTED-----------\n");
    printf("Note: Lower priority value means higher priority and more lottery tickets\n");

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount) {
        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                if (!enqueue(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);
                }
                int tickets = calculateTickets(processes[i]->priority);
                printf("Time %d: Process %d ARRIVED (Priority: %d, Tickets: %d)\n",
                       currentTime, processes[i]->pid, processes[i]->priority, tickets);
            }
        }

        // I/O 진행 및 완료 처리
        IO_Operation(&readyQueue, &waitQueue);

        // CPU 할당 - 로터리 방식으로 프로세스 선택
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            // 로터리 당첨을 통해 프로세스 선택
            runningProcess = selectProcessByLottery(&readyQueue);

            if (runningProcess) {
                runningProcess->status = RUNNING;
                int tickets = calculateTickets(runningProcess->priority);
                printf("Time %d: Process %d WON LOTTERY (Priority: %d, Tickets: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->priority, tickets);

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (previousPID == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                }
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // I/O 상태 시작 - 다음 상태는 IDLE
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
            // 로터리 알고리즘은 비선점형으로 구현 (프로세스는 종료, I/O 요청 또는 burst 완료까지 계속 실행)
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목)
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            // 유효한 로그만 유지
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    printf("Lottery Scheduling Simulation COMPLETED at time %d-----------\n", currentTime);
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(ganttLogArray, validLogs);
}

void runLongestIOShortestCPU(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // 간트 차트 초기화 - 시작은 IDLE 상태
    int previousPID = -1; // IDLE 상태의 PID
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    printf("LISC (Longest IO burst, Shortest CPU burst) Scheduling Simulation STARTED-----------\n");

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount) {
        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;

                // 총 IO 시간 계산
                int total_io_time = 0;
                for (int j = 0; j < processes[i]->io_count; j++) {
                    total_io_time += processes[i]->io_burst_times[j];
                }

                // enqueue_for_lisc 함수 사용
                if (!enqueue_for_lisc(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);
                }
                printf("Time %d: Process %d ARRIVED (Total IO Time: %d, CPU Time: %d)\n",
                       currentTime, processes[i]->pid, total_io_time, processes[i]->remaining_cpu_burst_time);
            }
        }

        // I/O 진행 및 완료 처리
        IO_Operation_LISC(&readyQueue, &waitQueue);

        // CPU 할당 - 큐의 첫 번째 프로세스 선택 (이미 LISC 순서로 정렬됨)
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue);

            if (runningProcess) {
                runningProcess->status = RUNNING;

                // 총 IO 시간 계산
                int total_io_time = 0;
                for (int i = 0; i < runningProcess->io_count; i++) {
                    total_io_time += runningProcess->io_burst_times[i];
                }

                printf("Time %d: Process %d RUNNING (Total IO Time: %d, CPU Time: %d)\n",
                       currentTime, runningProcess->pid, total_io_time, runningProcess->remaining_cpu_burst_time);

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (previousPID == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                }
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // I/O 상태 시작 - 다음 상태는 IDLE
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목)
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            // 유효한 로그만 유지
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    printf("LISC Scheduling Simulation COMPLETED at time %d-----------\n", currentTime);
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(ganttLogArray, validLogs);
}



void runPreemptiveLongestIOShortestCPU(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // 간트 차트 초기화 - 시작은 IDLE 상태
    int previousPID = -1; // IDLE 상태의 PID
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    printf("Preemptive LISC (Longest IO burst, Shortest CPU burst) Scheduling Simulation STARTED-----------\n");

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount) {
        bool processArrived = false;

        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;

                // 총 IO 시간 계산
                int total_io_time = 0;
                for (int j = 0; j < processes[i]->io_count; j++) {
                    total_io_time += processes[i]->io_burst_times[j];
                }

                // enqueue_for_lisc 함수 사용
                if (!enqueue_for_lisc(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);
                }
                printf("Time %d: Process %d ARRIVED (Total IO Time: %d, CPU Time: %d)\n",
                       currentTime, processes[i]->pid, total_io_time, processes[i]->remaining_cpu_burst_time);
                processArrived = true;
            }
        }

        // I/O 진행 및 완료 처리
        int initialWaitCount = waitQueue.count;
        IO_Operation_LISC(&readyQueue, &waitQueue);
        bool ioCompleted = (initialWaitCount > waitQueue.count);

        // 선점 판단: 새 프로세스 도착하거나 I/O가 완료되면 현재 실행 중인 프로세스와 비교
        if ((processArrived || ioCompleted) && runningProcess != NULL) {
            // 레디 큐에 프로세스가 있는지 확인
            if (!isEmpty(&readyQueue)) {
                // 레디 큐 맨 앞의 프로세스 확인 (dequeue 없이)
                Process *highestPriorityProcess = readyQueue.items[readyQueue.front];

                if (highestPriorityProcess) {
                    // 현재 실행 중인 프로세스의 총 IO 시간 계산
                    int running_total_io_time = 0;
                    for (int i = 0; i < runningProcess->io_count; i++) {
                        running_total_io_time += runningProcess->io_burst_times[i];
                    }

                    // 레디 큐 첫 번째 프로세스의 총 IO 시간 계산
                    int highest_total_io_time = 0;
                    for (int i = 0; i < highestPriorityProcess->io_count; i++) {
                        highest_total_io_time += highestPriorityProcess->io_burst_times[i];
                    }

                    // LISC 기준으로 선점 여부 결정:
                    // 1. IO 시간이 더 길거나
                    // 2. IO 시간이 같고 CPU 시간이 더 짧으면 선점
                    bool preempt = false;

                    if (highest_total_io_time > running_total_io_time) {
                        // 레디 큐의 프로세스가 IO 시간이 더 길면 선점
                        preempt = true;
                    } else if (highest_total_io_time == running_total_io_time &&
                              highestPriorityProcess->remaining_cpu_burst_time < runningProcess->remaining_cpu_burst_time) {
                        // IO 시간이 같고 CPU 시간이 더 짧으면 선점
                        preempt = true;
                    }

                    if (preempt) {
                        printf("Time %d: Process %d PREEMPTED by Process %d (IO Time: %d vs %d, CPU Time: %d vs %d)\n",
                               currentTime, runningProcess->pid, highestPriorityProcess->pid,
                               running_total_io_time, highest_total_io_time,
                               runningProcess->remaining_cpu_burst_time, highestPriorityProcess->remaining_cpu_burst_time);

                        // 현재 프로세스를 READY 상태로 변경하고 레디 큐에 추가
                        runningProcess->status = READY;
                        if (!enqueue_for_lisc(&readyQueue, runningProcess)) {
                            fprintf(stderr, "[Error] Ready queue full during preemption at time %d!\n", currentTime);
                        }

                        // 간트 차트 업데이트: 현재 실행 중이던 프로세스의 로그 종료
                        ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                        // 새 프로세스 디큐
                        runningProcess = dequeue(&readyQueue);

                        // 새 프로세스 로그 시작
                        ganttLogArray[ganttEntryCount].startTime = currentTime;
                        ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                        ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                        ganttEntryCount++;

                        // 새 프로세스 실행
                        runningProcess->status = RUNNING;
                        previousPID = runningProcess->pid;

                        printf("Time %d: Process %d RUNNING (Total IO Time: %d, CPU Time: %d)\n",
                               currentTime, runningProcess->pid, highest_total_io_time,
                               runningProcess->remaining_cpu_burst_time);
                    }
                }
            }
        }
        // CPU가 비어있고 레디 큐에 프로세스가 있는 경우 CPU 할당
        else if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue);

            if (runningProcess) {
                runningProcess->status = RUNNING;

                // 총 IO 시간 계산
                int total_io_time = 0;
                for (int i = 0; i < runningProcess->io_count; i++) {
                    total_io_time += runningProcess->io_burst_times[i];
                }

                printf("Time %d: Process %d RUNNING (Total IO Time: %d, CPU Time: %d)\n",
                       currentTime, runningProcess->pid, total_io_time, runningProcess->remaining_cpu_burst_time);

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (previousPID == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
                    previousPID = runningProcess->pid;
                }
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                // I/O 상태 시작 - 다음 상태는 IDLE
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }

                runningProcess = NULL;
                previousPID = -1; // IDLE 상태로 변경
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목)
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            // 유효한 로그만 유지
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    printf("Preemptive LISC Scheduling Simulation COMPLETED at time %d-----------\n", currentTime);
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(ganttLogArray, validLogs);
}





void resetProcesses(Process* processes[], int processCount)
{
    for (int i = 0; i < processCount; i++) {
        // 남은 실행 시간을 원래 burst 시간으로 복원
        processes[i]->remaining_cpu_burst_time = processes[i]->cpu_burst_time;
        processes[i]->remaining_io_burst_time = 0;
        
        // I/O 관련 필드 초기화
        processes[i]->current_io_index = 0;
        processes[i]->cpu_time_used = 0;

        // 상태를 NEW로 초기화
        processes[i]->status = NEW;

        // 측정된 메트릭 값들을 초기화
        processes[i]->completion_time = 0;
        processes[i]->waiting_time = 0;
        processes[i]->turnaround_time = 0;
    }
}

void runScheduler(Process* processes[], int processCount, int algorithm)
{
    resetProcesses(processes, processCount);

    switch (algorithm) {
    case FCFS:
        printf("FCFS algorithm selected\n");
        runFCFS(processes, processCount);
        break;

    case NonPreemptiveSJF:
        printf("SJF algorithm selected\n");
        runNonPreemptiveSJF(processes, processCount);
        break;

    case RoundRobin:
        printf("RoundRobin algorithm selected\n");
        runRoundRobin(processes, processCount);
        break;

    case PRIORITY:
        printf("Priority algorithm selected\n");
        runPriority(processes, processCount);
        break;

    case PreemptiveSJF:
        printf("Preemptive SJF (SRTF) algorithm selected\n");
        runPreemptiveSJF(processes, processCount);
        break;

    case PreemptivePriority:
        printf("Preemptive Priority algorithm selected\n");
        runPreemptivePriority(processes, processCount);
        break;

    case RoundRobinWithPriority:
        printf("RoundRobin With Priority algorithm selected\n");
        runRoundRobinWithPriority(processes, processCount);
        break;

    case LongestIOFirst:
        printf("Longest IO First algorithm selected\n");
        runLongestIOFirst(processes, processCount);
        break;

    case PreemptiveLongestIOFirst:
        printf("Preemptive Longest IO First algorithm selected\n");
        runPreemptiveLongestIOFirst(processes, processCount);
        break;

    case MultiLevelQueue:
        printf("Multi-Level Queue algorithm selected\n");
        runMultiLevelQueue(processes, processCount);
        break;

    case LOTTERY:
        printf("Priority-based Lottery Scheduling algorithm selected\n");
        runLotteryScheduling(processes, processCount);
        break;

    case LongestIOShortestCPU:
        printf("Longest IO Shortest CPU algorithm selected\n");
        runLongestIOShortestCPU(processes, processCount);
        break;

    case PreemptiveLongestIOShortestCPU:
        printf("Preemptive Longest IO Short Scheduling algorithm selected\n");
        runPreemptiveLongestIOShortestCPU(processes, processCount);
        break;






    default:
        printf("Invalid algorithm selected\n");
        break;
    }
}

void IO_Operation(Queue *readyQueue, Queue *waitQueue)
{
    int initial_waitQueue_count = waitQueue->count;
    if (initial_waitQueue_count == 0) {
        return;
    }

    for (int i = 0; i < initial_waitQueue_count; i++) {
        Process *process = dequeue(waitQueue);
        if (process == NULL) {
            printf("Error: NULL process dequeued from waitQueue\n");
            continue;
        }
        
        process->remaining_io_burst_time--;
        if (process->remaining_io_burst_time <= 0) {
            process->current_io_index++;
            process->status = READY;
            printf("Time %d: Process %d I/O completed, back to READY\n", 
                   process->current_io_index, process->pid);
            
            if (!enqueue(readyQueue, process)) {
                printf("Error enqueuing process after I/O completion\n");
            }
        } else {
            if (!enqueue(waitQueue, process)) {
                printf("Error re-enqueuing process to waitQueue\n");
            }
        }
    }
}

void IO_Operation_SJF(Queue *readyQueue, Queue *waitQueue)
{
    int initial_waitQueue_count = waitQueue->count;
    if (initial_waitQueue_count == 0) {
        return;
    }

    for (int i = 0; i < initial_waitQueue_count; i++) {
        Process *process = dequeue(waitQueue);
        if (process == NULL) {
            printf("Error: NULL process dequeued from waitQueue\n");
            continue;
        }

        process->remaining_io_burst_time--;
        if (process->remaining_io_burst_time <= 0) {
            process->current_io_index++;
            process->status = READY;
            printf("Time %d: Process %d I/O completed, back to READY\n",
                   process->current_io_index, process->pid);

            if (!enqueue_for_sjf(readyQueue, process)) {
                printf("Error enqueuing process after I/O completion\n");
            }
        } else {
            if (!enqueue(waitQueue, process)) {
                printf("Error re-enqueuing process to waitQueue\n");
            }
        }
    }
}


void IO_Operation_Priority(Queue *readyQueue, Queue *waitQueue)
{
    int initial_waitQueue_count = waitQueue->count;
    if (initial_waitQueue_count == 0) {
        return;
    }

    for (int i = 0; i < initial_waitQueue_count; i++) {
        Process *process = dequeue(waitQueue);
        if (process == NULL) {
            printf("Error: NULL process dequeued from waitQueue\n");
            continue;
        }

        process->remaining_io_burst_time--;
        if (process->remaining_io_burst_time <= 0) {
            process->current_io_index++;
            process->status = READY;
            printf("Time %d: Process %d I/O completed, back to READY\n",
                   process->current_io_index, process->pid);

            if (!enqueue_for_priority(readyQueue, process)) {
                printf("Error enqueuing process after I/O completion\n");
            }
        } else {
            if (!enqueue(waitQueue, process)) {
                printf("Error re-enqueuing process to waitQueue\n");
            }
        }
    }
}


// IO_Operation_LIF 함수
void IO_Operation_LIF(Queue *readyQueue, Queue *waitQueue)
{
    int initial_waitQueue_count = waitQueue->count;
    if (initial_waitQueue_count == 0) {
        return;
    }

    for (int i = 0; i < initial_waitQueue_count; i++) {
        Process *process = dequeue(waitQueue);
        if (process == NULL) {
            printf("Error: NULL process dequeued from waitQueue\n");
            continue;
        }

        process->remaining_io_burst_time--;
        if (process->remaining_io_burst_time <= 0) {
            process->current_io_index++;
            process->status = READY;
            printf("Time %d: Process %d I/O completed, back to READY\n",
                   process->current_io_index, process->pid);

            if (!enqueue_for_lif(readyQueue, process)) {
                printf("Error enqueuing process after I/O completion\n");
            }
        } else {
            if (!enqueue(waitQueue, process)) {
                printf("Error re-enqueuing process to waitQueue\n");
            }
        }
    }
}

void IO_Operation_LISC(Queue *readyQueue, Queue *waitQueue)
{
    int initial_waitQueue_count = waitQueue->count;
    if (initial_waitQueue_count == 0) {
        return;
    }

    for (int i = 0; i < initial_waitQueue_count; i++) {
        Process *process = dequeue(waitQueue);
        if (process == NULL) {
            printf("Error: NULL process dequeued from waitQueue\n");
            continue;
        }

        process->remaining_io_burst_time--;
        if (process->remaining_io_burst_time <= 0) {
            process->current_io_index++;
            process->status = READY;
            printf("Time %d: Process %d I/O completed, back to READY\n",
                   process->current_io_index, process->pid);

            if (!enqueue_for_lisc(readyQueue, process)) {
                printf("Error enqueuing process after I/O completion\n");
            }
        } else {
            if (!enqueue(waitQueue, process)) {
                printf("Error re-enqueuing process to waitQueue\n");
            }
        }
    }
}

