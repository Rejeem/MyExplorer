MyExplorer — High-Performance Disk Analyzer (C++17)

Overview

MyExplorer is a high-performance disk space analyzer written in C++17,
designed to handle very large filesystems (10M+ files) with a strong
focus on memory efficiency, cache-friendly data structures, and
non-blocking recursive filesystem scanning.

Core Goals

-   Scan large directory trees efficiently using std::filesystem
-   Compute aggregate folder sizes recursively
-   Maintain minimal memory footprint
-   Ensure responsive execution (non-blocking design)
-   Support CLI now and GUI later

Design Philosophy

Data-oriented design: - Minimize heap allocations - Use contiguous
memory layouts - Reduce cache misses - Separate core engine from UI

Key Challenges

1.  Memory scalability (~100 bytes/node target)
2.  Data structure design (vector + offsets, string pool)
3.  Parallel scanning (controlled threading + back-pressure)
4.  Sorting optimization (offset-based + cache-friendly comparisons)

Architecture (Planned)

Core Engine: - Scanner - Aggregator - Storage Layer - Query Engine

Interface: - CLI - GUI (future)

Performance Targets

-   O(N) scanning
-   O(N log N) sorting optimized
-   ~100 bytes/node
-   No UI blocking
-   Scalable to 10M+ files

Status

Early design phase. Architecture defined, implementation not started.
