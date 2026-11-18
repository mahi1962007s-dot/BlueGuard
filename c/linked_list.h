#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "zone.h"

/* ============================================================
 * UNIT 2 - SINGLY LINKED LIST
 * Historical environmental observations, built with malloc/free.
 * ============================================================ */
#define DATE_LEN 16

typedef struct ObservationNode {
    char  zoneId[ID_LEN];
    char  date[DATE_LEN];
    float plasticWaste;
    float waterQuality;
    float erosionLevel;
    float healthScore;
    struct ObservationNode *next;
} ObservationNode;

typedef struct {
    ObservationNode *head;
    int count;
} MonitoringHistory;

void historyInit(MonitoringHistory *h);

int  historyInsertAtBeginning(MonitoringHistory *h, const char *zoneId, const char *date,
                              float plastic, float water, float erosion, float health);
int  historyInsertAtEnd(MonitoringHistory *h, const char *zoneId, const char *date,
                        float plastic, float water, float erosion, float health);
/* Inserts so the list stays in date order (oldest first). Returns the new node, or NULL. */
ObservationNode *historyInsertSorted(MonitoringHistory *h, const char *zoneId, const char *date,
                                     float plastic, float water, float erosion, float health);

int  historyInsertAtPosition(MonitoringHistory *h, int position, const char *zoneId,
                             const char *date, float plastic, float water,
                             float erosion, float health);

int  historyDeleteAtBeginning(MonitoringHistory *h);
int  historyDeleteAtEnd(MonitoringHistory *h);
int  historyDeleteAtPosition(MonitoringHistory *h, int position);
int  historyDeleteByZoneId(MonitoringHistory *h, const char *zoneId);

ObservationNode *historySearch(const MonitoringHistory *h, const char *zoneId, int *comparisons);
int  historyUpdate(MonitoringHistory *h, const char *zoneId, float plastic, float water,
                   float erosion, float health);

void historyDisplay(const MonitoringHistory *h);
void historyClear(MonitoringHistory *h);

#endif /* LINKED_LIST_H */
