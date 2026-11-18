#ifndef ADVANCED_H
#define ADVANCED_H

#include "zone.h"
#include "linked_list.h"

/* ============================================================
 * ADVANCED MODULE - graphs, hashing, benchmarking and analytics.
 * Every function prints exactly ONE JSON line (used by --server mode).
 * No math library is needed (own square root).
 * ============================================================ */

/* Graph of zones: shoreline path + service-road shortcuts.
 * BFS / DFS traversal, Dijkstra shortest route, nearest-neighbour cleanup tour. */
void advGraph(const Zone zones[], int n);
void advBfs(const Zone zones[], int n, const char *sourceId);
void advRoute(const Zone zones[], int n, const char *fromId, const char *toId);
void advTour(const Zone zones[], int n, int count, const char *depotId);

/* Hash table (separate chaining) compared with linear and binary search. */
void advLookup(const Zone zones[], int n, const char *zoneId);

/* Times every sorting algorithm on generated data of growing size. */
void advBenchmark(int maxN);

/* Linear regression + moving average over a zone's observation history. */
void advForecast(const MonitoringHistory *h, const char *zoneId);

/* K-means clustering of zones by pollution profile. */
void advClusters(const Zone zones[], int n, int k);

/* Z-score anomaly detection (across zones and across a zone's own history). */
void advAnomalies(const Zone zones[], int n, const MonitoringHistory *h, float threshold);

/* Creates six weekly sample observations for every zone that has none.
 * The newest one equals the zone's current reading. Returns observations added. */
int advSeedHistory(MonitoringHistory *h, const Zone zones[], int n);

#endif
