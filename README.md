# BLUEGUARD — Beach Health Monitoring System

**Course:** ML2001 — Fundamentals of Data Structures
**Institute:** Vishwakarma Institute of Technology, Pune
**Scope:** Unit 1 (Arrays, Searching, Sorting), Unit 2 (Linked Lists), Unit 3 (Stacks & Queues)
**Language of the DSA core:** pure **C (C99)** — no C++ anywhere.

BLUEGUARD divides a beach into **12 zones**, computes a **Beach Health Score** for each zone, and
uses core data structures to store zones, search/rank them, keep a history of observations, track
urgent alerts and queue cleanup work. The web dashboard is a front-end for the C program.

## 1. Folder structure

```
BeachHealthMonitoring/
├── c/                 C DSA core (all data structures + algorithms)
│   ├── main.c           console menu + --server mode + one-shot mode
│   ├── zone.c/.h        Zone struct
│   ├── health_score.c/.h
│   ├── array_ops.c/.h   Unit 1: arrays, 2D matrix, CSV dataset loader
│   ├── search.c/.h      Linear + Binary Search
│   ├── sorting.c/.h     Bubble, Insertion, Merge, Quick, Heap, Radix
│   ├── linked_list.c/.h Unit 2: singly linked list (Monitoring History)
│   ├── stack.c/.h       Unit 3: array stack (Alerts, LIFO)
│   ├── queue.c/.h       Unit 3: circular queue (Cleanup tasks, FIFO)
│   ├── ny_dataset.c/.h  loader for the real NY beach dataset
│   ├── advanced.c/.h    graph (BFS/DFS/Dijkstra), hash table, benchmark, regression, k-means, z-scores
│   └── Makefile
├── data/beach_dataset.csv   12-zone sample dataset (fallback)
│   (put ny_beach_water_testing.csv here - the real dataset)
├── tests/ny_excerpt.csv     real-row excerpt used to test the loader
├── backend/server.js        Express server (relays requests to the C program)
├── frontend/                HTML / CSS / JS dashboard (unchanged theme; lab.js = analytics pages)
├── scripts/                 build.bat, run.bat, build.sh, run.sh
├── docs/                    report, viva questions, presentation, algorithms, tests
└── package.json
```

## 2. Requirements

* A C compiler: **gcc** (MinGW-w64 on Windows) — or clang / MSVC
* **Node.js 16+** (only for the web dashboard)

## 3. Quick start

### Windows
```
scripts\run.bat
```
### Linux / macOS
```
./scripts/run.sh
```
Both scripts compile the C program, run `npm install` once, and start the server.
Then open **http://localhost:3000**.

Manual steps:
```
cd c
gcc -std=c99 -Wall -Wextra -O2 -o beachdsa main.c zone.c health_score.c array_ops.c search.c sorting.c linked_list.c stack.c queue.c ny_dataset.c
cd ..
npm install
npm start
```
(If you forget to compile, `npm start` compiles the C program automatically.)

## 4. Console application (no Node.js needed)

```
cd c
make            # or the gcc command above
./beachdsa      # Windows: beachdsa.exe
```
This opens the 24-option menu (display zones, searches, sorts, linked list, stack, queue,
complexity table). Run it from inside the `c` folder so it finds `../data/beach_dataset.csv`.

One-shot JSON commands (used for testing):
```
./beachdsa zones
./beachdsa search binary Z07
./beachdsa sort quick health desc
```

## 5. Dataset (real data from a website)

The program uses **NY State Parks — Beach Water Testing: Beginning 2015** (data.ny.gov, dataset id
`wwwd-za77`; real E. coli / Enterococcus lab results, beach status and closure reasons).

**Get it (one download):**
1. Open https://data.ny.gov/d/wwwd-za77 (or the direct link
   https://data.ny.gov/api/v3/views/wwwd-za77/export.csv?accessType=DOWNLOAD).
2. Save the file as **`data/ny_beach_water_testing.csv`** inside this project (create nothing else).
3. Restart the program. It detects the file and builds **12 zones (Z01–Z12)** from the first 12
   beaches that have at least 30 valid E. coli results.

**How each real beach becomes a zone (all six readings are 0–100):**

| Reading | Source |
|---|---|
| water_quality | REAL: % of E. coli samples at or below 235 CFU/100 mL |
| environmental_anomaly | REAL: % of records whose reason is "Exceedance" |
| waste_coverage | REAL proxy: % of records tied to algae / bloom closures |
| visitor_load | REAL proxy: % of records where the beach was Open / Reopened |
| plastic_waste | SIMULATED (not in the dataset) |
| erosion_level | SIMULATED (not in the dataset) |

Always state in your report which columns are real and which are simulated, and cite the dataset.
The Health Score weights are project-defined, not an official standard.

If `data/ny_beach_water_testing.csv` is missing, the program falls back to `data/beach_dataset.csv`
(the 12-zone sample), and then to the same 12 zones built into the code. The data source in use is
shown in menu option 1 and in `/api/health`.

Optional settings: `BEACH_ZONES=8` (number of zones), `BEACH_MIN_SAMPLES=10` (minimum samples per
beach), `BEACH_DATA=path\to\file.csv` (use any file). `tests/ny_excerpt.csv` is a small excerpt of real
rows from this dataset used to test the loader.

## 6. Features

* Beach Health Score + LOW / MEDIUM / HIGH risk classification
* Linear Search and Binary Search by Zone ID (comparison counts + step trace)
* Bubble, Insertion, Merge, Quick and Radix Sort (comparisons, moves, time)
* Monitoring History as a singly linked list (add / delete / search / update / display)
* Alert Stack (LIFO) — push / pop / peek
* Task Queue (FIFO, circular array) — enqueue / dequeue / peek
* Web dashboard: overview charts, zone cards, readings table, lookup, rankings, log, alerts, tasks

## 6a. Analytics and Algorithm Lab

Two drop-down menus in the top bar hold nine extra pages. Every number is computed by the C engine
(`c/advanced.c`); the browser only draws it.

| Menu | Page | What it does | Technique (C) |
|---|---|---|---|
| Analytics | **Forecast** | Trend line, 3-reading moving average and a 3-step forecast per zone, plus an outlook table for all zones | Least-squares regression, moving average |
| Analytics | **Risk Clusters** | Groups zones by pollution profile (2-4 groups) on a scatter plot, with an elbow chart | K-means (farthest-first start) |
| Analytics | **Anomalies** | Readings far from the other zones, and sudden changes inside one zone's history | Z-score |
| Analytics | **Compare** | Two or three zones on a radar chart and a table with the best value highlighted | Array operations |
| Analytics | **Reports** | One-page summary (worst/best zones, alerts, tasks, risk groups); print or download CSV | Aggregation |
| Algorithm Lab | **Zone Network** | Pollution spread (BFS/DFS), shortest route (Dijkstra) and a cleanup tour between zones | Graph, adjacency matrix |
| Algorithm Lab | **Lookup Lab** | The same zone found by linear, binary and hash lookup, with a view of the hash table | Hash table, separate chaining |
| Algorithm Lab | **Benchmark** | Six sorts timed on 100 to 100,000 random records, log-scale chart | Heap, Merge, Quick, Radix, ... |
| Algorithm Lab | **Sort Visualizer** | Animated bars for six sorts, both directions, adjustable speed | Frame-by-frame playback |

Also new: **Heap sort** (`sorting.c`) is available in the Rankings page and the benchmark.

*Zone graph:* zones lie along the shore (each linked to its neighbour) and every third zone also has a
service road to the zone three places along. Distances are fixed, illustrative values in km, not
surveyed data.

*Forecast needs history.* A zone needs at least 3 observations (5 for the "sudden change" check). Add them in
the Observation Log, or press **Load sample history** on the Forecast page to give every empty zone six weekly
readings (the newest equals the zone's current reading; the older ones are generated sample data).

Extra API routes: `GET /api/graph`, `/api/graph/spread/:id`, `/api/graph/route?from=&to=`,
`/api/graph/tour?count=&depot=`, `/api/lookup/:id?`, `/api/benchmark?max=`, `/api/forecast/:id`,
`/api/clusters?k=`, `/api/anomalies?threshold=`, `POST /api/history/seed`.

## 7. How the web part works

`backend/server.js` starts the C program once as `beachdsa --server`. The browser calls
`/api/...`, the server writes one text command per line to the C program (e.g. `search|binary|Z07`)
and returns the C program's JSON answer. All state (zones, linked list, stack, queue) lives in C.

## 8. What was fixed compared to the previous version

* Entire core rewritten from C++ (classes, `std::string`, `std::vector`, `new/delete`, `<chrono>`)
  to plain C (structs, `char[]`, `malloc/free`, high-resolution timer).
* Binary Search used to break after a sort in the console menu; sorting now works on a copy and the
  master array stays ordered by Zone ID.
* The old backend kept its own JavaScript copy of the linked list / stack / queue and re-launched the
  program for every request; now the C program holds all state.
* Menu numeric prompts used to accept only integers; floats are now accepted and input is validated.
* The server compiles the C program automatically if the executable is missing.

## 9. Troubleshooting

| Problem | Fix |
|---|---|
| "C engine build failed" / "No C compiler" | Install gcc (MinGW-w64 on Windows) and make sure it is on PATH. |
| `Cannot find module 'express'` | Run `npm install`. |
| Port 3000 busy | `PORT=3001 npm start` (Windows cmd: `set PORT=3001 && npm start`). |
| Console app says "built-in sample data" | Run it from inside the `c` folder, or set `BEACH_DATA` to the CSV path. |
| Opening `index.html` by double-click shows no data | Use http://localhost:3000 — the page needs the server. |

## 10. Notes for evaluation

* No `qsort()`, no library containers — every algorithm and structure is hand-written in `c/`.
* Health Score weights are project-defined academic parameters, not an official standard.
* Only Unit 1–3 topics are implemented (no trees, graphs or hashing).
