# TEST_CASES.md — Beach Health Monitoring System

Manual/console test cases exercising every DSA operation. All test cases below were run
against the sample data set (12 zones, Z01–Z12) using the C console app / bridge mode.

## Searching

| # | Test | Input | Expected | Result |
|---|---|---|---|---|
| 1 | Linear Search — found | Zone ID `Z07` | Found, Central West, comparisons = 7 | PASS — `linearSearch` stops at index 6 (0-based), 7 comparisons |
| 2 | Linear Search — not found | Zone ID `Z99` | Not found, comparisons = 12 | PASS — scans all 12 zones, returns `found=false` |
| 3 | Binary Search — found | Zone ID `Z07` | Found, comparisons ≤ ⌈log2(12)⌉ = 4 | PASS — 3 comparisons (`low/mid/high` trace shown) |
| 4 | Binary Search — not found | Zone ID `Z99` | Not found, `low > high` terminates loop | PASS — returns `found=false` after ≤4 comparisons |

## Sorting

| # | Test | Input | Expected | Result |
|---|---|---|---|---|
| 5 | Bubble Sort | Sort by Health Score, ascending | Zones ordered lowest → highest health score | PASS — verified output strictly non-decreasing |
| 6 | Insertion Sort | Sort by Plastic Waste, descending | Zones ordered highest → lowest plastic | PASS |
| 7 | Merge Sort | Sort by Zone ID, ascending | Z01…Z12 in order | PASS — stable merge preserves order for ties |
| 8 | Quick Sort | Sort by Water Quality, descending | Zones ordered highest → lowest water quality | PASS |
| 9 (bonus) | Radix Sort | Sort by Health Score, ascending | Matches Bubble/Quick Sort result for the same key | PASS — 3-pass LSD radix produces identical ordering |

## Linked List (Monitoring History)

| # | Test | Action | Expected | Result |
|---|---|---|---|---|
| 10 | Insertion | `insertAtEnd("Z05", ...)` on empty list | List has 1 node, head == that node | PASS |
| 11 | Deletion | `historyDeleteAtBeginning()` on a 1-node list | List becomes empty, `head == NULL` | PASS |
| 12 | Search | `search("Z07")` after inserting Z01, Z04, Z07, Z12 | Found at 3rd comparison | PASS |
| 13 | Update | `historyUpdate("Z04", ...)` with new readings | Node's fields replaced, `next` pointer unchanged | PASS |
| 14 | Deletion on empty list | `historyDeleteAtBeginning()` on empty list | Returns `false`, no crash | PASS |
| 15 | Invalid position | `insertAtPosition(99, ...)` on a 4-node list | Returns `false` (out of range) | PASS |

## Stack (Alerts)

| # | Test | Action | Expected | Result |
|---|---|---|---|---|
| 16 | Push | `push()` 3 alerts | `size() == 3`, last pushed is on TOP | PASS |
| 17 | Pop | `pop()` after 3 pushes | Returns the most recently pushed alert (LIFO) | PASS |
| 18 | Underflow | `pop()` on empty stack | Returns `false`, no crash, stack remains empty | PASS |
| 19 | Overflow | `push()` after `MAX_ALERTS` (20) pushes | Returns `false`, 21st alert rejected | PASS |
| 20 | Peek | `peek()` after 2 pushes | Returns top alert without removing it, `size()` unchanged | PASS |

## Queue (Cleanup Tasks)

| # | Test | Action | Expected | Result |
|---|---|---|---|---|
| 21 | Enqueue | `enqueue()` 3 tasks | `size() == 3`, first enqueued is at FRONT | PASS |
| 22 | Dequeue | `dequeue()` after 3 enqueues | Returns the first task enqueued (FIFO) | PASS |
| 23 | Underflow | `dequeue()` on empty queue | Returns `false`, no crash | PASS |
| 24 | Overflow | `enqueue()` after `MAX_TASKS` (20) enqueues | Returns `false`, 21st task rejected | PASS |
| 25 | Circular wraparound | Enqueue 20, dequeue 5, enqueue 5 more | `rear` wraps past index 19 back to 0 correctly | PASS |

## Health Score / Risk Classification

| # | Test | Input | Expected | Result |
|---|---|---|---|---|
| 26 | Clamping (upper) | All factors 0 (perfect zone) | Score = 100, LOW RISK | PASS |
| 27 | Clamping (lower) | Extreme worst-case factors | Score clamped to 0, HIGH RISK | PASS |
| 28 | Boundary — LOW/MEDIUM | Score exactly 65.0 | Classified LOW RISK (>= 65 rule) | PASS |
| 29 | Boundary — MEDIUM/HIGH | Score exactly 45.0 | Classified MEDIUM RISK (>= 45 rule) | PASS |

## Input Validation (console app)

| # | Test | Input | Expected | Result |
|---|---|---|---|---|
| 30 | Invalid menu choice | `99` | "Invalid menu choice. Please select 1-24." | PASS |
| 31 | Non-numeric menu input | `abc` | Re-prompts until a valid number is entered | PASS (`readInt()` reads a line with `fgets`, validates it with `strtol` and retries) |
| 32 | Invalid search Zone ID | `ZZZ` | Reports "Zone not found", program continues | PASS |

## Backend Integration

| # | Test | Action | Expected | Result |
|---|---|---|---|---|
| 33 | Bridge — zones | `beachdsa zones` | Valid JSON array of 12 zones | PASS |
| 34 | Bridge — search | `beachdsa search binary Z07` | Valid JSON with `found`, `comparisons`, `trace` | PASS |
| 35 | Bridge — sort | `beachdsa sort quick health desc` | Valid JSON with `before`/`after` arrays and stats | PASS |
| 36 | Missing executable | Backend started before compiling C | `/api/search` and `/api/sort` return HTTP 503 with a clear message; `/api/zones` still works via the JSON fallback | PASS |
