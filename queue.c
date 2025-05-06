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
    // 큐가 가득 찼는지 확인
    if (isFull(queue)) {
        return false;
    }

    // 큐가 비어있는 경우, 그냥 맨 뒤(rear)에 추가
    if (isEmpty(queue)) {
        queue->items[queue->rear] = process;
        queue->rear = (queue->rear + 1) % MAX_QUEUE_CAPACITY;
        queue->count++;
        return true;
    }

    // 큐가 비어있지 않은 경우, 올바른 삽입 위치 찾기 및 삽입
    // remaining_cpu_burst_time이 짧은 프로세스가 큐의 앞쪽에 오도록 함.

    int insert_pos = -1; // 최종 삽입 위치
    int current_idx;     // 현재 비교 중인 큐 내 요소의 인덱스
    int i;

    // rear부터 시작하여 front쪽으로 이동하며 삽입 위치 탐색
    // 원형 큐이므로 인덱스 계산에 % 연산자 사용
    current_idx = (queue->rear + MAX_QUEUE_CAPACITY - 1) % MAX_QUEUE_CAPACITY; // rear 직전 요소부터 시작

    // count 만큼 반복하여 큐 내의 모든 요소와 비교
    for (i = 0; i < queue->count; i++) {
        // 현재 검사하는 요소(current_idx)의 burst time이 새 프로세스보다 크면,
        // 해당 요소를 한 칸 뒤로 이동시키고, 계속 앞쪽 요소와 비교 진행
        if (queue->items[current_idx]->remaining_cpu_burst_time > process->remaining_cpu_burst_time) {
            // 요소 이동: items[current_idx] -> items[(current_idx + 1) % MAX_QUEUE_CAPACITY]
            queue->items[(current_idx + 1) % MAX_QUEUE_CAPACITY] = queue->items[current_idx];

            // 삽입 위치는 현재 위치(current_idx)가 됨 (다음 반복에서 더 앞으로 갈 수도 있음)
            insert_pos = current_idx;

            // 다음 비교를 위해 앞쪽 인덱스로 이동 (원형 큐 고려)
            current_idx = (current_idx + MAX_QUEUE_CAPACITY - 1) % MAX_QUEUE_CAPACITY;
        }
        // 현재 검사하는 요소의 burst time이 새 프로세스보다 작거나 같으면,
        // 새 프로세스는 현재 요소의 바로 뒤에 위치해야 함. 탐색 및 이동 중지.
        else {
            insert_pos = (current_idx + 1) % MAX_QUEUE_CAPACITY; // 삽입 위치는 current_idx 바로 다음
            break; // 더 이상 앞쪽으로 갈 필요 없음
        }
    }

    // 만약 모든 기존 요소가 새 프로세스보다 burst time이 크다면,
    // insert_pos는 front 위치가 되어야 함. 위 루프에서 insert_pos가 마지막으로 업데이트된
    // current_idx (즉, front 바로 전 요소의 인덱스)로 설정되므로, 이미 올바른 상태임.
    // 만약 루프가 count만큼 다 돌았다면 (모든 요소를 뒤로 민 경우), insert_pos는 최종적으로 front 위치가 됨.
    if (i == queue->count) { // 모든 요소를 다 비교하고 밀었다면
        insert_pos = current_idx; // 맨 앞에 삽입
    }


    //  찾은 삽입 위치(insert_pos)에 새 프로세스 삽입
    queue->items[insert_pos] = process;

    // rear 및 count 업데이트
    queue->rear = (queue->rear + 1) % MAX_QUEUE_CAPACITY;
    queue->count++;

    return true;
}

bool enqueue_for_priority(Queue *queue, Process *process) {
    // 큐가 가득 찼는지 확인
    if (isFull(queue)) {
        return false;
    }

    // 큐가 비어있는 경우, 그냥 맨 뒤(rear)에 추가
    if (isEmpty(queue)) {
        queue->items[queue->rear] = process;
        queue->rear = (queue->rear + 1) % MAX_QUEUE_CAPACITY;
        queue->count++;
        return true;
    }

    // 큐가 비어있지 않은 경우, 우선순위에 따라 올바른 삽입 위치 찾기 및 삽입
    // 낮은 priority 값이 높은 우선순위를 의미하므로, priority가 낮은 프로세스가 큐의 앞쪽에 오도록 함.

    int insert_pos = -1; // 최종 삽입 위치
    int current_idx;     // 현재 비교 중인 큐 내 요소의 인덱스
    int i;

    // rear부터 시작하여 front쪽으로 이동하며 삽입 위치 탐색
    current_idx = (queue->rear + MAX_QUEUE_CAPACITY - 1) % MAX_QUEUE_CAPACITY; // rear 직전 요소부터 시작

    // count 만큼 반복하여 큐 내의 모든 요소와 비교
    for (i = 0; i < queue->count; i++) {
        // 현재 검사하는 요소(current_idx)의 priority가 새 프로세스보다 높으면(값이 크면),
        // 해당 요소를 한 칸 뒤로 이동시키고, 계속 앞쪽 요소와 비교 진행
        if (queue->items[current_idx]->priority > process->priority) {
            // 요소 이동: items[current_idx] -> items[(current_idx + 1) % MAX_QUEUE_CAPACITY]
            queue->items[(current_idx + 1) % MAX_QUEUE_CAPACITY] = queue->items[current_idx];

            // 삽입 위치는 현재 위치(current_idx)가 됨
            insert_pos = current_idx;

            // 다음 비교를 위해 앞쪽 인덱스로 이동 (원형 큐 고려)
            current_idx = (current_idx + MAX_QUEUE_CAPACITY - 1) % MAX_QUEUE_CAPACITY;
        }
        // 현재 검사하는 요소의 priority가 새 프로세스보다 낮거나 같으면,
        // 새 프로세스는 현재 요소의 바로 뒤에 위치해야 함. 탐색 및 이동 중지.
        else {
            insert_pos = (current_idx + 1) % MAX_QUEUE_CAPACITY; // 삽입 위치는 current_idx 바로 다음
            break; // 더 이상 앞쪽으로 갈 필요 없음
        }
    }

    // 모든 요소가 새 프로세스보다 priority가 높으면(값이 크면) 맨 앞에 삽입
    if (i == queue->count) {
        insert_pos = current_idx;
    }

    // 찾은 삽입 위치(insert_pos)에 새 프로세스 삽입
    queue->items[insert_pos] = process;

    // rear 및 count 업데이트
    queue->rear = (queue->rear + 1) % MAX_QUEUE_CAPACITY;
    queue->count++;

    return true;
}



// Longest IO First를 위한 특수 enqueue 함수
bool enqueue_for_lif(Queue *queue, Process *process) {
    // 큐가 가득 찼는지 확인
    if (isFull(queue)) {
        return false;
    }

    // 큐가 비어있는 경우, 그냥 맨 뒤(rear)에 추가
    if (isEmpty(queue)) {
        queue->items[queue->rear] = process;
        queue->rear = (queue->rear + 1) % MAX_QUEUE_CAPACITY;
        queue->count++;
        return true;
    }

    // 큐가 비어있지 않은 경우, 올바른 삽입 위치 찾기 및 삽입
    // 총 IO 시간이 긴 프로세스가 큐의 앞쪽에 오도록 함

    int insert_pos = -1; // 최종 삽입 위치
    int current_idx;     // 현재 비교 중인 큐 내 요소의 인덱스
    int i;

    // 프로세스의 총 IO 시간 계산
    int total_io_time = 0;
    for (i = 0; i < process->io_count; i++) {
        total_io_time += process->io_burst_times[i];
    }

    // rear부터 시작하여 front쪽으로 이동하며 삽입 위치 탐색
    current_idx = (queue->rear + MAX_QUEUE_CAPACITY - 1) % MAX_QUEUE_CAPACITY; // rear 직전 요소부터 시작

    // count 만큼 반복하여 큐 내의 모든 요소와 비교
    for (i = 0; i < queue->count; i++) {
        // 현재 프로세스의 총 IO 시간 계산
        int current_total_io_time = 0;
        for (int j = 0; j < queue->items[current_idx]->io_count; j++) {
            current_total_io_time += queue->items[current_idx]->io_burst_times[j];
        }

        // 현재 검사하는 요소의 total_io_time이 새 프로세스보다 작으면,
        // 해당 요소를 한 칸 뒤로 이동시키고, 계속 앞쪽 요소와 비교 진행
        if (current_total_io_time < total_io_time) {
            // 요소 이동: items[current_idx] -> items[(current_idx + 1) % MAX_QUEUE_CAPACITY]
            queue->items[(current_idx + 1) % MAX_QUEUE_CAPACITY] = queue->items[current_idx];

            // 삽입 위치는 현재 위치(current_idx)가 됨
            insert_pos = current_idx;

            // 다음 비교를 위해 앞쪽 인덱스로 이동 (원형 큐 고려)
            current_idx = (current_idx + MAX_QUEUE_CAPACITY - 1) % MAX_QUEUE_CAPACITY;
        }
        // 현재 검사하는 요소의 total_io_time이 새 프로세스보다 크거나 같으면,
        // 새 프로세스는 현재 요소의 바로 뒤에 위치해야 함. 탐색 및 이동 중지.
        else {
            insert_pos = (current_idx + 1) % MAX_QUEUE_CAPACITY; // 삽입 위치는 current_idx 바로 다음
            break; // 더 이상 앞쪽으로 갈 필요 없음
        }
    }

    // 모든 요소가 새 프로세스보다 total_io_time이 크면 맨 앞에 삽입
    if (i == queue->count) {
        insert_pos = current_idx;
    }

    // 찾은 삽입 위치(insert_pos)에 새 프로세스 삽입
    queue->items[insert_pos] = process;

    // rear 및 count 업데이트
    queue->rear = (queue->rear + 1) % MAX_QUEUE_CAPACITY;
    queue->count++;

    return true;
}

bool enqueue_for_lisc(Queue *queue, Process *process) {
    // 큐가 가득 찼는지 확인
    if (isFull(queue)) {
        return false;
    }

    // 큐가 비어있는 경우, 그냥 맨 뒤(rear)에 추가
    if (isEmpty(queue)) {
        queue->items[queue->rear] = process;
        queue->rear = (queue->rear + 1) % MAX_QUEUE_CAPACITY;
        queue->count++;
        return true;
    }

    // 프로세스의 총 IO 시간 계산
    int process_total_io_time = 0;
    for (int i = 0; i < process->io_count; i++) {
        process_total_io_time += process->io_burst_times[i];
    }

    // 프로세스의 남은 CPU 버스트 시간
    int process_cpu_time = process->remaining_cpu_burst_time;

    // 큐가 비어있지 않은 경우, 올바른 삽입 위치 찾기 및 삽입
    // 1. IO 버스트가 긴 순서대로 (내림차순)
    // 2. CPU 버스트가 짧은 순서대로 (오름차순)
    int insert_pos = -1; // 최종 삽입 위치
    int current_idx;     // 현재 비교 중인 큐 내 요소의 인덱스
    int i;

    // rear부터 시작하여 front쪽으로 이동하며 삽입 위치 탐색
    current_idx = (queue->rear + MAX_QUEUE_CAPACITY - 1) % MAX_QUEUE_CAPACITY; // rear 직전 요소부터 시작

    // count 만큼 반복하여 큐 내의 모든 요소와 비교
    for (i = 0; i < queue->count; i++) {
        // 현재 큐 내 프로세스의 총 IO 시간 계산
        int current_total_io_time = 0;
        for (int j = 0; j < queue->items[current_idx]->io_count; j++) {
            current_total_io_time += queue->items[current_idx]->io_burst_times[j];
        }

        // 현재 큐 내 프로세스의 남은 CPU 버스트 시간
        int current_cpu_time = queue->items[current_idx]->remaining_cpu_burst_time;

        // 우선 IO 시간으로 비교
        if (process_total_io_time > current_total_io_time) {
            // 새 프로세스의 IO 시간이 더 길면 현재 위치 앞에 삽입
            queue->items[(current_idx + 1) % MAX_QUEUE_CAPACITY] = queue->items[current_idx];
            insert_pos = current_idx;
            current_idx = (current_idx + MAX_QUEUE_CAPACITY - 1) % MAX_QUEUE_CAPACITY;
        }
        else if (process_total_io_time == current_total_io_time) {
            // IO 시간이 같으면 CPU 시간으로 비교
            if (process_cpu_time < current_cpu_time) {
                // 새 프로세스의 CPU 시간이 더 짧으면 현재 위치 앞에 삽입
                queue->items[(current_idx + 1) % MAX_QUEUE_CAPACITY] = queue->items[current_idx];
                insert_pos = current_idx;
                current_idx = (current_idx + MAX_QUEUE_CAPACITY - 1) % MAX_QUEUE_CAPACITY;
            }
            else {
                // 새 프로세스의 CPU 시간이 더 길거나 같으면 현재 위치 뒤에 삽입
                insert_pos = (current_idx + 1) % MAX_QUEUE_CAPACITY;
                break;
            }
        }
        else {
            // 새 프로세스의 IO 시간이 더 짧으면 현재 위치 뒤에 삽입
            insert_pos = (current_idx + 1) % MAX_QUEUE_CAPACITY;
            break;
        }
    }

    // 모든 요소가 새 프로세스보다 우선순위가 낮으면 맨 앞에 삽입
    if (i == queue->count) {
        insert_pos = current_idx;
    }

    // 찾은 삽입 위치(insert_pos)에 새 프로세스 삽입
    queue->items[insert_pos] = process;

    // rear 및 count 업데이트
    queue->rear = (queue->rear + 1) % MAX_QUEUE_CAPACITY;
    queue->count++;

    return true;
}
