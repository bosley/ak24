# Thread Pool Demo

This example demonstrates the AK24 thread pool functionality.

## Features Demonstrated

- Creating a thread pool with custom configuration
- Enqueuing lambda-based tasks
- Using completion callbacks
- Monitoring pool status
- Safe cleanup and shutdown

## Building

```bash
cd /path/to/ak24
make
```

## Running

```bash
./build/bin/thread-pool-demo
```

## What It Does

1. Creates a thread pool with 2-4 workers
2. Enqueues 10 tasks that simulate work
3. Each task has a completion callback
4. Shows pool status during execution
5. Waits for all tasks to complete
6. Performs clean shutdown

## Expected Output

You'll see tasks starting and completing in parallel, with callbacks firing as tasks finish. The output demonstrates concurrent execution and proper task lifecycle management.
