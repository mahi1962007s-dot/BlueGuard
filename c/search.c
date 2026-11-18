#include "search.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* Appends formatted text to the trace buffer without overflowing it. */
static void appendTrace(char *trace, size_t cap, const char *fmt, ...) {
    size_t len = strlen(trace);
    va_list ap;
    if (len >= cap - 1) return;
    va_start(ap, fmt);
    vsnprintf(trace + len, cap - len, fmt, ap);
    va_end(ap);
}

SearchResult linearSearch(const Zone zones[], int size, const char *targetId) {
    SearchResult r;
    int i;
    r.found = 0; r.index = -1; r.comparisons = 0; r.trace[0] = '\0';

    for (i = 0; i < size; i++) {
        r.comparisons++;
        appendTrace(r.trace, sizeof r.trace, "%s", zones[i].zoneId);
        if (strcmp(zones[i].zoneId, targetId) == 0) {
            r.found = 1;
            r.index = i;
            appendTrace(r.trace, sizeof r.trace, " [MATCH]");
            break;
        }
        if (i != size - 1) appendTrace(r.trace, sizeof r.trace, " -> ");
    }
    return r;
}

SearchResult binarySearch(const Zone zones[], int size, const char *targetId) {
    SearchResult r;
    int low = 0, high = size - 1;
    r.found = 0; r.index = -1; r.comparisons = 0; r.trace[0] = '\0';

    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = strcmp(zones[mid].zoneId, targetId);
        r.comparisons++;

        appendTrace(r.trace, sizeof r.trace, "low=%d mid=%d high=%d (%s) | ",
                    low, mid, high, zones[mid].zoneId);

        if (cmp == 0) {
            r.found = 1;
            r.index = mid;
            appendTrace(r.trace, sizeof r.trace, "MATCH");
            break;
        } else if (cmp < 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    if (!r.found) appendTrace(r.trace, sizeof r.trace, "NOT FOUND");
    return r;
}
