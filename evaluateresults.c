#include "evaluateresults.h"
#include "scheduler.h"
#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

void calculateMetrics(Process* process)
{
    int total_io_time = 0;
    for (int i = 0; i < process->io_count; i++) {
        total_io_time += process->io_burst_times[i];
    }

    if (process->status == TERMINATED) {
        process->turnaround_time = process->completion_time - process->arrival_time;
        process->waiting_time    = process->turnaround_time - process->cpu_burst_time - total_io_time;
        if (process->waiting_time < 0)
            process->waiting_time = 0;

    } else {
        // 종료되지 않은 프로세스가 이 함수를 불렀다는 것 자체가 오류 상황
        process->turnaround_time = -1;
        process->waiting_time    = -1;
    }
}

void printResults(Process* processes[], int processCount,int algorithm)
{
    int totalWaitingTime    = 0;
    int totalTurnaroundTime = 0;
    const char* algorithmNames[] = {
        "",         // 인덱스 0은 사용 안 함 (선택적)
        "FCFS",     // 인덱스 1 (ALG_FCFS)
        "SJF",      // 인덱스 2 (ALG_SJF)
        "RR",        // 인덱스 3 (ALG_RR)
        "Priority",
        "PreemptiveSJF",
        "PreemptivePriority",
        "RoundRobinWithPriority",
        "LongestIOFirst",
        "PreemptiveLongestIOFirst",
        "MultiLevelQueue",
        "LOTTERY",
        "LongestIOShortestCPU",
        "PreemptiveLongestIOShortestCPU",
        "HRRN",



    };
    printf("\n--- Scheduling Results for %s ---\n", algorithmNames[algorithm]);
    printf("PID\tArrival\tCPU_Burst\tIO_Count\tIO_Burst\tPriority\tStatus\tCompletion\tWaiting\tTurnaround\tRemaining_CPU\tCPU_Used\n");

    for (int i = 0; i < processCount; i++) {
        calculateMetrics(processes[i]);

        // 프로세스 상태를 문자열로 변환
        const char* status_str;
        switch(processes[i]->status) {
            case NEW: status_str = "NEW"; break;
            case READY: status_str = "READY"; break;
            case RUNNING: status_str = "RUNNING"; break;
            case WAITING: status_str = "WAITING"; break;
            case TERMINATED: status_str = "TERMINATED"; break;
            default: status_str = "UNKNOWN"; break;
        }
        
        printf("%d\t%d\t%d\t%d\t%d\t%d\t%s\t%d\t\t%d\t%d\t\t%d\t\t%d\n",
               processes[i]->pid,
               processes[i]->arrival_time,
               processes[i]->cpu_burst_time,
               processes[i]->io_count,
               processes[i]->io_burst_time,
               processes[i]->priority,
               status_str,
               processes[i]->completion_time,
               processes[i]->waiting_time,
               processes[i]->turnaround_time,
               processes[i]->remaining_cpu_burst_time,
               processes[i]->cpu_time_used);
        if (processes[i]->status == TERMINATED) {
            totalWaitingTime += processes[i]->waiting_time;
            totalTurnaroundTime += processes[i]->turnaround_time;
        }
    }

    if (processCount > 0) {
        printf("\nAverage Waiting Time: %.2f\n", (double)totalWaitingTime / processCount);
        printf("Average Turnaround Time: %.2f\n", (double)totalTurnaroundTime / processCount);
    }

    printf("---------------------------------------\n");
}

typedef struct {
    int algorithm;
    double avgWaitingTime;
    double avgTurnaroundTime;
} AlgorithmResult;

void printAllResults(Process* processes[], int processCount) {
    const int ALGORITHM_COUNT = 14;
    const char* algorithmNames[] = {
        "",               // 인덱스 0은 사용 안 함
        "FCFS",           // 인덱스 1 (FCFS)
        "SJF",            // 인덱스 2 (NonPreemptiveSJF)
        "RR",             // 인덱스 3 (RoundRobin)
        "Priority",       // 인덱스 4 (PRIORITY)
        "PreemptiveSJF",  // 인덱스 5 (PreemptiveSJF)
        "PreemptivePriority", // 인덱스 6 (PreemptivePriority)
        "RoundRobinWithPriority", // 인덱스 7 (RoundRobinWithPriority)
        "LongestIOFirst", // 인덱스 8 (LongestIOFirst)
        "PreemptiveLongestIOFirst", // 인덱스 9 (PreemptiveLongestIOFirst)
        "MultiLevelQueue", // 인덱스 10 (MultiLevelQueue)
        "LOTTERY",        // 인덱스 11 (LOTTERY)
        "LongestIOShortestCPU", // 인덱스 12 (LongestIOShortestCPU)
        "PreemptiveLongestIOShortestCPU", // 인덱스 13 (PreemptiveLongestIOShortestCPU)
        "HRRN"            // 인덱스 14 (HRRN)
    };
    
    AlgorithmResult results[ALGORITHM_COUNT + 1]; // 인덱스 1부터 14까지 사용
    
    printf("\n======= Running All Scheduling Algorithms and Comparing Results =======\n");
    printf("Number of processes: %d\n\n", processCount);
    
    // 모든 알고리즘 실행 및 결과 저장
    for (int alg = 1; alg <= ALGORITHM_COUNT; alg++) {
        printf("\n[%d/%d] Running %s algorithm...\n", alg, ALGORITHM_COUNT, algorithmNames[alg]);
        
        // 프로세스 상태 초기화를 위한 원본 프로세스 백업
        Process* backupProcesses[processCount];
        for (int i = 0; i < processCount; i++) {
            backupProcesses[i] = (Process*)malloc(sizeof(Process));
            memcpy(backupProcesses[i], processes[i], sizeof(Process));
        }
        
        // 알고리즘 실행
        runScheduler(processes, processCount, alg);
        
        // 결과 계산
        int totalWaitingTime = 0;
        int totalTurnaroundTime = 0;
        for (int i = 0; i < processCount; i++) {
            calculateMetrics(processes[i]);
            if (processes[i]->status == TERMINATED) {
                totalWaitingTime += processes[i]->waiting_time;
                totalTurnaroundTime += processes[i]->turnaround_time;
            }
        }
        
        // 평균 계산 및 저장
        results[alg].algorithm = alg;
        results[alg].avgWaitingTime = (double)totalWaitingTime / processCount;
        results[alg].avgTurnaroundTime = (double)totalTurnaroundTime / processCount;
        
        printf("%s average waiting time: %.2f\n", algorithmNames[alg], results[alg].avgWaitingTime);
        printf("%s average turnaround time: %.2f\n", algorithmNames[alg], results[alg].avgTurnaroundTime);
        
        // 원래 프로세스 상태로 복원
        for (int i = 0; i < processCount; i++) {
            memcpy(processes[i], backupProcesses[i], sizeof(Process));
            free(backupProcesses[i]);
        }
    }
    
    // 최적의 알고리즘 찾기
    int bestWaitingAlg = 1; // 초기값은 첫 번째 알고리즘
    int bestTurnaroundAlg = 1;
    double minWaitingTime = DBL_MAX;
    double minTurnaroundTime = DBL_MAX;
    
    for (int alg = 1; alg <= ALGORITHM_COUNT; alg++) {
        if (results[alg].avgWaitingTime < minWaitingTime) {
            minWaitingTime = results[alg].avgWaitingTime;
            bestWaitingAlg = alg;
        }
        
        if (results[alg].avgTurnaroundTime < minTurnaroundTime) {
            minTurnaroundTime = results[alg].avgTurnaroundTime;
            bestTurnaroundAlg = alg;
        }
    }
    
    // 결과 요약 출력
    printf("\n========== Results Summary ==========\n");
    printf("Analysis of %d algorithms:\n\n", ALGORITHM_COUNT);
    
    printf("Algorithm\t\tAvg Waiting Time\tAvg Turnaround Time\n");
    printf("----------------------------------------------\n");
    
    for (int alg = 1; alg <= ALGORITHM_COUNT; alg++) {
        printf("%s", algorithmNames[alg]);
        
        // 이름이 짧은 알고리즘은 탭을 추가로 출력해 정렬
        if (strlen(algorithmNames[alg]) < 16) {
            printf("\t");
        }
        if (strlen(algorithmNames[alg]) < 8) {
            printf("\t");
        }
        
        printf("\t%.2f", results[alg].avgWaitingTime);
        
        // 최소 대기 시간 알고리즘 표시
        if (alg == bestWaitingAlg) {
            printf(" (MIN)");
        } else {
            printf("       ");
        }
        
        printf("\t%.2f", results[alg].avgTurnaroundTime);
        
        // 최소 반환 시간 알고리즘 표시
        if (alg == bestTurnaroundAlg) {
            printf(" (최소)");
        }
        
        printf("\n");
    }
    
    printf("\nOptimal Algorithm Analysis:\n");
    printf("- Algorithm with minimum average waiting time: %s (%.2f)\n", 
           algorithmNames[bestWaitingAlg], results[bestWaitingAlg].avgWaitingTime);
    printf("- Algorithm with minimum average turnaround time: %s (%.2f)\n", 
           algorithmNames[bestTurnaroundAlg], results[bestTurnaroundAlg].avgTurnaroundTime);
    
    if (bestWaitingAlg == bestTurnaroundAlg) {
        printf("\nConclusion: %s algorithm shows the best performance for both waiting time and turnaround time.\n", 
               algorithmNames[bestWaitingAlg]);
    } else {
        printf("\nConclusion: %s algorithm has the best waiting time performance, while %s algorithm has the best turnaround time performance.\n", 
               algorithmNames[bestWaitingAlg], algorithmNames[bestTurnaroundAlg]);
    }
    
    printf("\n=================================\n");
}