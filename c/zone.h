#ifndef ZONE_H
#define ZONE_H

/* Maximum zones the array can hold (the sample dataset uses 12). */
#define MAX_ZONES 50
#define ID_LEN    8
#define NAME_LEN  48

/* Risk classification derived from the Beach Health Score. */
typedef enum {
    RISK_LOW,
    RISK_MEDIUM,
    RISK_HIGH,
    RISK_UNSET
} RiskLevel;

/* A single monitoring zone along the beach.
 * Stored inside a plain C array (Unit 1 - Arrays). */
typedef struct {
    char  zoneId[ID_LEN];      /* e.g. "Z01"        */
    char  zoneName[NAME_LEN];  /* e.g. "North Gate" */

    float plasticWaste;          /* 0-100 (%)                */
    float waterQuality;          /* 0-100 (%) higher = better */
    float erosionLevel;          /* 0-100 (%)                */
    float wasteCoverage;         /* 0-100 (%)                */
    float visitorLoad;           /* 0-100 (%)                */
    float environmentalAnomaly;  /* 0-100 (%)                */

    float     healthScore;       /* 0-100, computed          */
    RiskLevel riskLevel;         /* computed from healthScore */
} Zone;

void zoneInit(Zone *z, const char *id, const char *name,
              float plastic, float water, float erosion,
              float waste, float visitors, float anomaly);

const char *riskLevelToString(RiskLevel level);

#endif /* ZONE_H */
