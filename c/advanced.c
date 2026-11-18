#include "advanced.h"
#include "health_score.h"
#include "search.h"
#include "sorting.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define INF      1.0e9
#define BUCKETS  17
#define MAX_OBS  512

/* ---------------------------------------------------------------
 * small helpers (JSON output, math without libm)
 * --------------------------------------------------------------- */
static void jstr(const char *s) {
    putchar('"');
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') { putchar('\\'); putchar(c); }
        else if (c < 0x20) printf("\\u%04x", c);
        else putchar(c);
    }
    putchar('"');
}

static void jerr(int code, const char *msg) {
    printf("{\"ok\":false,\"code\":%d,\"error\":", code);
    jstr(msg);
    printf("}\n");
}

static double dabs(double x) { return x < 0 ? -x : x; }

static double dsqrt(double x) {
    double g;
    int i;
    if (x <= 0) return 0;
    g = x > 1 ? x : 1;
    for (i = 0; i < 40; i++) g = 0.5 * (g + x / g);
    return g;
}

static double clamp100(double v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }
static double round2(double v) { return (double)((long)(v * 100.0 + 0.5)) / 100.0; }

static int idxOf(const Zone z[], int n, const char *id) {
    char up[ID_LEN];
    int i;
    snprintf(up, sizeof up, "%s", id);
    for (i = 0; up[i]; i++) up[i] = (char)toupper((unsigned char)up[i]);
    for (i = 0; i < n; i++) if (strcmp(z[i].zoneId, up) == 0) return i;
    return -1;
}

static void jzone(const Zone *z) {
    printf("{\"zoneId\":"); jstr(z->zoneId);
    printf(",\"zoneName\":"); jstr(z->zoneName);
    printf(",\"healthScore\":%.2f,\"grade\":", z->healthScore);
    jstr(gradeFromScore(z->healthScore));
    printf(",\"riskLevel\":"); jstr(riskLevelToString(z->riskLevel));
    putchar('}');
}

/* ===============================================================
 * 1. GRAPH  (adjacency matrix)
 *    Zones lie along the shore (Z01-Z02-Z03 ...). Every third zone
 *    also has a service road to the zone three steps along.
 * =============================================================== */
typedef struct { int a, b; double km; int road; } Edge;

typedef struct {
    int    n;
    double w[MAX_ZONES][MAX_ZONES];     /* 0 = no edge */
    char   road[MAX_ZONES][MAX_ZONES];
    int    edgeCount;
    Edge   e[MAX_ZONES * 2];
} Graph;

static void addEdge(Graph *g, int a, int b, double km, int road) {
    km = round2(km);
    g->w[a][b] = g->w[b][a] = km;
    g->road[a][b] = g->road[b][a] = (char)road;
    g->e[g->edgeCount].a = a; g->e[g->edgeCount].b = b;
    g->e[g->edgeCount].km = km; g->e[g->edgeCount].road = road;
    g->edgeCount++;
}

static void buildGraph(Graph *g, int n) {
    int i, j;
    memset(g, 0, sizeof *g);
    g->n = n;
    for (i = 0; i < n - 1; i++) addEdge(g, i, i + 1, 0.40 + ((i * 5) % 7) * 0.12, 0);
    for (i = 0; i + 3 < n; i += 3) addEdge(g, i, i + 3, 1.10 + ((i * 3) % 4) * 0.15, 1);
    (void)j;
}

void advGraph(const Zone z[], int n) {
    Graph *g = (Graph *)malloc(sizeof(Graph));
    int i;
    if (!g) { jerr(500, "Out of memory"); return; }
    buildGraph(g, n);
    printf("{\"nodeCount\":%d,\"edgeCount\":%d,\"nodes\":[", n, g->edgeCount);
    for (i = 0; i < n; i++) { if (i) putchar(','); jzone(&z[i]); }
    printf("],\"edges\":[");
    for (i = 0; i < g->edgeCount; i++) {
        if (i) putchar(',');
        printf("{\"from\":"); jstr(z[g->e[i].a].zoneId);
        printf(",\"to\":"); jstr(z[g->e[i].b].zoneId);
        printf(",\"km\":%.2f,\"type\":\"%s\"}", g->e[i].km, g->e[i].road ? "road" : "shore");
    }
    printf("]}\n");
    free(g);
}

/* ---- BFS: level by level ---- */
static void bfs(const Graph *g, int src, int level[], int parent[], int order[], int *count, int *examined) {
    int queue[MAX_ZONES], head = 0, tail = 0, i;
    for (i = 0; i < g->n; i++) { level[i] = -1; parent[i] = -1; }
    *count = 0; *examined = 0;
    level[src] = 0;
    queue[tail++] = src;
    while (head < tail) {
        int u = queue[head++], v;
        order[(*count)++] = u;
        for (v = 0; v < g->n; v++) {
            if (g->w[u][v] <= 0) continue;
            (*examined)++;
            if (level[v] < 0) {
                level[v] = level[u] + 1;
                parent[v] = u;
                queue[tail++] = v;
            }
        }
    }
}

/* ---- DFS: dive along one branch before backing up ---- */
static void dfsRec(const Graph *g, int u, char seen[], int order[], int *count) {
    int v;
    seen[u] = 1;
    order[(*count)++] = u;
    for (v = 0; v < g->n; v++)
        if (g->w[u][v] > 0 && !seen[v]) dfsRec(g, v, seen, order, count);
}

void advBfs(const Zone z[], int n, const char *sourceId) {
    Graph *g;
    int level[MAX_ZONES], parent[MAX_ZONES], order[MAX_ZONES], dfsOrder[MAX_ZONES];
    char seen[MAX_ZONES];
    int src = idxOf(z, n, sourceId), count = 0, examined = 0, dcount = 0, maxLevel = 0, i, L;
    double load, exposure;
    if (src < 0) { jerr(404, "Unknown source zone"); return; }
    g = (Graph *)malloc(sizeof(Graph));
    if (!g) { jerr(500, "Out of memory"); return; }
    buildGraph(g, n);
    bfs(g, src, level, parent, order, &count, &examined);
    memset(seen, 0, sizeof seen);
    dfsRec(g, src, seen, dfsOrder, &dcount);
    for (i = 0; i < n; i++) if (level[i] > maxLevel) maxLevel = level[i];
    load = 100.0 - z[src].healthScore;

    printf("{\"source\":"); jstr(z[src].zoneId);
    printf(",\"load\":%.2f,\"decay\":0.6,\"maxLevel\":%d,\"edgesExamined\":%d,\"levels\":[", load, maxLevel, examined);
    for (L = 0; L <= maxLevel; L++) {
        int first = 1;
        if (L) putchar(',');
        printf("{\"level\":%d,\"zones\":[", L);
        for (i = 0; i < count; i++) {
            if (level[order[i]] != L) continue;
            if (!first) putchar(',');
            jstr(z[order[i]].zoneId);
            first = 0;
        }
        printf("]}");
    }
    printf("],\"bfsOrder\":[");
    for (i = 0; i < count; i++) { if (i) putchar(','); jstr(z[order[i]].zoneId); }
    printf("],\"dfsOrder\":[");
    for (i = 0; i < dcount; i++) { if (i) putchar(','); jstr(z[dfsOrder[i]].zoneId); }
    printf("],\"exposure\":[");
    for (i = 0; i < n; i++) {
        int k;
        exposure = load;
        for (k = 0; k < level[i]; k++) exposure *= 0.6;
        if (i) putchar(',');
        printf("{\"zoneId\":"); jstr(z[i].zoneId);
        printf(",\"level\":%d,\"exposure\":%.2f}", level[i], level[i] < 0 ? 0.0 : exposure);
    }
    printf("],\"tree\":[");
    {
        int first = 1;
        for (i = 0; i < n; i++) {
            if (parent[i] < 0) continue;
            if (!first) putchar(',');
            printf("{\"from\":"); jstr(z[parent[i]].zoneId);
            printf(",\"to\":"); jstr(z[i].zoneId); printf("}");
            first = 0;
        }
    }
    printf("]}\n");
    free(g);
}

/* ---- Dijkstra on the adjacency matrix, O(V^2) ---- */
static void dijkstra(const Graph *g, int src, int useRoads, double dist[], int prev[], int *relax) {
    char done[MAX_ZONES];
    int i, it;
    *relax = 0;
    for (i = 0; i < g->n; i++) { dist[i] = INF; prev[i] = -1; done[i] = 0; }
    dist[src] = 0;
    for (it = 0; it < g->n; it++) {
        int u = -1, v;
        for (i = 0; i < g->n; i++)
            if (!done[i] && (u < 0 || dist[i] < dist[u])) u = i;
        if (u < 0 || dist[u] >= INF) break;
        done[u] = 1;
        for (v = 0; v < g->n; v++) {
            if (g->w[u][v] <= 0 || done[v]) continue;
            if (!useRoads && g->road[u][v]) continue;
            if (dist[u] + g->w[u][v] < dist[v]) {
                dist[v] = dist[u] + g->w[u][v];
                prev[v] = u;
                (*relax)++;
            }
        }
    }
}

static int buildPath(const int prev[], int src, int dst, int path[]) {
    int tmp[MAX_ZONES], len = 0, cur = dst, i;
    while (cur >= 0 && len < MAX_ZONES) { tmp[len++] = cur; if (cur == src) break; cur = prev[cur]; }
    if (len == 0 || tmp[len - 1] != src) return 0;
    for (i = 0; i < len; i++) path[i] = tmp[len - 1 - i];
    return len;
}

static void printPathIds(const Zone z[], const int path[], int len) {
    int i;
    putchar('[');
    for (i = 0; i < len; i++) { if (i) putchar(','); jstr(z[path[i]].zoneId); }
    putchar(']');
}

void advRoute(const Zone z[], int n, const char *fromId, const char *toId) {
    Graph *g;
    double dist[MAX_ZONES], dist2[MAX_ZONES];
    int prev[MAX_ZONES], prev2[MAX_ZONES], path[MAX_ZONES], path2[MAX_ZONES];
    int a = idxOf(z, n, fromId), b = idxOf(z, n, toId), relax, relax2, len, len2, i;
    if (a < 0 || b < 0) { jerr(404, "Choose two valid zones"); return; }
    g = (Graph *)malloc(sizeof(Graph));
    if (!g) { jerr(500, "Out of memory"); return; }
    buildGraph(g, n);
    dijkstra(g, a, 1, dist, prev, &relax);
    dijkstra(g, a, 0, dist2, prev2, &relax2);
    len = buildPath(prev, a, b, path);
    len2 = buildPath(prev2, a, b, path2);

    printf("{\"from\":"); jstr(z[a].zoneId);
    printf(",\"to\":"); jstr(z[b].zoneId);
    printf(",\"distance\":%.2f,\"relaxations\":%d,\"path\":", dist[b], relax);
    printPathIds(z, path, len);
    printf(",\"legs\":[");
    for (i = 0; i + 1 < len; i++) {
        if (i) putchar(',');
        printf("{\"from\":"); jstr(z[path[i]].zoneId);
        printf(",\"to\":"); jstr(z[path[i + 1]].zoneId);
        printf(",\"km\":%.2f,\"type\":\"%s\"}", g->w[path[i]][path[i + 1]], g->road[path[i]][path[i + 1]] ? "road" : "shore");
    }
    printf("],\"shoreOnly\":{\"distance\":%.2f,\"path\":", dist2[b]);
    printPathIds(z, path2, len2);
    printf("},\"saved\":%.2f}\n", dist2[b] - dist[b]);
    free(g);
}

/* ---- cleanup tour: visit the worst zones, always going to the nearest next ---- */
void advTour(const Zone z[], int n, int count, const char *depotId) {
    Graph *g;
    int depot = *depotId ? idxOf(z, n, depotId) : 0;
    int node[10], m = 0, k, i, j;
    int picked[MAX_ZONES];
    double dm[10][10], prevs_dummy = 0;
    int pv[10][MAX_ZONES];
    int order[10], used[10], cur = 0, ordered = 0;
    double total = 0, naive = 0;
    (void)prevs_dummy;
    if (depot < 0) { jerr(404, "Unknown depot zone"); return; }
    if (count < 2) count = 2;
    if (count > 8) count = 8;
    if (count > n - 1) count = n - 1;
    if (count < 1) { jerr(400, "Not enough zones"); return; }
    g = (Graph *)malloc(sizeof(Graph));
    if (!g) { jerr(500, "Out of memory"); return; }
    buildGraph(g, n);

    memset(picked, 0, sizeof picked);
    picked[depot] = 1;
    node[m++] = depot;
    for (k = 0; k < count; k++) {          /* the `count` weakest zones (not the depot) */
        int best = -1;
        for (i = 0; i < n; i++)
            if (!picked[i] && (best < 0 || z[i].healthScore < z[best].healthScore)) best = i;
        picked[best] = 1;
        node[m++] = best;
    }
    for (i = 0; i < m; i++) {
        double dist[MAX_ZONES]; int relax;
        dijkstra(g, node[i], 1, dist, pv[i], &relax);
        for (j = 0; j < m; j++) dm[i][j] = dist[node[j]];
    }
    memset(used, 0, sizeof used);
    used[0] = 1;
    order[ordered++] = 0;
    while (ordered < m) {                   /* nearest neighbour */
        int best = -1;
        for (j = 1; j < m; j++)
            if (!used[j] && (best < 0 || dm[cur][j] < dm[cur][best])) best = j;
        used[best] = 1; order[ordered++] = best; total += dm[cur][best]; cur = best;
    }
    for (j = 1; j < m; j++) naive += dm[j - 1][j];   /* worst-first order, as scheduled */

    printf("{\"depot\":"); jstr(z[depot].zoneId);
    printf(",\"count\":%d,\"total\":%.2f,\"naiveTotal\":%.2f,\"saved\":%.2f,\"order\":[", count, total, naive, naive - total);
    for (i = 0; i < m; i++) { if (i) putchar(','); jstr(z[node[order[i]]].zoneId); }
    printf("],\"targets\":[");
    for (i = 1; i < m; i++) { if (i > 1) putchar(','); jzone(&z[node[i]]); }
    printf("],\"legs\":[");
    for (i = 0; i + 1 < m; i++) {
        int path[MAX_ZONES], len = buildPath(pv[order[i]], node[order[i]], node[order[i + 1]], path);
        if (i) putchar(',');
        printf("{\"from\":"); jstr(z[node[order[i]]].zoneId);
        printf(",\"to\":"); jstr(z[node[order[i + 1]]].zoneId);
        printf(",\"km\":%.2f,\"path\":", dm[order[i]][order[i + 1]]);
        printPathIds(z, path, len);
        putchar('}');
    }
    printf("]}\n");
    free(g);
}

/* ===============================================================
 * 2. HASH TABLE (separate chaining) vs linear vs binary search
 * =============================================================== */
typedef struct HNode { int idx; struct HNode *next; } HNode;

static unsigned long djb2(const char *s) {
    unsigned long h = 5381;
    while (*s) h = h * 33 + (unsigned char)*s++;
    return h;
}

void advLookup(const Zone z[], int n, const char *zoneId) {
    HNode *table[BUCKETS] = {0};
    int i, b, collisions = 0, longest = 0, hasId = (zoneId && *zoneId);
    char up[ID_LEN];

    snprintf(up, sizeof up, "%s", zoneId ? zoneId : "");
    for (i = 0; up[i]; i++) up[i] = (char)toupper((unsigned char)up[i]);

    for (i = 0; i < n; i++) {                 /* build: append to the end of each chain */
        HNode *node = (HNode *)malloc(sizeof(HNode));
        HNode **slot;
        if (!node) { jerr(500, "Out of memory"); return; }
        node->idx = i; node->next = NULL;
        slot = &table[djb2(z[i].zoneId) % BUCKETS];
        while (*slot) slot = &(*slot)->next;
        *slot = node;
    }
    for (b = 0; b < BUCKETS; b++) {
        int len = 0; HNode *c;
        for (c = table[b]; c; c = c->next) len++;
        if (len > 1) collisions += len - 1;
        if (len > longest) longest = len;
    }

    printf("{\"id\":"); jstr(up);
    if (hasId) {
        SearchResult lin = linearSearch(z, n, up), bin = binarySearch(z, n, up);
        unsigned long hv = djb2(up);
        int bucket = (int)(hv % BUCKETS), comps = 0, found = -1;
        HNode *c;
        for (c = table[bucket]; c; c = c->next) {
            comps++;
            if (strcmp(z[c->idx].zoneId, up) == 0) { found = c->idx; break; }
        }
        printf(",\"linear\":{\"found\":%s,\"comparisons\":%d,\"complexity\":\"O(n)\"}", lin.found ? "true" : "false", lin.comparisons);
        printf(",\"binary\":{\"found\":%s,\"comparisons\":%d,\"complexity\":\"O(log n)\"}", bin.found ? "true" : "false", bin.comparisons);
        printf(",\"hash\":{\"found\":%s,\"comparisons\":%d,\"hashValue\":%lu,\"bucket\":%d,\"complexity\":\"O(1) average\",\"chain\":[",
               found >= 0 ? "true" : "false", comps, hv, bucket);
        for (c = table[bucket]; c; c = c->next) { if (c != table[bucket]) putchar(','); jstr(z[c->idx].zoneId); }
        printf("]},\"zone\":");
        if (found >= 0) jzone(&z[found]); else printf("null");
    } else {
        printf(",\"linear\":null,\"binary\":null,\"hash\":null,\"zone\":null");
    }
    printf(",\"table\":{\"buckets\":%d,\"items\":%d,\"loadFactor\":%.2f,\"collisions\":%d,\"longestChain\":%d,\"function\":\"djb2(key) mod %d\",\"slots\":[",
           BUCKETS, n, (double)n / BUCKETS, collisions, longest, BUCKETS);
    for (b = 0; b < BUCKETS; b++) {
        HNode *c = table[b];
        if (b) putchar(',');
        putchar('[');
        for (; c; c = c->next) { if (c != table[b]) putchar(','); jstr(z[c->idx].zoneId); }
        putchar(']');
    }
    printf("]}}\n");

    for (b = 0; b < BUCKETS; b++) {
        HNode *c = table[b];
        while (c) { HNode *nx = c->next; free(c); c = nx; }
    }
}

/* ===============================================================
 * 3. BENCHMARK
 * =============================================================== */
static unsigned long lcgState;
static unsigned long lcgNext(void) {
    lcgState = lcgState * 1103515245UL + 12345UL;
    return (lcgState >> 8) & 0xFFFFFFUL;
}

/* intOnly: radix sort orders by the integer part of the score only */
static int isSortedAsc(const Zone z[], int n, int intOnly) {
    int i;
    for (i = 1; i < n; i++) {
        if (intOnly ? ((int)z[i - 1].healthScore > (int)z[i].healthScore)
                    : (z[i - 1].healthScore > z[i].healthScore)) return 0;
    }
    return 1;
}

void advBenchmark(int maxN) {
    static const int sizes[] = {100, 500, 1000, 5000, 10000, 20000, 50000, 100000};
    static const char *keys[] = {"bubble", "insertion", "merge", "quick", "heap", "radix"};
    const int nSizes = (int)(sizeof sizes / sizeof sizes[0]), nAlgo = 6;
    int si, ai, i, first = 1, used = 0;
    Zone *base, *work;
    if (maxN < 100) maxN = 10000;
    if (maxN > 100000) maxN = 100000;
    base = (Zone *)malloc(sizeof(Zone) * 100000);
    work = (Zone *)malloc(sizeof(Zone) * 100000);
    if (!base || !work) { free(base); free(work); jerr(500, "Out of memory"); return; }

    printf("{\"maxN\":%d,\"quadraticLimit\":20000,\"results\":[", maxN);
    for (si = 0; si < nSizes; si++) {
        int n = sizes[si];
        if (n > maxN) break;
        lcgState = 2026UL + (unsigned long)n;
        memset(base, 0, sizeof(Zone) * (size_t)n);
        for (i = 0; i < n; i++) {
            snprintf(base[i].zoneId, ID_LEN, "Z%d", i % 100000);
            base[i].healthScore = (float)(lcgNext() % 10000) / 100.0f;
        }
        if (!first) putchar(',');
        first = 0; used++;
        printf("{\"n\":%d,\"runs\":[", n);
        for (ai = 0; ai < nAlgo; ai++) {
            SortStats s;
            int skip = (ai < 2 && n > 20000);
            if (ai) putchar(',');
            if (skip) {
                printf("{\"key\":\"%s\",\"skipped\":true}", keys[ai]);
                continue;
            }
            memcpy(work, base, sizeof(Zone) * (size_t)n);
            switch (ai) {
                case 0: bubbleSort(work, n, SORT_HEALTH_SCORE, 1, &s); break;
                case 1: insertionSort(work, n, SORT_HEALTH_SCORE, 1, &s); break;
                case 2: mergeSort(work, n, SORT_HEALTH_SCORE, 1, &s); break;
                case 3: quickSort(work, n, SORT_HEALTH_SCORE, 1, &s); break;
                case 4: heapSort(work, n, SORT_HEALTH_SCORE, 1, &s); break;
                default: radixSortByHealthScore(work, n, 1, &s); break;
            }
            printf("{\"key\":\"%s\",\"name\":", keys[ai]); jstr(s.algorithmName);
            printf(",\"complexity\":"); jstr(s.timeComplexity);
            printf(",\"skipped\":false,\"ms\":%.4f,\"comparisons\":%lld,\"moves\":%lld,\"sorted\":%s}",
                   s.executionTimeMs, s.comparisons, s.moves, isSortedAsc(work, n, ai == 5) ? "true" : "false");
        }
        printf("]}");
    }
    printf("],\"sizes\":%d}\n", used);
    free(base); free(work);
}

/* ===============================================================
 * 4. FORECAST (least-squares line + moving average)
 * =============================================================== */
static int zoneHistory(const MonitoringHistory *h, const char *id, double ys[], char dates[][DATE_LEN], int cap) {
    const ObservationNode *c;
    int n = 0;
    for (c = h->head; c && n < cap; c = c->next)
        if (strcmp(c->zoneId, id) == 0) {
            ys[n] = c->healthScore;
            snprintf(dates[n], DATE_LEN, "%s", c->date);
            n++;
        }
    return n;
}

void advForecast(const MonitoringHistory *h, const char *zoneId) {
    static double ys[MAX_OBS];
    static char dates[MAX_OBS][DATE_LEN];
    char id[ID_LEN];
    int n, i, k;
    double sx = 0, sy = 0, sxx = 0, sxy = 0, den, slope, icpt, ssRes = 0, ssTot = 0, mean, sigma = 0;
    snprintf(id, sizeof id, "%s", zoneId);
    for (i = 0; id[i]; i++) id[i] = (char)toupper((unsigned char)id[i]);

    n = zoneHistory(h, id, ys, dates, MAX_OBS);
    printf("{\"zoneId\":"); jstr(id);
    if (n < 3) { printf(",\"enough\":false,\"count\":%d,\"needed\":3}\n", n); return; }

    for (i = 0; i < n; i++) { sx += i; sy += ys[i]; sxx += (double)i * i; sxy += i * ys[i]; }
    den = n * sxx - sx * sx;
    slope = den != 0 ? (n * sxy - sx * sy) / den : 0;
    icpt = (sy - slope * sx) / n;
    mean = sy / n;
    for (i = 0; i < n; i++) {
        double fit = icpt + slope * i;
        ssRes += (ys[i] - fit) * (ys[i] - fit);
        ssTot += (ys[i] - mean) * (ys[i] - mean);
    }
    if (n > 2) sigma = dsqrt(ssRes / (n - 2));

    printf(",\"enough\":true,\"count\":%d,\"slope\":%.3f,\"intercept\":%.2f,\"r2\":%.3f,\"sigma\":%.2f,\"trend\":\"%s\"",
           n, slope, icpt, ssTot > 0 ? 1.0 - ssRes / ssTot : 0.0, sigma,
           slope > 0.5 ? "Improving" : (slope < -0.5 ? "Worsening" : "Stable"));

    printf(",\"observations\":[");
    for (i = 0; i < n; i++) {
        double sum = 0; int lo = i - 2 < 0 ? 0 : i - 2, w = 0, j;
        for (j = lo; j <= i; j++) { sum += ys[j]; w++; }
        if (i) putchar(',');
        printf("{\"date\":"); jstr(dates[i]);
        printf(",\"health\":%.2f,\"fit\":%.2f,\"sma\":%.2f}", ys[i], clamp100(icpt + slope * i), sum / w);
    }
    printf("],\"forecast\":[");
    for (k = 1; k <= 3; k++) {
        double p = clamp100(icpt + slope * (n - 1 + k));
        if (k > 1) putchar(',');
        printf("{\"step\":%d,\"health\":%.2f,\"low\":%.2f,\"high\":%.2f,\"risk\":", k, p, clamp100(p - sigma), clamp100(p + sigma));
        jstr(riskLevelToString(classifyRisk((float)p)));
        printf(",\"grade\":"); jstr(gradeFromScore((float)p));
        printf("}");
    }
    printf("],\"drops\":[");
    if (n >= 4) {                           /* sudden drops: change well below the typical change */
        double dm = 0, dv = 0, ds; int first = 1;
        for (i = 1; i < n; i++) dm += ys[i] - ys[i - 1];
        dm /= (n - 1);
        for (i = 1; i < n; i++) dv += (ys[i] - ys[i - 1] - dm) * (ys[i] - ys[i - 1] - dm);
        ds = dsqrt(dv / (n - 1));
        for (i = 1; i < n; i++) {
            double d = ys[i] - ys[i - 1];
            if (ds > 0 && d < 0 && d < dm - 1.5 * ds) {
                if (!first) putchar(',');
                printf("{\"index\":%d,\"date\":", i); jstr(dates[i]); printf(",\"change\":%.2f}", d);
                first = 0;
            }
        }
    }
    printf("]}\n");
}

/* ===============================================================
 * 5. K-MEANS
 * =============================================================== */
#define FEAT 6

static void features(const Zone *z, double f[FEAT]) {
    f[0] = z->plasticWaste; f[1] = 100.0 - z->waterQuality; f[2] = z->erosionLevel;
    f[3] = z->wasteCoverage; f[4] = z->visitorLoad; f[5] = z->environmentalAnomaly;
}

static double dist2(const double a[FEAT], const double b[FEAT]) {
    double s = 0; int i;
    for (i = 0; i < FEAT; i++) s += (a[i] - b[i]) * (a[i] - b[i]);
    return s;
}

static double kmeans(const Zone z[], int n, int k, int assign[], double cent[][FEAT], int *iters) {
    double f[MAX_ZONES][FEAT], inertia = 0;
    int chosen[8], c, i, changed = 1, it = 0;
    for (i = 0; i < n; i++) features(&z[i], f[i]);

    /* deterministic farthest-first start: weakest zone, then the zone farthest from all centres */
    chosen[0] = 0;
    for (i = 1; i < n; i++) if (z[i].healthScore < z[chosen[0]].healthScore) chosen[0] = i;
    for (c = 1; c < k; c++) {
        int best = -1; double bestD = -1;
        for (i = 0; i < n; i++) {
            double nearest = INF; int q;
            for (q = 0; q < c; q++) { double d = dist2(f[i], f[chosen[q]]); if (d < nearest) nearest = d; }
            if (nearest > bestD) { bestD = nearest; best = i; }
        }
        chosen[c] = best;
    }
    for (c = 0; c < k; c++) memcpy(cent[c], f[chosen[c]], sizeof(double) * FEAT);
    for (i = 0; i < n; i++) assign[i] = -1;

    while (changed && it < 100) {
        double sum[8][FEAT]; int cnt[8];
        changed = 0; it++;
        for (i = 0; i < n; i++) {
            int best = 0; double bd = INF;
            for (c = 0; c < k; c++) { double d = dist2(f[i], cent[c]); if (d < bd) { bd = d; best = c; } }
            if (assign[i] != best) { assign[i] = best; changed = 1; }
        }
        memset(sum, 0, sizeof sum); memset(cnt, 0, sizeof cnt);
        for (i = 0; i < n; i++) { int q; cnt[assign[i]]++; for (q = 0; q < FEAT; q++) sum[assign[i]][q] += f[i][q]; }
        for (c = 0; c < k; c++) if (cnt[c]) { int q; for (q = 0; q < FEAT; q++) cent[c][q] = sum[c][q] / cnt[c]; }
    }
    for (i = 0; i < n; i++) inertia += dist2(f[i], cent[assign[i]]);
    *iters = it;
    return inertia;
}

void advClusters(const Zone z[], int n, int k) {
    static const char *tags2[] = {"Healthy", "Stressed"};
    static const char *tags3[] = {"Healthy", "Watch", "Critical"};
    static const char *tags4[] = {"Healthy", "Fair", "Watch", "Critical"};
    static const char *tags5[] = {"Excellent", "Healthy", "Watch", "Stressed", "Critical"};
    const char **tags;
    int assign[MAX_ZONES], rank[8], cnt[8], order[8], iters = 0, c, i, kk, maxK;
    double cent[8][FEAT], avg[8], inertia;
    if (n < 2) { jerr(400, "At least two zones are needed"); return; }
    if (k < 2) k = 3;
    if (k > 5) k = 5;
    if (k > n) k = n;
    inertia = kmeans(z, n, k, assign, cent, &iters);

    memset(cnt, 0, sizeof cnt); memset(avg, 0, sizeof avg);
    for (i = 0; i < n; i++) { cnt[assign[i]]++; avg[assign[i]] += z[i].healthScore; }
    for (c = 0; c < k; c++) { avg[c] = cnt[c] ? avg[c] / cnt[c] : -1; order[c] = c; }
    for (c = 0; c < k; c++)                    /* healthiest cluster first */
        for (i = c + 1; i < k; i++)
            if (avg[order[i]] > avg[order[c]]) { int t = order[c]; order[c] = order[i]; order[i] = t; }
    for (c = 0; c < k; c++) rank[order[c]] = c;
    tags = k == 2 ? tags2 : (k == 3 ? tags3 : (k == 4 ? tags4 : tags5));

    printf("{\"k\":%d,\"iterations\":%d,\"inertia\":%.2f,\"clusters\":[", k, iters, inertia);
    for (c = 0; c < k; c++) {
        int id = order[c], first = 1, q;
        if (c) putchar(',');
        printf("{\"rank\":%d,\"tag\":", c); jstr(tags[c]);
        printf(",\"size\":%d,\"avgHealth\":%.2f,\"centroid\":{\"plasticWaste\":%.2f,\"waterQuality\":%.2f,\"erosionLevel\":%.2f,\"wasteCoverage\":%.2f,\"visitorLoad\":%.2f,\"environmentalAnomaly\":%.2f},\"members\":[",
               cnt[id], avg[id] < 0 ? 0.0 : avg[id], cent[id][0], 100.0 - cent[id][1], cent[id][2], cent[id][3], cent[id][4], cent[id][5]);
        for (q = 0; q < n; q++) if (assign[q] == id) { if (!first) putchar(','); jstr(z[q].zoneId); first = 0; }
        printf("]}");
    }
    printf("],\"assign\":[");
    for (i = 0; i < n; i++) {
        if (i) putchar(',');
        printf("{\"zoneId\":"); jstr(z[i].zoneId); printf(",\"cluster\":%d}", rank[assign[i]]);
    }
    printf("],\"elbow\":[");
    maxK = n < 6 ? n : 6;
    for (kk = 1; kk <= maxK; kk++) {
        int a2[MAX_ZONES], it2; double c2[8][FEAT];
        if (kk > 1) putchar(',');
        printf("{\"k\":%d,\"inertia\":%.2f}", kk, kmeans(z, n, kk, a2, c2, &it2));
    }
    printf("]}\n");
}

/* ===============================================================
 * 6. ANOMALIES (z-score)
 * =============================================================== */
typedef struct { int zone, metric; double value, mean, std, z; } Flag;

static double metricValue(const Zone *z, int m) {
    switch (m) {
        case 0: return z->plasticWaste;  case 1: return z->waterQuality;
        case 2: return z->erosionLevel;  case 3: return z->wasteCoverage;
        case 4: return z->visitorLoad;   case 5: return z->environmentalAnomaly;
        default: return z->healthScore;
    }
}

void advAnomalies(const Zone z[], int n, const MonitoringHistory *h, float threshold) {
    static const char *names[] = {"Plastic waste", "Water quality", "Erosion", "Waste coverage",
                                  "Visitor load", "Environmental anomaly", "Health score"};
    static const int lowIsBad[] = {0, 1, 0, 0, 0, 0, 1};
    static Flag flags[MAX_ZONES * 7];
    static double ys[MAX_OBS];
    static char dates[MAX_OBS][DATE_LEN];
    double mean[7], sd[7];
    int nf = 0, m, i, j, shown, first, histChecked = 0;
    if (threshold < 0.5f) threshold = 1.5f;
    if (n < 3) { jerr(400, "At least three zones are needed"); return; }

    for (m = 0; m < 7; m++) {
        double s = 0, v = 0;
        for (i = 0; i < n; i++) s += metricValue(&z[i], m);
        mean[m] = s / n;
        for (i = 0; i < n; i++) { double d = metricValue(&z[i], m) - mean[m]; v += d * d; }
        sd[m] = dsqrt(v / n);
        if (sd[m] < 0.0001) continue;
        for (i = 0; i < n; i++) {
            double val = metricValue(&z[i], m), zs = (val - mean[m]) / sd[m];
            if (dabs(zs) >= threshold) {
                flags[nf].zone = i; flags[nf].metric = m; flags[nf].value = val;
                flags[nf].mean = mean[m]; flags[nf].std = sd[m]; flags[nf].z = zs; nf++;
            }
        }
    }
    for (i = 1; i < nf; i++) {                 /* insertion sort, largest |z| first */
        Flag key = flags[i]; j = i - 1;
        while (j >= 0 && dabs(flags[j].z) < dabs(key.z)) { flags[j + 1] = flags[j]; j--; }
        flags[j + 1] = key;
    }

    printf("{\"threshold\":%.2f,\"zonesChecked\":%d,\"flagCount\":%d,\"baseline\":[", threshold, n, nf);
    for (m = 0; m < 7; m++) {
        if (m) putchar(',');
        printf("{\"metric\":"); jstr(names[m]); printf(",\"mean\":%.2f,\"std\":%.2f}", mean[m], sd[m]);
    }
    printf("],\"zoneFlags\":[");
    shown = nf > 40 ? 40 : nf;
    for (i = 0; i < shown; i++) {
        int bad = flags[i].z > 0 ? !lowIsBad[flags[i].metric] : lowIsBad[flags[i].metric];
        if (i) putchar(',');
        printf("{\"zoneId\":"); jstr(z[flags[i].zone].zoneId);
        printf(",\"zoneName\":"); jstr(z[flags[i].zone].zoneName);
        printf(",\"metric\":"); jstr(names[flags[i].metric]);
        printf(",\"value\":%.2f,\"mean\":%.2f,\"std\":%.2f,\"z\":%.2f,\"direction\":\"%s\",\"concern\":%s}",
               flags[i].value, flags[i].mean, flags[i].std, flags[i].z,
               flags[i].z > 0 ? "high" : "low", bad ? "true" : "false");
    }
    printf("],\"historyFlags\":[");
    first = 1;
    for (i = 0; i < n; i++) {                  /* latest CHANGE vs this zone's usual changes (a steady trend is not an anomaly) */
        int cnt = zoneHistory(h, z[i].zoneId, ys, dates, MAX_OBS), q;
        double mu = 0, vv = 0, sdv, zs, latest;
        if (cnt < 5) continue;
        histChecked++;
        latest = ys[cnt - 1] - ys[cnt - 2];
        for (q = 1; q <= cnt - 2; q++) mu += ys[q] - ys[q - 1];
        mu /= (cnt - 2);
        for (q = 1; q <= cnt - 2; q++) { double dd = ys[q] - ys[q - 1] - mu; vv += dd * dd; }
        sdv = dsqrt(vv / (cnt - 2));
        if (sdv < 1.0) sdv = 1.0;              /* floor: ignore sub-point wobble */
        zs = (latest - mu) / sdv;
        if (dabs(zs) >= threshold) {
            if (!first) putchar(',');
            printf("{\"zoneId\":"); jstr(z[i].zoneId);
            printf(",\"zoneName\":"); jstr(z[i].zoneName);
            printf(",\"latest\":%.2f,\"change\":%.2f,\"usualChange\":%.2f,\"usualStd\":%.2f,\"z\":%.2f,\"observations\":%d,\"direction\":\"%s\"}",
                   ys[cnt - 1], latest, mu, sdv, zs, cnt, zs > 0 ? "up" : "down");
            first = 0;
        }
    }
    printf("],\"historyZonesChecked\":%d}\n", histChecked);
}

/* ===============================================================
 * 7. SAMPLE HISTORY
 * =============================================================== */
int advSeedHistory(MonitoringHistory *h, const Zone z[], int n) {
    int zi, k, added = 0;
    time_t now = time(NULL);
    for (zi = 0; zi < n; zi++) {
        const ObservationNode *c;
        int has = 0;
        double s = (((zi * 37) % 11) - 5) / 5.0;     /* -1 (worsening) .. +1 (improving) */
        for (c = h->head; c; c = c->next) if (strcmp(c->zoneId, z[zi].zoneId) == 0) { has = 1; break; }
        if (has) continue;
        for (k = 0; k < 6; k++) {
            Zone t = z[zi];
            double back = 5 - k, noise = (k == 5) ? 0.0 : (((zi * 13 + k * 7) % 5) - 2) * 0.7;
            time_t when = now - (time_t)(back * 7 * 86400);
            char date[DATE_LEN];
            struct tm *lt = localtime(&when);
            if (lt) strftime(date, sizeof date, "%d/%m/%Y", lt); else snprintf(date, sizeof date, "01/01/2026");
            t.plasticWaste = (float)clamp100(z[zi].plasticWaste - back * s * 2.2 + noise);
            t.waterQuality = (float)clamp100(z[zi].waterQuality + back * s * 1.6 - noise * 0.5);
            t.erosionLevel = (float)clamp100(z[zi].erosionLevel - back * s * 1.0 + noise * 0.4);
            applyHealthScore(&t);
            if (historyInsertSorted(h, z[zi].zoneId, date, t.plasticWaste, t.waterQuality, t.erosionLevel, t.healthScore))
                added++;
        }
    }
    return added;
}
