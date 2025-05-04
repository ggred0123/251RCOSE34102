#include "queue.h"
#include "process.h"
#include <stdio.h>

void initialize_queue(Queue *queue)
{
    queue->front = 0;
    queue->rear  = 0;
    queue->count = 0;
    // 다 초기화
}

bool isEmpty(Queue *queue)
{
    return queue->count == 0;
    // 큐 안에 아무것도 없는지 확인
}

bool isFull(Queue *queue)
{
    return queue->count == MAX_QUEUE_CAPACITY;
    // 큐 안에 maxqueuenum 만큼 찼는지 확인
}

// rear 자리에 프로세스 넣게
bool enqueue(Queue *queue, Process *process)
{
    if (isFull(queue)) {
        return false;
    } else {
        queue->items[queue->rear] = process;
        queue->rear               = (queue->rear + 1) % MAX_QUEUE_CAPACITY;
        queue->count++;
        return true;
    }

    Process *dequeue(Queue * queue)
    {
        if (isEmpty(queue)) {
            return NULL;
        }
        Process *item = queue->items[queue->front];
        queue->front  = (queue->front + 1) % MAX_QUEUE_CAPACITY;
        queue->count--;
        return item;
    }
