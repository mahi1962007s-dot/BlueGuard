#ifndef QUEUE_H
#define QUEUE_H

#include "zone.h"
#include "stack.h"   /* MSG_LEN, SEVERITY_LEN */

/* ============================================================
 * UNIT 3 - QUEUE (circular array implementation, FIFO)
 * Cleanup / inspection tasks handled strictly in order.
 * ============================================================ */
#define MAX_TASKS 20

typedef struct {
    char zoneId[ID_LEN];
    char description[MSG_LEN];    /* e.g. "Cleanup Z12" */
    char priority[SEVERITY_LEN];  /* LOW | MEDIUM | HIGH */
} Task;

typedef struct {
    Task tasks[MAX_TASKS];
    int  front;
    int  rear;
    int  count;
} TaskQueue;

void queueInit(TaskQueue *q);
int  queueIsEmpty(const TaskQueue *q);
int  queueIsFull(const TaskQueue *q);
int  queueEnqueue(TaskQueue *q, Task task);       /* 0 on overflow  */
int  queueDequeue(TaskQueue *q, Task *out);       /* 0 on underflow */
int  queuePeek(const TaskQueue *q, Task *out);    /* 0 if empty     */
void queueDisplay(const TaskQueue *q);
int  queueSize(const TaskQueue *q);
void queueClear(TaskQueue *q);

#endif /* QUEUE_H */
