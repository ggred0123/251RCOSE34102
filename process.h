
#ifndef PROCESS_H
#define PROCESS_H
#define CPU_BURST_TIME_DIVIDER 7
#define ARRIVAL_TIME_DIVIDER 10
#define PRIORITY_DIVIDER 6
#define MAX_IO_OPERATIONS 3
#define IO_BURST_TIME_DIVIDER 3

typedef enum { NEW, READY, RUNNING, WAITING, TERMINATED } Status;

typedef struct Process
{
    int    pid; // processId
    int    arrival_time;
    int    cpu_burst_time;           // cpu burst time
    int    remaining_cpu_burst_time; // 남아있는 cpu burst
    int    time_entered_ready; // 프로세스가 마지막으로 Ready Queue에 들어간 시간 hrrn위해서 설정
    int    io_burst_time;
    int    remaining_io_burst_time;
    int    current_io_index;
    int    cpu_time_used;
    int    io_count;
    int    io_trigger[MAX_IO_OPERATIONS];
    int    io_burst_times[MAX_IO_OPERATIONS];



    Status status;
    int    waiting_time;
    int    turnaround_time;
    int    completion_time;

    int     priority;


} Process;


Process *createProcess(int pid);

void ProcessInformation(const Process *process);





#endif // PROCESS_H
