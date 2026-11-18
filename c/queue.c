#include "queue.h"
#include <stdio.h>

void queueInit(TaskQueue *q) { q->front = 0; q->rear = -1; q->count = 0; }
int  queueIsEmpty(const TaskQueue *q) { return q->count == 0; }
int  queueIsFull(const TaskQueue *q)  { return q->count == MAX_TASKS; }

int queueEnqueue(TaskQueue *q, Task task) {
    if (queueIsFull(q)) return 0;
    q->rear = (q->rear + 1) % MAX_TASKS;
    q->tasks[q->rear] = task;
    q->count++;
    return 1;
}

int queueDequeue(TaskQueue *q, Task *out) {
    if (queueIsEmpty(q)) return 0;
    *out = q->tasks[q->front];
    q->front = (q->front + 1) % MAX_TASKS;
    q->count--;
    return 1;
}

int queuePeek(const TaskQueue *q, Task *out) {
    if (queueIsEmpty(q)) return 0;
    *out = q->tasks[q->front];
    return 1;
}

void queueDisplay(const TaskQueue *q) {
    int i, idx;
    if (queueIsEmpty(q)) { printf("Task queue is empty.\n"); return; }
    printf("FRONT\n");
    idx = q->front;
    for (i = 0; i < q->count; i++) {
        printf(" [%s] %s (%s)\n", q->tasks[idx].zoneId, q->tasks[idx].description, q->tasks[idx].priority);
        idx = (idx + 1) % MAX_TASKS;
    }
    printf("REAR\n");
}

int  queueSize(const TaskQueue *q) { return q->count; }
void queueClear(TaskQueue *q)      { q->front = 0; q->rear = -1; q->count = 0; }
