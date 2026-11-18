# PROJECT REPORT
## BLUEGUARD — Beach Health Monitoring System

---

### 1. Title
**BLUEGUARD: A Data-Structures-Driven Beach Health Monitoring System**

### 2. Abstract
BLUEGUARD is a course project for ML2001 — Fundamentals of Data Structures at Vishwakarma
Institute of Technology, Pune (2025–26). It simulates an environmental monitoring dashboard
that divides a beach into 12 zones, computes a Beach Health Score for each zone from six
environmental readings, and exposes the entire system — search, sort, historical monitoring,
alerts and cleanup tasks — through data structures covered in Units 1–3: Arrays, Linked Lists,
Stacks and Queues. The project consists of a C99 DSA core, a Node.js/Express backend that
bridges to that core, and an HTML/CSS/JS dashboard.

### 3. Introduction
Coastal environments are under continuous pressure from plastic pollution, declining water
quality, erosion and heavy visitor footfall. Environmental agencies and volunteer groups often
track these issues with manual spreadsheets or paper logs, which do not scale and cannot easily
flag a zone that is quietly deteriorating. This project builds a small but complete monitoring
system that automates zone scoring, ranking and alerting, while staying strictly within the
Data Structures and Algorithms the course has covered so far.

### 4. Problem Statement
Beaches are affected by increasing pollution, plastic waste, water-quality changes, coastal
erosion and other environmental problems; but monitoring these changes manually is difficult
and often fails to identify deteriorating areas early.

### 5. Motivation
Early detection of a declining zone allows cleanup crews to intervene before conditions become
severe. A structured, queryable model of "zone health" — rather than scattered manual notes —
makes trends visible and actionable, and doubles as a concrete, realistic case study for
practicing core DSA concepts.

### 6. Objectives
1. Represent beach zones and their environmental readings using arrays.
2. Compute an academically-defined Beach Health Score per zone.
3. Search zones by ID using both Linear and Binary Search.
4. Rank zones using five different sorting algorithms.
5. Maintain a growable historical log using a singly linked list.
6. Track the most urgent, most recent alerts using a stack (LIFO).
7. Schedule cleanup/inspection work fairly using a queue (FIFO).
8. Present all of the above through an interactive, visual dashboard.

### 7. Scope
This version of the project implements **only** Unit 1 (Arrays), Unit 2 (Linked Lists) and
Unit 3 (Stacks & Queues). Trees, Graphs, Hashing, Heaps and any other Unit 4–6 topic are
explicitly **out of scope** for this version and appear only under "Future Scope" (Section 25).

### 8. Existing System
Manual monitoring today typically relies on paper logs or spreadsheets updated by field staff.
This does not support fast lookup, ranking by severity, or a structured alert history, and it
is easy for a slowly-declining zone to go unnoticed between visits.

### 9. Proposed System
BLUEGUARD replaces the manual log with a small application: a fixed array of 12 zones with a
computed Health Score, real Linear/Binary Search, five real sorting algorithms for ranking,
a linked-list-based history log, a stack-based alert feed, and a queue-based task list — all
inspectable and demonstrable in real time through a web dashboard and a console app.

### 10. System Requirements

| Requirement | Details |
|---|---|
| C Compiler | Any C99-compliant compiler (gcc 7+, clang, or MSVC) |
| Node.js | v16 or later |
| Browser | Any modern browser (Chrome, Edge, Firefox) |
| OS | Windows, macOS or Linux |

### 11. System Architecture
```
 Browser (frontend/) --HTTP--> Express server (backend/server.js)
                                     |
                                     | spawns
                                     v
                          c/beachdsa (compiled C99 executable)
```
The frontend never talks to the C core directly; the Express backend starts the compiled
executable once as a child process (`beachdsa --server`), sends it one text command per line
(e.g. `sort|quick|health|desc`) and forwards its one-line JSON reply to the browser. All state —
the zone array, the Monitoring History linked list, the Alert Stack and the Task Queue — lives
inside that C process, so the web dashboard and the console menu (options 10–22) exercise the
exact same `c/linked_list.c`, `c/stack.c` and `c/queue.c` code.

### 12. Data Structures Used

| Structure | Unit | Used For |
|---|---|---|
| Array (`Zone[12]`) | 1 | Storing all beach zones |
| 2D Array (`float[12][6]`) | 1 | Raw environmental readings matrix |
| Singly Linked List | 2 | Monitoring History |
| Stack (array-based) | 3 | Recent environmental alerts |
| Queue (circular array) | 3 | Cleanup/inspection tasks |

### 13. Algorithms Used
Linear Search, Binary Search, Bubble Sort, Insertion Sort, Merge Sort, Quick Sort, Radix Sort.
See `docs/ALGORITHMS.md` for full explanations, pseudocode-level detail and complexity.

### 14. Array Implementation
Zones are stored in a plain, fixed-size C array `Zone zones[TOTAL_ZONES]` (`TOTAL_ZONES =
12`), populated once from simulated sample data (`loadSampleZones`). A companion 2D array
`float environmentalMatrix[12][6]` is built from the same zones (`buildEnvironmentalMatrix`)
for the "Environmental Data" view. All array access is O(1); scanning/aggregating over the
array (`displayZones`, `calculateAllHealthScores`) is O(n).

### 15. Searching
Linear Search (`linearSearch`) scans the array and counts comparisons; Binary Search
(`binarySearch`) assumes zones are sorted by `zoneId` (true of the sample data) and narrows a
`[low, high]` range each iteration. Both return a `SearchResult` with `found`, `index`,
`comparisons` and a human-readable `trace` string, used to drive the Search page's
step-by-step visualisation.

### 16. Sorting
Five algorithms — Bubble, Insertion, Merge, Quick and Radix Sort — are implemented manually in
`sorting.c` and share a single comparator (`shouldComeBefore`) that supports 4 sort keys
(Health Score, Plastic Waste, Water Quality, Zone ID) and both directions. Every run returns a
`SortStats` struct (algorithm name, time complexity, comparisons, moves, execution time,
record count) consumed by the Sorting Lab UI.

### 17. Linked List
`MonitoringHistory` (in `linked_list.c`) is a singly linked list of `ObservationNode` records
connected with raw pointers (`malloc`/`free`). It supports insertion (at
beginning, end, or a given position), deletion (at beginning, end, or a given position),
search, update, display and clear, and stores an arbitrary, growable number of past readings
per zone.

### 18. Stack
`AlertStack` (in `stack.c`) is an array-based Stack (`Alert alerts[MAX_ALERTS]`, `top` index,
no library stack) that holds the most recent environmental alerts. `push`/`pop`/`peek` run in
O(1) and check for overflow/underflow before touching the array, so the most urgent, most
recently raised alert is always available at TOP.

### 19. Queue
`TaskQueue` (in `queue.c`) is a circular array-based Queue (`Task tasks[MAX_TASKS]`, `front`/
`rear`/`count`, no library queue) that holds cleanup and inspection tasks. `enqueue`/`dequeue`/
`peek` run in O(1); indices wrap using modulo arithmetic so the fixed array is reused instead
of shifting elements on every dequeue.

### 20. Beach Health Score
```
Health Score = 100 - ( 0.22*plasticWaste + 0.24*(100 - waterQuality)
                      + 0.17*erosionLevel + 0.18*wasteCoverage
                      + 0.08*visitorLoad + 0.11*environmentalAnomaly )
```
Clamped to `[0, 100]`. Risk bands: 65–100 LOW, 45–64.99 MEDIUM, 0–44.99 HIGH. These weights are
simulated academic parameters chosen for this course project, not an official environmental
standard.

### 21. Complexity Analysis
See the full table in `docs/ALGORITHMS.md` §12. Summary: Array access O(1); Linear Search
O(n); Binary Search O(log n); Bubble/Insertion Sort O(n²); Merge Sort O(n log n) with O(n)
space; Quick Sort average O(n log n), worst O(n²); Radix Sort O(d·(n+b)); Linked List
search/traverse O(n), insert-at-beginning O(1); Stack and Queue operations all O(1).

### 22. Testing
36 test cases covering searching, sorting, linked-list operations, stack/queue
overflow/underflow, health-score clamping/boundaries, console input validation, and backend
integration were executed; all passed. Full details in `docs/TEST_CASES.md`.

### 23. Results
Running the system on the 12 sample zones (Z01–Z12) produces an average Beach Health Score of
**56.23 / 100**, with **3 zones LOW risk**, **6 zones MEDIUM risk**, and **3 zones HIGH risk**
(Z07 Central West, Z10 South West, Z12 South Gate — the three zones with the highest combined
plastic, erosion and waste-coverage readings). Binary Search locates any zone in at most 4
comparisons versus up to 12 for Linear Search on the same data. Quick Sort and Merge Sort
complete the 12-record dataset with far fewer comparisons than Bubble/Insertion Sort, matching
their theoretical O(n log n) vs O(n²) complexities.

### 24. Limitations
- Data is simulated, not from live sensors or a real API.
- Structures use fixed capacities (`MAX_ALERTS`, `MAX_TASKS` = 20) appropriate for a
  demonstration, not production scale.
- No persistence: history/alerts/tasks reset when the backend restarts (values are simply
  re-seeded from the same sample data).
- No authentication or multi-user support — this is a single-operator academic demo.
- The Health Score formula and weights are project-defined, not an official standard.

### 25. Future Scope
- **After Unit 4:** Introduce Trees/Binary Search Trees for faster hierarchical zone lookup
  and grouping (e.g. by coastline segment).
- **After Unit 5:** Introduce Graphs with BFS/DFS, shortest-path and Minimum Spanning Tree
  algorithms to plan efficient cleanup routes between zones.
- **After Unit 6:** Introduce Hashing/hash tables for O(1) average-case zone lookup, replacing
  Linear/Binary Search for very large zone counts.
- None of the above are implemented in this version; they are documented here only.

### 26. Conclusion
BLUEGUARD demonstrates that a realistic, end-to-end environmental monitoring problem can be
modelled entirely with Unit 1–3 data structures: arrays for structured storage, searching and
sorting for querying and ranking, a linked list for an open-ended history log, and a stack and
queue for two different real-world orderings (urgency-first vs. fairness-first). The project
is fully working, testable, and ready to be extended once Units 4–6 are covered.

### 27. References
1. Course material, ML2001 — Fundamentals of Data Structures, VIT Pune (Units 1–3 syllabus).
2. Cormen, Leiserson, Rivest, Stein — *Introduction to Algorithms* (for algorithm definitions
   and complexity analysis referenced throughout).
3. Standard C99 language reference (cppreference.com) — for `<stdio.h>`, `<stdlib.h>`, `<string.h>`, and general syntax.
4. Node.js and Express.js official documentation — for the backend bridge server.
