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

    // 모든 프로세스가 종료될 때까지 루프 실행 (시간 제한 또는 추가 종료 조건 추가)
    int maxSimulationTime = 1000; // 시뮬레이션 최대 시간 제한
    // 모든 프로세스가 종료되었거나 시뮬레이션 최대 시간에 도달한 경우 종료
    while (terminatedCount < processCount && currentTime < maxSimulationTime) {
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
        IO_Operation(&readyQueue, &waitQueue, &terminatedCount, &currentTime);

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

void runSJFCombined(Process* processes[], int processCount, bool isPreemptive) {

    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // 간트 차트 초기화 - 시작은 IDLE 상태
    int previousPID = -1; // IDLE 상태의 PID, 또는 이전 실행 프로세스 PID
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    if (isPreemptive) {
        printf("Combined Preemptive SJF (SRTF) Simulation STARTED-----------\n");
    } else {
        printf("Combined Non-Preemptive SJF Simulation STARTED-----------\n");
    }

    // 모든 프로세스가 종료될 때까지 루프 실행
    int maxSimulationTime = 1000; // 시뮬레이션 최대 시간 제한
    while (terminatedCount < processCount && currentTime < maxSimulationTime) {
        bool newEventOccurred = false; // 새 프로세스 도착 또는 I/O 완료 여부

        // 1. 현재 시간에 도착한 프로세스를 Ready Queue에 추가 (SJF 순서로)
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                if (!enqueue_for_sjf(&readyQueue, processes[i])) { // [cite: 10] for enqueue_for_sjf usage
                    fprintf(stderr, "[Error] SJF: Ready queue full at time %d for P%d!\n", currentTime, processes[i]->pid);
                }
                printf("Time %d: Process %d ARRIVED (Burst: %d)\n", currentTime, processes[i]->pid, processes[i]->remaining_cpu_burst_time);
                newEventOccurred = true;
            }
        }

        // 2. I/O 진행 및 완료 처리 (완료된 프로세스는 SJF 순서로 Ready Queue에 추가)
        int initialWaitQueueCount = waitQueue.count;
        IO_Operation_SJF(&readyQueue, &waitQueue, &terminatedCount, &currentTime); // [cite: 10] IO_Operation_SJF uses enqueue_for_sjf
        if (waitQueue.count < initialWaitQueueCount || (initialWaitQueueCount > 0 && readyQueue.count > (readyQueue.count - (initialWaitQueueCount - waitQueue.count)))) { // crude check if something moved to readyQ
             newEventOccurred = true;
        }


        // 3. 선점 판단 (Preemptive SJF인 경우 그리고 새 이벤트 발생 시)
        if (isPreemptive && newEventOccurred && runningProcess != NULL) {
            if (!isEmpty(&readyQueue)) {
                Process *shortestInQueue = readyQueue.items[readyQueue.front]; // Peek at the shortest job in ready queue
                if (shortestInQueue && shortestInQueue->remaining_cpu_burst_time < runningProcess->remaining_cpu_burst_time) {
                    printf("Time %d: Process %d (Rem: %d) PREEMPTED by Process %d (Rem: %d)\n",
                           currentTime, runningProcess->pid, runningProcess->remaining_cpu_burst_time,
                           shortestInQueue->pid, shortestInQueue->remaining_cpu_burst_time);

                    // 현재 실행 중인 프로세스의 간트 차트 로그 종료
                    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == runningProcess->pid) {
                       ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                    }


                    runningProcess->status = READY;
                    if (!enqueue_for_sjf(&readyQueue, runningProcess)) { // Add running process back to ready queue (sorted)
                        fprintf(stderr, "[Error] SJF: Ready queue full during preemption at time %d!\n", currentTime);
                    }

                    runningProcess = NULL; // CPU가 비게 됨, 다음 단계에서 새 프로세스 선택
                    // previousPID = -1; // Will be set when new process runs or stays idle
                                      // Setting previousPID to the preempted process PID or -1 for IDLE handling
                                      // is important. If we set to -1, it ensures IDLE log is properly handled.
                                      // If we set to the preempted PID, the next if(runningProcess == NULL) will pick shortest.
                }
            }
        }

        // 4. CPU 할당 (CPU가 비어있고 Ready Queue에 프로세스가 있을 경우)
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue); // Ready queue is already sorted by SJF logic

            if (runningProcess) {
                runningProcess->status = RUNNING;
                printf("Time %d: Process %d RUNNING (Remaining Burst: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->remaining_cpu_burst_time);

                // 간트 차트 업데이트: 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime; // IDLE 상태 종료
                }
                // Else if preemption occurred, the preempted process log was closed. New log for new process.

                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;
                previousPID = runningProcess->pid;
            }
        }

        // 5. CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사 - 음수 및 오버플로우 방지
            if (runningProcess->remaining_cpu_burst_time <= 0 || runningProcess->cpu_time_used >= runningProcess->cpu_burst_time) {
                // 남은 CPU 시간이 음수가 되지 않도록 보장
                runningProcess->remaining_cpu_burst_time = 0;
                // CPU 사용량이 초과되지 않도록 제한
                if (runningProcess->cpu_time_used > runningProcess->cpu_burst_time) {
                    runningProcess->cpu_time_used = runningProcess->cpu_burst_time;
                }

                if (runningProcess->status != TERMINATED) {
                    // 중복 방지
                    runningProcess->status = TERMINATED;
                    runningProcess->completion_time = currentTime + 1;
                    terminatedCount++;

                    printf("Time %d: Process %d TERMINATED (CPU used: %d, Remaining: %d)\n",
                           currentTime + 1, runningProcess->pid,
                           runningProcess->cpu_time_used, runningProcess->remaining_cpu_burst_time);

                    ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1; // 현재 프로세스 로그 종료

                    runningProcess = NULL; // CPU 비움
                    previousPID = -1; // 다음 상태는 IDLE일 수 있음
                }
                // 다음 IDLE 로그 시작 준비 (실제 시작은 다음 루프에서 runningProcess가 NULL일 때)
                if (terminatedCount < processCount) { // 모든 프로세스가 종료된게 아니라면 IDLE 시작 가능
                    ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                    ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                    ganttLogArray[ganttEntryCount].endTime = 0;
                    ganttEntryCount++;
                }

            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {

                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d, Remaining CPU: %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time,
                       runningProcess->remaining_cpu_burst_time);

                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1; // 현재 프로세스 로그 종료

                if (!enqueue(&waitQueue, runningProcess)) { // Wait Queue는 FCFS
                    fprintf(stderr, "[Error] SJF: Wait queue full for P%d at time %d!\n", runningProcess->pid, currentTime);
                }

                runningProcess = NULL; // CPU 비움
                previousPID = -1; // 다음 상태는 IDLE일 수 있음

                // 다음 IDLE 로그 시작 준비
                 if (terminatedCount < processCount) {
                    ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                    ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                    ganttLogArray[ganttEntryCount].endTime = 0;
                    ganttEntryCount++;
                }
            }
        } else { // CPU is IDLE
            // 현재 IDLE 상태이며, 이전 로그가 IDLE이 아니었거나, 첫 번째 IDLE 로그가 아니라면 새 IDLE 로그를 시작.
            // 또는, 이전 IDLE 로그가 이미 현재 시간에 시작된 것이 아니라면.
            if (ganttEntryCount == 0 || (ganttLogArray[ganttEntryCount-1].pid != -1 && ganttLogArray[ganttEntryCount-1].endTime != 0) ||
                (ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].startTime != currentTime && ganttLogArray[ganttEntryCount-1].endTime == 0) ) {
                 // This condition is tricky. The original FCFS started an IDLE log initially and updated its end time.
                 // If the last active log was a process, and it just ended (endTime set), a new IDLE log should start.
                 // If the last log was already IDLE and still current (endTime=0), do nothing.
                 if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid != -1 && ganttLogArray[ganttEntryCount-1].endTime != 0) {
                     // Previous was a process and it has ended. Start new IDLE.
                     ganttLogArray[ganttEntryCount].startTime = currentTime;
                     ganttLogArray[ganttEntryCount].pid = -1;
                     ganttLogArray[ganttEntryCount].endTime = 0;
                     ganttEntryCount++;
                 } else if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime != 0) {
                     // Previous was IDLE and it has ended (should not happen if still idle). Start new IDLE.
                     ganttLogArray[ganttEntryCount].startTime = currentTime;
                     ganttLogArray[ganttEntryCount].pid = -1;
                     ganttLogArray[ganttEntryCount].endTime = 0;
                     ganttEntryCount++;
                 }
                 // If the current log is already an open IDLE log, we don't need to start a new one.
                 // The initial IDLE log handles the t=0 case.
            }
            previousPID = -1; // CPU is IDLE
        }


        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정 (모든 프로세스가 종료된 후)
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목)
    int validLogs = 0;
    GanttChartLog finalGanttLogs[MAX_GANTTCHART_LOG]; // Use a temporary array for valid logs
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            finalGanttLogs[validLogs++] = ganttLogArray[i];
        } else if (ganttLogArray[i].pid == -1 && ganttLogArray[i].startTime == ganttLogArray[i].endTime && ganttLogArray[i].startTime == currentTime && terminatedCount == processCount){
            // Special case: if the very last action was a process termination at 'currentTime',
            // an IDLE log might have been prepared for 'currentTime' but it's immediately the end.
            // Don't add this zero-duration IDLE log at the very end of simulation.
        } else if (ganttLogArray[i].pid != -1 && ganttLogArray[i].startTime == ganttLogArray[i].endTime) {
            // Also remove zero-duration process logs, though less likely with ++currentTime logic
             finalGanttLogs[validLogs++] = ganttLogArray[i]; // Or skip if truly 0 duration
        }
    }


    if (isPreemptive) {
        printf("Combined Preemptive SJF (SRTF) Simulation COMPLETED at time %d-----------\n", currentTime);
    } else {
        printf("Combined Non-Preemptive SJF Simulation COMPLETED at time %d-----------\n", currentTime);
    }
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(finalGanttLogs, validLogs); // [cite: 10] for drawGanttChart usage
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
        IO_Operation(&readyQueue, &waitQueue, &terminatedCount, &currentTime);

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




void runPriorityCombined(Process* processes[], int processCount, bool isPreemptive) {
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // 간트 차트 초기화 - 시작은 IDLE 상태
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    if (isPreemptive) {
        printf("Combined Preemptive Priority Scheduling Simulation STARTED-----------\n");
    } else {
        printf("Combined Non-Preemptive Priority Scheduling Simulation STARTED-----------\n");
    }
    printf("Note: Lower priority value means higher priority.\n");

    int maxSimulationTime = 1000;
    while (terminatedCount < processCount && currentTime < maxSimulationTime) {
        bool newEventOccurred = false; // 새 프로세스 도착 또는 I/O 완료 여부

        // 1. 현재 시간에 도착한 프로세스를 Ready Queue에 추가 (Priority 순서로)
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                if (!enqueue_for_priority(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Priority: Ready queue full at time %d for P%d!\n", currentTime, processes[i]->pid);
                }
                printf("Time %d: Process %d ARRIVED (Priority: %d, Burst: %d)\n",
                       currentTime, processes[i]->pid, processes[i]->priority, processes[i]->remaining_cpu_burst_time);
                newEventOccurred = true;
            }
        }

        // 2. I/O 진행 및 완료 처리 (완료된 프로세스는 Priority 순서로 Ready Queue에 추가)
        int initialWaitQueueCount = waitQueue.count;
        int readyQueueCountBeforeIO = readyQueue.count;
        IO_Operation_Priority(&readyQueue, &waitQueue, &terminatedCount, &currentTime); // Priority용 I/O 처리 함수
        if (waitQueue.count < initialWaitQueueCount || readyQueue.count > readyQueueCountBeforeIO) {
             newEventOccurred = true;
        }

        // 3. 선점 판단 (Preemptive Priority인 경우)
        // SJF와 유사하게 newEventOccurred를 선점 조건 중 하나로 고려할 수 있으나,
        // Priority는 레디큐에 더 높은 우선순위가 들어오면 언제든 선점 가능.
        // 여기서는 newEventOccurred를 명시적 조건으로 넣지 않고, 레디큐의 상태만 봄.
        if (isPreemptive && runningProcess != NULL) {
            if (!isEmpty(&readyQueue)) {
                Process *highestPriorityInQueue = readyQueue.items[readyQueue.front]; // Peek
                if (highestPriorityInQueue && highestPriorityInQueue->priority < runningProcess->priority) {
                    printf("Time %d: Process %d (Prio: %d) PREEMPTED by Process %d (Prio: %d)\n",
                           currentTime, runningProcess->pid, runningProcess->priority,
                           highestPriorityInQueue->pid, highestPriorityInQueue->priority);

                    // 현재 실행 중인 프로세스의 간트 차트 로그 종료
                    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == runningProcess->pid && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                       ganttLogArray[ganttEntryCount-1].endTime = currentTime; // 선점은 현재 시간 기점
                    }

                    runningProcess->status = READY;
                    if (!enqueue_for_priority(&readyQueue, runningProcess)) {
                        fprintf(stderr, "[Error] Priority: Ready queue full during preemption at time %d!\n", currentTime);
                    }
                    runningProcess = NULL; // CPU가 비게 됨
                    // newEventOccurred = true; // CPU 할당 로직이 실행되도록
                }
            }
        }

        // 4. CPU 할당 (CPU가 비어있고 Ready Queue에 프로세스가 있을 경우)
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue);

            if (runningProcess) {
                runningProcess->status = RUNNING;
                printf("Time %d: Process %d RUNNING (Priority: %d, Rem Burst: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->priority, runningProcess->remaining_cpu_burst_time);

                // 간트 차트 업데이트:
                // 이전 로그가 IDLE(-1)이었고 아직 열려있다면(endTime == 0), 현재 시간으로 닫는다.
                if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                }
                // 이전 로그가 다른 프로세스였고 (선점 등으로) 이미 닫혔을 수 있다.
                // 만약 이전 로그가 현재 실행할 프로세스가 아니고, 아직 열려있다면(endTime == 0) 닫는다.
                // (이 경우는 비선점 스케줄링에서 한 프로세스가 끝나고 다른 프로세스가 시작될 때 발생 가능,
                // 또는 선점 후 다른 프로세스가 바로 시작될 때 이전 프로세스 로그는 이미 위에서 닫혔어야 함)
                else if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid != runningProcess->pid && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                     ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                }


                // 새 프로세스 로그 시작:
                // 현재 프로세스가 이전 간트 로그의 프로세스와 다르거나, 이전 로그가 이미 종료된 경우 새 로그를 시작한다.
                if (ganttEntryCount == 0 || ganttLogArray[ganttEntryCount-1].pid != runningProcess->pid || ganttLogArray[ganttEntryCount-1].endTime != 0) {
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
                }
            }
        }

        // 5. CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->remaining_cpu_burst_time = 0;
                if (runningProcess->cpu_time_used > runningProcess->cpu_burst_time) { // Defensive
                    runningProcess->cpu_time_used = runningProcess->cpu_burst_time;
                }

                if (runningProcess->status != TERMINATED) { // 중복 종료 방지
                    runningProcess->status = TERMINATED;
                    runningProcess->completion_time = currentTime + 1; // 종료는 현재 시간 단위 실행 후
                    terminatedCount++;
                    printf("Time %d: Process %d TERMINATED (CPU used: %d, Rem: %d)\n",
                           currentTime + 1, runningProcess->pid,
                           runningProcess->cpu_time_used, runningProcess->remaining_cpu_burst_time);

                    // 현재 실행 중인 프로세스의 간트 차트 로그 종료
                    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == runningProcess->pid && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                        ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;
                    }
                    runningProcess = NULL; // CPU 비움

                    // SJF처럼 여기서 바로 다음 IDLE 로그 준비 (만약 모든 프로세스가 종료되지 않았다면)
                    if (terminatedCount < processCount) {
                        // 이전 로그가 IDLE이 아니거나, 이미 닫힌 IDLE인 경우에만 새 IDLE 시작
                        if (ganttEntryCount == 0 || ganttLogArray[ganttEntryCount-1].pid != -1 || (ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime != 0) ) {
                             // 그리고 그 IDLE이 현재 시간+1 에서 시작하는지 확인 (중복 방지)
                             bool addIdle = true;
                             if(ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime == 0 && ganttLogArray[ganttEntryCount-1].startTime == currentTime + 1) {
                                 addIdle = false;
                             }
                             if(addIdle){
                                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                                ganttLogArray[ganttEntryCount].endTime = 0;
                                ganttEntryCount++;
                             }
                        }
                    }
                } else { // 이미 TERMINATED (이론상 도달 안함)
                     if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == runningProcess->pid && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                         ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;
                     }
                     runningProcess = NULL;
                }
            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {

                // 현재 실행 중인 프로세스의 간트 차트 로그 종료
                if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == runningProcess->pid && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1; // I/O 요청도 현재 시간 단위 실행 후
                }

                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;
                printf("Time %d: Process %d requests I/O (Duration %d, Rem CPU: %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time,
                       runningProcess->remaining_cpu_burst_time);

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Priority: Wait queue full for P%d at time %d!\n", runningProcess->pid, currentTime);
                }
                runningProcess = NULL; // CPU 비움

                // SJF처럼 여기서 바로 다음 IDLE 로그 준비 (모든 프로세스가 종료되지 않았다면)
                 if (terminatedCount < processCount) {
                    if (ganttEntryCount == 0 || ganttLogArray[ganttEntryCount-1].pid != -1 || (ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime != 0) ) {
                        bool addIdle = true;
                        if(ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime == 0 && ganttLogArray[ganttEntryCount-1].startTime == currentTime + 1) {
                            addIdle = false;
                        }
                        if(addIdle){
                           ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                           ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                           ganttLogArray[ganttEntryCount].endTime = 0;
                           ganttEntryCount++;
                        }
                    }
                }
            }
        } else { // CPU is IDLE (runningProcess == NULL)
            // 이전에 IDLE 로그를 즉시 시작하는 로직(프로세스 종료/IO시)과 중복될 수 있으므로,
            // 현재 시간에서 IDLE 로그가 이미 열려있지 않은 경우에만 새로 시작하거나 기존 IDLE을 이어간다.
            if (terminatedCount < processCount) { // 모든 프로세스가 종료된게 아니라면 IDLE 가능
                // 마지막 로그가 IDLE 이고 아직 열려있는지 확인 (endTime == 0)
                // 또한, 그 열린 IDLE이 현재 시간에 시작된 것인지, 아니면 이전부터 이어져 온 것인지 확인.
                bool needToStartNewIdleLog = true;
                if (ganttEntryCount > 0) {
                    if (ganttLogArray[ganttEntryCount - 1].pid == -1 && ganttLogArray[ganttEntryCount - 1].endTime == 0) {
                        // 이미 IDLE 로그가 열려있음. 새로 시작할 필요 없음.
                        needToStartNewIdleLog = false;
                    }
                    // 만약 이전 로그가 프로세스였고, 아직 닫히지 않았다면 (이런일은 없어야 함, 위에서 닫혔어야 함)
                    // 여기서 닫고 새 IDLE 시작
                    else if (ganttLogArray[ganttEntryCount-1].pid != -1 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                         ganttLogArray[ganttEntryCount-1].endTime = currentTime; // 여기서 현재 시간으로 닫음.
                    }
                }


                // 새로운 IDLE 로그가 필요하고, 이전 로그가 (현재시간에 시작된 열린 IDLE이 아닌) 다른 것이라면.
                if (needToStartNewIdleLog) {
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = -1;
                    ganttLogArray[ganttEntryCount].endTime = 0;
                    ganttEntryCount++;
                }
            }
        }

        currentTime++;
    } // while 루프 종료

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효한 간트 차트 로그만 필터링 (SJF와 유사하게)
    int validLogs = 0;
    GanttChartLog finalGanttLogs[MAX_GANTTCHART_LOG];
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            finalGanttLogs[validLogs++] = ganttLogArray[i];
        } else if (ganttLogArray[i].pid == -1 && ganttLogArray[i].startTime == ganttLogArray[i].endTime &&
                   ganttLogArray[i].startTime == currentTime && terminatedCount == processCount) {
            // 시뮬레이션이 종료되는 'currentTime'에 생성된 0 길이 IDLE 로그는 무시 (SJF 스타일)
        }
        // 0-duration process logs는 startTime < endTime 에 의해 이미 걸러짐.
        // SJF 코드의 `else if (ganttLogArray[i].pid != -1 && ganttLogArray[i].startTime == ganttLogArray[i].endTime)`
        // 부분은 보통 발생하지 않으므로, startTime < endTime 조건으로 충분할 수 있음.
    }

    if (isPreemptive) {
        printf("Combined Preemptive Priority Scheduling Simulation COMPLETED at time %d-----------\n", currentTime);
    } else {
        printf("Combined Non-Preemptive Priority Scheduling Simulation COMPLETED at time %d-----------\n", currentTime);
    }
    if (currentTime >= maxSimulationTime && terminatedCount < processCount) {
        printf("Warning: Simulation ended due to maxSimulationTime (%d) with %d processes not terminated.\n", maxSimulationTime, processCount - terminatedCount);
    }
    // printf("Total gantt chart logs initially: %d\n", ganttEntryCount); // 디버깅용
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(finalGanttLogs, validLogs);
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
        IO_Operation_Priority(&readyQueue, &waitQueue, &terminatedCount, &currentTime);

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

void runLIFCombined(Process* processes[], int processCount, bool isPreemptive) {
    int currentTime = 0;        // 현재 시뮬레이션 시간
    int terminatedCount = 0;    // 종료된 프로세스 수
    int ganttEntryCount = 0;    // 간트 차트 로그 항목 수

    Queue readyQueue;           // 준비 큐
    Queue waitQueue;            // 대기 큐 (I/O 작업용)
    Process *runningProcess = NULL; // 현재 CPU에서 실행 중인 프로세스
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG]; // 간트 차트 기록 배열

    int previousPID = -1; // 이전에 실행된 프로세스 ID (-1은 IDLE)
    // 간트 차트 초기화: 시간 0에서 IDLE 상태로 시작
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1; // IDLE 상태 표시
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 종료되지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue); // 준비 큐 초기화
    initialize_queue(&waitQueue);  // 대기 큐 초기화

    if (isPreemptive) {
        printf("통합 선점형 Longest IO First (LIF) 스케줄링 시뮬레이션 시작-----------\n");
    } else {
        printf("통합 비선점형 Longest IO First (LIF) 스케줄링 시뮬레이션 시작-----------\n");
    }

    // 모든 프로세스가 종료될 때까지 시뮬레이션 루프 실행
    while (terminatedCount < processCount) {
        bool newEventOccurred = false; // 새로운 이벤트(도착/I_O완료) 발생 여부 플래그

        // 1. 프로세스 도착 처리: 현재 시간에 도착한 프로세스를 준비 큐에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                // enqueue_for_lif 함수는 process->io_burst_time (총 I/O 시간)을 기준으로 정렬
                if (!enqueue_for_lif(&readyQueue, processes[i])) {
                    fprintf(stderr, "[오류] LIF: P%d 도착 시 준비 큐 가득 참 (시간: %d)!\n", processes[i]->pid, currentTime);
                }
                printf("시간 %d: 프로세스 %d 도착 (총 IO 시간: %d)\n",
                       currentTime, processes[i]->pid, processes[i]->io_burst_time);
                newEventOccurred = true;
            }
        }

        // 2. I/O 작업 처리: 대기 큐의 프로세스들 I/O 진행 및 완료 시 준비 큐로 이동
        int initialWaitQueueCount = waitQueue.count;
        IO_Operation_LIF(&readyQueue, &waitQueue, &terminatedCount, &currentTime); // IO_Operation_LIF는 내부적으로 enqueue_for_lif 사용
        if (waitQueue.count < initialWaitQueueCount || (initialWaitQueueCount > 0 && readyQueue.count > (readyQueue.count - (initialWaitQueueCount - waitQueue.count)))) {
             newEventOccurred = true; // I/O 완료로 인한 이벤트 발생
        }

        // 3. 선점 로직 (선점형 LIF이고 새로운 이벤트가 발생했을 경우)
        if (isPreemptive && newEventOccurred && runningProcess != NULL) {
            if (!isEmpty(&readyQueue)) {
                Process *longestIOInQueue = readyQueue.items[readyQueue.front]; // 준비 큐에서 가장 긴 I/O 시간 프로세스 확인 (Peek)

                // process->io_burst_time은 프로세스의 총 초기 I/O 시간을 저장해야 함
                if (longestIOInQueue && longestIOInQueue->io_burst_time > runningProcess->io_burst_time) {
                    printf("시간 %d: 프로세스 %d (총IO: %d) 선점됨. 선점자: 프로세스 %d (총IO: %d)\n",
                           currentTime, runningProcess->pid, runningProcess->io_burst_time,
                           longestIOInQueue->pid, longestIOInQueue->io_burst_time);

                    // 현재 실행 중인 프로세스의 간트 차트 로그 종료
                    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == runningProcess->pid) {
                       ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                    }

                    runningProcess->status = READY; // 상태를 READY로 변경
                    if (!enqueue_for_lif(&readyQueue, runningProcess)) { // 선점된 프로세스를 준비 큐에 다시 추가 (LIF 순서로 정렬)
                        fprintf(stderr, "[오류] LIF: 선점 시 준비 큐 가득 참 (시간: %d)!\n", currentTime);
                    }
                    runningProcess = NULL; // CPU 비움
                }
            }
        }

        // 4. CPU 할당 로직: CPU가 비어있고 준비 큐에 프로세스가 있는 경우
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue); // 준비 큐는 LIF 순서로 정렬되어 있음

            if (runningProcess) {
                runningProcess->status = RUNNING; // 상태를 RUNNING으로 변경
                printf("시간 %d: 프로세스 %d 실행 시작 (총 IO: %d, 남은 CPU: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->io_burst_time, runningProcess->remaining_cpu_burst_time);

                // 이전 로그가 IDLE 상태였다면 IDLE 로그 종료
                if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                }

                // 새 프로세스 실행에 대한 간트 차트 로그 시작
                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 종료되지 않음
                ganttEntryCount++;
                previousPID = runningProcess->pid; // 이전 실행 프로세스 ID 업데이트
            }
        }

        // 5. CPU 실행 및 상태 전이
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--; // 남은 CPU 시간 감소
            runningProcess->cpu_time_used++;            // 사용한 CPU 시간 증가

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED; // 상태를 TERMINATED로 변경
                runningProcess->completion_time = currentTime + 1; // 종료 시간 기록 (다음 시간 단위 시작 시점)
                terminatedCount++; // 종료된 프로세스 수 증가
                printf("시간 %d: 프로세스 %d 종료됨\n", currentTime + 1, runningProcess->pid);

                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1; // 현재 프로세스 간트 차트 로그 종료
                runningProcess = NULL; // CPU 비움
                previousPID = -1;      // 다음 상태는 IDLE일 수 있음

                // 모든 프로세스가 종료되지 않았다면 IDLE 로그 시작 준비
                if (terminatedCount < processCount) {
                    ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                    ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                    ganttLogArray[ganttEntryCount].endTime = 0;
                    ganttEntryCount++;
                }
            }
            // I/O 요청 검사
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {

                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index]; // 남은 I/O 시간 설정
                runningProcess->status = WAITING; // 상태를 WAITING으로 변경

                printf("시간 %d: 프로세스 %d I/O 요청 (I/O 시간: %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1; // 현재 프로세스 간트 차트 로그 종료

                if (!enqueue(&waitQueue, runningProcess)) { // 대기 큐는 FCFS로 동작
                    fprintf(stderr, "[오류] LIF: P%d I/O 요청 시 대기 큐 가득 참 (시간: %d)!\n", runningProcess->pid, currentTime);
                }
                runningProcess = NULL; // CPU 비움
                previousPID = -1;      // 다음 상태는 IDLE일 수 있음

                // 모든 프로세스가 종료되지 않았다면 IDLE 로그 시작 준비
                if (terminatedCount < processCount) {
                    ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                    ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                    ganttLogArray[ganttEntryCount].endTime = 0;
                    ganttEntryCount++;
                }
            }
        } else { // CPU가 IDLE 상태인 경우
             // 이전 로그가 실행 중인 프로세스였고 방금 종료/선점되었거나,
             // 또는 이전 로그가 IDLE이었지만 현재 시간과 시작 시간이 다른 경우 (즉, 새로운 IDLE 구간 시작)
             if (ganttEntryCount == 0 || (ganttLogArray[ganttEntryCount-1].pid != -1 && ganttLogArray[ganttEntryCount-1].endTime != 0) ||
                (ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].startTime != currentTime && ganttLogArray[ganttEntryCount-1].endTime == 0) ) {
                 // 이전 로그가 프로세스였고 종료되었다면, 새 IDLE 로그 시작
                 if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid != -1 && ganttLogArray[ganttEntryCount-1].endTime != 0) {
                     ganttLogArray[ganttEntryCount].startTime = currentTime;
                     ganttLogArray[ganttEntryCount].pid = -1;
                     ganttLogArray[ganttEntryCount].endTime = 0;
                     ganttEntryCount++;
                 }
                 // 이전 로그가 IDLE이었고 종료되었다면 (이 경우는 거의 없음), 새 IDLE 로그 시작
                 else if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime != 0) {
                     ganttLogArray[ganttEntryCount].startTime = currentTime;
                     ganttLogArray[ganttEntryCount].pid = -1;
                     ganttLogArray[ganttEntryCount].endTime = 0;
                     ganttEntryCount++;
                 }
                 // 현재 로그가 이미 열린 IDLE 로그가 아니라면 (초기 상태 포함)
                 // 이 부분은 초기 IDLE 로그가 이미 처리하므로, 추가적인 IDLE 로그 시작은 이전 상태에 따라 결정됨.
            }
            previousPID = -1; // CPU가 IDLE임을 표시
        }
        currentTime++; // 시간 증가
    }

    // 마지막 간트 차트 로그 항목의 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효한 간트 차트 로그만 필터링 (시작 시간 < 종료 시간)
    int validLogs = 0;
    GanttChartLog finalGanttLogs[MAX_GANTTCHART_LOG]; // 유효한 로그를 저장할 임시 배열
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) { // 유효한 로그 조건
            finalGanttLogs[validLogs++] = ganttLogArray[i];
        } else if (ganttLogArray[i].pid == -1 && ganttLogArray[i].startTime == ganttLogArray[i].endTime && ganttLogArray[i].startTime == currentTime && terminatedCount == processCount){
            // 시뮬레이션이 끝나는 시점에 생성된 0 길이 IDLE 로그는 제외
        } else if (ganttLogArray[i].pid != -1 && ganttLogArray[i].startTime == ganttLogArray[i].endTime) {
            // 0 길이 프로세스 로그도 유효하다고 판단하여 추가 (발생 가능성은 낮음)
             finalGanttLogs[validLogs++] = ganttLogArray[i];
        }
    }

    // 시뮬레이션 종료 메시지 출력
    if (isPreemptive) {
        printf("통합 선점형 Longest IO First (LIF) 스케줄링 시뮬레이션 완료 (시간: %d)-----------\n", currentTime);
    } else {
        printf("통합 비선점형 Longest IO First (LIF) 스케줄링 시뮬레이션 완료 (시간: %d)-----------\n", currentTime);
    }
    printf("유효한 간트 차트 로그 수: %d\n", validLogs);
    drawGanttChart(finalGanttLogs, validLogs); // 간트 차트 그리기 함수 호출
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
        IO_Operation(&readyQueue, &waitQueue, &terminatedCount, &currentTime);

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

void runLISCCombined(Process* processes[], int processCount, bool isPreemptive) {
    int currentTime = 0;        // 현재 시뮬레이션 시간
    int terminatedCount = 0;    // 종료된 프로세스 수
    int ganttEntryCount = 0;    // 간트 차트 로그 항목 수

    Queue readyQueue;           // 준비 큐
    Queue waitQueue;            // 대기 큐 (I/O 작업용)
    Process *runningProcess = NULL; // 현재 CPU에서 실행 중인 프로세스
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG]; // 간트 차트 기록 배열

    int previousPID = -1; // 이전에 실행된 프로세스 ID (-1은 IDLE)
    // 간트 차트 초기화: 시간 0에서 IDLE 상태로 시작
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1; // IDLE 상태 표시
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 종료되지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue); // 준비 큐 초기화
    initialize_queue(&waitQueue);  // 대기 큐 초기화

    if (isPreemptive) {
        printf("통합 선점형 LISC (Longest IO, Shortest CPU) 스케줄링 시뮬레이션 시작-----------\n");
    } else {
        printf("통합 비선점형 LISC (Longest IO, Shortest CPU) 스케줄링 시뮬레이션 시작-----------\n");
    }

    // 모든 프로세스가 종료될 때까지 시뮬레이션 루프 실행
    while (terminatedCount < processCount) {
        bool newEventOccurred = false; // 새로운 이벤트(도착/I_O완료) 발생 여부 플래그

        // 1. 프로세스 도착 처리: 현재 시간에 도착한 프로세스를 준비 큐에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                // enqueue_for_lisc 함수는 총 I/O 시간 (긴 순) -> 남은 CPU 시간 (짧은 순) 기준으로 정렬
                if (!enqueue_for_lisc(&readyQueue, processes[i])) {
                    fprintf(stderr, "[오류] LISC: P%d 도착 시 준비 큐 가득 참 (시간: %d)!\n", processes[i]->pid, currentTime);
                }
                printf("시간 %d: 프로세스 %d 도착 (총 IO: %d, CPU 시간: %d)\n",
                       currentTime, processes[i]->pid, processes[i]->io_burst_time, processes[i]->remaining_cpu_burst_time);
                newEventOccurred = true;
            }
        }

        // 2. I/O 작업 처리: 대기 큐의 프로세스들 I/O 진행 및 완료 시 준비 큐로 이동
        int initialWaitQueueCount = waitQueue.count;
        IO_Operation_LISC(&readyQueue, &waitQueue, &terminatedCount, &currentTime); // IO_Operation_LISC는 내부적으로 enqueue_for_lisc 사용
         if (waitQueue.count < initialWaitQueueCount || (initialWaitQueueCount > 0 && readyQueue.count > (readyQueue.count - (initialWaitQueueCount - waitQueue.count)))) {
             newEventOccurred = true; // I/O 완료로 인한 이벤트 발생
        }

        // 3. 선점 로직 (선점형 LISC이고 새로운 이벤트가 발생했을 경우)
        if (isPreemptive && newEventOccurred && runningProcess != NULL) {
            if (!isEmpty(&readyQueue)) {
                Process *candidateProcess = readyQueue.items[readyQueue.front]; // 준비 큐에서 최우선순위 LISC 프로세스 확인 (Peek)

                if (candidateProcess) {
                    bool shouldPreempt = false;
                    // LISC 우선순위 비교:
                    // 1. 총 I/O 시간이 더 긴 경우
                    if (candidateProcess->io_burst_time > runningProcess->io_burst_time) {
                        shouldPreempt = true;
                    }
                    // 2. 총 I/O 시간이 같고, 남은 CPU 시간이 더 짧은 경우
                    else if (candidateProcess->io_burst_time == runningProcess->io_burst_time &&
                               candidateProcess->remaining_cpu_burst_time < runningProcess->remaining_cpu_burst_time) {
                        shouldPreempt = true;
                    }

                    if (shouldPreempt) {
                        printf("시간 %d: 프로세스 %d (IO: %d, CPU: %d) 선점됨. 선점자: 프로세스 %d (IO: %d, CPU: %d)\n",
                               currentTime,
                               runningProcess->pid, runningProcess->io_burst_time, runningProcess->remaining_cpu_burst_time,
                               candidateProcess->pid, candidateProcess->io_burst_time, candidateProcess->remaining_cpu_burst_time);

                        // 현재 실행 중인 프로세스의 간트 차트 로그 종료
                        if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == runningProcess->pid) {
                           ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                        }

                        runningProcess->status = READY; // 상태를 READY로 변경
                        if (!enqueue_for_lisc(&readyQueue, runningProcess)) { // 선점된 프로세스를 준비 큐에 다시 추가 (LISC 순서로 정렬)
                            fprintf(stderr, "[오류] LISC: 선점 시 준비 큐 가득 참 (시간: %d)!\n", currentTime);
                        }
                        runningProcess = NULL; // CPU 비움
                    }
                }
            }
        }

        // 4. CPU 할당 로직: CPU가 비어있고 준비 큐에 프로세스가 있는 경우
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue); // 준비 큐는 LISC 순서로 정렬되어 있음

            if (runningProcess) {
                runningProcess->status = RUNNING; // 상태를 RUNNING으로 변경
                printf("시간 %d: 프로세스 %d 실행 시작 (총 IO: %d, 남은 CPU: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->io_burst_time, runningProcess->remaining_cpu_burst_time);

                // 이전 로그가 IDLE 상태였다면 IDLE 로그 종료
                if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                }

                // 새 프로세스 실행에 대한 간트 차트 로그 시작
                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 종료되지 않음
                ganttEntryCount++;
                previousPID = runningProcess->pid; // 이전 실행 프로세스 ID 업데이트
            }
        }

        // 5. CPU 실행 및 상태 전이
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--; // 남은 CPU 시간 감소
            runningProcess->cpu_time_used++;            // 사용한 CPU 시간 증가

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED; // 상태를 TERMINATED로 변경
                runningProcess->completion_time = currentTime + 1; // 종료 시간 기록
                terminatedCount++; // 종료된 프로세스 수 증가
                printf("시간 %d: 프로세스 %d 종료됨\n", currentTime + 1, runningProcess->pid);

                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;
                runningProcess = NULL;
                previousPID = -1;

                if (terminatedCount < processCount) {
                    ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                    ganttLogArray[ganttEntryCount].pid = -1;
                    ganttLogArray[ganttEntryCount].endTime = 0;
                    ganttEntryCount++;
                }
            }
            // I/O 요청 검사
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {

                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("시간 %d: 프로세스 %d I/O 요청 (I/O 시간: %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                if (!enqueue(&waitQueue, runningProcess)) { // 대기 큐는 FCFS
                    fprintf(stderr, "[오류] LISC: P%d I/O 요청 시 대기 큐 가득 참 (시간: %d)!\n", runningProcess->pid, currentTime);
                }
                runningProcess = NULL;
                previousPID = -1;

                if (terminatedCount < processCount) {
                    ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                    ganttLogArray[ganttEntryCount].pid = -1;
                    ganttLogArray[ganttEntryCount].endTime = 0;
                    ganttEntryCount++;
                }
            }
        } else { // CPU가 IDLE 상태
            if (ganttEntryCount == 0 || (ganttLogArray[ganttEntryCount-1].pid != -1 && ganttLogArray[ganttEntryCount-1].endTime != 0) ||
                (ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].startTime != currentTime && ganttLogArray[ganttEntryCount-1].endTime == 0) ) {
                 if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid != -1 && ganttLogArray[ganttEntryCount-1].endTime != 0) {
                     ganttLogArray[ganttEntryCount].startTime = currentTime;
                     ganttLogArray[ganttEntryCount].pid = -1;
                     ganttLogArray[ganttEntryCount].endTime = 0;
                     ganttEntryCount++;
                 } else if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime != 0) {
                     ganttLogArray[ganttEntryCount].startTime = currentTime;
                     ganttLogArray[ganttEntryCount].pid = -1;
                     ganttLogArray[ganttEntryCount].endTime = 0;
                     ganttEntryCount++;
                 }
            }
            previousPID = -1;
        }
        currentTime++; // 시간 증가
    }

    // 마지막 간트 차트 로그 항목의 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효한 간트 차트 로그만 필터링
    int validLogs = 0;
    GanttChartLog finalGanttLogs[MAX_GANTTCHART_LOG];
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            finalGanttLogs[validLogs++] = ganttLogArray[i];
        } else if (ganttLogArray[i].pid == -1 && ganttLogArray[i].startTime == ganttLogArray[i].endTime && ganttLogArray[i].startTime == currentTime && terminatedCount == processCount){
            // 끝에서 0길이 IDLE 로그는 제외
        } else if (ganttLogArray[i].pid != -1 && ganttLogArray[i].startTime == ganttLogArray[i].endTime) {
             finalGanttLogs[validLogs++] = ganttLogArray[i];
        }
    }

    // 시뮬레이션 종료 메시지
    if (isPreemptive) {
        printf("통합 선점형 LISC 스케줄링 시뮬레이션 완료 (시간: %d)-----------\n", currentTime);
    } else {
        printf("통합 비선점형 LISC 스케줄링 시뮬레이션 완료 (시간: %d)-----------\n", currentTime);
    }
    printf("유효한 간트 차트 로그 수: %d\n", validLogs);
    drawGanttChart(finalGanttLogs, validLogs); // 간트 차트 출력
}

void runHRRN(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG]; // ganttchart.h 필요

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    // 간트 차트 초기화
    int previousPID = -1;
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;
    ganttLogArray[ganttEntryCount].endTime = 0;
    ganttEntryCount++;

    printf("HRRN Scheduling Simulation STARTED-----------\n");

    while (terminatedCount < processCount) {
        // 1. 프로세스 도착 처리
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                processes[i]->time_entered_ready = currentTime; // Ready Queue 진입 시간 기록!
                if (!enqueue(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] HRRN: Ready queue full on arrival for P%d!\n", processes[i]->pid);
                }
                printf("Time %d: Process %d ARRIVED (Entered Ready at %d)\n",
                       currentTime, processes[i]->pid, processes[i]->time_entered_ready);
            }
        }

        // 2. I/O 처리
        IO_Operation_HRRN(&readyQueue, &waitQueue, currentTime, &terminatedCount);

        // 3. CPU 할당 (Highest Response Ratio 찾기)
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            Process *selectedProcess = NULL;
            double highestRatio = -1.0; // 또는 DBL_MIN 사용
            int readyCount = readyQueue.count;
            Process *tempStorage[MAX_QUEUE_CAPACITY]; // 임시 저장소

            // 준비 큐에서 모든 프로세스를 꺼내 확인하고 임시 저장
            for (int i = 0; i < readyCount; i++) {
                tempStorage[i] = dequeue(&readyQueue);
                if (tempStorage[i] == NULL) continue;

                int waitingTime = currentTime - tempStorage[i]->time_entered_ready;
                if (waitingTime < 0) waitingTime = 0;

                // 실행 시간(Burst Time)이 0인 경우 처리 (일반적으로 양수여야 함)
                int burstTime = tempStorage[i]->cpu_burst_time; // 전체 버스트 시간 기준

                    double ratio = (double)(waitingTime + burstTime) / burstTime;
                    printf("  - Checking P%d: Wait=%d, Burst=%d, Ratio=%.2f\n", tempStorage[i]->pid, waitingTime, burstTime, ratio);

                    if (selectedProcess == NULL || ratio > highestRatio) {
                        highestRatio = ratio;
                        selectedProcess = tempStorage[i];
                    }
                    // 타이 브레이킹: 응답률 같으면 먼저 들어온 프로세스 선택 (FIFO)
                    // (현재 로직은 먼저 찾은 프로세스를 유지하므로 FIFO와 유사)
                }


            // 선택되지 않은 프로세스들을 다시 준비 큐에 넣음
            for (int i = 0; i < readyCount; i++) {
                if (tempStorage[i] != selectedProcess) {
                    enqueue(&readyQueue, tempStorage[i]);
                }
            }

            // 선택된 프로세스를 실행 상태로 설정
            runningProcess = selectedProcess;
            if (runningProcess) {
                runningProcess->status = RUNNING;
                runningProcess->time_entered_ready = -1; // 더 이상 준비 큐에서 대기하지 않음
                printf("Time %d: Process %d selected (HRRN Ratio=%.2f). Running.\n",
                       currentTime, runningProcess->pid, highestRatio);

                // 간트 차트 로깅: 새 프로세스 시작
                if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime; // 이전 로그 종료
                }
                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;
                previousPID = runningProcess->pid;
            }
        }

        // 4. CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1; // 현재 로그 종료
                runningProcess = NULL;
                previousPID = -1;
                // 다음 시간부터 IDLE 로그 시작
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1;
                ganttLogArray[ganttEntryCount].endTime = 0;
                ganttEntryCount++;
            }
            // I/O 요청 검사
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {
                runningProcess->status = WAITING;
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                printf("Time %d: Process %d requests I/O (Duration %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1; // 현재 로그 종료

                if (!enqueue(&waitQueue, runningProcess)) { // Wait 큐로 이동
                    fprintf(stderr, "[Error] HRRN: Wait queue full for P%d!\n", runningProcess->pid);
                }
                runningProcess = NULL;
                previousPID = -1;
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1;
                ganttLogArray[ganttEntryCount].endTime = 0;
                ganttEntryCount++;
            }
            // HRRN은 비선점형이므로, 다른 검사(선점 등)는 필요 없음
        } else { // CPU IDLE
             if (previousPID != -1 || (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid != -1)) {
                 if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                      ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                 }
                 ganttLogArray[ganttEntryCount].startTime = currentTime;
                 ganttLogArray[ganttEntryCount].pid = -1;
                 ganttLogArray[ganttEntryCount].endTime = 0;
                 ganttEntryCount++;
             }
             previousPID = -1;
             printf("Time %d: CPU IDLE\n", currentTime);
        }

        currentTime++;
    }

    // 마지막 로그 정리 및 간트 차트 출력
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }
    // 유효 로그 필터링 (기존과 유사)
    int validLogs = 0;
    GanttChartLog finalLogs[MAX_GANTTCHART_LOG];
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            finalLogs[validLogs++] = ganttLogArray[i];
        }
    }

    printf("HRRN Scheduling Simulation COMPLETED at time %d-----------\n", currentTime);
    drawGanttChart(finalLogs, validLogs); // ganttchart.c 함수 호출
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
        runSJFCombined(processes, processCount, false);
        break;

    case RoundRobin:
        printf("RoundRobin algorithm selected\n");
        runRoundRobin(processes, processCount);
        break;

    case PRIORITY:
        printf("Priority algorithm selected\n");
        runPriorityCombined(processes, processCount, false);
        break;

    case PreemptiveSJF:
        printf("Preemptive SJF (SRTF) algorithm selected\n");
        runSJFCombined(processes, processCount, true);
        break;

    case PreemptivePriority:
        printf("Preemptive Priority algorithm selected\n");
        runPriorityCombined(processes, processCount, true);
        break;

    case RoundRobinWithPriority:
        printf("RoundRobin With Priority algorithm selected\n");
        runRoundRobinWithPriority(processes, processCount);
        break;

    case LongestIOFirst:
        printf("Longest IO First algorithm selected\n");
        runLIFCombined(processes, processCount, false);
        break;

    case PreemptiveLongestIOFirst:
        printf("Preemptive Longest IO First algorithm selected\n");
        runLIFCombined(processes, processCount, true);
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
        runLISCCombined(processes, processCount, false);
        break;

    case PreemptiveLongestIOShortestCPU:
        printf("Preemptive Longest IO Short Scheduling algorithm selected\n");
        runLISCCombined(processes, processCount, true);
        break;
    case HRRN:
        printf("HRRN algorithm selected\n");
        runHRRN(processes, processCount);
        break;

    default:
        printf("Invalid algorithm selected\n");
        break;
    }
}

// 기본 IO_Operation 함수
void IO_Operation(Queue *readyQueue, Queue *waitQueue, int *terminatedCount, int *currentTime)
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

            // CPU 버스트 잔여량 확인
            if (process->remaining_cpu_burst_time <= 0) {
                process->remaining_cpu_burst_time = 0; // 음수 방지
                if (process->status != TERMINATED) {
                    process->status = TERMINATED;
                    process->completion_time = *currentTime;
                    (*terminatedCount)++;
                    printf("Time %d: Process %d I/O completed and TERMINATED\n",
                          *currentTime, process->pid);
                }
            } else {
                // CPU 버스트가 남아있으면 READY 상태로
                process->status = READY;
                printf("Time %d: Process %d I/O completed, back to READY (Remaining CPU: %d)\n",
                      *currentTime, process->pid, process->remaining_cpu_burst_time);

                // FCFS는 일반 enqueue 사용
                if (!enqueue(readyQueue, process)) {
                    printf("Error: Ready queue full when adding P%d after I/O\n", process->pid);
                }
            }
        } else {
            // I/O가 아직 완료되지 않은 경우 wait 큐에 다시 추가
            if (!enqueue(waitQueue, process)) {
                printf("Error re-enqueuing process to waitQueue\n");
            }
        }
    }
}

// SJF용 IO_Operation 함수
void IO_Operation_SJF(Queue *readyQueue, Queue *waitQueue, int *terminatedCount, int *currentTime)
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


        if (process->remaining_io_burst_time <= 0) {
            process->current_io_index++;

            // 중요: I/O 완료 후 CPU 버스트가 남아있는지 확인
            if (process->remaining_cpu_burst_time <= 0) {
                // CPU 버스트가 없으면 종료
                if (process->status != TERMINATED) {
                    process->status = TERMINATED;
                    process->completion_time = *currentTime;
                    (*terminatedCount)++;
                    printf("Time %d: Process %d I/O completed and TERMINATED\n",
                          *currentTime, process->pid);
                }
            } else {
                // CPU 버스트가 남아있으면 READY 상태로
                process->status = READY;
                process->time_entered_ready = *currentTime;
                printf("Time %d: Process %d I/O completed, back to READY (Remaining CPU: %d)\n",
                      *currentTime, process->pid, process->remaining_cpu_burst_time);

                // SJF 정렬 로직으로 Ready 큐에 추가
                if (!enqueue_for_sjf(readyQueue, process)) {
                    printf("Error: SJF Ready queue full when adding P%d after I/O\n", process->pid);
                }
            }
        } else {
            // I/O가 아직 완료되지 않은 경우 wait 큐에 다시 추가
            if (!enqueue(waitQueue, process)) {
                printf("Error re-enqueuing process to waitQueue\n");
            }
        }
        process->remaining_io_burst_time--;
    }
}

// Priority용 IO_Operation 함수
void IO_Operation_Priority(Queue *readyQueue, Queue *waitQueue, int *terminatedCount, int *currentTime)
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


        if (process->remaining_io_burst_time <= 0) {
            process->current_io_index++;

            // 중요: I/O 완료 후 CPU 버스트가 남아있는지 확인
            if (process->remaining_cpu_burst_time <= 0) {
                // CPU 버스트가 없으면 종료
                if (process->status != TERMINATED) {
                    process->status = TERMINATED;
                    process->completion_time = *currentTime;
                    (*terminatedCount)++;
                    printf("Time %d: Process %d I/O completed and TERMINATED\n",
                          *currentTime, process->pid);
                }
            } else {
                // CPU 버스트가 남아있으면 READY 상태로
                process->status = READY;
                process->time_entered_ready = *currentTime;
                printf("Time %d: Process %d I/O completed, back to READY (Remaining CPU: %d)\n",
                      *currentTime, process->pid, process->remaining_cpu_burst_time);

                // priority 정렬 로직으로 Ready 큐에 추가
                if (!enqueue_for_priority(readyQueue, process)) {
                    printf("Error: SJF Ready queue full when adding P%d after I/O\n", process->pid);
                }
            }
        } else {
            // I/O가 아직 완료되지 않은 경우 wait 큐에 다시 추가
            if (!enqueue(waitQueue, process)) {
                printf("Error re-enqueuing process to waitQueue\n");
            }
        }
        process->remaining_io_burst_time--;
    }
}

// LIF용 IO_Operation 함수
void IO_Operation_LIF(Queue *readyQueue, Queue *waitQueue, int *terminatedCount, int *currentTime)
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

        if (process->remaining_io_burst_time <= 0) {
            process->current_io_index++;

            // I/O 완료 후 CPU 버스트가 남아있는지 확인
            if (process->remaining_cpu_burst_time <= 0) {
                // CPU 버스트가 없으면 종료
                if (process->status != TERMINATED) {
                    process->status = TERMINATED;
                    process->completion_time = *currentTime;
                    (*terminatedCount)++;
                    printf("Time %d: Process %d I/O completed and TERMINATED\n",
                          *currentTime, process->pid);
                }
            } else {
                // CPU 버스트가 남아있으면 READY 상태로
                process->status = READY;
                printf("Time %d: Process %d I/O completed, back to READY (IO Time: %d, Remaining CPU: %d)\n",
                      *currentTime, process->pid, process->io_burst_time, process->remaining_cpu_burst_time);

                // LIF 정렬 로직으로 Ready 큐에 추가
                if (!enqueue_for_lif(readyQueue, process)) {
                    printf("Error: LIF Ready queue full when adding P%d after I/O\n", process->pid);
                }
            }
        } else {
            // I/O가 아직 완료되지 않은 경우 wait 큐에 다시 추가
            if (!enqueue(waitQueue, process)) {
                printf("Error re-enqueuing process to waitQueue\n");
            }
        }
        process->remaining_io_burst_time--;
    }
}

// LISC용 IO_Operation 함수
void IO_Operation_LISC(Queue *readyQueue, Queue *waitQueue,int *terminatedCount, int *currentTime)
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

        if (process->remaining_io_burst_time <= 0) {
            process->current_io_index++;

            // I/O 완료 후 CPU 버스트가 남아있는지 확인
            if (process->remaining_cpu_burst_time <= 0) {
                // CPU 버스트가 없으면 종료
                process->remaining_cpu_burst_time = 0;  // 방지책
                if (process->status != TERMINATED) {
                    process->status = TERMINATED;
                    process->completion_time = *currentTime;
                    (*terminatedCount)++;
                    printf("Time %d: Process %d I/O completed and TERMINATED\n",
                          *currentTime, process->pid);
                }
            } else {
                // CPU 버스트가 남아있으면 READY 상태로
                process->status = READY;
                process->time_entered_ready = *currentTime;
                printf("Time %d: Process %d I/O completed, back to READY (IO: %d, Remaining CPU: %d)\n",
                      *currentTime, process->pid, process->io_burst_time, process->remaining_cpu_burst_time);

                // LISC 정렬 로직으로 Ready 큐에 추가
                if (!enqueue_for_lisc(readyQueue, process)) {
                    printf("Error: LISC Ready queue full when adding P%d after I/O\n", process->pid);
                }
            }
        } else {
            // I/O가 아직 완료되지 않은 경우 wait 큐에 다시 추가
            if (!enqueue(waitQueue, process)) {
                printf("Error re-enqueuing process to waitQueue\n");
            }
        }
        process->remaining_io_burst_time--;
    }
}

void IO_Operation_HRRN(Queue *readyQueue, Queue *waitQueue, int currentTime, int *terminatedCount)
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

        // I/O 진행
        process->remaining_io_burst_time--;

        if (process->remaining_io_burst_time <= 0) {
            process->current_io_index++;

            // 중요: I/O 완료 후 CPU 버스트가 남아있는지 확인
            if (process->remaining_cpu_burst_time <= 0) {
                // CPU 버스트가 없으면 종료
                if (process->status != TERMINATED) {
                    process->status = TERMINATED;
                    process->completion_time = currentTime;
                    (*terminatedCount)++;
                    printf("Time %d: Process %d I/O completed and TERMINATED\n",
                          currentTime, process->pid);
                }
            } else {
                // CPU 버스트가 남아있으면 READY 상태로
                process->status = READY;
                process->time_entered_ready = currentTime;
                printf("Time %d: Process %d I/O completed, back to READY (Remaining CPU: %d)\n",
                      currentTime, process->pid, process->remaining_cpu_burst_time);

                // HRRN의 경우, 단순히 Ready 큐에 추가
                // (실제 비율 계산과 선택은 CPU 할당 시점에서 이루어짐)
                if (!enqueue(readyQueue, process)) {
                    printf("Error: HRRN Ready queue full when adding P%d after I/O\n", process->pid);
                }
            }
        } else {
            // I/O가 아직 완료되지 않은 경우 wait 큐에 다시 추가
            if (!enqueue(waitQueue, process)) {
                printf("Error re-enqueuing process to waitQueue\n");
            }
        }
    }
}