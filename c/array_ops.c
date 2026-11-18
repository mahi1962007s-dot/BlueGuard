#include "array_ops.h"
#include "health_score.h"
#include "ny_dataset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---------- small helpers ---------- */

static char *trim(char *s) {
    char *end;
    while (*s && (isspace((unsigned char)*s) || *s == '"')) s++;
    end = s + strlen(s);
    while (end > s && (isspace((unsigned char)end[-1]) || end[-1] == '"')) end--;
    *end = '\0';
    return s;
}

static int parseNumber(const char *s, float *out) {
    char *endp;
    double v;
    if (*s == '\0') return 0;
    v = strtod(s, &endp);
    if (*endp != '\0') return 0;
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    *out = (float)v;
    return 1;
}

/* ---------- dataset loading ---------- */

int loadZonesFromCSV(Zone zones[], int maxSize, const char *path) {
    FILE *fp = fopen(path, "r");
    char line[512];
    int count = 0;
    if (!fp) return -1;

    while (count < maxSize && fgets(line, sizeof line, fp)) {
        char *fields[8];
        char *p = line;
        int n = 0;
        float v[6];
        int ok = 1, i;
        char *first;

        first = trim(line);
        if (*first == '\0' || *first == '#') continue;

        /* split on commas (at most 8 fields) */
        p = line;
        while (n < 8) {
            char *comma = strchr(p, ',');
            fields[n++] = p;
            if (!comma) break;
            *comma = '\0';
            p = comma + 1;
        }
        if (n < 8) continue;
        for (i = 0; i < 8; i++) fields[i] = trim(fields[i]);

        for (i = 0; i < 6; i++) {
            if (!parseNumber(fields[2 + i], &v[i])) { ok = 0; break; }
        }
        if (!ok) continue; /* header row or malformed row -> skip */

        zoneInit(&zones[count], fields[0], fields[1], v[0], v[1], v[2], v[3], v[4], v[5]);
        for (i = 0; zones[count].zoneId[i]; i++)
            zones[count].zoneId[i] = (char)toupper((unsigned char)zones[count].zoneId[i]);
        count++;
    }
    fclose(fp);
    return count > 0 ? count : -1;
}

int loadSampleZones(Zone zones[], int maxSize) {
    typedef struct {
        const char *id, *name;
        float plastic, water, erosion, waste, visitors, anomaly;
    } Raw;

    static const Raw raw[12] = {
        {"Z01", "North Gate",     31, 76, 22, 28, 61, 10},
        {"Z02", "North East",     44, 68, 30, 35, 55, 18},
        {"Z03", "East Point",     58, 60, 35, 47, 66, 22},
        {"Z04", "East Bay",       67, 52, 40, 55, 70, 28},
        {"Z05", "Central East",   52, 58, 33, 44, 60, 20},
        {"Z06", "Central",        60, 55, 38, 50, 82, 25},
        {"Z07", "Central West",   72, 48, 61, 58, 45, 30},
        {"Z08", "West Bay",       49, 63, 28, 40, 58, 15},
        {"Z09", "West Point",     36, 71, 24, 30, 50, 12},
        {"Z10", "South West",     63, 46, 74, 62, 40, 35},
        {"Z11", "South Central",  40, 66, 27, 33, 47, 14},
        {"Z12", "South Gate",     81, 40, 55, 70, 38, 42}
    };
    int n = (maxSize < 12) ? maxSize : 12;
    int i;
    for (i = 0; i < n; i++)
        zoneInit(&zones[i], raw[i].id, raw[i].name, raw[i].plastic, raw[i].water,
                 raw[i].erosion, raw[i].waste, raw[i].visitors, raw[i].anomaly);
    return n;
}

int loadZones(Zone zones[], int maxSize, const char *explicitPath,
              char *sourceOut, int sourceLen) {
    const char *dirEnv = getenv("BEACH_DATA_DIR");
    const char *fileEnv = getenv("BEACH_DATA");
    const char *dirs[4];
    char path[512];
    int nd = 0, i, n;

    if (explicitPath && *explicitPath) {
        n = loadZonesFromNYDataset(zones, maxSize, explicitPath);
        if (n <= 0) n = loadZonesFromCSV(zones, maxSize, explicitPath);
        if (n > 0) { snprintf(sourceOut, sourceLen, "dataset file: %s", explicitPath); return n; }
    }
    if (fileEnv && *fileEnv) {
        n = loadZonesFromNYDataset(zones, maxSize, fileEnv);
        if (n <= 0) n = loadZonesFromCSV(zones, maxSize, fileEnv);
        if (n > 0) { snprintf(sourceOut, sourceLen, "dataset file: %s", fileEnv); return n; }
    }

    if (dirEnv && *dirEnv) dirs[nd++] = dirEnv;
    dirs[nd++] = "data";
    dirs[nd++] = "../data";
    dirs[nd++] = "../../data";

    /* 1) real website dataset (NY State Parks Beach Water Testing) */
    for (i = 0; i < nd; i++) {
        snprintf(path, sizeof path, "%s/ny_beach_water_testing.csv", dirs[i]);
        n = loadZonesFromNYDataset(zones, maxSize, path);
        if (n > 0) {
            snprintf(sourceOut, sourceLen, "real dataset: NY State Parks Beach Water Testing (%s)", path);
            return n;
        }
    }
    /* 2) 12-zone sample CSV */
    for (i = 0; i < nd; i++) {
        snprintf(path, sizeof path, "%s/beach_dataset.csv", dirs[i]);
        n = loadZonesFromCSV(zones, maxSize, path);
        if (n > 0) {
            snprintf(sourceOut, sourceLen, "sample CSV: %s", path);
            return n;
        }
    }
    /* 3) built-in sample */
    n = loadSampleZones(zones, maxSize);
    snprintf(sourceOut, sourceLen, "built-in sample data (no CSV found)");
    return n;
}

void sortZonesById(Zone zones[], int size) {
    int i, j;
    for (i = 1; i < size; i++) {
        Zone key = zones[i];
        j = i - 1;
        while (j >= 0 && strcmp(zones[j].zoneId, key.zoneId) > 0) {
            zones[j + 1] = zones[j];
            j--;
        }
        zones[j + 1] = key;
    }
}

/* ---------- matrix / scores / display ---------- */

void buildEnvironmentalMatrix(const Zone zones[], int size,
                              float matrix[][MATRIX_COLUMNS]) {
    int i;
    for (i = 0; i < size; i++) {
        matrix[i][0] = zones[i].plasticWaste;
        matrix[i][1] = zones[i].waterQuality;
        matrix[i][2] = zones[i].erosionLevel;
        matrix[i][3] = zones[i].wasteCoverage;
        matrix[i][4] = zones[i].visitorLoad;
        matrix[i][5] = zones[i].environmentalAnomaly;
    }
}

void calculateAllHealthScores(Zone zones[], int size) {
    int i;
    for (i = 0; i < size; i++) applyHealthScore(&zones[i]);
}

void displayZones(const Zone zones[], int size) {
    int i;
    printf("%-6s%-26s%-10s%-14s%-10s%-10s\n",
           "ID", "Name", "Health", "Risk", "Plastic", "Water");
    printf("--------------------------------------------------------------------------\n");
    for (i = 0; i < size; i++) {
        printf("%-6s%-26.24s%-10.1f%-14s%-10.1f%-10.1f\n",
               zones[i].zoneId, zones[i].zoneName, zones[i].healthScore,
               riskLevelToString(zones[i].riskLevel),
               zones[i].plasticWaste, zones[i].waterQuality);
    }
}

void displayEnvironmentalMatrix(const float matrix[][MATRIX_COLUMNS], int size) {
    int i, j;
    printf("%-8s%-10s%-10s%-10s%-10s%-10s%-10s\n",
           "Zone#", "Plastic", "Water", "Erosion", "Waste", "Visitors", "Anomaly");
    printf("----------------------------------------------------------------\n");
    for (i = 0; i < size; i++) {
        printf("%-8d", i + 1);
        for (j = 0; j < MATRIX_COLUMNS; j++) printf("%-10.1f", matrix[i][j]);
        printf("\n");
    }
}

int indexOfZoneId(const Zone zones[], int size, const char *zoneId) {
    int i;
    for (i = 0; i < size; i++)
        if (strcmp(zones[i].zoneId, zoneId) == 0) return i;
    return -1;
}
