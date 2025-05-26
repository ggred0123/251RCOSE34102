#ifndef QUEUE_H
#define QUEUE_H
#include "process.h"
#include "stdbool.h"
#define MAX_QUEUE_CAPACITY 100

typedef struct Queue {
    Process *items[MAX_QUEUE_CAPACITY]; // 프로세스 포인터 저장 배열
    int      front;                     // 큐 시작 인덱스
    int      rear;                      // 큐 마지막 인덱스
    int      count;                     // 큐에 있는 요소의 개수
} Queue;

// 큐 초기화
void initialize_queue(Queue *queue);

// 큐 비어있나?
bool isEmpty(Queue *queue);

// 큐 가득찼나?
bool isFull(Queue *queue);

// 큐 맨 뒤에 프로세스 추가 성공시 true 가득 차있으면 false
bool enqueue(Queue *queue, Process *process);

// 큐 맨 앞에서 프로세스 제거하고 반환하는 함수.. 큐 비어있으면 NULL 반환
Process *dequeue(Queue *queue);

bool enqueue_for_sjf(Queue *queue, Process *process);

bool enqueue_for_priority(Queue *queue, Process *process);

bool enqueue_for_lif(Queue *queue, Process *process);

bool enqueue_for_lisc(Queue *queue, Process *process);

bool enqueue_for_hrrn(Queue *queue, Process *process, int currentTime);



#endif // QUEUE_H

