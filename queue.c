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

bool enqueue_for_sjf(Queue *queue, Process *process) {
    if (isFull(queue)) {
        return false;
    }
    //없으면 그냥 들어가자
    if (isEmpty(queue)) {
        queue->items[queue->rear] = process;
        queue->rear = (queue->rear + 1) % MAX_QUEUE_CAPACITY;
        queue->count++;
        return true;
    }


    // 임시 큐 생성하여 정렬된 상태로 요소들을 옮기고
    Process *temp[MAX_QUEUE_CAPACITY];
    int tempCount = 0;

    // 현재 큐의 모든 프로세스를 임시 배열로 복사하자
    while (!isEmpty(queue)) {
        temp[tempCount++] = dequeue(queue);
    }

    // 새 프로세스 삽입할 위치 찾기
    int insertIdx = tempCount;
    for (int i = 0; i < tempCount; i++) {
        if (process->remaining_cpu_burst_time < temp[i]->remaining_cpu_burst_time) {
            insertIdx = i;
            break;
        }
    }

    // 임시 배열에서 다시 큐로 정렬된 순서대로 복원
    for (int i = 0; i < tempCount + 1; i++) {
        if (i == insertIdx) {
            enqueue(queue, process); // 새 프로세스 삽입
        }
        if (i < tempCount) {
            enqueue(queue, temp[i]); // 기존 프로세스 삽입
        }
    }

    return true;



}

bool enqueue_for_priority(Queue *queue, Process *process) {
    if (isFull(queue)) {
        return false;
    }

    // 큐가 비어있으면 바로 삽입
    if (isEmpty(queue)) {
        queue->items[queue->rear] = process;
        queue->rear = (queue->rear + 1) % MAX_QUEUE_CAPACITY;
        queue->count++;
        return true;
    }

    // 임시 큐 생성하여 정렬된 상태로 요소들을 옮김
    Process *temp[MAX_QUEUE_CAPACITY];
    int tempCount = 0;

    // 현재 큐의 모든 프로세스를 임시 배열로 복사
    while (!isEmpty(queue)) {
        temp[tempCount++] = dequeue(queue);
    }

    // 새 프로세스 삽입할 위치 찾기
    int insertIdx = tempCount;
    for (int i = 0; i < tempCount; i++) {
        if (process->priority < temp[i]->priority) {
            insertIdx = i;
            break;
        }
    }

    // 임시 배열에서 다시 큐로 정렬된 순서대로 복원
    for (int i = 0; i < tempCount + 1; i++) {
        if (i == insertIdx) {
            enqueue(queue, process); // 새 프로세스 삽입
        }
        if (i < tempCount) {
            enqueue(queue, temp[i]); // 기존 프로세스 삽입
        }
    }

    return true;
}




// Longest IO First를 위한 특수 enqueue 함수
bool enqueue_for_lif(Queue *queue, Process *process) {
    if (isFull(queue)) {
        return false;
    }


    // 큐가 비어있으면 바로 삽입
    if (isEmpty(queue)) {
        queue->items[queue->rear] = process;
        queue->rear = (queue->rear + 1) % MAX_QUEUE_CAPACITY;
        queue->count++;
        return true;
    }

    // 임시 큐 생성하여 정렬된 상태로 요소들을 옮김
    Process *temp[MAX_QUEUE_CAPACITY];
    int tempCount = 0;

    // 현재 큐의 모든 프로세스를 임시 배열로 복사
    while (!isEmpty(queue)) {
        temp[tempCount++] = dequeue(queue);
    }

    // 새 프로세스의 총 IO 시간 계산
    int new_total_io_time = 0;
    for (int i = process->current_io_index; i < process->io_count; i++) {
        new_total_io_time += process->io_burst_times[i];
    }

    // 새 프로세스 삽입할 위치 찾기 (총 IO 시간이 긴 것이 앞에 오도록)
    int insertIdx = tempCount;
    for (int i = 0; i < tempCount; i++) {
        // 현재 임시 배열 요소의 총 IO 시간 계산
        int current_total_io_time = 0;
        for (int j = temp[i]->current_io_index; j < temp[i]->io_count; j++) {
            current_total_io_time += temp[i]->io_burst_times[j];
        }

        // 새 프로세스의 IO 시간이 더 길면 해당 위치에 삽입
        if (new_total_io_time > current_total_io_time) {
            insertIdx = i;
            break;
        }
    }

    // 임시 배열에서 다시 큐로 정렬된 순서대로 복원
    for (int i = 0; i < tempCount + 1; i++) {
        if (i == insertIdx) {
            enqueue(queue, process); // 새 프로세스 삽입
        }
        if (i < tempCount) {
            enqueue(queue, temp[i]); // 기존 프로세스 삽입
        }
    }

    return true;
}

bool enqueue_for_lisc(Queue *queue, Process *process) {
    if (isFull(queue)) {
        return false;
    }

    // 큐가 비어있으면 바로 삽입
    if (isEmpty(queue)) {
        queue->items[queue->rear] = process;
        queue->rear = (queue->rear + 1) % MAX_QUEUE_CAPACITY;
        queue->count++;
        return true;
    }

    // 임시 큐 생성하여 정렬된 상태로 요소들을 옮김
    Process *temp[MAX_QUEUE_CAPACITY];
    int tempCount = 0;

    // 현재 큐의 모든 프로세스를 임시 배열로 복사
    while (!isEmpty(queue)) {
        temp[tempCount++] = dequeue(queue);
    }

    // 새 프로세스의 총 IO 시간 계산
    int process_total_io_time = 0;
    for (int i = process->current_io_index; i < process->io_count; i++) {
        process_total_io_time += process->io_burst_times[i];
    }

    // 새 프로세스의 남은 CPU 버스트 시간
    int process_cpu_time = process->remaining_cpu_burst_time;

    // 새 프로세스 삽입할 위치 찾기
    int insertIdx = tempCount;
    for (int i = 0; i < tempCount; i++) {
        // 현재 임시 배열 요소의 총 IO 시간 계산
        int current_total_io_time = 0;
        for (int j = temp[i]->current_io_index; j < temp[i]->io_count; j++) {
            current_total_io_time += temp[i]->io_burst_times[j];
        }

        // 현재 임시 배열 요소의 남은 CPU 버스트 시간
        int current_cpu_time = temp[i]->remaining_cpu_burst_time;

        // 우선 IO 시간으로 비교
        if (process_total_io_time > current_total_io_time) {
            // 새 프로세스의 IO 시간이 더 길면 현재 위치에 삽입
            insertIdx = i;
            break;
        }
        else if (process_total_io_time == current_total_io_time) {
            // IO 시간이 같으면 CPU 시간으로 비교
            if (process_cpu_time < current_cpu_time) {
                // 새 프로세스의 CPU 시간이 더 짧으면 현재 위치에 삽입
                insertIdx = i;
                break;
            }
        }
    }

    // 임시 배열에서 다시 큐로 정렬된 순서대로 복원
    for (int i = 0; i < tempCount + 1; i++) {
        if (i == insertIdx) {
            enqueue(queue, process); // 새 프로세스 삽입
        }
        if (i < tempCount) {
            enqueue(queue, temp[i]); // 기존 프로세스 삽입
        }
    }

    return true;
}

bool enqueue_for_hrrn(Queue *queue, Process *process, int currentTime) {
    if (isFull(queue)) {
        return false;
    }

    // 큐가 비어있으면 바로 삽입
    if (isEmpty(queue)) {
        queue->items[queue->rear] = process;
        queue->rear = (queue->rear + 1) % MAX_QUEUE_CAPACITY;
        queue->count++;
        return true;
    }

    // 새 프로세스의 응답률 계산
    int newWaitingTime = currentTime - process->time_entered_ready;
    if (newWaitingTime < 0) newWaitingTime = 0;
    double newRatio = (double)(newWaitingTime + process->remaining_cpu_burst_time) / process->remaining_cpu_burst_time;

    // 임시 큐 생성하여 정렬된 상태로 요소들을 옮김
    Process *temp[MAX_QUEUE_CAPACITY];
    int tempCount = 0;

    // 현재 큐의 모든 프로세스를 임시 배열로 복사
    while (!isEmpty(queue)) {
        temp[tempCount++] = dequeue(queue);
    }

    // 새 프로세스 삽입할 위치 찾기 (응답률이 높은 순서로 정렬)
    int insertIdx = tempCount;
    for (int i = 0; i < tempCount; i++) {
        int existingWaitingTime = currentTime - temp[i]->time_entered_ready;

        double existingRatio = (double)(existingWaitingTime + temp[i]->remaining_cpu_burst_time) / temp[i]->remaining_cpu_burst_time;

        // 새 프로세스의 응답률이 더 높거나 같으면 (같으면 FIFO 순서 유지.. 등호가 안들어갔으니까..)
        if (newRatio > existingRatio) {
            insertIdx = i;
            break;
            }
    }

    // 임시 배열에서 다시 큐로 정렬된 순서대로 복원
    for (int i = 0; i < tempCount + 1; i++) {
        if (i == insertIdx) {
            enqueue(queue, process); // 새 프로세스 삽입
        }
        if (i < tempCount) {
            enqueue(queue, temp[i]); // 기존 프로세스 삽입
        }
    }

    return true;
}


