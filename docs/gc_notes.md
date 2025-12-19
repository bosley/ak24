GC THREADING NOTES

PROBLEM
Boehm GC was segfaulting on macOS when using pthreads. Previous workaround was to allocate all memory in main thread only.

ROOT CAUSE
Threads created with pthread_create() are not registered with Boehm GC. When GC tries to suspend threads during collection cycles to scan their stacks, it doesn't know about unregistered threads. This causes segfaults on macOS/Darwin.

SOLUTION
Use GC_pthread_create() instead of pthread_create() to properly register threads with the collector.

IMPLEMENTATION
Added AK24_THREAD_* macros in kernel.h that redirect to GC-aware functions when GC is enabled:

- AK24_THREAD_CREATE -> GC_pthread_create (with GC) or pthread_create (without GC)
- AK24_THREAD_JOIN -> pthread_join (same for both)
- AK24_THREAD_DETACH -> pthread_detach (same for both)

Also define GC_THREADS=1 before including gc.h to enable pthread support and pull in gc_pthread_redirects.h

USAGE
Always use AK24_THREAD_CREATE when creating threads that will interact with GC-allocated memory.

RESULT
Threads can now safely allocate GC memory. No more segfaults on macOS. Tests pass consistently.
