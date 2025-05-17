//
// Created by 김영민 on 25. 5. 4.
//

#ifndef EVALUATERESULTS_H
#define EVALUATERESULTS_H
#include "process.h"

void calculateMetrics(Process *process);

void printResults(Process* processes[], int processCount, int algorithm);

// 모든 스케줄링 알고리즘 실행 및 결과 비교 분석 함수
void printAllResults(Process* processes[], int processCount);

#endif //EVALUATERESULTS_H
