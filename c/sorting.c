#define _POSIX_C_SOURCE 200809L
#include "sorting.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
static double nowMs(void) {
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1000.0 / (double)f.QuadPart;
}
#else
#include <time.h>
static double nowMs(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}
#endif

static void initStats(SortStats *s, const char *name, const char *cx, int size) {
    snprintf(s->algorithmName, sizeof s->algorithmName, "%s", name);
    snprintf(s->timeComplexity, sizeof s->timeComplexity, "%s", cx);
    s->comparisons = 0;
    s->moves = 0;
    s->executionTimeMs = 0.0;
    s->recordCount = size;
}

static void swapZones(Zone *a, Zone *b) {
    Zone t = *a; *a = *b; *b = t;
}

int shouldComeBefore(const Zone *a, const Zone *b, SortKey key, int ascending) {
    int cmp = 0;
    switch (key) {
        case SORT_HEALTH_SCORE:
            cmp = (a->healthScore < b->healthScore) ? -1 : (a->healthScore > b->healthScore ? 1 : 0);
            break;
        case SORT_PLASTIC_WASTE:
            cmp = (a->plasticWaste < b->plasticWaste) ? -1 : (a->plasticWaste > b->plasticWaste ? 1 : 0);
            break;
        case SORT_WATER_QUALITY:
            cmp = (a->waterQuality < b->waterQuality) ? -1 : (a->waterQuality > b->waterQuality ? 1 : 0);
            break;
        case SORT_ZONE_ID:
            cmp = strcmp(a->zoneId, b->zoneId);
            break;
    }
    return ascending ? (cmp < 0) : (cmp > 0);
}

/* ---------------- BUBBLE SORT - O(n^2) ---------------- */
void bubbleSort(Zone zones[], int size, SortKey key, int ascending, SortStats *stats) {
    int i, j;
    double t0 = nowMs();
    initStats(stats, "Bubble Sort", "O(n^2)", size);

    for (i = 0; i < size - 1; i++) {
        int swapped = 0;
        for (j = 0; j < size - 1 - i; j++) {
            stats->comparisons++;
            if (shouldComeBefore(&zones[j + 1], &zones[j], key, ascending)) {
                swapZones(&zones[j], &zones[j + 1]);
                stats->moves++;
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
    stats->executionTimeMs = nowMs() - t0;
}

/* ---------------- INSERTION SORT - O(n^2) ---------------- */
void insertionSort(Zone zones[], int size, SortKey key, int ascending, SortStats *stats) {
    int i, j;
    double t0 = nowMs();
    initStats(stats, "Insertion Sort", "O(n^2)", size);

    for (i = 1; i < size; i++) {
        Zone item = zones[i];
        j = i - 1;
        while (j >= 0) {
            stats->comparisons++;
            if (shouldComeBefore(&item, &zones[j], key, ascending)) {
                zones[j + 1] = zones[j];
                stats->moves++;
                j--;
            } else {
                break;
            }
        }
        zones[j + 1] = item;
    }
    stats->executionTimeMs = nowMs() - t0;
}

/* ---------------- MERGE SORT - O(n log n) ---------------- */
static void mergeParts(Zone zones[], int left, int mid, int right,
                       SortKey key, int ascending, SortStats *stats) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    int i = 0, j = 0, k = left;
    Zone *L = (Zone *)malloc(sizeof(Zone) * (size_t)n1);
    Zone *R = (Zone *)malloc(sizeof(Zone) * (size_t)n2);

    if (!L || !R) { free(L); free(R); return; }
    memcpy(L, zones + left, sizeof(Zone) * (size_t)n1);
    memcpy(R, zones + mid + 1, sizeof(Zone) * (size_t)n2);

    while (i < n1 && j < n2) {
        stats->comparisons++;
        if (!shouldComeBefore(&R[j], &L[i], key, ascending)) zones[k++] = L[i++];
        else                                                 zones[k++] = R[j++];
        stats->moves++;
    }
    while (i < n1) { zones[k++] = L[i++]; stats->moves++; }
    while (j < n2) { zones[k++] = R[j++]; stats->moves++; }
    free(L);
    free(R);
}

static void mergeSortRec(Zone zones[], int left, int right,
                         SortKey key, int ascending, SortStats *stats) {
    if (left >= right) return;
    {
        int mid = left + (right - left) / 2;
        mergeSortRec(zones, left, mid, key, ascending, stats);
        mergeSortRec(zones, mid + 1, right, key, ascending, stats);
        mergeParts(zones, left, mid, right, key, ascending, stats);
    }
}

void mergeSort(Zone zones[], int size, SortKey key, int ascending, SortStats *stats) {
    double t0 = nowMs();
    initStats(stats, "Merge Sort", "O(n log n)", size);
    if (size > 1) mergeSortRec(zones, 0, size - 1, key, ascending, stats);
    stats->executionTimeMs = nowMs() - t0;
}

/* ---------------- QUICK SORT - avg O(n log n), worst O(n^2) ---------------- */
static int partitionZones(Zone zones[], int low, int high,
                          SortKey key, int ascending, SortStats *stats) {
    Zone pivot = zones[high];
    int i = low - 1, j;

    for (j = low; j < high; j++) {
        stats->comparisons++;
        if (shouldComeBefore(&zones[j], &pivot, key, ascending)) {
            i++;
            swapZones(&zones[i], &zones[j]);
            stats->moves++;
        }
    }
    swapZones(&zones[i + 1], &zones[high]);
    stats->moves++;
    return i + 1;
}

static void quickSortRec(Zone zones[], int low, int high,
                         SortKey key, int ascending, SortStats *stats) {
    if (low < high) {
        int p = partitionZones(zones, low, high, key, ascending, stats);
        quickSortRec(zones, low, p - 1, key, ascending, stats);
        quickSortRec(zones, p + 1, high, key, ascending, stats);
    }
}

void quickSort(Zone zones[], int size, SortKey key, int ascending, SortStats *stats) {
    double t0 = nowMs();
    initStats(stats, "Quick Sort", "Average O(n log n), Worst O(n^2)", size);
    if (size > 1) quickSortRec(zones, 0, size - 1, key, ascending, stats);
    stats->executionTimeMs = nowMs() - t0;
}

/* ---------------- RADIX SORT (LSD, base 10) on integer health score ---------------- */
void radixSortByHealthScore(Zone zones[], int size, int ascending, SortStats *stats) {
    double t0 = nowMs();
    int maxVal = 0, place, i, d;
    Zone *output;
    initStats(stats, "Radix Sort", "O(d * (n + b))", size);

    if (size <= 1) { stats->executionTimeMs = nowMs() - t0; return; }
    output = (Zone *)malloc(sizeof(Zone) * (size_t)size);
    if (!output) return;

    for (i = 0; i < size; i++) {
        int v = (int)zones[i].healthScore;
        if (v > maxVal) maxVal = v;
        stats->comparisons++;
    }

    for (place = 1; maxVal / place > 0; place *= 10) {
        int count[10] = {0};

        for (i = 0; i < size; i++)
            count[((int)zones[i].healthScore / place) % 10]++;

        if (ascending) { for (d = 1; d < 10; d++) count[d] += count[d - 1]; }
        else           { for (d = 8; d >= 0; d--) count[d] += count[d + 1]; }

        for (i = size - 1; i >= 0; i--) {
            int digit = ((int)zones[i].healthScore / place) % 10;
            output[--count[digit]] = zones[i];
            stats->moves++;
        }
        for (i = 0; i < size; i++) zones[i] = output[i];
    }
    free(output);
    stats->executionTimeMs = nowMs() - t0;
}

/* ---------------- HEAP SORT - O(n log n) ---------------- */
/* Max-heap with respect to the requested order: the element that must come LAST
 * sits at the root, so repeatedly moving the root to the end yields the order. */
static void heapSiftDown(Zone zones[], int root, int end, SortKey key, int ascending, SortStats *stats) {
    while (2 * root + 1 <= end) {
        int child = 2 * root + 1, big = root;
        stats->comparisons++;
        if (shouldComeBefore(&zones[big], &zones[child], key, ascending)) big = child;
        if (child + 1 <= end) {
            stats->comparisons++;
            if (shouldComeBefore(&zones[big], &zones[child + 1], key, ascending)) big = child + 1;
        }
        if (big == root) return;
        swapZones(&zones[root], &zones[big]);
        stats->moves++;
        root = big;
    }
}

void heapSort(Zone zones[], int size, SortKey key, int ascending, SortStats *stats) {
    int start, end;
    double t0 = nowMs();
    initStats(stats, "Heap Sort", "O(n log n)", size);
    if (size > 1) {
        for (start = (size - 2) / 2; start >= 0; start--)
            heapSiftDown(zones, start, size - 1, key, ascending, stats);
        for (end = size - 1; end > 0; end--) {
            swapZones(&zones[0], &zones[end]);
            stats->moves++;
            heapSiftDown(zones, 0, end - 1, key, ascending, stats);
        }
    }
    stats->executionTimeMs = nowMs() - t0;
}
