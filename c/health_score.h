#ifndef HEALTH_SCORE_H
#define HEALTH_SCORE_H

#include "zone.h"

/* Beach Health Score formula (project-defined weights, NOT an official
 * environmental standard):
 *
 *   Health = 100 - ( 0.22*plastic + 0.24*(100-water) + 0.17*erosion
 *                  + 0.18*wasteCoverage + 0.08*visitors + 0.11*anomaly )
 *
 * clamped to [0,100].
 * Risk bands: >=65 LOW, >=45 MEDIUM, otherwise HIGH.
 */
float     calculateHealthScore(const Zone *zone);
RiskLevel classifyRisk(float healthScore);
void      applyHealthScore(Zone *zone);

/* Letter grade A-F (project-defined bands): A>=80, B>=65, C>=55, D>=45, else F. */
const char *gradeFromScore(float healthScore);

/* Plain-language swim advisory based on water quality and overall health. */
const char *swimAdvisory(const Zone *zone);

#endif /* HEALTH_SCORE_H */
