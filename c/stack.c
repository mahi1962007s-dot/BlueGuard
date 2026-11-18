#include "stack.h"
#include <stdio.h>

void stackInit(AlertStack *s) { s->top = -1; }
int  stackIsEmpty(const AlertStack *s) { return s->top == -1; }
int  stackIsFull(const AlertStack *s)  { return s->top == MAX_ALERTS - 1; }

int stackPush(AlertStack *s, Alert alert) {
    if (stackIsFull(s)) return 0;
    s->alerts[++s->top] = alert;
    return 1;
}

int stackPop(AlertStack *s, Alert *out) {
    if (stackIsEmpty(s)) return 0;
    *out = s->alerts[s->top--];
    return 1;
}

int stackPeek(const AlertStack *s, Alert *out) {
    if (stackIsEmpty(s)) return 0;
    *out = s->alerts[s->top];
    return 1;
}

void stackDisplay(const AlertStack *s) {
    int i;
    if (stackIsEmpty(s)) { printf("Alert stack is empty.\n"); return; }
    printf("TOP\n");
    for (i = s->top; i >= 0; i--)
        printf(" [%s] %s (%s)\n", s->alerts[i].zoneId, s->alerts[i].message, s->alerts[i].severity);
    printf("BOTTOM\n");
}

int  stackSize(const AlertStack *s)  { return s->top + 1; }
void stackClear(AlertStack *s)       { s->top = -1; }
