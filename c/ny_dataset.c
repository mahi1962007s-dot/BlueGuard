#include "ny_dataset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BEACHES   256
#define EXCEED_LIMIT  235.0   /* E. coli single-sample limit, CFU/100 mL */
#define DEFAULT_ZONES 12
#define DEFAULT_MIN_SAMPLES 30

typedef struct {
    char name[NAME_LEN];
    long total;    /* every record for this beach                 */
    long valid;    /* records that carry a numeric E. coli result */
    long pass;     /* valid results <= EXCEED_LIMIT               */
    long exceed;   /* records whose reason is "Exceedance"        */
    long algae;    /* records closed/cleared for algae blooms     */
    long open;     /* records with status Open / Reopened         */
} BeachAcc;

/* Splits one CSV line (handles "quoted, fields"). Modifies the line. */
static int parseCsvLine(char *line, char *fields[], int maxFields) {
    int n = 0;
    char *p = line;
    while (n < maxFields) {
        if (*p == '"') {
            char *start, *w;
            int atComma;
            p++;
            start = w = p;
            while (*p) {
                if (*p == '"') {
                    if (p[1] == '"') { *w++ = '"'; p += 2; continue; }
                    p++;
                    break;
                }
                *w++ = *p++;
            }
            atComma = (*p == ',');
            *w = '\0';
            fields[n++] = start;
            if (!atComma) break;
            p++;
        } else {
            fields[n++] = p;
            while (*p && *p != ',') p++;
            if (*p == ',') { *p = '\0'; p++; } else break;
        }
    }
    return n;
}

static int parseResult(const char *modifier, const char *text, double *out) {
    char buf[32];
    size_t i, j = 0;
    char *endp;
    for (i = 0; text[i] && j < sizeof buf - 1; i++)
        if (text[i] != ',') buf[j++] = text[i];   /* "1,110" -> "1110" */
    buf[j] = '\0';
    if (j == 0) return 0;
    *out = strtod(buf, &endp);
    if (*endp != '\0') return 0;
    if (modifier[0] == '>') *out += 0.5;          /* ">2,000" is above 2000 */
    return 1;
}

static float clampPct(double v) {
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    return (float)v;
}

int loadZonesFromNYDataset(Zone zones[], int maxSize, const char *path) {
    static BeachAcc acc[MAX_BEACHES];
    static char line[2048];
    FILE *fp = fopen(path, "r");
    int nb = 0, count = 0, wanted, minSamples, i;
    const char *env;

    /* Plastic waste and erosion are NOT in this dataset; they stay simulated
     * (same values as the built-in sample) so the score formula still works. */
    static const float simPlastic[12] = {31, 44, 58, 67, 52, 60, 72, 49, 36, 63, 40, 81};
    static const float simErosion[12] = {22, 30, 35, 40, 33, 38, 61, 28, 24, 74, 27, 55};

    if (!fp) return -1;

    if (!fgets(line, sizeof line, fp) || !strstr(line, "State Park Beach")) {
        fclose(fp);
        return -1;
    }

    while (fgets(line, sizeof line, fp)) {
        char *f[12];
        size_t len = strlen(line);
        int n, b;
        double val;
        BeachAcc *a;

        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
        if (len == 0) continue;
        n = parseCsvLine(line, f, 12);
        if (n < 10 || f[1][0] == '\0') continue;

        for (b = 0; b < nb; b++)
            if (strcmp(acc[b].name, f[1]) == 0) break;
        if (b == nb) {
            if (nb == MAX_BEACHES) continue;
            memset(&acc[nb], 0, sizeof acc[nb]);
            snprintf(acc[nb].name, NAME_LEN, "%s", f[1]);
            nb++;
        }
        a = &acc[b];

        a->total++;
        if (parseResult(f[5], f[6], &val)) {
            a->valid++;
            if (val <= EXCEED_LIMIT) a->pass++;
        }
        if (strcmp(f[9], "Exceedance") == 0) a->exceed++;
        if (strstr(f[9], "Algal") || strstr(f[9], "algae")) a->algae++;
        if (strncmp(f[8], "Open", 4) == 0 || strcmp(f[8], "Reopened") == 0) a->open++;
    }
    fclose(fp);

    wanted = DEFAULT_ZONES;
    env = getenv("BEACH_ZONES");
    if (env && atoi(env) > 0) wanted = atoi(env);
    if (wanted > maxSize) wanted = maxSize;

    minSamples = DEFAULT_MIN_SAMPLES;
    env = getenv("BEACH_MIN_SAMPLES");
    if (env && atoi(env) > 0) minSamples = atoi(env);

    for (i = 0; i < nb && count < wanted; i++) {
        BeachAcc *a = &acc[i];
        char id[ID_LEN];
        if (a->valid < minSamples || a->total == 0) continue;

        snprintf(id, sizeof id, "Z%02d", (count + 1) % 100);
        zoneInit(&zones[count], id, a->name,
                 simPlastic[count % 12],                              /* simulated        */
                 clampPct(100.0 * a->pass / a->valid),                /* water quality    */
                 simErosion[count % 12],                              /* simulated        */
                 clampPct(100.0 * a->algae / a->total),               /* waste/bloom cover*/
                 clampPct(100.0 * a->open / a->total),                /* visitor load     */
                 clampPct(100.0 * a->exceed / a->total));             /* anomaly          */
        count++;
    }
    return count > 0 ? count : -1;
}
