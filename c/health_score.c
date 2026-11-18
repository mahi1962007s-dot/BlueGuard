#include "health_score.h"

float calculateHealthScore(const Zone *zone) {
    float penalty =
        0.22f * zone->plasticWaste +
        0.24f * (100.0f - zone->waterQuality) +
        0.17f * zone->erosionLevel +
        0.18f * zone->wasteCoverage +
        0.08f * zone->visitorLoad +
        0.11f * zone->environmentalAnomaly;

    float score = 100.0f - penalty;
    if (score < 0.0f)   score = 0.0f;
    if (score > 100.0f) score = 100.0f;
    return score;
}

RiskLevel classifyRisk(float healthScore) {
    if (healthScore >= 65.0f) return RISK_LOW;
    if (healthScore >= 45.0f) return RISK_MEDIUM;
    return RISK_HIGH;
}

void applyHealthScore(Zone *zone) {
    zone->healthScore = calculateHealthScore(zone);
    zone->riskLevel = classifyRisk(zone->healthScore);
}

const char *gradeFromScore(float s) {
    if (s >= 80.0f) return "A";
    if (s >= 65.0f) return "B";
    if (s >= 55.0f) return "C";
    if (s >= 45.0f) return "D";
    return "F";
}

const char *swimAdvisory(const Zone *z) {
    if (z->waterQuality < 50.0f || z->healthScore < 45.0f) return "Not recommended";
    if (z->waterQuality < 65.0f || z->healthScore < 65.0f) return "Swim with caution";
    return "Safe to swim";
}
