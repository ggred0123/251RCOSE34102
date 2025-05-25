//
// Created by 김영민 on 25. 5. 5.
//
#include "ganttchart.h"
#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h> // memset

// GanttChartLog 구조체 배열과 로그 개수를 인자로 받아 간트 차트를 그리는 함수
void drawGanttChart(GanttChartLog logs[], int logCount) {

    // 프로세스 실행 세부 정보 출력

    printf("\nProcess Execution Details:\n");

    printf("------------------------\n");



    // 로그를 시작 시간 기준으로 정렬 (버블 정렬)

    for (int i = 0; i < logCount - 1; i++) {

        for (int j = 0; j < logCount - i - 1; j++) {

            if (logs[j].startTime > logs[j + 1].startTime) {

                // 로그 순서 바꾸기

                GanttChartLog temp = logs[j];

                logs[j] = logs[j + 1];

                logs[j + 1] = temp;

            }

        }

    }



    // 프로세스 실행 기간 표시

    for (int i = 0; i < logCount; i++) {

        if (logs[i].pid > 0) { // 실제 프로세스만 표시

            int duration = logs[i].endTime - logs[i].startTime;

            printf("Process %-2d: Time %d-%d (Duration: %d)\n",

                  logs[i].pid, logs[i].startTime, logs[i].endTime, duration);

        }

    }



    // idle 시간 표시

    printf("\nIdle Times:\n");

    printf("----------\n");



    for (int i = 0; i < logCount; i++) {

        if (logs[i].pid == -1) { // 유휴 기간 표시

            int duration = logs[i].endTime - logs[i].startTime;

            if (duration > 0) { // 유효한 기간만 표시

                printf("Idle: Time %d-%d (Duration: %d)\n",

                      logs[i].startTime, logs[i].endTime, duration);

            }

        }

    }



    printf("\n============================================\n");

    // 동적으로 할당된 timeMap 메모리 해제
    // 이 코드가 없으면 메모리 누수가 발생할 수 있음
   // free(timeMap);
}