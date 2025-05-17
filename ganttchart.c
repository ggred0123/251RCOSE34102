//
// Created by 김영민 on 25. 5. 5.
//
#include "ganttchart.h"
#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h> // memset

// GanttChartLog 구조체 배열과 로그 개수를 인자로 받아 간트 차트를 그리는 함수
void drawGanttChart(GanttChartLog logs[], int logCount) {
    // 로그 데이터가 하나도 없으면 (logCount가 0 이하이면)
    if (logCount <= 0) {
        // 간트 차트를 표시할 데이터가 없다는 메시지를 출력하고 함수를 종료
        printf("\nNo Gantt chart data to display.\n");
        return;
    }

    // 간트 차트의 전체 시간 중 가장 마지막 시간을 저장할 변수 초기화
    int finalTime = 0;
    // 모든 로그를 순회하면서
    for (int i = 0; i < logCount; i++) {
        // 현재 로그의 종료 시간(logs[i].endTime)이 finalTime보다 크면
        if (logs[i].endTime > finalTime) {
            // finalTime을 현재 로그의 종료 시간으로 업데이트
            finalTime = logs[i].endTime;
        }
    }

    // finalTime이 0 이하라는 것은 (즉, 어떤 프로세스도 실행되지 않았거나, 종료 시간이 기록되지 않은 경우)
    if (finalTime <= 0) {
        // 실행 시간이 기록되지 않았다는 메시지를 출력하고 함수를 종료
        printf("No execution time recorded.\n");
        return;
    }

    // 간트 차트 제목 출력
    printf("\n================ GANTT CHART ================\n\n");


    // timeMap[t]는 시간 t에서 실행 중인 프로세스의 PID를 저장. -1은 유휴 상태를 의미.
    int *timeMap = (int *)malloc(sizeof(int) * finalTime);
    // 메모리 할당에 실패하면
    if (!timeMap) {
        // 오류 메시지를 출력하고 함수를 종료
        perror("Error allocating memory for Gantt chart");
        return;
    }

    // timeMap 배열의 모든 요소를 -1로 초기화 (모든 시간을 유휴 상태로 설정)
    for (int i = 0; i < finalTime; i++) {
        timeMap[i] = -1; // -1은 CPU가 유휴(idle) 상태임을 나타냄
    }

    // 입력된 로그 데이터(logs 배열)를 바탕으로 timeMap을 채움
    // 각 로그는 특정 프로세스(pid)가 특정 시간(startTime부터 endTime-1까지) 동안 실행되었음을 나타냄
    for (int i = 0; i < logCount; i++) {
        // 현재 로그의 시작 시간부터 (종료 시간 직전 또는 finalTime 직전 중 작은 값까지) 반복
        for (int t = logs[i].startTime; t < logs[i].endTime && t < finalTime; t++) {
            // 시간 t에 현재 로그의 프로세스 ID(logs[i].pid)를 기록
            timeMap[t] = logs[i].pid;
        }
    }

    // 간트 차트를 한 줄에 표시할 시간 단위의 수 (예: 한 줄에 25개의 시간 단위를 표시)
    int norm = 25;

    // 간트 차트를 여러 줄에 걸쳐 그림. 각 줄은 'norm'개의 시간 단위를 포함.
    // i는 현재 그리고 있는 줄의 인덱스 (0부터 시작)
    // finalTime / norm 은 전체 줄의 개수를 대략적으로 나타냄 (올림 처리)
    for (int i = 0; i <= (finalTime / norm); i++) {

        // 프로세스 ID 표시 줄
        // 현재 줄(i번째 줄)에 해당하는 'norm'개의 시간 단위를 순회
        for (int j = 0; j < norm; j++) {
            // 현재 시간 단위를 계산 (예: 0번째 줄이면 0~24, 1번째 줄이면 25~49)
            int currentTime = i * norm + j;

            // 현재 시간이 전체 종료 시간(finalTime) 이상이면, 더 이상 그릴 내용이 없으므로
            if (currentTime >= finalTime) {
                printf("|"); // 줄의 끝을 표시하고
                break;       // 안쪽 루프를 빠져나감
            }

            // 현재 시간(currentTime)에 실행 중인 프로세스의 PID를 가져옴
            int currentPid = timeMap[currentTime];
            // 이전 시간(currentTime - 1)에 실행 중이던 프로세스의 PID를 가져옴
            // currentTime이 0인 경우 (차트의 시작), 이전 PID는 비교 대상이 없으므로 특수한 값(-999)으로 설정
            int previousPid = (currentTime == 0) ? -999 : timeMap[currentTime - 1];

            // 현재 시간(currentTime)에 CPU가 유휴 상태(-1)인 경우
            if (currentPid == -1) {
                // 현재 시간이 0이거나 (차트 시작) 또는 이전 시간에는 프로세스가 실행 중이었다가 유휴 상태로 바뀐 경우
                if (currentTime == 0 || previousPid != -1) {
                    printf("|idle "); // 유휴 상태 시작을 표시 ("|idle ")
                } else {
                    // 이전 시간에도 유휴 상태였으면, 계속 유휴 상태이므로 공백으로 표시
                    printf("      "); // 6칸 공백 (이전 "|idle "와 너비를 맞추기 위함)
                }
            }
            // 현재 시간(currentTime)에 프로세스가 실행 중인 경우
            else {
                // 현재 시간이 0이거나 (차트 시작) 또는 이전 시간에 실행되던 프로세스와 다른 프로세스가 실행되는 경우 (문맥 교환 발생)
                if (currentTime == 0 || previousPid != currentPid) {
                    // 새로운 프로세스 시작을 표시 (예: "|P12   ")
                    // P%-4d: P 문자를 출력하고, 4자리 공간을 확보하여 PID를 왼쪽 정렬로 출력
                    printf("|P%-4d", currentPid);
                } else {
                    // 이전 시간과 같은 프로세스가 계속 실행 중이면 공백으로 표시
                    printf("      "); // 6칸 공백 (이전 "|P%-4d"와 너비를 맞추기 위함)
                }
            }
        }
        printf("|\n"); // 프로세스 ID 표시 줄의 끝을 표시하고 줄 바꿈

        // 2. 구분선 표시 줄
        // 현재 줄(i번째 줄)에 해당하는 'norm'개의 시간 단위를 순회하며 구분선 출력
        for (int j = 0; j < norm; j++) {
            int currentTime = i * norm + j; // 현재 시간 단위 계산

            // 현재 시간이 전체 종료 시간(finalTime) 이상이면
            if (currentTime >= finalTime) {
                printf("|"); // 구분선의 끝을 표시하고
                break;       // 안쪽 루프를 빠져나감
            }

            // 각 시간 단위마다 "|~~~~~" 형태의 구분선을 출력
            printf("|~~~~~");
        }
        printf("|\n"); // 구분선 줄의 끝을 표시하고 줄 바꿈

        //  시간 눈금 표시 줄
        // 현재 줄(i번째 줄)에 해당하는 'norm'개의 시간 단위를 순회하며 시간 값 출력
        for (int j = 0; j < norm; j++) {
            int currentTime = i * norm + j; // 현재 시간 단위 계산

            // 현재 시간이 전체 종료 시간(finalTime) 이상이면
            if (currentTime >= finalTime) {
                // 마지막 시간 값을 6자리 공간에 왼쪽 정렬로 출력
                printf("%-6d", currentTime);
                break; // 안쪽 루프를 빠져나감
            }

            // 현재 시간 값을 6자리 공간을 확보하여 왼쪽 정렬로 출력
            printf("%-6d", currentTime);
        }

        printf("\n\n"); // 한 묶음(프로세스 ID, 구분선, 시간 눈금)의 출력이 끝나면 두 줄을 띄어 다음 묶음과 구분
    }

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



    // 유휴 시간 표시

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
    free(timeMap);
}