
# MyExplorer — High-Performance Disk Analyzer (C++17)

## Note on Tooling

This project was developed with the assistance of AI tools, including a local setup using Cline with Gemma 4 (E4B), primarily for code organization and documentation.

All architectural decisions, performance constraints, and benchmarking were defined and validated manually.

AI was used as a productivity tool to improve clarity and speed of implementation, not as a replacement for design or engineering decisions.


## Overview

MyExplorer is a high-performance disk space analyzer written in C++17,
designed to handle very large filesystems (10M+ files) with a strong focus on:

* memory efficiency
* cache-friendly data structures
* parallel filesystem traversal

---

## Core Goals

* Efficiently scan large directory trees using `std::filesystem`
* Compute recursive directory sizes
* Minimize memory footprint (~100 bytes/node target)
* Maintain responsiveness (non-blocking design)
* Support both CLI (current) and GUI (future)

---

## Design Philosophy

**Data-Oriented Design**

* Minimize heap allocations
* Favor contiguous memory layouts
* Reduce cache misses
* Use offsets instead of pointers where possible
* Separate core engine from presentation layer

---

## Architecture (Planned)

**Core Engine**

* Scanner (filesystem traversal)
* Aggregator (size computation)
* Storage Layer (memory pools, string pool)
* Query Engine (sorting, filtering)

**Interfaces**

* CLI (current)
* GUI (planned)

---

## Performance Targets

* O(N) filesystem traversal
* O(N log N) optimized sorting
* ~100 bytes per node
* Scalable to 10M+ files

---

## Threading Strategy

The current implementation uses a thread pool sized to the number of physical CPU cores.

However, benchmarking revealed that the workload is **primarily I/O-bound**, not CPU-bound.

* Performance scales well up to ~4 threads
* Beyond that, gains diminish due to disk I/O saturation
* Additional threads introduce contention and scheduling overhead

In a production environment, the thread count should be:

* dynamically tuned
* or user-configurable

---

## Benchmarks

### Environment Notes

* Windows filesystem
* SSD storage
* Tests performed with administrator privileges (to avoid permission-related bias)

---

### Benchmark: `C:/Windows`

| Threads | Nodes   | Time (s) | Speedup |
| ------- | ------- | -------- | ------- |
| 1       | 347,827 | 19.50    | 1.0x    |
| 2       | 347,827 | 12.64    | 1.54x   |
| 4       | 347,827 | 7.81     | 2.50x   |
| 12      | 347,827 | 5.73     | 3.40x   |

---

### Benchmark: `C:/`

| Threads | Nodes     | Time (s) | Speedup |
| ------- | --------- | -------- | ------- |
| 1       | 1,143,006 | 60.48    | 1.0x    |
| 2       | 1,143,007 | 38.36    | 1.58x   |
| 4       | 1,143,008 | 22.74    | 2.66x   |
| 12      | 1,143,008 | 14.95    | 4.04x   |

---

## Performance Analysis

Key observations:

* Strong scaling from 1 → 4 threads
* Diminishing returns beyond 4–6 threads
* Maximum speedup (~3–4x) limited by disk throughput
* Workload identified as **I/O-bound**

This confirms that:

* adding threads improves latency hiding
* but does not linearly improve throughput

---

## Current Limitations (Known Issues)

* Thread count is static (not adaptive)
* I/O scheduling is not yet optimized
* No dynamic back-pressure in task queue
* Memory pools are preallocated (not yet fully dynamic)

---

## Status

This project is currently in an **iterative optimization phase**.

The current version validates:

* memory layout strategy
* multithreaded scanning model
* large-scale filesystem traversal

Future iterations will focus on:

* adaptive concurrency
* improved memory management
* real-time streaming of results
* GUI integration

---

## Why This Project Exists

This project is an exploration of:

* systems-level performance in C++
* memory-aware design
* real-world scalability constraints

It is intentionally built to go beyond "working code" and focus on **how code behaves at scale**.
