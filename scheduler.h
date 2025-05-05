
#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "process.h"
#include "queue.h"
#define FCFS 1
#define SJF 2
#define RoundRobin 3


//메인 스케줄러 함수로 지정된 알고리즘으로 프로세스들 스케줄링 해준다. 위에 define으로 여기서는 Int로 들어가고 입력시에는 알아보기 쉽게 이름으로
void runScheduler(Process* processes[], int processCount, int algorithm);


//프로세스 상태, 메트릭 초기화 함수
void resetProcesses(Process* processes[], int processCount);

//IO 작업 처리
void IO_Operation(Queue *readyQueue, Queue *waitQueue);





#endif //SCHEDULER_H
