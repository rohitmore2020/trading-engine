# Low-Latency C++ Trading Engine

A high-performance, deterministic electronic trading engine and exchange simulator implemented in modern C++20.

## Core Architectural Principles

1. **Correct → Measured → Fast**: Correctness and determinism are strictly prioritized before optimization.
2. **Single-Writer Order Book**: The matching core runs single-threaded without locks on the order book. Concurrency is handled around the core via bounded lock-free/low-latency queues.
3. **Deterministic Core**: For any given sequence of inputs, the matching core generates an identical, reproducible sequence of trade and book-update events.
4. **Integer Arithmetic**: Prices and quantities are represented strictly as integer ticks and fixed-point numbers to prevent floating-point inaccuracies.
5. **Clean Library Boundary**: All domain and matching logic resides in `te_core` (static library), allowing executables, integration tests, and benchmarks to link against the exact same code.

## Project Structure

```text
trading-engine/
├── CMakeLists.txt            # Root CMake build configuration
├── CMakePresets.json         # Standard presets: debug, release, asan, tsan
├── cmake/                    # Interface targets (Warnings, Sanitizers)
├── include/te/               # Public headers (namespace te)
├── src/                      # Core library implementation
├── apps/                     # Thin executable wrappers (engine, clients)
├── tests/                    # Unit, invariant, and differential tests
├── benchmarks/               # Micro and macro performance benchmarks
├── configs/                  # Engine configuration files
├── scripts/                  # Profiling, benchmark runner, and demo scripts
└── docs/                     # Design documents and profiling reports
```

## Quick Start

### Prerequisites
- Clang (Apple Clang ≥ 15 or LLVM Clang) or GCC ≥ 12
- CMake ≥ 3.22
- Ninja build system
- Git

### Build and Run Tests

```bash
# Configure debug build
cmake --preset debug

# Build
cmake --build --preset debug

# Run test suite
ctest --preset debug
```

### Build Configurations

- **Debug**: `-O0 -g` with assertions and invariant checking.
- **Release**: `-O3 -DNDEBUG` optimized production build.
- **AddressSanitizer (ASan + UBSan)**: `cmake --preset asan`
- **ThreadSanitizer (TSan)**: `cmake --preset tsan`
