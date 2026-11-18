# VIVA_QUESTIONS.md — Beach Health Monitoring System

50+ viva questions based on Units 1–3 and this project's actual implementation, with answers.

## General / Foundations

**1. What is a data structure?**
A way of organizing and storing data in memory so it can be accessed and modified efficiently. In this project, zones are stored in an array, history in a linked list, alerts in a stack, and tasks in a queue.

**2. What is an Abstract Data Type (ADT)?**
A logical description of a data structure — the operations it supports (e.g. push/pop for a Stack) — without specifying how it is implemented internally.

**3. What is the difference between a linear and a non-linear data structure?**
In a linear structure, elements are arranged sequentially, and each element (except the first/last) has exactly one predecessor and successor (arrays, linked lists, stacks, queues). Non-linear structures (trees, graphs) allow hierarchical or many-to-many relationships. This project uses only linear structures.

**4. What is the difference between static and dynamic data structures?**
Static structures (arrays) have a fixed size decided at compile/allocation time. Dynamic structures (linked lists) grow and shrink at runtime using `new`/`delete`. `Zone zones[12]` is static; `MonitoringHistory` is dynamic.

**5. What is the difference between an individual variable and a data structure?**
A variable stores a single value; a data structure organizes many related values together with defined operations, e.g. all 12 zones in one `Zone[]` array instead of 12 separate variables.

**6. What is Asymptotic Notation, and why use it?**
A way to describe an algorithm's growth rate (time/space) as input size grows, ignoring constants and machine-specific details, so algorithms can be compared fairly. This project uses Big-O.

**7. What is time complexity?**
A measure of how the number of basic operations an algorithm performs grows with input size n, e.g. Linear Search is O(n).

**8. What is space complexity?**
A measure of how much extra memory an algorithm needs beyond the input, e.g. Merge Sort needs O(n) extra space for its temporary buffers, while Quick Sort needs O(log n) for its recursion stack.

## Arrays

**9. What is an array?**
A fixed-size, contiguous block of memory holding elements of the same type, accessed by index in O(1).

**10. Why did you use an array for the 12 zones?**
The number of zones is fixed and known in advance, index-based access is O(1), and it is the simplest structure the syllabus (Unit 1) allows for this dataset.

**11. What is a multidimensional array, and where is it used here?**
An array of arrays, e.g. `float environmentalMatrix[12][6]` — 12 rows (zones) by 6 columns (plastic, water, erosion, waste, visitors, anomaly).

**12. What is a Sparse Matrix, and does this project use one?**
A matrix where most elements are zero, stored more compactly than a full 2D array. This project's `environmentalMatrix` is dense (few zero entries), so a sparse representation is not used — only a plain 2D array.

**13. What is memory representation of an array?**
Array elements are stored in contiguous memory addresses, so element `i` is at `base_address + i * element_size`, enabling O(1) index access.

**14. What is an ordered list?**
A list whose elements are kept in sorted order, e.g. zones sorted by `zoneId` ascending, which is required for Binary Search to work correctly.

## Searching

**15. What is Linear Search?**
An algorithm that checks each element in sequence until the target is found or the array ends. O(n) worst case.

**16. What is Binary Search?**
A divide-and-conquer search on a **sorted** array: compare the target with the middle element, discard the half that cannot contain it, and repeat. O(log n).

**17. Why must Binary Search use sorted data?**
Because it eliminates half the search space at each step by assuming everything left of the middle is smaller and everything right is larger; on unsorted data that assumption is false and the algorithm can miss the target.

**18. What is the best case of Linear Search? Of Binary Search?**
Linear Search best case is O(1) (target at index 0). Binary Search best case is also O(1) (target is the middle element on the first check).

**19. How many comparisons does Binary Search need for 12 zones in the worst case?**
⌈log2(12)⌉ = 4 comparisons.

**20. What does the "comparisons" counter in your project measure?**
The number of times a zone's `zoneId` is compared against the search target — shown live in the Search page and returned by the C `SearchResult` struct.

## Sorting

**21. What is Bubble Sort, and how does it work?**
Repeatedly compares and swaps adjacent out-of-order elements, "bubbling" the largest/smallest element to its final position each pass. O(n²).

**22. How did you optimise Bubble Sort in your implementation?**
By tracking whether any swap happened in a pass; if not, the array is already sorted and the loop exits early (best case becomes O(n)).

**23. What is Insertion Sort?**
Builds a sorted prefix of the array by taking each new element and shifting larger sorted elements right until it finds its correct position. O(n²) worst case, efficient on nearly-sorted data.

**24. What is Merge Sort, and what is its complexity?**
A divide-and-conquer algorithm that splits the array into halves, recursively sorts each half, and merges the two sorted halves. O(n log n) in all cases, using O(n) extra space.

**25. What is Quick Sort, and what is its complexity?**
A divide-and-conquer algorithm that partitions the array around a pivot so smaller elements are left and larger are right, then recursively sorts each side. Average O(n log n); worst case O(n²) if pivots are consistently poor.

**26. Why might Quick Sort be faster than Merge Sort in practice despite the same average complexity?**
Quick Sort sorts in place (no extra array allocation) and has better cache locality, so its constant factors are usually smaller.

**27. What pivot does your Quick Sort implementation use?**
The last element of the current sub-array (`zones[high]`), a simple and common choice (Lomuto partition scheme).

**28. What is Radix Sort, and why does it fit this project?**
A non-comparison sort that processes numbers digit by digit (least significant first) using counting sort as a stable sub-routine. It fits because Health Scores are bounded to 0–100 (at most 3 digits), so only 3 passes are needed.

**29. Why doesn't the project use `qsort()`?**
The syllabus requires understanding and implementing the algorithms manually; using a built-in library call would hide the actual logic the teacher needs to evaluate.

**30. How does your Sorting Lab track algorithm performance?**
A `SortStats` struct records the algorithm name, time complexity, number of comparisons, number of moves/swaps, execution time (via a high-resolution timer: `clock_gettime` / `QueryPerformanceCounter`), and record count for every run.

## Linked Lists

**31. What is a linked list?**
A dynamic linear data structure made of nodes, where each node stores data plus a pointer to the next node; nodes are not necessarily contiguous in memory.

**32. Why is a linked list "dynamic"?**
Its size is not fixed in advance — nodes are created (`new`) and destroyed (`delete`) at runtime as data is added or removed, unlike a fixed-size array.

**33. What is a node?**
A single element of a linked list containing the actual data fields plus a pointer to the next node (`ObservationNode` here, with `zoneId`, `date`, environmental readings, and `next`).

**34. What is dynamic memory allocation?**
Allocating memory at runtime (via `new` in C) rather than at compile time, so the linked list can grow as needed; matching `delete` calls free that memory.

**35. What is a pointer?**
A variable that stores the memory address of another variable/object — `ObservationNode *next` points to the next node in the chain, or `NULL` if there isn't one.

**36. Why did you choose a Singly Linked List for Monitoring History instead of an array?**
The number of historical observations is not known in advance and grows over time; a linked list avoids a fixed capacity and only allocates memory as needed.

**37. What is the time complexity of inserting at the beginning of a linked list? At the end?**
Beginning: O(1) — just repoint `head`. End: O(n) — must traverse to the last node first (unless a `tail` pointer is also maintained, which this implementation does not use).

**38. How do you delete a node from a linked list without leaking memory?**
Save a pointer to the node being removed, relink the previous node's `next` to skip it, then call `free` on the saved pointer.

**39. What happens if you call `historyDeleteAtBeginning()` on an empty list?**
The function checks `head == NULL` first and returns `0` without touching memory — no crash.

**40. Why is `historyClear()` called before the program exits?**
It walks the list calling `free()` on every node, so no heap memory is leaked (C has no destructors, so cleanup must be explicit).

## Stacks

**41. What is a stack?**
A linear data structure that follows Last-In-First-Out (LIFO): the last element added is the first one removed.

**42. Why is a stack LIFO?**
Because insertion (`push`) and removal (`pop`) both happen at the same end (the `top`), so the most recently added element is always the first accessible one.

**43. Why did you use a Stack for alerts instead of a Queue?**
Because the most recent, most urgent alert should be the one an operator sees and can act on first — LIFO order naturally surfaces the newest alert.

**44. What is stack overflow, and how do you prevent it in your project?**
Attempting to push onto a full stack. `AlertStack::push()` checks `isFull()` (`top == MAX_ALERTS - 1`) before writing and returns `false` if the stack is full.

**45. What is stack underflow, and how do you prevent it?**
Attempting to pop from an empty stack. `AlertStack::pop()` checks `isEmpty()` (`top == -1`) first and returns `false` instead of reading invalid memory.

**46. What are the time complexities of push, pop and peek?**
All O(1) — each only touches the element at `top` and adjusts one integer index.

## Queues

**47. What is a queue?**
A linear data structure that follows First-In-First-Out (FIFO): the first element added is the first one removed.

**48. Why is a queue FIFO?**
Because insertion happens at the `rear` and removal happens at the `front`, so elements are served in the exact order they arrived.

**49. Why did you use a Queue for cleanup tasks instead of a Stack?**
Field tasks should be handled fairly, in the order they were raised, not last-raised-first; FIFO matches real-world task scheduling.

**50. What is the difference between a Stack and a Queue?**
A Stack removes the most recently added element first (LIFO, one open end); a Queue removes the earliest added element first (FIFO, two ends — front for removal, rear for insertion).

**51. What happens when the queue is empty and you call `dequeue()`?**
`isEmpty()` (`count == 0`) is checked first, and the function returns `false` without reading invalid memory — no crash.

**52. Why is your queue implemented as a circular array instead of a simple array?**
A plain array queue would waste space at the front after repeated dequeues; wrapping `front`/`rear` with modulo arithmetic reuses that freed space, so the same fixed array can be filled and drained indefinitely without a real overflow until it's genuinely full.

**53. What does `(rear + 1) % MAX_TASKS` accomplish?**
It advances the `rear` index and wraps it back to 0 once it passes the end of the array, implementing the circular behaviour.

## Project-Specific

**54. Why is the health score formula weighted the way it is?**
Water quality and plastic waste are weighted most heavily (0.24 and 0.22) since they most directly affect beach usability and safety; visitor load is weighted least (0.08) since it is a secondary factor. These weights are simulated academic parameters chosen for this course project.

**55. How does the Node.js backend avoid "fake DSA"?**
It never re-implements any data structure in JavaScript. The backend starts the compiled C executable once in `--server` mode; every search, sort, linked-list, stack and queue request is sent to it as a text command and its real JSON output is returned. If the executable cannot be built, the API reports an error instead of faking the result.

**56. What would you add after completing Unit 4, 5 and 6?**
After Unit 4: Trees/BSTs for faster hierarchical zone lookup. After Unit 5: Graphs, BFS/DFS and MST for cleanup-route optimisation between zones. After Unit 6: Hashing for O(1) average zone lookup. None of these are implemented in the current version.
