#ifndef NY_DATASET_H
#define NY_DATASET_H

#include "zone.h"

/* ============================================================
 * REAL DATASET LOADER (Unit 1 - arrays + file handling)
 * Reads the NY State Parks "Beach Water Testing: Beginning 2015"
 * CSV (data.ny.gov, dataset id wwwd-za77) and turns the first N
 * beaches with enough samples into N Zone records.
 *
 * Returns the number of zones filled, or -1 if the file is missing
 * or is not this dataset (header must contain "State Park Beach").
 * ============================================================ */
int loadZonesFromNYDataset(Zone zones[], int maxSize, const char *path);

#endif /* NY_DATASET_H */
