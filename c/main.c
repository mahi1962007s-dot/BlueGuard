/* ============================================================
 * BLUEGUARD - BEACH HEALTH MONITORING ENGINE (plain C99)
 * ============================================================
 *
 * Three ways to run the same code:
 *
 *  1. beachdsa                 -> interactive 24-option console menu
 *  2. beachdsa --server        -> persistent mode used by the Node.js backend:
 *                                 reads one command per line from stdin and
 *                                 prints one JSON line per command. All state
 *                                 (zones, linked list, stack, queue) lives here.
 *  3. beachdsa <command> ...   -> one-shot mode: prints one JSON line and exits
 *                                 e.g.  beachdsa zones
 *                                       beachdsa search binary Z07
 *                                       beachdsa sort quick health desc
 *
 * Command format inside --server mode uses '|' as separator:
 *   zones | health | search|linear|Z07 | sort|merge|plastic|desc
 *   hist_list | hist_add|0|Z03|24/09/2026|40|60|30|55 | hist_del|Z03 ...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "zone.h"
#include "health_score.h"
#include "array_ops.h"
#include "search.h"
#include "sorting.h"
#include "linked_list.h"
#include "stack.h"
#include "queue.h"
#include "advanced.h"

/* ---------------------------------------------------------------
 * Shared application state
 * --------------------------------------------------------------- */
typedef struct {
    Zone  zones[MAX_ZONES];   /* always kept sorted by zoneId (needed by Binary Search) */
    int   zoneCount;
    float matrix[MAX_ZONES][MATRIX_COLUMNS];
    char  dataSource[256];
    MonitoringHistory history;
    AlertStack alerts;
    TaskQueue  tasks;
} AppState;

static const Zone *findZone(const AppState *st, const char *id) {
    int i = indexOfZoneId(st->zones, st->zoneCount, id);
    return i >= 0 ? &st->zones[i] : NULL;
}

/* ---------------------------------------------------------------
 * Rule-based alerts and cleanup planning (derived from the data)
 * --------------------------------------------------------------- */
enum { ISSUE_NONE = -1, ISSUE_WATER, ISSUE_PLASTIC, ISSUE_EROSION, ISSUE_WASTE, ISSUE_ANOMALY, ISSUE_GENERAL };

static const char *ISSUE_ALERT[] = {
    "Poor water quality", "High plastic pollution", "Severe erosion",
    "Waste coverage rising", "Environmental anomaly", "Overall health critical"
};
static const char *ISSUE_TASK[] = {
    "Water-quality retest", "Plastic collection", "Erosion inspection",
    "Waste clearance", "Anomaly investigation", "Full site inspection"
};

/* The most pressing problem of a zone, or ISSUE_NONE if it needs no action. */
static int dominantIssue(const Zone *z) {
    if (z->waterQuality <= 50.0f)         return ISSUE_WATER;
    if (z->plasticWaste >= 65.0f)         return ISSUE_PLASTIC;
    if (z->erosionLevel >= 60.0f)         return ISSUE_EROSION;
    if (z->wasteCoverage >= 60.0f)        return ISSUE_WASTE;
    if (z->environmentalAnomaly >= 35.0f) return ISSUE_ANOMALY;
    if (z->riskLevel == RISK_HIGH)        return ISSUE_GENERAL;
    return ISSUE_NONE;
}

/* Copies the zones and orders them worst-first with the project's own quick sort. */
static void worstFirst(const AppState *st, Zone work[]) {
    SortStats stats;
    memcpy(work, st->zones, sizeof(Zone) * (size_t)st->zoneCount);
    quickSort(work, st->zoneCount, SORT_HEALTH_SCORE, 1, &stats);
}

static void pushAlertFor(AppState *st, const Zone *z, int issue, const char *suffix) {
    Alert a;
    memset(&a, 0, sizeof a);
    snprintf(a.zoneId, ID_LEN, "%.7s", z->zoneId);
    snprintf(a.message, MSG_LEN, "%.7s %.40s - %s%s", z->zoneId, z->zoneName, ISSUE_ALERT[issue], suffix);
    snprintf(a.severity, SEVERITY_LEN, "%s", z->riskLevel == RISK_HIGH ? "HIGH" : "MEDIUM");
    stackPush(&st->alerts, a);
}

/* Pushes one alert per problem zone. Worst zones are pushed LAST so they end up on top. */
static void generateAlerts(AppState *st) {
    Zone work[MAX_ZONES];
    int pick[MAX_ALERTS], issue[MAX_ALERTS], n = 0, i;
    worstFirst(st, work);
    for (i = 0; i < st->zoneCount && n < MAX_ALERTS; i++) {
        int is = dominantIssue(&work[i]);
        if (is == ISSUE_NONE || work[i].riskLevel == RISK_LOW) continue;
        pick[n] = i; issue[n] = is; n++;
    }
    for (i = n - 1; i >= 0; i--) pushAlertFor(st, &work[pick[i]], issue[i], "");
}

/* Schedules cleanup for the `limit` worst zones. Returns how many tasks were added. */
static int planCleanup(AppState *st, int limit) {
    Zone work[MAX_ZONES];
    int added = 0, i;
    worstFirst(st, work);
    for (i = 0; i < st->zoneCount && added < limit; i++) {
        Task t;
        int is = dominantIssue(&work[i]);
        if (is == ISSUE_NONE || work[i].riskLevel == RISK_LOW) continue;
        { /* skip zones that already have a scheduled task */
            int k, idx = st->tasks.front, dup = 0;
            for (k = 0; k < st->tasks.count; k++, idx = (idx + 1) % MAX_TASKS)
                if (strcmp(st->tasks.tasks[idx].zoneId, work[i].zoneId) == 0) dup = 1;
            if (dup) continue;
        }
        memset(&t, 0, sizeof t);
        snprintf(t.zoneId, ID_LEN, "%.7s", work[i].zoneId);
        snprintf(t.description, MSG_LEN, "%s - %.7s", ISSUE_TASK[is], work[i].zoneId);
        snprintf(t.priority, SEVERITY_LEN, "%s", work[i].riskLevel == RISK_HIGH ? "HIGH" : "MEDIUM");
        if (!queueEnqueue(&st->tasks, t)) break;
        added++;
    }
    return added;
}

static void initializeState(AppState *st) {
    st->zoneCount = loadZones(st->zones, MAX_ZONES, NULL, st->dataSource, sizeof st->dataSource);
    calculateAllHealthScores(st->zones, st->zoneCount);
    sortZonesById(st->zones, st->zoneCount);
    buildEnvironmentalMatrix(st->zones, st->zoneCount, st->matrix);

    historyInit(&st->history);
    stackInit(&st->alerts);
    queueInit(&st->tasks);

    generateAlerts(st);
    planCleanup(st, 3);
}

/* ---------------------------------------------------------------
 * Tiny manual JSON helpers (no external library)
 * --------------------------------------------------------------- */
static void jsonStr(const char *s) {
    putchar('"');
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') { putchar('\\'); putchar(c); }
        else if (c == '\n') fputs("\\n", stdout);
        else if (c == '\r') fputs("\\r", stdout);
        else if (c == '\t') fputs("\\t", stdout);
        else if (c < 0x20) printf("\\u%04x", c);
        else putchar(c);
    }
    putchar('"');
}

static void jsonZone(const Zone *z) {
    printf("{\"zoneId\":"); jsonStr(z->zoneId);
    printf(",\"zoneName\":"); jsonStr(z->zoneName);
    printf(",\"plasticWaste\":%.2f,\"waterQuality\":%.2f,\"erosionLevel\":%.2f"
           ",\"wasteCoverage\":%.2f,\"visitorLoad\":%.2f,\"environmentalAnomaly\":%.2f"
           ",\"healthScore\":%.2f,\"riskLevel\":",
           z->plasticWaste, z->waterQuality, z->erosionLevel, z->wasteCoverage,
           z->visitorLoad, z->environmentalAnomaly, z->healthScore);
    jsonStr(riskLevelToString(z->riskLevel));
    printf(",\"grade\":"); jsonStr(gradeFromScore(z->healthScore));
    printf(",\"advisory\":"); jsonStr(swimAdvisory(z));
    putchar('}');
}

static void jsonZoneArray(const Zone zones[], int n) {
    int i;
    putchar('[');
    for (i = 0; i < n; i++) { if (i) putchar(','); jsonZone(&zones[i]); }
    putchar(']');
}

static void jsonObservation(const ObservationNode *o) {
    printf("{\"zoneId\":"); jsonStr(o->zoneId);
    printf(",\"date\":"); jsonStr(o->date);
    printf(",\"plasticWaste\":%.2f,\"waterQuality\":%.2f,\"erosionLevel\":%.2f,\"healthScore\":%.2f}",
           o->plasticWaste, o->waterQuality, o->erosionLevel, o->healthScore);
}

static void jsonAlert(const Alert *a) {
    printf("{\"zoneId\":"); jsonStr(a->zoneId);
    printf(",\"message\":"); jsonStr(a->message);
    printf(",\"severity\":"); jsonStr(a->severity); putchar('}');
}

static void jsonTask(const Task *t) {
    printf("{\"zoneId\":"); jsonStr(t->zoneId);
    printf(",\"description\":"); jsonStr(t->description);
    printf(",\"priority\":"); jsonStr(t->priority); putchar('}');
}

static void jsonError(int code, const char *msg) {
    printf("{\"ok\":false,\"code\":%d,\"error\":", code);
    jsonStr(msg);
    printf("}\n");
}

/* ---------------------------------------------------------------
 * Field helpers
 * --------------------------------------------------------------- */
static void upperInPlace(char *s) {
    for (; *s; s++) *s = (char)toupper((unsigned char)*s);
}

static int parseFloatField(const char *s, float *out) {
    char *endp;
    double v;
    if (!s || !*s) return 0;
    v = strtod(s, &endp);
    if (*endp != '\0') return 0;
    *out = (float)v;
    return 1;
}

static int splitFields(char *line, char *fields[], int maxFields) {
    int n = 0;
    char *p = line;
    while (n < maxFields) {
        char *bar = strchr(p, '|');
        fields[n++] = p;
        if (!bar) break;
        *bar = '\0';
        p = bar + 1;
    }
    return n;
}

static SortKey parseSortKey(const char *s) {
    if (strcmp(s, "plastic") == 0) return SORT_PLASTIC_WASTE;
    if (strcmp(s, "water") == 0)   return SORT_WATER_QUALITY;
    if (strcmp(s, "zoneid") == 0)  return SORT_ZONE_ID;
    return SORT_HEALTH_SCORE;
}

static void runSort(const char *algo, SortKey key, int ascending, Zone work[], int n, SortStats *stats) {
    if      (strcmp(algo, "bubble") == 0)    bubbleSort(work, n, key, ascending, stats);
    else if (strcmp(algo, "insertion") == 0) insertionSort(work, n, key, ascending, stats);
    else if (strcmp(algo, "merge") == 0)     mergeSort(work, n, key, ascending, stats);
    else if (strcmp(algo, "radix") == 0)     radixSortByHealthScore(work, n, ascending, stats);
    else if (strcmp(algo, "heap") == 0)      heapSort(work, n, key, ascending, stats);
    else                                     quickSort(work, n, key, ascending, stats);
}

/* ---------------------------------------------------------------
 * Command dispatcher (shared by --server and one-shot modes)
 * Prints exactly one JSON line per command.
 * --------------------------------------------------------------- */
static void handleCommand(AppState *st, char *line) {
    char *f[8] = {0};
    int nf = splitFields(line, f, 8);
    const char *cmd = f[0];
    int i;
    (void)nf;
    for (i = 1; i < 8; i++) if (!f[i]) f[i] = (char *)"";

    if (strcmp(cmd, "zones") == 0) {
        jsonZoneArray(st->zones, st->zoneCount);
        putchar('\n');

    } else if (strcmp(cmd, "health") == 0) {
        float sum = 0; int low = 0, med = 0, high = 0;
        for (i = 0; i < st->zoneCount; i++) {
            sum += st->zones[i].healthScore;
            if (st->zones[i].riskLevel == RISK_LOW) low++;
            else if (st->zones[i].riskLevel == RISK_MEDIUM) med++;
            else high++;
        }
        printf("{\"totalZones\":%d,\"averageHealth\":%.2f,\"lowRisk\":%d,\"mediumRisk\":%d,\"highRisk\":%d,\"dataSource\":",
               st->zoneCount, st->zoneCount ? sum / st->zoneCount : 0.0f, low, med, high);
        jsonStr(st->dataSource);
        printf("}\n");

    } else if (strcmp(cmd, "search") == 0) {
        SearchResult r;
        char id[ID_LEN];
        snprintf(id, sizeof id, "%s", f[2]);
        upperInPlace(id);
        r = (strcmp(f[1], "binary") == 0)
            ? binarySearch(st->zones, st->zoneCount, id)
            : linearSearch(st->zones, st->zoneCount, id);
        printf("{\"method\":"); jsonStr(strcmp(f[1], "binary") == 0 ? "binary" : "linear");
        printf(",\"found\":%s,\"index\":%d,\"comparisons\":%d,\"trace\":", r.found ? "true" : "false", r.index, r.comparisons);
        jsonStr(r.trace);
        printf(",\"zone\":");
        if (r.found) jsonZone(&st->zones[r.index]); else printf("null");
        printf("}\n");

    } else if (strcmp(cmd, "sort") == 0) {
        Zone work[MAX_ZONES];
        SortStats stats;
        int ascending = (strcmp(f[3], "desc") != 0);
        memcpy(work, st->zones, sizeof(Zone) * (size_t)st->zoneCount);

        printf("{\"before\":"); jsonZoneArray(work, st->zoneCount);
        runSort(f[1], parseSortKey(f[2]), ascending, work, st->zoneCount, &stats);
        printf(",\"algorithm\":"); jsonStr(stats.algorithmName);
        printf(",\"timeComplexity\":"); jsonStr(stats.timeComplexity);
        printf(",\"comparisons\":%lld,\"moves\":%lld,\"executionTimeMs\":%.6f,\"recordCount\":%d,\"after\":",
               stats.comparisons, stats.moves, stats.executionTimeMs, stats.recordCount);
        jsonZoneArray(work, st->zoneCount);
        printf("}\n");

    } else if (strcmp(cmd, "hist_list") == 0) {
        ObservationNode *cur = st->history.head;
        printf("{\"count\":%d,\"records\":[", st->history.count);
        while (cur) { jsonObservation(cur); cur = cur->next; if (cur) putchar(','); }
        printf("]}\n");

    } else if (strcmp(cmd, "hist_add") == 0) {
        /* hist_add|atBeginning(ignored)|zone|date|plastic|water|erosion   (health is COMPUTED here) */
        float p, w, e, h;
        char id[ID_LEN];
        const Zone *base;
        Zone tmp;
        ObservationNode *node, *cur, *last = NULL;
        int zi, zoneUpdated = 0;
        if (!*f[2] || !*f[3] || !parseFloatField(f[4], &p) || !parseFloatField(f[5], &w) ||
            !parseFloatField(f[6], &e)) {
            jsonError(400, "Zone, date and the three readings are required");
            return;
        }
        if (p < 0 || p > 100 || w < 0 || w > 100 || e < 0 || e > 100) {
            jsonError(400, "Readings must be between 0 and 100");
            return;
        }
        snprintf(id, sizeof id, "%s", f[2]); upperInPlace(id);
        base = findZone(st, id);
        if (!base) { jsonError(404, "Unknown zone - check the zone ID"); return; }
        tmp = *base;
        tmp.plasticWaste = p; tmp.waterQuality = w; tmp.erosionLevel = e;
        applyHealthScore(&tmp);
        h = tmp.healthScore;

        /* 1) date-ordered insert into the linked list */
        node = historyInsertSorted(&st->history, id, f[3], p, w, e, h);
        if (!node) { jsonError(500, "Out of memory"); return; }

        /* 2) if this is now the NEWEST observation of the zone, it becomes the zone's current state */
        for (cur = st->history.head; cur; cur = cur->next)
            if (strcmp(cur->zoneId, id) == 0) last = cur;
        if (last == node) {
            Alert a, keep[MAX_ALERTS];
            int kept = 0, k;
            zi = indexOfZoneId(st->zones, st->zoneCount, id);
            st->zones[zi].plasticWaste = p;
            st->zones[zi].waterQuality = w;
            st->zones[zi].erosionLevel = e;
            applyHealthScore(&st->zones[zi]);
            buildEnvironmentalMatrix(st->zones, st->zoneCount, st->matrix);
            zoneUpdated = 1;

            /* 3) refresh this zone's alert: pop everything, push back all alerts of OTHER zones,
                  then raise a new one only if the zone still has a problem */
            while (stackPop(&st->alerts, &a) && kept < MAX_ALERTS) keep[kept++] = a;
            for (k = kept - 1; k >= 0; k--)
                if (strcmp(keep[k].zoneId, id) != 0) stackPush(&st->alerts, keep[k]);
            { int is = dominantIssue(&st->zones[zi]);
              if (is != ISSUE_NONE && st->zones[zi].riskLevel != RISK_LOW)
                  pushAlertFor(st, &st->zones[zi], is, " (new reading)"); }
        }
        printf("{\"ok\":true,\"message\":\"Observation added\",\"count\":%d,\"healthScore\":%.2f,\"grade\":\"%s\",\"zoneUpdated\":%s}\n",
               st->history.count, h, gradeFromScore(h), zoneUpdated ? "true" : "false");

    } else if (strcmp(cmd, "hist_zone") == 0) {
        /* every observation of one zone, oldest first (full list traversal) */
        char id[ID_LEN];
        ObservationNode *cur = st->history.head;
        int first = 1;
        snprintf(id, sizeof id, "%s", f[1]); upperInPlace(id);
        printf("{\"records\":[");
        while (cur) {
            if (strcmp(cur->zoneId, id) == 0) {
                if (!first) putchar(',');
                jsonObservation(cur);
                first = 0;
            }
            cur = cur->next;
        }
        printf("]}\n");

    } else if (strcmp(cmd, "alert_clear") == 0) {
        stackClear(&st->alerts);
        printf("{\"ok\":true,\"message\":\"All alerts cleared\"}\n");

    } else if (strcmp(cmd, "task_clear") == 0) {
        queueClear(&st->tasks);
        printf("{\"ok\":true,\"message\":\"All tasks cleared\"}\n");

    } else if (strcmp(cmd, "plan") == 0) {
        /* plan|N : sort zones worst-first and enqueue cleanup tasks for the worst N */
        int n = atoi(f[1]);
        int added;
        if (n < 1) n = 3;
        if (n > MAX_TASKS) n = MAX_TASKS;
        added = planCleanup(st, n);
        printf("{\"ok\":true,\"added\":%d,\"count\":%d}\n", added, queueSize(&st->tasks));

    } else if (strcmp(cmd, "hist_del") == 0) {
        char id[ID_LEN];
        snprintf(id, sizeof id, "%s", f[1]); upperInPlace(id);
        if (historyDeleteByZoneId(&st->history, id))
            printf("{\"ok\":true,\"message\":\"Observation deleted\",\"count\":%d}\n", st->history.count);
        else jsonError(404, "Observation not found");

    } else if (strcmp(cmd, "hist_upd") == 0) {
        float p, w, e, h;
        char id[ID_LEN];
        if (!parseFloatField(f[2], &p) || !parseFloatField(f[3], &w) ||
            !parseFloatField(f[4], &e) || !parseFloatField(f[5], &h)) {
            jsonError(400, "Four numeric readings are required"); return;
        }
        snprintf(id, sizeof id, "%s", f[1]); upperInPlace(id);
        if (historyUpdate(&st->history, id, p, w, e, h)) printf("{\"ok\":true,\"message\":\"Observation updated\"}\n");
        else jsonError(404, "Observation not found");

    } else if (strcmp(cmd, "hist_search") == 0) {
        char id[ID_LEN];
        int comps = 0;
        ObservationNode *n;
        snprintf(id, sizeof id, "%s", f[1]); upperInPlace(id);
        n = historySearch(&st->history, id, &comps);
        printf("{\"found\":%s,\"comparisons\":%d,\"record\":", n ? "true" : "false", comps);
        if (n) jsonObservation(n); else printf("null");
        printf("}\n");

    } else if (strcmp(cmd, "alert_list") == 0) {
        printf("{\"count\":%d,\"alerts\":[", stackSize(&st->alerts));
        for (i = st->alerts.top; i >= 0; i--) { jsonAlert(&st->alerts.alerts[i]); if (i) putchar(','); }
        printf("]}\n");

    } else if (strcmp(cmd, "alert_push") == 0) {
        Alert a;
        memset(&a, 0, sizeof a);
        if (!*f[1] || !*f[3]) { jsonError(400, "zoneId and message are required"); return; }
        snprintf(a.zoneId, ID_LEN, "%s", f[1]); upperInPlace(a.zoneId);
        snprintf(a.severity, SEVERITY_LEN, "%s", *f[2] ? f[2] : "MEDIUM"); upperInPlace(a.severity);
        snprintf(a.message, MSG_LEN, "%s", f[3]);
        if (!stackPush(&st->alerts, a)) { jsonError(409, "Stack overflow: alert stack is full"); return; }
        printf("{\"ok\":true,\"message\":\"Alert pushed\",\"top\":"); jsonAlert(&a); printf("}\n");

    } else if (strcmp(cmd, "alert_pop") == 0) {
        Alert a;
        if (!stackPop(&st->alerts, &a)) { jsonError(409, "Stack underflow: alert stack is empty"); return; }
        printf("{\"ok\":true,\"message\":\"Alert popped\",\"alert\":"); jsonAlert(&a); printf("}\n");

    } else if (strcmp(cmd, "task_list") == 0) {
        int idx = st->tasks.front;
        printf("{\"count\":%d,\"tasks\":[", queueSize(&st->tasks));
        for (i = 0; i < st->tasks.count; i++) {
            jsonTask(&st->tasks.tasks[idx]);
            idx = (idx + 1) % MAX_TASKS;
            if (i != st->tasks.count - 1) putchar(',');
        }
        printf("]}\n");

    } else if (strcmp(cmd, "task_push") == 0) {
        Task t;
        memset(&t, 0, sizeof t);
        if (!*f[1] || !*f[3]) { jsonError(400, "zoneId and description are required"); return; }
        snprintf(t.zoneId, ID_LEN, "%s", f[1]); upperInPlace(t.zoneId);
        snprintf(t.priority, SEVERITY_LEN, "%s", *f[2] ? f[2] : "MEDIUM"); upperInPlace(t.priority);
        snprintf(t.description, MSG_LEN, "%s", f[3]);
        if (!queueEnqueue(&st->tasks, t)) { jsonError(409, "Queue overflow: task queue is full"); return; }
        printf("{\"ok\":true,\"message\":\"Task enqueued\",\"front\":");
        { Task fr; queuePeek(&st->tasks, &fr); jsonTask(&fr); }
        printf("}\n");

    } else if (strcmp(cmd, "task_pop") == 0) {
        Task t;
        if (!queueDequeue(&st->tasks, &t)) { jsonError(409, "Queue underflow: task queue is empty"); return; }
        printf("{\"ok\":true,\"message\":\"Task dequeued\",\"task\":"); jsonTask(&t); printf("}\n");

    } else if (strcmp(cmd, "graph") == 0) {
        advGraph(st->zones, st->zoneCount);

    } else if (strcmp(cmd, "bfs") == 0) {
        advBfs(st->zones, st->zoneCount, f[1]);

    } else if (strcmp(cmd, "route") == 0) {
        advRoute(st->zones, st->zoneCount, f[1], f[2]);

    } else if (strcmp(cmd, "tour") == 0) {
        advTour(st->zones, st->zoneCount, atoi(f[1]), f[2]);

    } else if (strcmp(cmd, "lookup") == 0) {
        advLookup(st->zones, st->zoneCount, f[1]);

    } else if (strcmp(cmd, "bench") == 0) {
        advBenchmark(atoi(f[1]));

    } else if (strcmp(cmd, "forecast") == 0) {
        advForecast(&st->history, f[1]);

    } else if (strcmp(cmd, "clusters") == 0) {
        advClusters(st->zones, st->zoneCount, atoi(f[1]));

    } else if (strcmp(cmd, "anomaly") == 0) {
        float thr = 1.5f;
        parseFloatField(f[1], &thr);
        advAnomalies(st->zones, st->zoneCount, &st->history, thr);

    } else if (strcmp(cmd, "hist_seed") == 0) {
        int added = advSeedHistory(&st->history, st->zones, st->zoneCount);
        printf("{\"ok\":true,\"added\":%d,\"count\":%d}\n", added, st->history.count);

    } else {
        jsonError(400, "Unknown command");
    }
}

static int runServerMode(AppState *st) {
    char line[1024];
    fprintf(stderr, "[beachdsa] server ready, %d zones, source: %s\n", st->zoneCount, st->dataSource);
    while (fgets(line, sizeof line, stdin)) {
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = '\0';
        if (n == 0) continue;
        if (strcmp(line, "quit") == 0) break;
        handleCommand(st, line);
        fflush(stdout);
    }
    return 0;
}

static int runOneShotMode(AppState *st, int argc, char *argv[]) {
    char line[1024] = "";
    int i;
    for (i = 1; i < argc; i++) {
        if (i > 1) strncat(line, "|", sizeof line - strlen(line) - 1);
        strncat(line, argv[i], sizeof line - strlen(line) - 1);
    }
    handleCommand(st, line);
    return 0;
}

/* ---------------------------------------------------------------
 * INTERACTIVE CONSOLE MENU
 * --------------------------------------------------------------- */
static void readLine(const char *prompt, char *buf, size_t size) {
    size_t n;
    if (prompt) { printf("%s", prompt); fflush(stdout); }
    if (!fgets(buf, (int)size, stdin)) { buf[0] = '\0'; return; }
    n = strlen(buf);
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = '\0';
}

static int readInt(const char *prompt) {
    char buf[64];
    char *endp;
    long v;
    for (;;) {
        readLine(prompt, buf, sizeof buf);
        v = strtol(buf, &endp, 10);
        if (buf[0] != '\0' && *endp == '\0') return (int)v;
        prompt = "Invalid input. Please enter a whole number: ";
    }
}

static float readFloat(const char *prompt) {
    char buf[64];
    float v;
    for (;;) {
        readLine(prompt, buf, sizeof buf);
        if (parseFloatField(buf, &v)) return v;
        prompt = "Invalid input. Please enter a number: ";
    }
}

static void printMenu(void) {
    printf("\n==============================================\n");
    printf("       BEACH HEALTH MONITORING SYSTEM\n");
    printf("==============================================\n");
    printf(" 1. Display All Zones\n");
    printf(" 2. Calculate Health Scores\n");
    printf(" 3. Display Environmental Matrix\n");
    printf(" 4. Linear Search\n");
    printf(" 5. Binary Search\n");
    printf(" 6. Bubble Sort\n");
    printf(" 7. Insertion Sort\n");
    printf(" 8. Merge Sort\n");
    printf(" 9. Quick Sort\n");
    printf("10. Add Observation\n");
    printf("11. Delete Observation\n");
    printf("12. Search Observation\n");
    printf("13. Update Observation\n");
    printf("14. Display Monitoring History\n");
    printf("15. Push Alert\n");
    printf("16. Pop Alert\n");
    printf("17. Peek Alert\n");
    printf("18. Display Alert Stack\n");
    printf("19. Add Cleanup Task\n");
    printf("20. Process Cleanup Task\n");
    printf("21. Peek Cleanup Task\n");
    printf("22. Display Task Queue\n");
    printf("23. Display Complexity\n");
    printf("24. Exit\n");
    printf("==============================================\n");
}

static void printComplexity(void) {
    printf("\nOperation                          Complexity\n");
    printf("---------------------------------------------\n");
    printf("Array Access                       O(1)\n");
    printf("Linear Search                      O(n)\n");
    printf("Binary Search                      O(log n)\n");
    printf("Bubble Sort                        O(n^2)\n");
    printf("Insertion Sort                     O(n^2)\n");
    printf("Merge Sort                         O(n log n)\n");
    printf("Quick Sort                         Avg O(n log n)\n");
    printf("Radix Sort                         O(d*(n+b))\n");
    printf("Linked List Search                 O(n)\n");
    printf("Linked List Insert at Beginning    O(1)\n");
    printf("Stack Push / Pop / Peek            O(1)\n");
    printf("Queue Enqueue / Dequeue / Peek     O(1)\n");
}

static void printSortStats(const SortStats *s) {
    printf("\nAlgorithm       : %s", s->algorithmName);
    printf("\nTime Complexity : %s", s->timeComplexity);
    printf("\nComparisons     : %lld", s->comparisons);
    printf("\nMoves/Swaps     : %lld", s->moves);
    printf("\nExecution Time  : %.6f ms", s->executionTimeMs);
    printf("\nRecords         : %d\n", s->recordCount);
}

static void showSearch(const AppState *st, int binary) {
    char id[ID_LEN];
    SearchResult r;
    readLine("Enter Zone ID (e.g. Z07): ", id, sizeof id);
    upperInPlace(id);
    r = binary ? binarySearch(st->zones, st->zoneCount, id)
               : linearSearch(st->zones, st->zoneCount, id);
    printf("\nTrace: %s\n", r.trace);
    if (r.found) {
        const Zone *z = &st->zones[r.index];
        printf("Zone Found\nZone ID: %s\nZone Name: %s\nHealth Score: %.2f\nRisk: %s\n",
               z->zoneId, z->zoneName, z->healthScore, riskLevelToString(z->riskLevel));
    } else {
        printf("Zone not found.\n");
    }
    printf("Comparisons: %d\n", r.comparisons);
    if (binary) printf("Best Case: O(1)  Average/Worst: O(log n)\n");
}

static void runInteractiveMenu(AppState *st) {
    int running = 1;
    while (running) {
        int choice;
        printMenu();
        choice = readInt("Enter choice: ");

        switch (choice) {
        case 1:
            printf("\n[Note: Data source: %s]\n\n", st->dataSource);
            displayZones(st->zones, st->zoneCount);
            break;

        case 2:
            calculateAllHealthScores(st->zones, st->zoneCount);
            printf("\nHealth scores recalculated for all %d zones.\n", st->zoneCount);
            break;

        case 3:
            buildEnvironmentalMatrix(st->zones, st->zoneCount, st->matrix);
            printf("\n");
            displayEnvironmentalMatrix(st->matrix, st->zoneCount);
            break;

        case 4: showSearch(st, 0); break;
        case 5: showSearch(st, 1); break;

        case 6: case 7: case 8: case 9: {
            /* Sorts a COPY so the master array stays ordered by Zone ID
             * (Binary Search depends on that). */
            Zone work[MAX_ZONES];
            SortStats stats;
            int kc, dc;
            SortKey key;
            printf("Sort by: 1) Health Score  2) Plastic Waste  3) Water Quality  4) Zone ID\n");
            kc = readInt("Choose key (1-4): ");
            key = (kc == 2) ? SORT_PLASTIC_WASTE : (kc == 3) ? SORT_WATER_QUALITY :
                  (kc == 4) ? SORT_ZONE_ID : SORT_HEALTH_SCORE;
            dc = readInt("Order: 1) Ascending  2) Descending: ");
            memcpy(work, st->zones, sizeof(Zone) * (size_t)st->zoneCount);
            if (choice == 6)      bubbleSort(work, st->zoneCount, key, dc != 2, &stats);
            else if (choice == 7) insertionSort(work, st->zoneCount, key, dc != 2, &stats);
            else if (choice == 8) mergeSort(work, st->zoneCount, key, dc != 2, &stats);
            else                  quickSort(work, st->zoneCount, key, dc != 2, &stats);
            printf("\nSorted result:\n");
            displayZones(work, st->zoneCount);
            printSortStats(&stats);
            break;
        }

        case 10: {
            char id[ID_LEN], date[DATE_LEN];
            float p, w, e, h;
            readLine("Zone ID: ", id, sizeof id); upperInPlace(id);
            readLine("Date (dd/mm/yyyy): ", date, sizeof date);
            p = readFloat("Plastic Waste: ");
            w = readFloat("Water Quality: ");
            e = readFloat("Erosion Level: ");
            h = readFloat("Health Score: ");
            printf(historyInsertAtEnd(&st->history, id, date, p, w, e, h)
                   ? "Observation added.\n" : "Could not add observation.\n");
            break;
        }

        case 11: {
            int mode, ok;
            if (st->history.head == NULL) { printf("History is empty.\n"); break; }
            mode = readInt("Delete (0=beginning, 1=end, 2=position): ");
            if (mode == 0)      ok = historyDeleteAtBeginning(&st->history);
            else if (mode == 1) ok = historyDeleteAtEnd(&st->history);
            else                ok = historyDeleteAtPosition(&st->history, readInt("Position (0-indexed): "));
            printf(ok ? "Observation deleted.\n" : "Invalid position.\n");
            break;
        }

        case 12: {
            char id[ID_LEN];
            int comps = 0;
            ObservationNode *n;
            readLine("Zone ID to search: ", id, sizeof id); upperInPlace(id);
            n = historySearch(&st->history, id, &comps);
            if (n) printf("Found -> Zone: %s Date: %s Health: %.1f\n", n->zoneId, n->date, n->healthScore);
            else   printf("Not found in history.\n");
            printf("Comparisons: %d\n", comps);
            break;
        }

        case 13: {
            char id[ID_LEN];
            float p, w, e, h;
            readLine("Zone ID to update: ", id, sizeof id); upperInPlace(id);
            p = readFloat("New Plastic Waste: ");
            w = readFloat("New Water Quality: ");
            e = readFloat("New Erosion Level: ");
            h = readFloat("New Health Score: ");
            printf(historyUpdate(&st->history, id, p, w, e, h)
                   ? "Observation updated.\n" : "Zone ID not found in history.\n");
            break;
        }

        case 14:
            printf("\nMonitoring History (%d records):\n", st->history.count);
            historyDisplay(&st->history);
            break;

        case 15: {
            Alert a;
            memset(&a, 0, sizeof a);
            readLine("Zone ID: ", a.zoneId, sizeof a.zoneId); upperInPlace(a.zoneId);
            readLine("Message: ", a.message, sizeof a.message);
            readLine("Severity (LOW/MEDIUM/HIGH): ", a.severity, sizeof a.severity); upperInPlace(a.severity);
            printf(stackPush(&st->alerts, a) ? "Alert pushed.\n" : "Stack overflow: alert stack is full.\n");
            break;
        }

        case 16: {
            Alert a;
            if (stackPop(&st->alerts, &a)) printf("Popped: %s\n", a.message);
            else printf("Stack underflow: alert stack is empty.\n");
            break;
        }

        case 17: {
            Alert a;
            if (stackPeek(&st->alerts, &a)) printf("Top alert: %s\n", a.message);
            else printf("Alert stack is empty.\n");
            break;
        }

        case 18:
            printf("\n");
            stackDisplay(&st->alerts);
            break;

        case 19: {
            Task t;
            memset(&t, 0, sizeof t);
            readLine("Zone ID: ", t.zoneId, sizeof t.zoneId); upperInPlace(t.zoneId);
            readLine("Description: ", t.description, sizeof t.description);
            readLine("Priority (LOW/MEDIUM/HIGH): ", t.priority, sizeof t.priority); upperInPlace(t.priority);
            printf(queueEnqueue(&st->tasks, t) ? "Task enqueued.\n" : "Queue overflow: task queue is full.\n");
            break;
        }

        case 20: {
            Task t;
            if (queueDequeue(&st->tasks, &t)) printf("Processed: %s\n", t.description);
            else printf("Queue underflow: task queue is empty.\n");
            break;
        }

        case 21: {
            Task t;
            if (queuePeek(&st->tasks, &t)) printf("Next task: %s\n", t.description);
            else printf("Task queue is empty.\n");
            break;
        }

        case 22:
            printf("\n");
            queueDisplay(&st->tasks);
            break;

        case 23:
            printComplexity();
            break;

        case 24:
            printf("Exiting Beach Health Monitoring System. Goodbye!\n");
            running = 0;
            break;

        default:
            printf("Invalid menu choice. Please select 1-24.\n");
        }
    }
}

int main(int argc, char *argv[]) {
    static AppState state;   /* static: large struct, zero-initialised */
    int rc = 0;

    initializeState(&state);

    if (argc > 1 && strcmp(argv[1], "--server") == 0) rc = runServerMode(&state);
    else if (argc > 1)                                rc = runOneShotMode(&state, argc, argv);
    else                                              runInteractiveMenu(&state);

    historyClear(&state.history);
    return rc;
}
