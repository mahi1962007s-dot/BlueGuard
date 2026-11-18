#ifndef STACK_H
#define STACK_H

#include "zone.h"

/* ============================================================
 * UNIT 3 - STACK (array implementation, LIFO)
 * Recent environmental alerts. Newest alert is always on TOP.
 * ============================================================ */
#define MAX_ALERTS   20
#define MSG_LEN      96
#define SEVERITY_LEN 12

typedef struct {
    char zoneId[ID_LEN];
    char message[MSG_LEN];        /* e.g. "Z12 - High Plastic Pollution" */
    char severity[SEVERITY_LEN];  /* LOW | MEDIUM | HIGH */
} Alert;

typedef struct {
    Alert alerts[MAX_ALERTS];
    int   top;
} AlertStack;

void stackInit(AlertStack *s);
int  stackIsEmpty(const AlertStack *s);
int  stackIsFull(const AlertStack *s);
int  stackPush(AlertStack *s, Alert alert);       /* 0 on overflow  */
int  stackPop(AlertStack *s, Alert *out);         /* 0 on underflow */
int  stackPeek(const AlertStack *s, Alert *out);  /* 0 if empty     */
void stackDisplay(const AlertStack *s);
int  stackSize(const AlertStack *s);
void stackClear(AlertStack *s);

#endif /* STACK_H */
