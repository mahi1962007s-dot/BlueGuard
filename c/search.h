#ifndef SEARCH_H
#define SEARCH_H

#include "zone.h"

/* ============================================================
 * UNIT 1 - SEARCHING
 * ============================================================ */
typedef struct {
    int  found;
    int  index;        /* array index, -1 if not found */
    int  comparisons;  /* element comparisons performed */
    char trace[1024];  /* human readable step trace     */
} SearchResult;

/* Linear Search by zoneId. Best O(1), Average/Worst O(n). */
SearchResult linearSearch(const Zone zones[], int size, const char *targetId);

/* Binary Search by zoneId. PRECONDITION: zones[] sorted ascending by zoneId.
 * Best O(1), Average/Worst O(log n). */
SearchResult binarySearch(const Zone zones[], int size, const char *targetId);

#endif /* SEARCH_H */
