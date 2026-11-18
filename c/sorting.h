#ifndef SORTING_H
#define SORTING_H

#include "zone.h"

/* ============================================================
 * UNIT 1 - SORTING (all implemented manually, no qsort())
 * Each function sorts a Zone array IN PLACE and fills SortStats.
 * ============================================================ */
typedef enum {
    SORT_HEALTH_SCORE,
    SORT_PLASTIC_WASTE,
    SORT_WATER_QUALITY,
    SORT_ZONE_ID
} SortKey;

typedef struct {
    char      algorithmName[32];
    char      timeComplexity[48];
    long long comparisons;
    long long moves;          /* swaps / shifts / merges depending on algorithm */
    double    executionTimeMs;
    int       recordCount;
} SortStats;

/* Returns 1 if a should be ordered before b for the given key/direction. */
int shouldComeBefore(const Zone *a, const Zone *b, SortKey key, int ascending);

void bubbleSort(Zone zones[], int size, SortKey key, int ascending, SortStats *stats);
void insertionSort(Zone zones[], int size, SortKey key, int ascending, SortStats *stats);
void mergeSort(Zone zones[], int size, SortKey key, int ascending, SortStats *stats);
void quickSort(Zone zones[], int size, SortKey key, int ascending, SortStats *stats);

void heapSort(Zone zones[], int size, SortKey key, int ascending, SortStats *stats);

/* LSD Radix Sort (base 10) on the integer part of the Health Score. */
void radixSortByHealthScore(Zone zones[], int size, int ascending, SortStats *stats);

#endif /* SORTING_H */
