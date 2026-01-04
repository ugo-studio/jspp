# Performance Analysis Report: JSPP vs. JIT Engines

## Overview
This report analyzes the performance discrepancy between JSPP (transpiled C++) and JIT-optimized JavaScript engines (Bun/JSC, Node.js/V8) using a recursive Fibonacci benchmark (`fib(30)`).

## Benchmark Results
- **Bun:** ~14ms
- **JSPP (--release):** ~73ms

## Root Cause Analysis
While C++ is natively faster than JavaScript, JSPP is currently slower because it **emulates** JavaScript's dynamic semantics at runtime rather than **optimizing** them into native machine logic.

### 1. Dynamic Typing Overhead (Boxing/Unboxing)
In JSPP, every variable is an `AnyValue` (using NaN-boxing). 
- **In Bun/Node:** After a few iterations, the JIT compiler realizes `n` is always an integer. It generates machine code that uses raw CPU registers for arithmetic.
- **In JSPP:** For every `n - 1`, the C++ code executes a overloaded `operator-` which:
    1. Checks the internal bitmask of `lhs` and `rhs` to verify they are numbers.
    2. Extracts the `double` values.
    3. Performs the subtraction.
    4. Re-boxes the result into a 64-bit NaN-boxed `AnyValue`.

### 2. Function Call Overhead
Every recursive call in JSPP is a "Heavy Call":
- **Virtual Dispatch:** The call goes through `AnyValue::call`, which dereferences a heap-allocated `JsFunction` object.
- **Variant Visitation:** The underlying callable is stored in a `std::variant<std::function<...>>`. Calling it requires visiting the variant (a switch-case on the type index).
- **Argument Handling:** Arguments are passed as a `std::span` of `AnyValue`. Constructing this span for every recursion involves creating a temporary C-style array on the stack.

### 3. Lack of Type Inference
The C++ compiler (`g++`) is incredibly powerful, but it can only optimize the code it sees. Because the transpiler generates code where every operation is performed on `AnyValue` objects, `g++` cannot "guess" that these will always be numbers. It must keep the type-checking logic intact to remain spec-compliant.

## Code Comparison

### JavaScript Source
```javascript
function fib(n) {
    if (n < 2) return n;
    return fib(n - 1) + fib(n - 2);
}
```

### Generated C++ (Simplified)
```cpp
// Every '+' and '-' is a function call to a complex operator overload
return ((*fib).call(..., (n - 1)) + (*fib).call(..., (n - 2)));
```

## How to Improve Performance
To achieve or exceed JIT speeds, JSPP needs to move from **Dynamic Emulation** to **Static Optimization**:

1.  **Type Inference:** Implement a static analysis phase in the transpiler to identify variables that are always numbers.
2.  **Specialized Code Generation:** If `n` is inferred as a number, generate `double n` or `int64_t n` instead of `AnyValue n`.
3.  **Direct Function Calls:** If the target of a call is known at compile time, emit a direct C++ function call instead of using the `AnyValue::call` dispatch system.
