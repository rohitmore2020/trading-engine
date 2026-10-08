# Trading Engine Architecture & Design Notes

## Core Invariants and Rules

1. **Deterministic Execution**:
   - The matching core never consults non-deterministic sources (such as wall-clock time) for ordering decisions.
   - Input arrival sequence uniquely determines execution outcome.

2. **Price Representation**:
   - All prices are represented as `int64_t` ticks (e.g. tick size of 0.01 $\to$ $100.25 is stored as `10025`).
   - Floating-point arithmetic (`float`, `double`) is strictly prohibited in price and accounting logic.

3. **Memory Ownership**:
   - Order books exclusively own resting orders.
   - No `std::shared_ptr` or reference-counting overhead on the matching hot path.

4. **Error Handling Strategy**:
   - **Invalid Client Input**: Yields a `Reject` event; never throws C++ exceptions.
   - **Programming Bug (Invariant Violation)**: Triggers an assertion or explicit diagnostic abort in Debug builds; counted and logged in Release builds.
   - **System Errors (Network / Configuration)**: Handled during initialization via explicit status codes or startup exceptions before entering the matching loop. The core matching path is `noexcept`.
