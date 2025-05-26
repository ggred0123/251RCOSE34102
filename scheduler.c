

#include "scheduler.h"
#include "ganttchart.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
void runFCFS(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;// 이걸로 프로세스 끝나는 타이밍 맞춘다..
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // 간트 차트 초기화 - 시작은 IDLE 상태 거의 뭐 변경되면 일단 다 idle 로 갔다가..

    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음 기본 할당한 상태..
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    printf("FCFS Simulation STARTED-----------\n");

    // 모든 프로세스가 종료될 때까지 루프 실행
    // 모든 프로세스가 종료되었거나 시뮬레이션 최대 시간에 도달한 경우 종료
    while (terminatedCount < processCount ) {
        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                if (!enqueue(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);//오류 처리는 stderr메크로 사용
                }
                printf("Time %d: Process %d ARRIVED\n", currentTime, processes[i]->pid);
            }
        }

        // I/O 진행 및 완료 처리- 이거 한번 하면 IO 작업 1초씩 한 것..)
        IO_Operation(&readyQueue, &waitQueue, &terminatedCount, &currentTime);

        // 프로세스에 cpu 할당하는 과정
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue);
            if (runningProcess) {
                runningProcess->status = RUNNING;
                printf("Time %d: Process %d RUNNING\n", currentTime, runningProcess->pid);

                if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime; // IDLE 상태 종료
                }


                    
                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;


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

                if (!enqueue(&waitQueue, runningProcess)) {// waitqueue에 할당..
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }

                runningProcess = NULL;
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목) 일단 끝날 때 -1 로 만들어줘서 이게 필요함
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
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    if (isPreemptive) {//선점인지 아닌지
        printf("Combined Preemptive SJF (SRTF) Simulation STARTED-----------\n");
    } else {
        printf("Combined Non-Preemptive SJF Simulation STARTED-----------\n");
    }

    // 모든 프로세스가 종료될 때까지 루프 실행

    while (terminatedCount < processCount ) {
        bool newEventOccurred = false; // 새 프로세스 도착 또는 I/O 완료 여부.. 선점방식에서는 여기에 민감해진다

        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가해주자
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                if (!enqueue_for_sjf(&readyQueue, processes[i])) { // enqueue도 스케줄링 방식 별로 분리해놓았다..(sjf 기준으로 정렬되는 enqueue임)
                    fprintf(stderr, "[Error] SJF: Ready queue full at time %d for P%d!\n", currentTime, processes[i]->pid);
                }
                printf("Time %d: Process %d ARRIVED (Burst: %d)\n", currentTime, processes[i]->pid, processes[i]->remaining_cpu_burst_time);
                newEventOccurred = true;// 새로운 프로세스가 레디큐에 와도 선점될 수 있음
            }
        }

        //  I/O 진행 및 완료 처리 (완료된 프로세스는 SJF 순서로 Ready Queue에 추가)
        int initialWaitQueueCount = waitQueue.count;
        IO_Operation_SJF(&readyQueue, &waitQueue, &terminatedCount, &currentTime); //
        if (waitQueue.count < initialWaitQueueCount ) {
             newEventOccurred = true;// wait에서 ready로 간 프로세스가 있다면 선점 발생 가능
        }


        // 선점 판단 (Preemptive SJF인 경우 그리고 새 이벤트 발생 시 또한 running process 가 없을때 new event가 발생했다고 선점이라고 할 수는 없으니 현재 할당된 프로세스가 있는지도 보자)
        if (isPreemptive && newEventOccurred && runningProcess != NULL) {
            if (!isEmpty(&readyQueue)) {
                Process *shortestInQueue = readyQueue.items[readyQueue.front]; // enqueue에서 정렬해주기 때문에 맨 앞에 프로세스랑만 비교하면 된다
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

                }
            }
        }

        // CPU 할당 (CPU가 비어있고 Ready Queue에 프로세스가 있을 경우)
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


                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
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
                }
                // 다음 IDLE 로그 시작 준비 (실제 시작은 다음 루프에서 runningProcess가 NULL일 때)
                    ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                    ganttLogArray[ganttEntryCount].pid = -1; // IDLE 상태로 보낸다.(들어올지 안들어올지 여기서는 모르니까)
                    ganttLogArray[ganttEntryCount].endTime = 0;
                    ganttEntryCount++;


            }
            // I/O 요청 확인 - 구체적인건 fcfs랑 동일하게 설정
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {

                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d, Remaining CPU: %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time,
                       runningProcess->remaining_cpu_burst_time);

                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1; // 현재 프로세스 로그 종료

                if (!enqueue(&waitQueue, runningProcess)) { //
                    fprintf(stderr, "[Error] SJF: Wait queue full for P%d at time %d!\n", runningProcess->pid, currentTime);
                }

                runningProcess = NULL; // CPU 비움

                // 다음 IDLE 로그 시작 준비

                    ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                    ganttLogArray[ganttEntryCount].pid = -1; // 일단 IDLE
                    ganttLogArray[ganttEntryCount].endTime = 0;
                    ganttEntryCount++;

            }
        }


        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정 (모든 프로세스가 종료된 후)
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거 (시작 시간과 종료 시간이 같은 항목 다시 말하지만 일단 다 IDLE 로 보내기 때문에 이거 필요한거다
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


    if (isPreemptive) {
        printf("Combined Preemptive SJF (SRTF) Simulation COMPLETED at time %d-----------\n", currentTime);
    } else {
        printf("Combined Non-Preemptive SJF Simulation COMPLETED at time %d-----------\n", currentTime);
    }
    printf("Valid gantt chart logs: %d\n", validLogs);

    drawGanttChart(ganttLogArray, validLogs);
}




void runRoundRobin(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;
    int timeQuantum;
    int quantumCounter = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    printf("Enter time quantum for Round Robin: ");
    scanf("%d", &timeQuantum);


    // 간트 차트 초기화
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0;
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    printf("Round Robin Simulation (Quantum=%d) STARTED-----------\n", timeQuantum);

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

        //  현재 실행 중인 프로세스가 Time Quantum을 모두 사용했으면 프로세스 바꿔준다
        if (runningProcess != NULL && quantumCounter >= timeQuantum) {
            printf("Time %d: Process %d (Quantum expired)\n", currentTime, runningProcess->pid);

            // 현재 실행 중인 프로세스의 간트 차트 로그 종료
            if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == runningProcess->pid) {
                 ganttLogArray[ganttEntryCount-1].endTime = currentTime;
            }

            runningProcess->status = READY;
            if (!enqueue(&readyQueue, runningProcess)) { //다시 뒤로 들어감
                fprintf(stderr, "[Error] Ready queue full during preemption at time %d!\n", currentTime);
            }

            //  CPU 비움 처리 및 IDLE 로그 즉시 시작
            runningProcess = NULL;
            quantumCounter = 0;

            ganttLogArray[ganttEntryCount].startTime = currentTime;
            ganttLogArray[ganttEntryCount].pid = -1; // 일단 IDLE
            ganttLogArray[ganttEntryCount].endTime = 0;
            ganttEntryCount++;
        }

        //  CPU 할당
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue);
            quantumCounter = 0;

            if (runningProcess) {
                runningProcess->status = RUNNING;
                printf("Time %d: Process %d RUNNING (Remaining Burst: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->remaining_cpu_burst_time);

                //  이전 IDLE 로그 종료
                if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                } else if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                    //  이전 로그가 IDLE이 아닌데 열려있다면 강제 종료
                     ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                }


                // 새 프로세스 로그 시작
                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                ganttLogArray[ganttEntryCount].endTime = 0;
                ganttEntryCount++;
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;
            quantumCounter++;

            //  프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);

                // 현재 프로세스 로그 종료
                if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == runningProcess->pid) {
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;
                }

                // CPU 비움 처리 및 IDLE 로그 즉시 시작
                runningProcess = NULL;
                quantumCounter = 0;

                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0;
                ganttEntryCount++;
            }
            //  I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {

                printf("Time %d: Process %d requests I/O (Duration %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->io_burst_times[runningProcess->current_io_index]);

                // 현재 프로세스 로그 종료
                if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == runningProcess->pid) {
                     ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;
                }

                runningProcess->status = WAITING;
                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }

                // CPU 비움 처리 및 IDLE 로그 즉시 시작
                runningProcess = NULL;
                quantumCounter = 0;

                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0;
                ganttEntryCount++;
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
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

    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    if (isPreemptive) {
        printf("Combined Preemptive Priority Simulation STARTED-----------\n");
    } else {
        printf("Combined Non-Preemptive Priority Simulation STARTED-----------\n");
    }

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount ) {
        bool newEventOccurred = false; // 새 프로세스 도착 또는 I/O 완료 여부. 선점방식에서는 여기에 민감해진다

        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                // enqueue도 스케줄링 방식 별로 분리 (priority 기준으로 정렬되는 enqueue임)
                if (!enqueue_for_priority(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Priority: Ready queue full at time %d for P%d!\n", currentTime, processes[i]->pid);
                }
                // Assuming Process struct has 'priority' field for this print statement
                printf("Time %d: Process %d ARRIVED (Priority: %d, Burst: %d)\n", currentTime, processes[i]->pid, processes[i]->priority, processes[i]->cpu_burst_time);
                newEventOccurred = true; // 새로운 프로세스가 레디큐에 와도 선점될 수 있음
            }
        }

        // I/O 진행 및 완료 처리 (완료된 프로세스는 Priority 순서로 Ready Queue에 추가)
        int initialWaitQueueCount = waitQueue.count;
        IO_Operation_Priority(&readyQueue, &waitQueue, &terminatedCount, &currentTime);
        if (waitQueue.count < initialWaitQueueCount ) {
             newEventOccurred = true; // wait에서 ready로 간 프로세스가 있다면 선점 발생 가능
        }

        // 선점 판단 (Preemptive Priority인 경우 그리고 새 이벤트 발생 시, 현재 할당된 프로세스가 있는 경우)
        if (isPreemptive && newEventOccurred && runningProcess != NULL) {
            if (!isEmpty(&readyQueue)) {
                // enqueue_for_priority에서 정렬해주기 때문에 맨 앞의 프로세스가 가장 높은 우선순위를 가짐
                Process *highestPriorityInQueue = readyQueue.items[readyQueue.front];
                // 낮은 숫자일수록 높은 우선순위 (priority 값이 작을수록 우선순위가 높음)
                if (highestPriorityInQueue && highestPriorityInQueue->priority < runningProcess->priority) {
                    printf("Time %d: Process %d (Prio: %d) PREEMPTED by Process %d (Prio: %d)\n",
                           currentTime, runningProcess->pid, runningProcess->priority,
                           highestPriorityInQueue->pid, highestPriorityInQueue->priority);

                    // 현재 실행 중인 프로세스의 간트 차트 로그 종료
                    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == runningProcess->pid) {
                       ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                    }

                    runningProcess->status = READY;
                    // Add running process back to ready queue (sorted by priority)
                    if (!enqueue_for_priority(&readyQueue, runningProcess)) {
                        fprintf(stderr, "[Error] Priority: Ready queue full during preemption at time %d!\n", currentTime);
                    }

                    runningProcess = NULL; // CPU가 비게 됨, 다음 단계에서 새 프로세스 선택
                }
            }
        }

        // CPU 할당 (CPU가 비어있고 Ready Queue에 프로세스가 있을 경우)
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue); // Ready queue is already sorted by Priority logic

            if (runningProcess) {
                runningProcess->status = RUNNING;
                printf("Time %d: Process %d RUNNING (Priority: %d, Remaining Burst: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->priority, runningProcess->remaining_cpu_burst_time);

                // 간트 차트 업데이트: 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime; // IDLE 상태 종료
                }

                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0 || runningProcess->cpu_time_used >= runningProcess->cpu_burst_time) {
                runningProcess->remaining_cpu_burst_time = 0;
                if (runningProcess->cpu_time_used > runningProcess->cpu_burst_time) {
                    runningProcess->cpu_time_used = runningProcess->cpu_burst_time;
                }

                if (runningProcess->status != TERMINATED) {
                    runningProcess->status = TERMINATED;
                    runningProcess->completion_time = currentTime + 1;
                    terminatedCount++;

                    printf("Time %d: Process %d TERMINATED (CPU used: %d, Remaining: %d)\n",
                           currentTime + 1, runningProcess->pid,
                           runningProcess->cpu_time_used, runningProcess->remaining_cpu_burst_time);

                    ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                    runningProcess = NULL;

                }
                // 다음 IDLE 로그 시작 준비
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1;
                ganttLogArray[ganttEntryCount].endTime = 0;
                ganttEntryCount++;

            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {

                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d, Remaining CPU: %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time,
                       runningProcess->remaining_cpu_burst_time);

                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Priority: Wait queue full for P%d at time %d!\n", runningProcess->pid, currentTime);
                }

                runningProcess = NULL;



                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1;
                ganttLogArray[ganttEntryCount].endTime = 0;
                ganttEntryCount++;
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    if (isPreemptive) {
        printf("Combined Preemptive Priority Simulation COMPLETED at time %d-----------\n", currentTime);
    } else {
        printf("Combined Non-Preemptive Priority Simulation COMPLETED at time %d-----------\n", currentTime);
    }
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

    // 간트 차트 초기화 - 시작은 IDLE 상태
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


            // Time Quantum 만료 확인
            if (quantumCounter >= timeQuantum) {
                printf("Time %d: Process %d QUANTUM EXPIRED\n", currentTime, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                // 프로세스를 다시 Ready Queue에 넣음
                runningProcess->status = READY;
                if (!enqueue_for_priority(&readyQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);
                }

                runningProcess = NULL;
                quantumCounter = 0;

                // 다음 상태는 IDLE로 설정 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;
            }
        }

        // CPU 할당 (
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue);
            quantumCounter = 0; // 새 프로세스가 할당되면 quantum 카운터 초기화해준다

            if (runningProcess) {
                runningProcess->status = RUNNING;
                printf("Time %d: Process %d RUNNING (Priority: %d, Remaining Burst: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->priority,
                       runningProcess->remaining_cpu_burst_time);

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                // 이전 로그가 아직 열려있는 IDLE 로그이면 종료
                if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                }






                // 새 프로세스 로그 시작
                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                ganttLogArray[ganttEntryCount].endTime = 0;
                ganttEntryCount++;
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

void runLIFCombined(Process* processes[], int processCount, bool isPreemptive) {//longestIOFirst
    int currentTime = 0;        // 현재 시뮬레이션 시간
    int terminatedCount = 0;    // 종료된 프로세스 수
    int ganttEntryCount = 0;    // 간트 차트 로그 항목 수

    Queue readyQueue;           // 준비 큐
    Queue waitQueue;            // 대기 큐 (I/O 작업용)
    Process *runningProcess = NULL; // 현재 CPU에서 실행 중인 프로세스
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG]; // 간트 차트 기록 배열

    // 간트 차트 초기화: 시간 0에서 IDLE 상태로 시작
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1; // IDLE 상태 표시
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 종료되지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue); // 준비 큐 초기화
    initialize_queue(&waitQueue);  // 대기 큐 초기화

    if (isPreemptive) {
        printf("Combined Preemptive Longest IO First (LIF) Scheduling Simulation Start-----------\n");
    } else {
        printf("Combined NonPreemptive Longest IO First (LIF) Scheduling Simulation Start-----------\n");
    }

    // 모든 프로세스가 종료될 때까지 시뮬레이션 루프 실행
    while (terminatedCount < processCount) {
        bool newEventOccurred = false; // 새로운 이벤트(도착/I_O완료) 발생 여부 플래그

        // 현재 시간에 도착한 프로세스를 준비 큐에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                // enqueue_for_lif 함수는 process->io_burst_time 총 I/O 시간을 기준으로 정렬한다.
                if (!enqueue_for_lif(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Ready queue full at time %d!\n", currentTime);
                }
                printf("Time %d: Process %d Arrived (Total IO time: %d)\n",
                       currentTime, processes[i]->pid, processes[i]->io_burst_time);
                newEventOccurred = true;
            }
        }

        // I/O 작업 처리: 대기 큐의 프로세스들 I/O 진행 및 완료 시 준비 큐로 이동
        int initialWaitQueueCount = waitQueue.count;
        IO_Operation_LIF(&readyQueue, &waitQueue, &terminatedCount, &currentTime); // IO_Operation_LIF는 내부적으로 enqueue_for_lif 사용
        if (waitQueue.count < initialWaitQueueCount ) {
             newEventOccurred = true; // I/O 완료로 인한 새 이벤트 발생
        }

        // 선점형이고 새 이벤트 발생했을 상황 선점인지 아닌지
        if (isPreemptive && newEventOccurred && runningProcess != NULL) {
            if (!isEmpty(&readyQueue)) {
                Process *longestIOInQueue = readyQueue.items[readyQueue.front]; // 준비 큐에서 가장 긴 I/O 시간 프로세스 확인

                // process->io_burst_time은 프로세스의 총 초기 I/O 시간 기준
                if (longestIOInQueue && longestIOInQueue->io_burst_time > runningProcess->io_burst_time) {
                    printf("Time %d: Process %d (Total IO: %d) Preempted. Preempted by Process %d (total  IO: %d)\n",
                           currentTime, runningProcess->pid, runningProcess->io_burst_time,
                           longestIOInQueue->pid, longestIOInQueue->io_burst_time);

                    // 현재 실행 중인 프로세스의 간트 차트 로그 종료
                    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == runningProcess->pid) {
                       ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                    }

                    runningProcess->status = READY; // 상태를 READY로 변경
                    if (!enqueue_for_lif(&readyQueue, runningProcess)) { // 선점된 프로세스를 준비 큐에 다시 추가 (LIF 순서로 정렬)
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                    }
                    runningProcess = NULL; // CPU 비움
                }
            }
        }

        // CPU 할당 로직: CPU가 비어있고 준비 큐에 프로세스가 있는 경우
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue); // 준비 큐는 LIF 순서로 정렬되어 있음

            if (runningProcess) {
                runningProcess->status = RUNNING; // 상태를 RUNNING으로 변경
                printf("Time %d: Process %d RUNNING (Total IO: %d, Remaining CPU: %d)\n",
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
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--; // 남은 CPU 시간 감소
            runningProcess->cpu_time_used++;            // 사용한 CPU 시간 증가

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED; // 상태를 TERMINATED로 변경
                runningProcess->completion_time = currentTime + 1; // 종료 시간 기록 (다음 시간 단위 시작 시점)
                terminatedCount++; // 종료된 프로세스 수 증가
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);

                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1; // 현재 프로세스 간트 차트 로그 종료
                runningProcess = NULL; // CPU 비움

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

                printf("Time %d: Process %d I/O Request (I/O Time: %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1; // 현재 프로세스 간트 차트 로그 종료

                if (!enqueue(&waitQueue, runningProcess)) { // 대기 큐
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }
                runningProcess = NULL; // CPU 비움

                // 모든 프로세스가 종료되지 않았다면 IDLE 로그 시작 준비
                if (terminatedCount < processCount) {
                    ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                    ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                    ganttLogArray[ganttEntryCount].endTime = 0;
                    ganttEntryCount++;
                }
            }
        }
        currentTime++; // 시간 증가
    }

    // 마지막 간트 차트 로그 항목의 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효한 간트 차트 로그만 필터링 (시작 시간 < 종료 시간)
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

    // 시뮬레이션 종료 메시지 출력
    if (isPreemptive) {
        printf("Combined Preemptive Longest IO First (LIF) Scheduling Simulation Completed (Time: %d)-----------\n", currentTime);
    } else {
        printf("Combined NONPreemptive Longest IO First (LIF) Scheduling Simulation Completed (Time: %d)-----------\n", currentTime);
    }
    printf("Valid logs number: %d\n", validLogs);
    drawGanttChart(ganttLogArray, validLogs); // 간트 차트 그리기 함수 호출
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
    printf("Time allocation: 70%% foreground, 30%% background\n");

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
        IO_Operation(&foregroundReadyQueue,&foregroundWaitQueue,&terminatedCount,&currentTime);


        // Background queue I/O 작업 처리
        IO_Operation(&backgroundReadyQueue,&backgroundWaitQueue,&terminatedCount,&currentTime);


        // 큐 선택: 70%는 foreground, 30%는 background 큐에 할당
        int whichQueue;
        if (currentTime % 10 < 7) { // 70% 확률로 foreground 큐 선택 cpu 할당 비율 높게!
            whichQueue = 0; // foreground
        } else { // 20% 확률로 background 큐 선택
            whichQueue = 1; // background
        }

        // 현재 서비스 중인 큐가 다르고, 선택된 큐에 프로세스가 있으면 선점
        // 이게 밑에서 할당된 큐에 프로세스가 없으면 다른 큐에 있는 프로세스 실행하도록 했는데 자기 할당 시간에 프로세스 생기면 그거 실행하는게 맞지
        if (runningProcess != NULL && currentQueue != whichQueue) {
            if ((whichQueue == 0 && !isEmpty(&foregroundReadyQueue)) ||
                (whichQueue == 1 && !isEmpty(&backgroundReadyQueue))) {


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
            }
        }

        // CPU 할당 (이전 프로세스가 종료되었거나 선점되었을 때)
        if (runningProcess == NULL) {
            // 선택된 큐에서 프로세스를 가져옴
            if (whichQueue == 0 && !isEmpty(&foregroundReadyQueue)) {
                runningProcess = dequeue(&foregroundReadyQueue);
                currentQueue = 0; // fore
                quantumCounter = 0; // foreground 큐는 Round Robin 사용
                printf("Time %d: Process %d RUNNING from Foreground Queue\n",
                       currentTime, runningProcess->pid);
            } else if (whichQueue == 1 && !isEmpty(&backgroundReadyQueue)) {
                runningProcess = dequeue(&backgroundReadyQueue);
                currentQueue = 1; //back
                printf("Time %d: Process %d RUNNING from Background Queue\n",
                       currentTime, runningProcess->pid);
            }
            // 선택된 큐가 비어있으면 다른 큐를 확인
            else if (whichQueue == 0 && !isEmpty(&backgroundReadyQueue)) {
                runningProcess = dequeue(&backgroundReadyQueue);
                currentQueue = 1; //back
                printf("Time %d: Process %d RUNNING from Background Queue (Foreground empty)\n",
                       currentTime, runningProcess->pid);
            } else if (whichQueue == 1 && !isEmpty(&foregroundReadyQueue)) {
                runningProcess = dequeue(&foregroundReadyQueue);
                currentQueue = 0; //fore
                quantumCounter = 0; // foreground 큐는 Round Robin 사용
                printf("Time %d: Process %d RUNNING from Foreground Queue (Background empty)\n",
                       currentTime, runningProcess->pid);
            }

            // 프로세스 할당 성공 시 간트 차트 업데이트
            if (runningProcess) {
                runningProcess->status = RUNNING;

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (ganttLogArray[ganttEntryCount-1].pid == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
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

                // 다음 상태는 IDLE로 일단 해놓기
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                quantumCounter = 0;
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

Process* selectProcessByLottery(Queue* readyQueue) {// 티켓 수에 따른 프로세스 선택 확률 계산해주는 함수 따로 구현
    if (isEmpty(readyQueue)) {
        return NULL;
    }


    int totalTickets = 0;
    Process* processes[MAX_QUEUE_CAPACITY];
    int tickets[MAX_QUEUE_CAPACITY];
    int count = 0;

    // 큐에서 모든 프로세스와 티켓 정보 수집
    int current_idx = readyQueue->front;
    for (int i = 0; i < readyQueue->count; i++) {
        Process* process = readyQueue->items[current_idx];
        if (process) {
            processes[count] = process;
            tickets[count] = calculateTickets(process->priority);
            totalTickets += tickets[count];
            count++;
        }
        current_idx = (current_idx + 1) % MAX_QUEUE_CAPACITY;
    }

    if (totalTickets == 0) {
        return NULL;
    }

    // 당첨 티켓 선택
    int winningTicket = rand() % totalTickets;

    // 당첨 프로세스 찾기
    int ticketCounter = 0;
    Process* selectedProcess = NULL;

    for (int i = 0; i < count; i++) {
        ticketCounter += tickets[i];
        if (winningTicket < ticketCounter) {
            selectedProcess = processes[i];
            break;
        }
    }

    if (!selectedProcess) {
        return NULL; // 이 부분에 도달하면 안 됨
    }

    // 안전하게 큐에서 선택된 프로세스 제거
    Queue tempQueue;
    initialize_queue(&tempQueue);
    Process* ActuallySelectedProcess = NULL;

    while (!isEmpty(readyQueue)) {
        Process* dequeuedProc = dequeue(readyQueue);
        if (dequeuedProc->pid == selectedProcess->pid && !ActuallySelectedProcess) {
            ActuallySelectedProcess = dequeuedProc;
        } else {
            enqueue(&tempQueue, dequeuedProc);
        }
    }

    // 임시 큐의 내용을 다시 readyQueue로 복원
    while (!isEmpty(&tempQueue)) {
        enqueue(readyQueue, dequeue(&tempQueue));
    }

    return ActuallySelectedProcess;
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
                if (ganttLogArray[ganttEntryCount-1].pid == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
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

                // I/O 상태 시작 - 다음 상태는 IDLE로 해놓자
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Wait queue full at time %d!\n", currentTime);
                }

                runningProcess = NULL;
            }
            // 일단 로터리 알고리즘은 비선점형으로...
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

void runLISCCombined(Process* processes[], int processCount, bool isPreemptive) {//LongestIOShortestCPU
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    // 간트 차트 초기화: 시간 0에서 IDLE 상태로 시작
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;
    ganttLogArray[ganttEntryCount].endTime = 0;
    ganttEntryCount++;

    initialize_queue(&readyQueue); // 준비 큐 초기화
    initialize_queue(&waitQueue);  // 대기 큐 초기화

    if (isPreemptive) {
        printf("Combined Preemptive LISC (Longest IO, Shortest CPU) Scheduling Simulation STARTED-----------\n");
    } else {
        printf("Combined Non-Preemptive LISC (Longest IO, Shortest CPU) Scheduling Simulation STARTED-----------\n");
    }

    // 모든 프로세스가 종료될 때까지 시뮬레이션 루프 실행
    while (terminatedCount < processCount) {
        bool newEventOccurred = false; // 새로운 이벤트 있냐..

        // 프로세스 도착 처리: 현재 시간에 도착한 프로세스를 준비 큐에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                // enqueue_for_lisc 함수는 총 I/O 시간 (긴 순) -> 남은 CPU 시간 (짧은 순) 기준으로 정렬
                if (!enqueue_for_lisc(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] LISC: Ready queue full for P%d at time %d!\n", processes[i]->pid, currentTime);
                }
                printf("Time %d: Process %d ARRIVED (Total IO: %d, CPU Burst: %d)\n",
                       currentTime, processes[i]->pid, processes[i]->io_burst_time, processes[i]->remaining_cpu_burst_time);
                newEventOccurred = true;
            }
        }

        // I/O 작업 처리: 대기 큐의 프로세스들 I/O 진행 및 완료 시 준비 큐로 이동
        int initialWaitQueueCount = waitQueue.count;
        IO_Operation_LISC(&readyQueue, &waitQueue, &terminatedCount, &currentTime); // IO_Operation_LISC는 내부적으로 enqueue_for_lisc 사용
         if (waitQueue.count < initialWaitQueueCount || (initialWaitQueueCount > 0 && readyQueue.count > (readyQueue.count - (initialWaitQueueCount - waitQueue.count)))) {
             newEventOccurred = true; // I/O 완료로 인한 이벤트 발생
        }

        //  선점 로직 (선점형 LISC이고 새로운 이벤트가 발생했을 경우)
        if (isPreemptive && newEventOccurred && runningProcess != NULL) {
            if (!isEmpty(&readyQueue)) {
                Process *candidateProcess = readyQueue.items[readyQueue.front]; // 준비 큐에서 최우선순위 LISC 프로세스 확인 (Peek)

                if (candidateProcess) {
                    bool PreemptOrNot = false;
                    // LISC 우선순위 비교:
                    // 총 I/O 시간이 더 긴 경우
                    if (candidateProcess->io_burst_time > runningProcess->io_burst_time) {
                        PreemptOrNot = true;
                    }
                    // 총 I/O 시간이 같고, 남은 CPU 시간이 더 짧은 경우
                    else if (candidateProcess->io_burst_time == runningProcess->io_burst_time &&
                               candidateProcess->remaining_cpu_burst_time < runningProcess->remaining_cpu_burst_time) {
                        PreemptOrNot = true;
                    }

                    if (PreemptOrNot) {
                        printf("Time %d: Process %d (IO: %d, CPU: %d) PREEMPTED by Process %d (IO: %d, CPU: %d)\n",
                               currentTime,
                               runningProcess->pid, runningProcess->io_burst_time, runningProcess->remaining_cpu_burst_time,
                               candidateProcess->pid, candidateProcess->io_burst_time, candidateProcess->remaining_cpu_burst_time);

                        // 현재 실행 중인 프로세스의 간트 차트 로그 종료
                        if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == runningProcess->pid) {
                           ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                        }

                        runningProcess->status = READY; // 상태를 READY로 변경
                        if (!enqueue_for_lisc(&readyQueue, runningProcess)) { // 선점된 프로세스를 준비 큐에 다시 추가 (LISC 순서로 정렬)
                            fprintf(stderr, "[Error] LISC: Ready queue full during preemption at time %d!\n", currentTime);
                        }
                        runningProcess = NULL; // CPU 비움
                    }
                }
            }
        }

        // CPU 할당 로직: CPU가 비어있고 준비 큐에 프로세스가 있는 경우
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
            }
        }

        // CPU 실행..
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("시간 %d: 프로세스 %d 종료됨\n", currentTime + 1, runningProcess->pid);

                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;
                runningProcess = NULL;

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

                printf("Time %d: Process %d requests I/O (Duration: %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time);

                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                if (!enqueue(&waitQueue, runningProcess)) { // 대기 큐는 FCFS
                    fprintf(stderr, "[Error] LISC: Wait queue full for P%d at time %d!\n", runningProcess->pid, currentTime);
                }
                runningProcess = NULL;

                if (terminatedCount < processCount) {
                    ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                    ganttLogArray[ganttEntryCount].pid = -1;
                    ganttLogArray[ganttEntryCount].endTime = 0;
                    ganttEntryCount++;
                }
            }
        }
        currentTime++; // 시간 증가
    }

    // 마지막 간트 차트 로그 항목의 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효한 간트 차트 로그만 남겨두자
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

    // 시뮬레이션 종료 메시지
    if (isPreemptive) {
        printf("Combined Preemptive LISC Scheduling Simulation COMPLETED at time %d-----------\n", currentTime);
    } else {
        printf("Combined Non-Preemptive LISC Scheduling Simulation COMPLETED at time %d-----------\n", currentTime);
    }
    printf("Valid gantt chart logs: %d\n", validLogs);
    drawGanttChart(ganttLogArray, validLogs); // 간트 차트 출력
}

void runHRRN(Process* processes[], int processCount)
{
    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    // 간트 차트 초기화
    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;
    ganttLogArray[ganttEntryCount].endTime = 0;
    ganttEntryCount++;

    printf("HRRN Scheduling Simulation STARTED-----------\n");

    while (terminatedCount < processCount) {
        // 프로세스 도착 처리
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                processes[i]->time_entered_ready = currentTime;

                //응답률 순으로 정렬해주는 enqueue
                if (!enqueue_for_hrrn(&readyQueue, processes[i], currentTime)) {
                    fprintf(stderr, "[Error] HRRN: Ready queue full on arrival for P%d!\n", processes[i]->pid);
                }
                printf("Time %d: Process %d ARRIVED (Entered Ready at %d)\n",
                       currentTime, processes[i]->pid, processes[i]->time_entered_ready);
            }
        }

        // I/O 처리
        IO_Operation_HRRN(&readyQueue, &waitQueue, currentTime, &terminatedCount);

        // CPU 할당 큐의 맨 앞이 항상 최고 응답률
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue); // 큐에서 첫 번째 프로세스 선택

            if (runningProcess) {
                runningProcess->status = RUNNING;
                runningProcess->time_entered_ready = -1;

                // 응답률 계산
                int waitingTime = currentTime - runningProcess->time_entered_ready;
                double ratio = (double)(waitingTime + runningProcess->cpu_burst_time) / runningProcess->cpu_burst_time;

                printf("Time %d: Process %d selected (HRRN Ratio=%.2f). Running.\n",
                       currentTime, runningProcess->pid, ratio);

                // 간트 차트 로깅
                if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                }
                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                ganttLogArray[ganttEntryCount].endTime = 0;
                ganttEntryCount++;
            }
        }

        // CPU 실행 부분은 기존과 동일
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0) {
                runningProcess->status = TERMINATED;
                runningProcess->completion_time = currentTime + 1;
                terminatedCount++;
                printf("Time %d: Process %d TERMINATED\n", currentTime + 1, runningProcess->pid);
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;
                runningProcess = NULL;

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
                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] HRRN: Wait queue full for P%d!\n", runningProcess->pid);
                }
                runningProcess = NULL;
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1;
                ganttLogArray[ganttEntryCount].endTime = 0;
                ganttEntryCount++;
            }
        }

        currentTime++;
    }

    // 마지막 로그 정리 및 간트 차트 출력
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

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

    printf("HRRN Scheduling Simulation COMPLETED at time %d-----------\n", currentTime);
    drawGanttChart(ganttLogArray, validLogs);
}

void runMultiLevelFeedBackQueue(Process* processes[], int processCount)
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

    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&foregroundReadyQueue);
    initialize_queue(&backgroundReadyQueue);
    initialize_queue(&foregroundWaitQueue);
    initialize_queue(&backgroundWaitQueue);

    printf("Multi-Level FeedBack Queue Scheduling Simulation STARTED-----------\n");
    printf("Foreground queue (priority <= %d): Round Robin (quantum=%d)\n", PRIORITY_DIVIDER/2, timeQuantum);
    printf("Background queue (priority > %d): FCFS\n", PRIORITY_DIVIDER/2);
    printf("Time allocation: 70%% foreground, 30%% background\n");

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
                    processes[i]->time_entered_ready = currentTime;
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
        IO_Operation(&foregroundReadyQueue,&foregroundWaitQueue,&terminatedCount,&currentTime);


        // Background queue I/O 작업 처리
        IO_Operation_Background(&backgroundReadyQueue,&backgroundWaitQueue,&terminatedCount,currentTime);


        // 큐 선택: 70%는 foreground, 30%는 background 큐에 할당
        int whichQueue;
        if (currentTime % 10 < 7) { // 70% 확률로 foreground 큐 선택 cpu 할당 비율 높게!
            whichQueue = 0; // foreground
        } else { // 20% 확률로 background 큐 선택
            whichQueue = 1; // background
        }

        // 현재 서비스 중인 큐가 다르고, 선택된 큐에 프로세스가 있으면 선점
        // 이게 밑에서 할당된 큐에 프로세스가 없으면 다른 큐에 있는 프로세스 실행하도록 했는데 자기 할당 시간에 프로세스 생기면 그거 실행하는게 맞지
        if (runningProcess != NULL && currentQueue != whichQueue) {
            if ((whichQueue == 0 && !isEmpty(&foregroundReadyQueue)) ||
                (whichQueue == 1 && !isEmpty(&backgroundReadyQueue))) {


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
            }
        }

        // CPU 할당 (이전 프로세스가 종료되었거나 선점되었을 때)
        if (runningProcess == NULL) {
            // 선택된 큐에서 프로세스를 가져옴
            if (whichQueue == 0 && !isEmpty(&foregroundReadyQueue)) {
                runningProcess = dequeue(&foregroundReadyQueue);
                currentQueue = 0; // fore
                quantumCounter = 0; // foreground 큐는 Round Robin 사용
                printf("Time %d: Process %d RUNNING from Foreground Queue\n",
                       currentTime, runningProcess->pid);
            } else if (whichQueue == 1 && !isEmpty(&backgroundReadyQueue)) {
                runningProcess = dequeue(&backgroundReadyQueue);
                currentQueue = 1; //back
                printf("Time %d: Process %d RUNNING from Background Queue\n",
                       currentTime, runningProcess->pid);
            }
            // 선택된 큐가 비어있으면 다른 큐를 확인
            else if (whichQueue == 0 && !isEmpty(&backgroundReadyQueue)) {
                runningProcess = dequeue(&backgroundReadyQueue);
                currentQueue = 1; //back
                printf("Time %d: Process %d RUNNING from Background Queue (Foreground empty)\n",
                       currentTime, runningProcess->pid);
            } else if (whichQueue == 1 && !isEmpty(&foregroundReadyQueue)) {
                runningProcess = dequeue(&foregroundReadyQueue);
                currentQueue = 0; //fore
                quantumCounter = 0; // foreground 큐는 Round Robin 사용
                printf("Time %d: Process %d RUNNING from Foreground Queue (Background empty)\n",
                       currentTime, runningProcess->pid);
            }

            // 프로세스 할당 성공 시 간트 차트 업데이트
            if (runningProcess) {
                runningProcess->status = RUNNING;

                // 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (ganttLogArray[ganttEntryCount-1].pid == -1) {
                    // 마지막 로그가 IDLE이면 종료
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                    // 새 프로세스 로그 시작
                    ganttLogArray[ganttEntryCount].startTime = currentTime;
                    ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                    ganttEntryCount++;
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

                // 다음 상태는 IDLE로 일단 해놓기
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                quantumCounter = 0;
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
            }
            // foreground 큐의 time quantum 만료 확인 (background 큐는 FCFS이므로 확인 안함)
            else if (currentQueue == 0 && quantumCounter >= timeQuantum) {
                printf("Time %d: Process %d QUANTUM EXPIRED (Returned to Foreground Queue)\n",
                       currentTime, runningProcess->pid);

                // 현재 프로세스 로그 종료
                ganttLogArray[ganttEntryCount-1].endTime = currentTime;

                // 프로세스가 타임퀀텀내에 안끝났으므로 background queue 로 강등시켜준다.
                runningProcess->status = READY;
                runningProcess->priority = PRIORITY_DIVIDER/2 + 1;
                if (!enqueue(&backgroundReadyQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Foreground queue full during quantum expiration!\n");
                }

                // 다음 상태는 IDLE로 시작 (다음 루프에서 프로세스가 할당되면 변경됨)
                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = -1; // IDLE
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;

                runningProcess = NULL;
                quantumCounter = 0;
            }
        }


        //5초에 한번씩 큐 변동 조건 확인하자..aging으로 해서..
        if (currentTime % 5 ==0) {
            Queue temp_for_upgrade;
            initialize_queue(&temp_for_upgrade);
            for (int i = 0; i < backgroundReadyQueue.count; i++) {
                Process* process = dequeue(&backgroundReadyQueue); // background에서 하나씩 꺼냄


                // 15 타임 유닛 이상 backgroundReadyQueue에서 대기했는지 확인
                if ((currentTime - process->time_entered_ready) > 15) {
                    printf("Time %d: Process %d (Original Prio: %d) AGING from BackgroundQ (waited > 15 units).\n",
                           currentTime, process->pid, process->priority);

                    // Foreground 큐로 옮기기 위해 우선순위 조정
                    process->priority = PRIORITY_DIVIDER / 2;

                    if (!enqueue(&foregroundReadyQueue, process)) { // Foreground 큐로 이동
                        fprintf(stderr, "[Error] Aging: ForegroundReadyQueue full for P%d at time %d. Returning to Background.\n",
                                process->pid, currentTime);
                        // Foreground 큐가 가득 찼다면, 다시 background 큐로 넣음
                        enqueue(&temp_for_upgrade, process);
                    } else {
                        printf("Time %d: Process %d MOVED to ForegroundQueue (New Prio: %d).\n",
                               currentTime, process->pid, process->priority);
                        // Foreground 큐로 성공적으로 이동했다면, Foreground 큐에서의 대기 시간 등을 위한
                    }
                } else {
                    // 아직 에이징 대상이 아니면 임시 큐에 다시 보관
                    enqueue(&temp_for_upgrade, process);
                }
            }

            //  임시 큐에 있던 프로세스들을 다시 backgroundReadyQueue로 복원
            while (!isEmpty(&temp_for_upgrade)) {
                enqueue(&backgroundReadyQueue, dequeue(&temp_for_upgrade));
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
void runPriorityAgingCombined(Process* processes[], int processCount, bool isPreemptive) {

    int currentTime = 0;
    int terminatedCount = 0;
    int ganttEntryCount = 0;

    Queue readyQueue;
    Queue waitQueue;
    Process *runningProcess = NULL;
    GanttChartLog ganttLogArray[MAX_GANTTCHART_LOG];

    ganttLogArray[ganttEntryCount].startTime = currentTime;
    ganttLogArray[ganttEntryCount].pid = -1;  // IDLE 상태
    ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
    ganttEntryCount++;

    initialize_queue(&readyQueue);
    initialize_queue(&waitQueue);

    if (isPreemptive) {
        printf("Combined Preemptive Priority Simulation STARTED-----------\n");
    } else {
        printf("Combined Non-Preemptive Priority Simulation STARTED-----------\n");
    }

    // 모든 프로세스가 종료될 때까지 루프 실행
    while (terminatedCount < processCount ) {
        bool newEventOccurred = false; // 새 프로세스 도착 또는 I/O 완료 여부. 선점방식에서는 여기에 민감해진다

        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        for (int i = 0; i < processCount; i++) {
            if (processes[i] && processes[i]->status == NEW && processes[i]->arrival_time <= currentTime) {
                processes[i]->status = READY;
                processes[i]->time_entered_ready = currentTime;
                // enqueue도 스케줄링 방식 별로 분리 (priority 기준으로 정렬되는 enqueue임)
                if (!enqueue_for_priority(&readyQueue, processes[i])) {
                    fprintf(stderr, "[Error] Priority: Ready queue full at time %d for P%d!\n", currentTime, processes[i]->pid);
                }
                // Assuming Process struct has 'priority' field for this print statement
                printf("Time %d: Process %d ARRIVED (Priority: %d, Burst: %d)\n", currentTime, processes[i]->pid, processes[i]->priority, processes[i]->cpu_burst_time);
                newEventOccurred = true; // 새로운 프로세스가 레디큐에 와도 선점될 수 있음
            }
        }

        // I/O 진행 및 완료 처리 (완료된 프로세스는 Priority 순서로 Ready Queue에 추가)
        int initialWaitQueueCount = waitQueue.count;
        IO_Operation_Aging_Priority(&readyQueue, &waitQueue, &terminatedCount, currentTime);
        if (waitQueue.count < initialWaitQueueCount ) {
            newEventOccurred = true; // wait에서 ready로 간 프로세스가 있다면 선점 발생 가능
        }

        // 선점 판단 (Preemptive Priority인 경우 그리고 새 이벤트 발생 시, 현재 할당된 프로세스가 있는 경우)
        if (isPreemptive && newEventOccurred && runningProcess != NULL) {
            if (!isEmpty(&readyQueue)) {
                // enqueue_for_priority에서 정렬해주기 때문에 맨 앞의 프로세스가 가장 높은 우선순위를 가짐
                Process *highestPriorityInQueue = readyQueue.items[readyQueue.front];
                // 낮은 숫자일수록 높은 우선순위 (priority 값이 작을수록 우선순위가 높음)
                if (highestPriorityInQueue && highestPriorityInQueue->priority < runningProcess->priority) {
                    printf("Time %d: Process %d (Prio: %d) PREEMPTED by Process %d (Prio: %d)\n",
                           currentTime, runningProcess->pid, runningProcess->priority,
                           highestPriorityInQueue->pid, highestPriorityInQueue->priority);

                    // 현재 실행 중인 프로세스의 간트 차트 로그 종료
                    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == runningProcess->pid) {
                        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
                    }

                    runningProcess->status = READY;
                    // Add running process back to ready queue (sorted by priority)
                    if (!enqueue_for_priority(&readyQueue, runningProcess)) {
                        fprintf(stderr, "[Error] Priority: Ready queue full during preemption at time %d!\n", currentTime);
                    }

                    runningProcess = NULL; // CPU가 비게 됨, 다음 단계에서 새 프로세스 선택
                }
            }
        }

        // CPU 할당 (CPU가 비어있고 Ready Queue에 프로세스가 있을 경우)
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            runningProcess = dequeue(&readyQueue); // Ready queue is already sorted by Priority logic

            if (runningProcess) {
                runningProcess->status = RUNNING;
                printf("Time %d: Process %d RUNNING (Priority: %d, Remaining Burst: %d)\n",
                       currentTime, runningProcess->pid, runningProcess->priority, runningProcess->remaining_cpu_burst_time);

                // 간트 차트 업데이트: 이전 IDLE 로그 종료 및 새 프로세스 로그 시작
                if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].pid == -1 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
                    ganttLogArray[ganttEntryCount-1].endTime = currentTime; // IDLE 상태 종료
                }

                ganttLogArray[ganttEntryCount].startTime = currentTime;
                ganttLogArray[ganttEntryCount].pid = runningProcess->pid;
                ganttLogArray[ganttEntryCount].endTime = 0; // 아직 끝나지 않음
                ganttEntryCount++;
            }
        }

        // CPU 실행
        if (runningProcess != NULL) {
            runningProcess->remaining_cpu_burst_time--;
            runningProcess->cpu_time_used++;

            // 프로세스 종료 검사
            if (runningProcess->remaining_cpu_burst_time <= 0 || runningProcess->cpu_time_used >= runningProcess->cpu_burst_time) {
                runningProcess->remaining_cpu_burst_time = 0;
                if (runningProcess->cpu_time_used > runningProcess->cpu_burst_time) {
                    runningProcess->cpu_time_used = runningProcess->cpu_burst_time;
                }

                if (runningProcess->status != TERMINATED) {
                    runningProcess->status = TERMINATED;
                    runningProcess->completion_time = currentTime + 1;
                    terminatedCount++;

                    printf("Time %d: Process %d TERMINATED (CPU used: %d, Remaining: %d)\n",
                           currentTime + 1, runningProcess->pid,
                           runningProcess->cpu_time_used, runningProcess->remaining_cpu_burst_time);

                    ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                    runningProcess = NULL;

                }
                // 다음 IDLE 로그 시작 준비
                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1;
                ganttLogArray[ganttEntryCount].endTime = 0;
                ganttEntryCount++;

            }
            // I/O 요청 확인
            else if (runningProcess->current_io_index < runningProcess->io_count &&
                     runningProcess->cpu_time_used == runningProcess->io_trigger[runningProcess->current_io_index]) {

                runningProcess->remaining_io_burst_time = runningProcess->io_burst_times[runningProcess->current_io_index];
                runningProcess->status = WAITING;

                printf("Time %d: Process %d requests I/O (Duration %d, Remaining CPU: %d)\n",
                       currentTime + 1, runningProcess->pid, runningProcess->remaining_io_burst_time,
                       runningProcess->remaining_cpu_burst_time);

                ganttLogArray[ganttEntryCount-1].endTime = currentTime + 1;

                if (!enqueue(&waitQueue, runningProcess)) {
                    fprintf(stderr, "[Error] Priority: Wait queue full for P%d at time %d!\n", runningProcess->pid, currentTime);
                }

                runningProcess = NULL;



                ganttLogArray[ganttEntryCount].startTime = currentTime + 1;
                ganttLogArray[ganttEntryCount].pid = -1;
                ganttLogArray[ganttEntryCount].endTime = 0;
                ganttEntryCount++;
                     }
        }
        //5초에 한번씩 aging 시켜주자
        if (currentTime % 5 == 0 && !isEmpty(&readyQueue)) {
            for (int i = 0; i < readyQueue.count; i++) {
                Process *process = readyQueue.items[i];
                if (currentTime - process->time_entered_ready > 15) {
                    process->priority = process->priority / 2; // 최소값 제한
                }
            }

            Process *process = dequeue(&readyQueue);

            if (!enqueue_for_priority(&readyQueue, process)) {
                fprintf(stderr, "[Error] Priority: Ready queue %d!\n", currentTime);
            }
        }

        currentTime++;
    }

    // 마지막 로그 항목 종료 시간 설정
    if (ganttEntryCount > 0 && ganttLogArray[ganttEntryCount-1].endTime == 0) {
        ganttLogArray[ganttEntryCount-1].endTime = currentTime;
    }

    // 유효하지 않은 로그 항목 제거
    int validLogs = 0;
    for (int i = 0; i < ganttEntryCount; i++) {
        if (ganttLogArray[i].startTime < ganttLogArray[i].endTime) {
            if (i != validLogs) {
                ganttLogArray[validLogs] = ganttLogArray[i];
            }
            validLogs++;
        }
    }

    if (isPreemptive) {
        printf("Combined Preemptive Priority Simulation COMPLETED at time %d-----------\n", currentTime);
    } else {
        printf("Combined Non-Preemptive Priority Simulation COMPLETED at time %d-----------\n", currentTime);
    }
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

    case MultiLevelFeedbackQueue:
        printf("Multi-Level Feedback Queue algorithm selected\n");
        runMultiLevelFeedBackQueue(processes, processCount);
        break;

    case PriorityAgingNonPreemtive:
        printf("Priority Aging algorithm selected\n");
        runPriorityAgingCombined(processes, processCount, false);
        break;

    case PriorityAgingPreemtive:
        printf("Priority Aging algorithm selected\n");
        runPriorityAgingCombined(processes, processCount, true);
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
        process->remaining_io_burst_time--;
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

                if (!enqueue_for_hrrn(readyQueue, process,currentTime)) {
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

void IO_Operation_Background(Queue *readyQueue, Queue *waitQueue, int *terminatedCount, int currentTime)
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

            // CPU 버스트 잔여량 확인
            if (process->remaining_cpu_burst_time <= 0) {
                process->remaining_cpu_burst_time = 0; // 음수 방지
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
        process->remaining_io_burst_time--;
    }
}
void IO_Operation_Aging_Priority(Queue *readyQueue, Queue *waitQueue, int *terminatedCount, int currentTime)
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

            // CPU 버스트 잔여량 확인
            if (process->remaining_cpu_burst_time <= 0) {
                process->remaining_cpu_burst_time = 0; // 음수 방지
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
        process->remaining_io_burst_time--;
    }
}