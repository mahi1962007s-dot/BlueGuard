# PRESENTATION.md — Beach Health Monitoring System
### 15-slide deck outline with speaker notes (ML2001 — Units 1–3)

---

### Slide 1 — Title
**BLUEGUARD: Beach Health Monitoring System**
ML2001 — Fundamentals of Data Structures | VIT Pune | 2025–26

*Speaker notes:* Introduce the project name, course, and that this version covers Units 1–3 only (Arrays, Linked Lists, Stacks & Queues).

---

### Slide 2 — Problem Statement
Beaches are affected by pollution, plastic waste, water-quality changes and coastal erosion, but monitoring these changes manually is difficult and often fails to catch deteriorating zones early.

*Speaker notes:* Emphasise "manual monitoring fails to identify deteriorating areas early" — that's the gap this project addresses.

---

### Slide 3 — Motivation
Environmental damage compounds quickly once it starts; early, structured detection lets cleanup crews act before a zone becomes critical.

*Speaker notes:* Connect to real coastal management challenges without overclaiming — this is a simulated academic model, not a live environmental system.

---

### Slide 4 — Objectives
1. Monitor beach conditions using environmental data
2. Divide the beach into zones and score each one
3. Search, sort and rank zones
4. Maintain history, alerts and cleanup tasks
5. Demonstrate real DSA usage end-to-end

*Speaker notes:* Map each objective directly to a data structure covered later in the deck.

---

### Slide 5 — Proposed Solution
A 12-zone beach model with a computed Health Score per zone, backed entirely by Unit 1–3 data structures, exposed through a web dashboard and a C console app.

*Speaker notes:* Stress "no Trees/Graphs/Hashing" — everything is built from arrays, linked lists, stacks and queues.

---

### Slide 6 — System Architecture
Frontend (HTML/CSS/JS) → Backend (Node.js/Express) → C99 DSA Core (compiled executable)

*Speaker notes:* Explain the backend is a thin bridge — it calls the compiled C binary for Search/Sort rather than re-implementing algorithms in JavaScript.

---

### Slide 7 — Array (Unit 1)
`Zone zones[12]` stores every zone; `float environmentalMatrix[12][6]` stores raw readings.

*Speaker notes:* Show the Environmental Data page — point out it is a literal 2D array, columns 0–5 mapped to specific readings.

---

### Slide 8 — Searching (Unit 1)
Linear Search — O(n) — scans zone by zone.
Binary Search — O(log n) — needs sorted Zone IDs.

*Speaker notes:* Demonstrate the Search page live: same query, two methods, compare comparison counts.

---

### Slide 9 — Sorting (Unit 1)
Bubble, Insertion, Merge, Quick, Radix — all hand-written, all reporting comparisons/moves/time.

*Speaker notes:* Run the Sorting Lab live; highlight that `qsort()` is never used.

---

### Slide 10 — Linked List (Unit 2)
Singly linked `ObservationNode` chain stores Monitoring History; grows/shrinks dynamically via `malloc`/`free`.

*Speaker notes:* Add an observation live and show the chain visual update in the dashboard.

---

### Slide 11 — Stack (Unit 3)
Array-based `AlertStack`, LIFO. Most recent, most urgent alert is always on TOP.

*Speaker notes:* Push an alert, show it appears at TOP; explain overflow/underflow guards.

---

### Slide 12 — Queue (Unit 3)
Circular array-based `TaskQueue`, FIFO. Cleanup tasks are processed in the order raised.

*Speaker notes:* Enqueue a task, show it joins at REAR and leaves from FRONT; explain the circular wraparound.

---

### Slide 13 — Dashboard
Stat cards, zone cards, 4 charts, a Rankings page, and an observation log, alert feed and task schedule.

*Speaker notes:* Briefly tour the UI; mention the coastal command-center visual theme.

---

### Slide 14 — Results
12/12 zones scored and classified; all search, sort, linked-list, stack and queue test cases in `docs/TEST_CASES.md` pass.

*Speaker notes:* Cite a couple of concrete numbers, e.g. "3 zones LOW risk, 6 MEDIUM, 3 HIGH" from the live sample data.

---

### Slide 15 — Future Scope & Conclusion
After Unit 4: Trees/BST. After Unit 5: Graphs/BFS/DFS/MST for cleanup-route planning. After Unit 6: Hashing for O(1) lookup. Conclusion: this version proves Units 1–3 can model a realistic environmental monitoring problem end-to-end.

*Speaker notes:* Reiterate that no Unit 4–6 concepts are implemented yet — only documented as future work — and thank the panel.
