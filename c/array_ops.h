#ifndef ARRAY_OPS_H
#define ARRAY_OPS_H

#include "zone.h"

/* ============================================================
 * UNIT 1 - ARRAYS
 * The beach zones live in a plain fixed-size C array of Zone.
 * ============================================================ */

/* 2D "environmental matrix" (Unit 1 - multidimensional arrays).
 * Rows = zone index, Columns = plastic, water, erosion, waste,
 * visitors, anomaly. */
#define MATRIX_COLUMNS 6

/* Loads zones from a CSV dataset file. Returns number of zones read,
 * or -1 if the file cannot be opened / has no valid rows. */
int loadZonesFromCSV(Zone zones[], int maxSize, const char *path);

/* Built-in 12-zone sample dataset (used if no CSV file is found). */
int loadSampleZones(Zone zones[], int maxSize);

/* Tries: explicit path, $BEACH_DATA, data/beach_dataset.csv and a few
 * relative variants; falls back to the built-in sample.
 * Writes a description of the source into sourceOut. Returns zone count. */
int loadZones(Zone zones[], int maxSize, const char *explicitPath,
              char *sourceOut, int sourceLen);

/* Sorts the array by zoneId (insertion sort) - required for Binary Search. */
void sortZonesById(Zone zones[], int size);

void buildEnvironmentalMatrix(const Zone zones[], int size,
                              float matrix[][MATRIX_COLUMNS]);
void calculateAllHealthScores(Zone zones[], int size);

void displayZones(const Zone zones[], int size);
void displayEnvironmentalMatrix(const float matrix[][MATRIX_COLUMNS], int size);

/* Index of zoneId by plain traversal; -1 if absent. */
int indexOfZoneId(const Zone zones[], int size, const char *zoneId);

#endif /* ARRAY_OPS_H */
