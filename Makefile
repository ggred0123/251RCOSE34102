cpuscheduler : main.o process.o queue.o ganttchart.o evaluateresults.o scheduler.o
	gcc main.o process.o queue.o ganttchart.o evaluateresults.o scheduler.o -o cpu_scheduler

main.o : main.c
	gcc -c main.c -o main.o

process.o : process.c
	gcc -c process.c -o process.o

scheduler.o : scheduler.c
	gcc -c scheduler.c -o scheduler.o

queue.o : queue.c
	gcc -c queue.c -o queue.o

ganttchart.o : ganttchart.c
	gcc -c ganttchart.c -o ganttchart.o

evaluateresults.o : evaluateresults.c
	gcc -c evaluateresults.c -o evaluateresults.o