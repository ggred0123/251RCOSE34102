
#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "process.h"
#include "queue.h"
#define MAX_TOTAL_TICKETS_POSSIBLE (MAX_QUEUE_CAPACITY * PRIORITY_DIVIDER * 2)
#define FCFS 1
#define NonPreemptiveSJF 2
#define RoundRobin 3
#define PRIORITY 4
#define PreemptiveSJF 5
#define PreemptivePriority 6
#define RoundRobinWithPriority 7
#define LongestIOFirst 8
#define PreemptiveLongestIOFirst 9
#define MultiLevelQueue 10
#define LOTTERY 11
#define LongestIOShortestCPU 12
#define PreemptiveLongestIOShortestCPU 13
#define HRRN 14


//메인 스케줄러 함수로 지정된 알고리즘으로 프로세스들 스케줄링 해준다. 위에 define으로 여기서는 Int로 들어가고 입력시에는 알아보기 쉽게 이름으로
void runScheduler(Process* processes[], int processCount, int algorithm);

void runFCFS(Process* processes[], int processCount);

void runNonPreemptiveSJF(Process* processes[], int processCount);

void runRoundRobin(Process* processes[], int processCount);

void runPriority(Process* processes[], int processCount);

void runPreemptiveSJF(Process* processes[], int processCount);

void runRoundRobinWithPriority(Process* processes[], int processCount);


void runLongestIOFirst(Process* processes[], int processCount);

void runPreemptiveLongestIOFirst(Process* processes[], int processCount);

void runMultiLevelQueue(Process* processes[], int processCount);


//프로세스 상태, 메트릭 초기화 함수
void resetProcesses(Process* processes[], int processCount);

//IO 작업 처리
void IO_Operation(Queue *readyQueue, Queue *waitQueue,int *terminatedCount, int *currentTime);

void IO_Operation_SJF(Queue *readyQueue, Queue *waitQueue, int *terminatedCount, int *currentTime);

void IO_Operation_Priority(Queue *readyQueue, Queue *waitQueue, int *terminatedCount, int *currentTime);

void IO_Operation_LIF(Queue *readyQueue, Queue *waitQueue, int *terminatedCount, int *currentTime);

void IO_Operation_LISC(Queue *readyQueue, Queue *waitQueue, int *terminatedCount, int *currentTime);

void IO_Operation_HRRN(Queue *readyQueue, Queue *waitQueue, int currentTime, int *terminatedCount);

typedef bool (*EnqueueFunction)(Queue*, Process*);


void IO_Operation_Base(
    Queue *readyQueue,
    Queue *waitQueue,
    bool (*enqueueFunc)(Queue*, Process*),
    int currentTime,
    bool useCurrentTime,
    const char *algorithmName,
    int *terminatedCount);

#endif //SCHEDULER_H
