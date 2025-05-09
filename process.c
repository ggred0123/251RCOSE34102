#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

Process *createProcess(const int pid)
{
    Process *process = (Process *)malloc(sizeof(Process));
    if (process == NULL) {
        perror("Error allocating memory");
        return NULL;
    }
    process->pid = pid;

    srand(time(NULL) + pid*3); // rand 를 위한 시드.. 근데 이제 time(NULL) 만 넣으면 빠르게 생성할 때 겹칠 수도..?

    // 기본 정보 랜덤 생성
    process->arrival_time             = rand() % ARRIVAL_TIME_DIVIDER + 1;
    process->cpu_burst_time           = rand()% CPU_BURST_TIME_DIVIDER + 1;
    process->remaining_cpu_burst_time = process->cpu_burst_time;
    process->io_count                 = rand() % (MAX_IO_OPERATIONS );
    process->priority = rand() % PRIORITY_DIVIDER +1;

    int recent_IO = 0; // 이전 IO 발생 시점

    for (int i = 0; i < process->io_count; i++) {

        // IO 가 가능한 시간 할당범위가 말도안되게 되는 것을 방지하기 위한 범위
        int next_IO         = recent_IO + 1;
        int max_possible_IO = process->cpu_burst_time - 1;

        if (next_IO > max_possible_IO) {
            process->io_count = i;
            break;
        } // io_count 재조정.. 뭔가 잘못된 상황에서도 정상적으로 작동할 수 있게

        process->io_trigger[i]     = next_IO + rand() % (max_possible_IO - next_IO +
                                                     1); // IO 발생 시간 결정(가능한 범위 내에서)
        recent_IO                  = process->io_trigger[i];
        process->io_burst_times[i] = rand() % (IO_BURST_TIME_DIVIDER) + 1;
    }

    for (int i = 0; i < process->io_count; i++) {
        process->io_burst_time+= process->io_burst_times[i];
    }


    process->current_io_index     = 0;
    process->cpu_time_used = 0; // 누적 시간 초기화
    process->status               = NEW;
    process->completion_time = 0;
    process->waiting_time    = 0;
    process->turnaround_time = 0;

    return process;
}

void ProcessInformation(const Process *process)
{
    printf("Process ID: %d\n", process->pid);
    printf("Arrival Time: %d\n", process->arrival_time);
    printf("CPU Burst Time: %d\n", process->cpu_burst_time);
    printf("IO Burst Time: %d\n", process->io_burst_time);
    printf("Priority: %d\n", process->priority);

}