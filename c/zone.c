#include "zone.h"
#include <stdio.h>

void zoneInit(Zone *z, const char *id, const char *name,
              float plastic, float water, float erosion,
              float waste, float visitors, float anomaly) {
    snprintf(z->zoneId, ID_LEN, "%s", id);
    snprintf(z->zoneName, NAME_LEN, "%s", name);
    z->plasticWaste = plastic;
    z->waterQuality = water;
    z->erosionLevel = erosion;
    z->wasteCoverage = waste;
    z->visitorLoad = visitors;
    z->environmentalAnomaly = anomaly;
    z->healthScore = 0;
    z->riskLevel = RISK_UNSET;
}

const char *riskLevelToString(RiskLevel level) {
    switch (level) {
        case RISK_LOW:    return "LOW RISK";
        case RISK_MEDIUM: return "MEDIUM RISK";
        case RISK_HIGH:   return "HIGH RISK";
        default:          return "UNKNOWN";
    }
}
