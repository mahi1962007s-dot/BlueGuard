#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ObservationNode *newNode(const char *zoneId, const char *date,
                                float plastic, float water, float erosion, float health) {
    ObservationNode *n = (ObservationNode *)malloc(sizeof(ObservationNode));
    if (!n) return NULL;
    snprintf(n->zoneId, ID_LEN, "%s", zoneId);
    snprintf(n->date, DATE_LEN, "%s", date);
    n->plasticWaste = plastic;
    n->waterQuality = water;
    n->erosionLevel = erosion;
    n->healthScore = health;
    n->next = NULL;
    return n;
}

void historyInit(MonitoringHistory *h) { h->head = NULL; h->count = 0; }

int historyInsertAtBeginning(MonitoringHistory *h, const char *zoneId, const char *date,
                             float plastic, float water, float erosion, float health) {
    ObservationNode *n = newNode(zoneId, date, plastic, water, erosion, health);
    if (!n) return 0;
    n->next = h->head;
    h->head = n;
    h->count++;
    return 1;
}

int historyInsertAtEnd(MonitoringHistory *h, const char *zoneId, const char *date,
                       float plastic, float water, float erosion, float health) {
    ObservationNode *n = newNode(zoneId, date, plastic, water, erosion, health);
    if (!n) return 0;
    if (h->head == NULL) {
        h->head = n;
    } else {
        ObservationNode *cur = h->head;
        while (cur->next != NULL) cur = cur->next;
        cur->next = n;
    }
    h->count++;
    return 1;
}

/* dd/mm/yyyy -> yyyymmdd, so two dates can be compared as plain numbers. */
static long dateKey(const char *date) {
    int d = 0, m = 0, y = 0;
    if (sscanf(date, "%d/%d/%d", &d, &m, &y) != 3) return 0;
    return (long)y * 10000L + (long)m * 100L + (long)d;
}

ObservationNode *historyInsertSorted(MonitoringHistory *h, const char *zoneId, const char *date,
                                     float plastic, float water, float erosion, float health) {
    ObservationNode *n = newNode(zoneId, date, plastic, water, erosion, health);
    long key;
    if (!n) return NULL;
    key = dateKey(date);
    if (h->head == NULL || dateKey(h->head->date) > key) {
        n->next = h->head;              /* new oldest record: becomes the head */
        h->head = n;
    } else {
        ObservationNode *cur = h->head;
        /* walk until the next node is newer; equal dates keep the order they were added */
        while (cur->next != NULL && dateKey(cur->next->date) <= key) cur = cur->next;
        n->next = cur->next;
        cur->next = n;
    }
    h->count++;
    return n;
}

int historyInsertAtPosition(MonitoringHistory *h, int position, const char *zoneId,
                            const char *date, float plastic, float water,
                            float erosion, float health) {
    ObservationNode *n, *cur;
    int i;
    if (position < 0 || position > h->count) return 0;
    if (position == 0) return historyInsertAtBeginning(h, zoneId, date, plastic, water, erosion, health);

    n = newNode(zoneId, date, plastic, water, erosion, health);
    if (!n) return 0;
    cur = h->head;
    for (i = 0; i < position - 1; i++) cur = cur->next;
    n->next = cur->next;
    cur->next = n;
    h->count++;
    return 1;
}

int historyDeleteAtBeginning(MonitoringHistory *h) {
    ObservationNode *del;
    if (h->head == NULL) return 0;
    del = h->head;
    h->head = h->head->next;
    free(del);
    h->count--;
    return 1;
}

int historyDeleteAtEnd(MonitoringHistory *h) {
    ObservationNode *cur;
    if (h->head == NULL) return 0;
    if (h->head->next == NULL) {
        free(h->head);
        h->head = NULL;
        h->count--;
        return 1;
    }
    cur = h->head;
    while (cur->next->next != NULL) cur = cur->next;
    free(cur->next);
    cur->next = NULL;
    h->count--;
    return 1;
}

int historyDeleteAtPosition(MonitoringHistory *h, int position) {
    ObservationNode *cur, *del;
    int i;
    if (position < 0 || position >= h->count) return 0;
    if (position == 0) return historyDeleteAtBeginning(h);

    cur = h->head;
    for (i = 0; i < position - 1; i++) cur = cur->next;
    del = cur->next;
    cur->next = del->next;
    free(del);
    h->count--;
    return 1;
}

/* Deletes the FIRST node that matches zoneId. */
int historyDeleteByZoneId(MonitoringHistory *h, const char *zoneId) {
    ObservationNode *cur = h->head, *prev = NULL;
    while (cur != NULL) {
        if (strcmp(cur->zoneId, zoneId) == 0) {
            if (prev) prev->next = cur->next;
            else      h->head = cur->next;
            free(cur);
            h->count--;
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

ObservationNode *historySearch(const MonitoringHistory *h, const char *zoneId, int *comparisons) {
    ObservationNode *cur = h->head;
    *comparisons = 0;
    while (cur != NULL) {
        (*comparisons)++;
        if (strcmp(cur->zoneId, zoneId) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

int historyUpdate(MonitoringHistory *h, const char *zoneId, float plastic, float water,
                  float erosion, float health) {
    ObservationNode *cur = h->head;
    while (cur != NULL) {
        if (strcmp(cur->zoneId, zoneId) == 0) {
            cur->plasticWaste = plastic;
            cur->waterQuality = water;
            cur->erosionLevel = erosion;
            cur->healthScore = health;
            return 1;
        }
        cur = cur->next;
    }
    return 0;
}

void historyDisplay(const MonitoringHistory *h) {
    ObservationNode *cur = h->head;
    printf("HEAD\n");
    while (cur != NULL) {
        printf(" |\n v\n[%s | %s | Health: %.1f]\n", cur->zoneId, cur->date, cur->healthScore);
        cur = cur->next;
    }
    printf(" |\n v\nNULL\n");
}

void historyClear(MonitoringHistory *h) {
    while (h->head != NULL) {
        ObservationNode *del = h->head;
        h->head = h->head->next;
        free(del);
    }
    h->count = 0;
}
