//
// Created by 김영민 on 25. 5. 5.
//
#include "ganttchart.h"
#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h> // memset

void drawGanttChart(GanttChartLog logs[], int logCount) {
    if (logCount <= 0) {
        printf("\nNo Gantt chart data to display.\n");
        return;
    }

    // Find the final end time
    int finalTime = 0;
    for (int i = 0; i < logCount; i++) {
        if (logs[i].endTime > finalTime) {
            finalTime = logs[i].endTime;
        }
    }

    if (finalTime <= 0) {
        printf("No execution time recorded.\n");
        return;
    }

    printf("\n================ GANTT CHART ================\n\n");
    
    // Create time-to-PID mapping array
    int *timeMap = (int *)malloc(sizeof(int) * finalTime);
    if (!timeMap) {
        perror("Error allocating memory for Gantt chart");
        return;
    }
    
    // Initialize all slots to idle (-1)
    for (int i = 0; i < finalTime; i++) {
        timeMap[i] = -1; // -1 represents IDLE
    }
    
    // Fill in the mapping based on logs
    for (int i = 0; i < logCount; i++) {
        for (int t = logs[i].startTime; t < logs[i].endTime && t < finalTime; t++) {
            timeMap[t] = logs[i].pid;
        }
    }
    
    // Print Gantt chart with 20 time units per line
    int norm = 20; // Number of time units per line
    
    for (int i = 0; i <= (finalTime / norm); i++) {
        // Print process info line
        for (int j = 0; j < norm; j++) {
            int currentTime = i * norm + j;
            
            if (currentTime >= finalTime) {
                printf("|");
                break;
            }
            
            // Current and previous process ID
            int currentPid = timeMap[currentTime];
            int previousPid = (currentTime == 0) ? -999 : timeMap[currentTime - 1];
            
            // IDLE state
            if (currentPid == -1) {
                if (currentTime == 0 || previousPid != -1) {
                    printf("|idle ");
                } else {
                    printf("      ");
                }
            } 
            // Process running
            else {
                if (currentTime == 0 || previousPid != currentPid) {
                    printf("|P%-4d", currentPid);
                } else {
                    printf("      ");
                }
            }
        }
        printf("|\n"); // End of process info line
        
        // Print separator line
        for (int j = 0; j < norm; j++) {
            int currentTime = i * norm + j;
            
            if (currentTime >= finalTime) {
                printf("|");
                break;
            }
            
            printf("|~~~~~");
        }
        printf("|\n"); // End of separator line
        
        // Print time scale
        for (int j = 0; j < norm; j++) {
            int currentTime = i * norm + j;
            
            if (currentTime >= finalTime) {
                printf("%-6d", currentTime);
                break;
            }
            
            printf("%-6d", currentTime);
        }
        
        printf("\n\n"); // Space between chunks
    }
    
    // Print process execution details
    printf("\nProcess Execution Details:\n");
    printf("------------------------\n");
    
    // Sort logs by startTime for clearer display
    for (int i = 0; i < logCount - 1; i++) {
        for (int j = 0; j < logCount - i - 1; j++) {
            if (logs[j].startTime > logs[j + 1].startTime) {
                // Swap logs
                GanttChartLog temp = logs[j];
                logs[j] = logs[j + 1];
                logs[j + 1] = temp;
            }
        }
    }
    
    // Display process execution periods
    for (int i = 0; i < logCount; i++) {
        if (logs[i].pid > 0) { // Only show actual processes
            int duration = logs[i].endTime - logs[i].startTime;
            printf("Process %-2d: Time %d-%d (Duration: %d)\n", 
                  logs[i].pid, logs[i].startTime, logs[i].endTime, duration);
        }
    }
    
    // Print idle times
    printf("\nIdle Times:\n");
    printf("----------\n");
    
    for (int i = 0; i < logCount; i++) {
        if (logs[i].pid == -1) { // Show idle periods
            int duration = logs[i].endTime - logs[i].startTime;
            if (duration > 0) { // Only show valid periods
                printf("Idle: Time %d-%d (Duration: %d)\n", 
                      logs[i].startTime, logs[i].endTime, duration);
            }
        }
    }
    
    printf("\n============================================\n");
    
    free(timeMap);
}