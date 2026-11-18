# ALGORITHMS.md — Beach Health Monitoring System

This document explains every algorithm implemented in `c/`, in plain language, with its
complexity and where it is used in the project. All algorithms are implemented manually in
C99; none use `qsort()` or any library container.

---

## 1. Beach Health Score (`health_score.c`)

```
Health Score = 100 - ( 0.22*plasticWaste
                      + 0.24*(100 - waterQuality)
                      + 0.17*erosionLevel
                      + 0.18*wasteCoverage
                      + 0.08*visitorLoad
                      + 0.11*environmentalAnomaly )
```
The result is clamped to `[0, 100]`. These weights are **simulated academic parameters** for
this course project, not an official environmental standard.

| Score range | Risk |
|---|---|
| 65 – 100 | LOW RISK |
| 45 – 64.99 | MEDIUM RISK |
| 0 – 44.99 | HIGH RISK |

Complexity: O(1) per zone, O(n) to score all n zones.

---

## 2. Linear Search (`search.c`)

Scans the zone array from index 0 upward, comparing `zoneId` at each step, until a match is
found or the array ends.

- Best Case: **O(1)** (match at index 0)
- Average / Worst Case: **O(n)**
- Used for: finding a zone by ID without requiring sorted data (Search page, method = Linear).

## 3. Binary Search (`search.c`)

Repeatedly halves the search range `[low, high]`, comparing the middle element's `zoneId`
against the target and discarding the half that cannot contain it.

**Precondition: the zones array must be sorted by `zoneId` ascending** (true for the sample
data, since Z01…Z12 are already in ID order).

- Best Case: **O(1)**
- Average / Worst Case: **O(log n)**
- Used for: fast zone lookup by ID (Search page, method = Binary).

---

## 4. Bubble Sort (`sorting.c`)

Repeatedly steps through the array, swapping adjacent elements that are out of order. Each full
pass "bubbles" the next-largest (or smallest) element into place. Stops early if a full pass
makes no swaps.

- Time Complexity: **O(n²)** (worst/average), **O(n)** best case with the early-exit optimisation
- Space: O(1) extra

## 5. Insertion Sort (`sorting.c`)

Builds the sorted portion of the array one element at a time: takes the next element and shifts
larger (already-sorted) elements right until the correct slot is found.

- Time Complexity: **O(n²)** (worst/average), **O(n)** best case (already sorted)
- Space: O(1) extra

## 6. Merge Sort (`sorting.c`)

Classic divide-and-conquer: splits the array in half recursively, sorts each half, then merges
the two sorted halves back together in linear time.

- Time Complexity: **O(n log n)** in all cases
- Space: O(n) extra (temporary buffers during merge)

## 7. Quick Sort (`sorting.c`)

Divide-and-conquer with in-place partitioning: picks a pivot (last element), moves smaller
elements left and larger elements right of it, then recursively sorts each side.

- Time Complexity: **Average O(n log n)**, **Worst O(n²)** (rare, pathological pivot choices)
- Space: O(log n) recursion stack

## 8. Radix Sort (`sorting.c`)

A non-comparison sort. Because the Health Score is always in `[0, 100]`, it has at most 3
decimal digits, so Radix Sort processes the ones, tens and hundreds digit in three LSD
(least-significant-digit-first) passes using counting sort as a stable sub-routine.

- Time Complexity: **O(d·(n + b))** where d = number of digits (3 here), b = base (10)
- Space: O(n + b) extra
- Used for: sorting strictly by Health Score in the Sorting Lab (radix option).

All five sorting algorithms in `sorting.c` share the same `shouldComeBefore()` comparator,
which understands 4 sort keys (Health Score, Plastic Waste, Water Quality, Zone ID) and both
directions (ascending/descending), so the Sorting Lab UI can mix and match freely.

---

## 9. Singly Linked List (`linked_list.c`)

`ObservationNode` holds one historical reading plus a `next` pointer. `MonitoringHistory`
manages a chain of these nodes with `head` as the entry point.

| Operation | Complexity |
|---|---|
| Insert at beginning | O(1) |
| Insert at end / position | O(n) (must walk to the position) |
| Delete at beginning | O(1) |
| Delete at end / position | O(n) |
| Search by zoneId | O(n) |
| Update by zoneId | O(n) |
| Display / traverse | O(n) |

Used for: the Monitoring History page, storing an arbitrary number of past observations without
a fixed array size limit.

---

## 10. Stack — array implementation (`stack.c`)

`AlertStack` uses a fixed array `alerts[MAX_ALERTS]` and an integer `top` index.
`push()` increments `top` then writes; `pop()` reads then decrements `top`; both check for
overflow (`top == MAX_ALERTS - 1`) / underflow (`top == -1`) first.

| Operation | Complexity |
|---|---|
| push | O(1) |
| pop | O(1) |
| peek | O(1) |
| display (all elements) | O(n) |

Used for: the Alert Stack page — the most recently raised, most urgent alert is always on TOP
(Last-In-First-Out), matching how an operator would want to see the newest alert first.

---

## 11. Queue — circular array implementation (`queue.c`)

`TaskQueue` uses a fixed array `tasks[MAX_TASKS]` with `front`, `rear` and `count`. Both indices
wrap around using modulo arithmetic (`(index + 1) % MAX_TASKS`), so the array is reused as a
circular buffer instead of shifting elements on every dequeue.

| Operation | Complexity |
|---|---|
| enqueue | O(1) |
| dequeue | O(1) |
| peek | O(1) |
| display (all elements) | O(n) |

Used for: the Task Queue page — cleanup and inspection tasks are processed strictly in the
order they were raised (First-In-First-Out), which is the fair way to schedule field work.

---

## 13. Heap Sort (`sorting.c`)

Builds a max-heap (with respect to the requested order, so the element that must come last is the root), then
repeatedly swaps the root with the last unsorted element and sifts the new root down.
Time O(n log n) in every case, space O(1), not stable.

## 14. Zone graph - BFS, DFS, Dijkstra (`advanced.c`)

Zones are the vertices of an undirected weighted graph stored as an adjacency matrix: a shoreline edge between
neighbouring zones plus a service-road edge between every third zone and the zone three places along.

* **BFS** (queue) visits zones hop by hop; used to model pollution spread - each hop keeps 60% of the source's
  load (`100 - health`). O(V + E) with a list, O(V²) with the matrix.
* **DFS** (recursion) follows one branch to its end before backing up. O(V²) with the matrix.
* **Dijkstra** finds the shortest distance (km) between two zones by repeatedly settling the nearest unsettled
  vertex. O(V²). It is also run with roads disabled to show what the roads save.
* **Cleanup tour:** the k weakest zones are visited by a nearest-neighbour rule using Dijkstra distances. This is a
  greedy heuristic, not an optimal travelling-salesman tour.

## 15. Hash table (`advanced.c`)

Separate chaining with 17 buckets and the djb2 hash (`h = h*33 + c`, start 5381) taken modulo 17. Average lookup
O(1), worst case O(n) if every key lands in one bucket. The Lookup Lab shows comparisons for linear (O(n)),
binary (O(log n), needs sorted IDs) and hash lookup for the same key.

## 16. Linear regression and moving average (`advanced.c`)

For a zone's observations (x = 0, 1, 2 ...; y = health score) the least-squares line is
`slope = (n*Σxy - Σx*Σy) / (n*Σx² - (Σx)²)`, `intercept = (Σy - slope*Σx) / n`. R² = 1 - SSres/SStot and
σ = sqrt(SSres / (n-2)). Forecasts extend the line 1-3 readings and are clamped to 0-100. A 3-reading moving
average smooths the series. A change counts as a sudden drop when it is more than 1.5 standard deviations below
the zone's average change. It is a straight-line trend, so it cannot predict a change in direction.

## 17. K-means clustering (`advanced.c`)

Each zone becomes a 6-number vector (plastic, 100 - water quality, erosion, waste coverage, visitors, anomaly).
Start: the weakest zone, then repeatedly the zone farthest from all chosen centres (deterministic). Repeat
"assign to nearest centre, move centre to the mean" until nothing changes. Clusters are ranked by average health
and named (e.g. Healthy / Watch / Critical). The elbow chart shows total within-cluster spread for 1-6 groups.

## 18. Z-score anomaly detection (`advanced.c`)

`z = (value - mean) / standard deviation`. A reading is flagged when |z| is at or above the chosen threshold.
Two checks: each zone's readings against all zones, and each zone's latest change against that zone's usual
changes (needs 5 observations; the deviation has a floor of 1 point so tiny wobble is ignored).
Flags show whether the direction is a concern (e.g. low water quality) or better than typical.

## 19. Benchmark (`advanced.c`)

Sorts random health scores (fixed seed) of 100 ... 100,000 records with each algorithm and reports the engine's
own timing, comparison and move counts, and checks the output is ordered. Bubble and Insertion are skipped above
20,000 records. Radix sorts the whole-number part of the score only, so it is checked on that basis.

## 12. Summary Complexity Table

| Structure / Algorithm | Time Complexity | Space Complexity |
|---|---|---|
| Array access | O(1) | O(1) |
| Linear Search | O(n) | O(1) |
| Binary Search | O(log n) | O(1) |
| Bubble Sort | O(n²) | O(1) |
| Insertion Sort | O(n²) | O(1) |
| Merge Sort | O(n log n) | O(n) |
| Quick Sort | Avg O(n log n), Worst O(n²) | O(log n) |
| Heap Sort | O(n log n) | O(1) |
| Radix Sort | O(d·(n+b)) | O(n+b) |
| BFS / DFS (adjacency matrix) | O(V²) | O(V) |
| Dijkstra (adjacency matrix) | O(V²) | O(V) |
| Hash table lookup | Avg O(1), Worst O(n) | O(n) |
| Linear regression (n observations) | O(n) | O(1) |
| K-means (k clusters, i rounds) | O(i·n·k) | O(n) |
| Z-score scan | O(n) | O(1) |
| Linked List search/traverse | O(n) | O(1) |
| Linked List insert at beginning | O(1) | O(1) |
| Stack push/pop/peek | O(1) | O(1) |
| Queue enqueue/dequeue/peek | O(1) | O(1) |
