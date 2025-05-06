//
// Created by 김영민 on 25. 5. 5.
//

#ifndef GANTTCHART_H
#define GANTTCHART_H
#include "process.h"
#define MAX_TIME 500
#define MAX_GANTTCHART_LOG 500
typedef struct GanttChartLog
{
    int startTime;
    int endTime;
    int pid;
} GanttChartLog;


void drawGanttChart(GanttChartLog logs[], int logCount);



#endif //GANTTCHART_H
