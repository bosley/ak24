# AK24 Platform Abstraction Layers

```mermaid
graph TB
    subgraph APP["Application Layer"]
        style APP fill:#4A90E2,stroke:#2E5C8A,color:#fff
        A1[User Application Code]
        A2[Modules & Plugins]
    end

    subgraph KERNEL["Kernel API"]
        style KERNEL fill:#50C878,stroke:#2E7D4E,color:#fff
        K1[AK24_ALLOC/FREE]
        K2[AK24_THREAD_CREATE/JOIN]
        K3[Thread Pool]
        K4[Data Structures<br/>list, map, buffer, etc]
    end

    subgraph ABSTRACTIONS["Platform Abstractions"]
        style ABSTRACTIONS fill:#F5A623,stroke:#C17D11,color:#fff
        P1[AK_THREAD]
        P2[AK_MUTEX]
        P3[AK_COND]
        P4[Memory Macros]
    end

    subgraph IMPL_POSIX["POSIX Implementation"]
        style IMPL_POSIX fill:#BD10E0,stroke:#7B0B92,color:#fff
        I1[pthread_t]
        I2[pthread_mutex_t]
        I3[pthread_cond_t]
        I4[malloc/free]
    end

    subgraph IMPL_WIN["Windows Implementation"]
        style IMPL_WIN fill:#BD10E0,stroke:#7B0B92,color:#fff
        W1[HANDLE]
        W2[CRITICAL_SECTION]
        W3[CONDITION_VARIABLE]
        W4[HeapAlloc/Free]
    end

    subgraph GC["Optional GC Layer"]
        style GC fill:#FF6B6B,stroke:#C92A2A,color:#fff
        G1[Boehm GC]
        G2[GC_pthread_create]
        G3[GC_MALLOC]
    end

    APP --> KERNEL
    KERNEL --> ABSTRACTIONS

    ABSTRACTIONS --> IMPL_POSIX
    ABSTRACTIONS --> IMPL_WIN

    IMPL_POSIX -.-> GC
    IMPL_WIN -.-> GC

    classDef appStyle fill:#4A90E2,stroke:#2E5C8A,stroke-width:3px,color:#fff
    classDef kernelStyle fill:#50C878,stroke:#2E7D4E,stroke-width:3px,color:#fff
    classDef abstractStyle fill:#F5A623,stroke:#C17D11,stroke-width:3px,color:#fff
    classDef implStyle fill:#BD10E0,stroke:#7B0B92,stroke-width:3px,color:#fff
    classDef gcStyle fill:#FF6B6B,stroke:#C92A2A,stroke-width:3px,color:#fff
```

```mermaid
graph LR
    subgraph L1["Layer 1: Application"]
        style L1 fill:#4A90E2,stroke:#2E5C8A,color:#fff,stroke-width:4px
        APP[" "]
    end

    subgraph L2["Layer 2: Kernel API"]
        style L2 fill:#50C878,stroke:#2E7D4E,color:#fff,stroke-width:4px
        KERN[" "]
    end

    subgraph L3["Layer 3: Platform Abstraction"]
        style L3 fill:#F5A623,stroke:#C17D11,color:#fff,stroke-width:4px
        ABS[" "]
    end

    subgraph L4["Layer 4: Platform Implementation"]
        style L4 fill:#BD10E0,stroke:#7B0B92,color:#fff,stroke-width:4px
        IMPL[" "]
    end

    subgraph L5["Layer 5: Optional GC"]
        style L5 fill:#FF6B6B,stroke:#C92A2A,color:#fff,stroke-width:4px
        GC[" "]
    end

    L1 --> L2 --> L3 --> L4
    L4 -.-> L5

    style APP fill:#4A90E2,stroke:none
    style KERN fill:#50C878,stroke:none
    style ABS fill:#F5A623,stroke:none
    style IMPL fill:#BD10E0,stroke:none
    style GC fill:#FF6B6B,stroke:none
```

## Legend

- 🔵 **Application Layer** - User code, modules, plugins
- 🟢 **Kernel API** - High-level AK24 functions and data structures
- 🟠 **Platform Abstraction** - AK_* types and macros (cross-platform interface)
- 🟣 **Platform Implementation** - POSIX pthread or Windows threads
- 🔴 **Optional GC Layer** - Boehm garbage collector (compile-time option)
