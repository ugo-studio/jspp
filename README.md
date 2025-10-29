# JSPP (JavaScript++)

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/ugo-studio/jspp)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**JSPP is a modern, experimental transpiler that converts JavaScript code into high-performance, standard C++23.**

The primary goal of this project is to achieve a near-perfect translation of JavaScript's dynamic nature and core features into the statically-typed, compiled world of C++, exploring modern C++ capabilities to bridge the gap between these two powerful languages.

## About The Project

JavaScript is flexible and dynamic; C++ is performant and type-safe. JSPP aims to offer the best of both worlds. By transpiling JS to C++, we can potentially run JavaScript logic in environments where C++ is native, with significant performance gains and opportunities for low-level interoperability.

This project serves as a deep dive into compiler design, language semantics, and the expressive power of modern C++. The current implementation translates an entire JavaScript file into a single C++ `main` function, cleverly using `std::shared_ptr<std::any>` to replicate JavaScript's dynamic typing, garbage-collection-like memory management, and complex features like closures.

## Features

JSPP currently supports a foundational set of JavaScript features:

- **Dynamic Variables:** Declaration (`let`, `const` and `var`), assignment, and type changes at runtime.
- **Primitive Types:** `undefined`, `null`, `boolean`, `number`, and `string`.
- **Functions:**
  - Function declarations and arrow functions.
  - Correct hoisting for both variables and functions.
  - Closures with proper lexical scoping and lifetime management.
- **Operators:** Basic arithmetic (`+`, `-`, `*`) and assignment (`=`).
- **Control Flow:** `void` operator.
- **Built-in APIs:** A `console` object with `log()`, `warn()`, and `error()` methods.

## Reserved Keywords

JSPP reserves certain keywords to avoid conflicts with the generated C++ code and internal mechanisms. The following keywords are reserved and cannot be used as variable names:

- `std`: Reserved to prevent conflicts with the C++ standard library namespace.
- `jspp`: Reserved for internal use by the JSPP transpiler.

Using these keywords as variable names will result in a `SyntaxError`.

## How It Works

The transpilation process is a classic three-stage pipeline:

1.  **Parsing:** The incoming JavaScript code is parsed into an Abstract Syntax Tree (AST) using the powerful TypeScript compiler API. This gives us a structured, traversable representation of the code.

2.  **Analysis:** The `TypeAnalyzer` traverses the AST. While it doesn't perform traditional static type checking, it plays a crucial role in understanding the code's structure. It identifies scopes (global, function, block) and detects when variables are "captured" by closures.

3.  **Code Generation:** The `CodeGenerator` performs a final traversal of the AST. It translates each node into its C++ equivalent.
    - All variables are declared as `std::shared_ptr<JsValue>` (where `JsValue` is an alias for `std::any`). This approach elegantly mimics JavaScript's dynamic types and reference-based memory model.
    - Closures are implemented as C++ lambdas that capture `shared_ptr`s by value, ensuring variable lifetimes are correctly extended beyond their original scope.
    - The entire script is wrapped into a single `main` function, with hoisting logic carefully replicated to ensure correct execution order.

## Getting Started

To get a local copy up and running, follow these simple steps.

### Prerequisites

- **Bun:** This project uses Bun for package management and script execution.
  - [Install Bun](https://bun.sh/docs/installation)
- **C++ Compiler:** A compiler with support for C++23 is required. This project is tested with `g++`.
  - `g++` (MinGW on Windows, or available via build-essentials on Linux)

### Installation

1.  Clone the repo:
    ```sh
    git clone https://github.com/ugo-studio/jspp.git
    ```
2.  Install dependencies:
    ```sh
    bun install
    ```

## Usage

The primary way to use JSPP is to run the test suite. This will transpile all the JavaScript test cases in `test/cases/`, build the resulting C++ files, and run them.

```sh
bun run tests
```

## Roadmap

This project is ambitious, and there is a long and exciting road ahead. Here is a high-level overview of the planned features and the project's current standing.

---

### **Phase 1: Core Language Features** `(Current Position)`

This phase focuses on building a solid foundation that correctly models JavaScript's core runtime behavior.

- [x] Dynamic Variables & Primitives (`let`, `const`, `var`, `undefined`, `null`, `string`, `number`, `boolean`, etc.)
- [x] Function Declarations & Arrow Functions
- [x] Correct Hoisting for Variables and Functions
- [x] Closures & Lexical Scoping (via `std::shared_ptr` capture)
- [x] Basic `console` API
- [x] `if-elseif-else` conditions
- [x] Error Handling: `try`/`catch`/`finally` blocks and `throw`.

---

### **Phase 2: Expanded Language Support**

This phase will broaden the range of supported JavaScript syntax and features.

- [ ] **Objects:** Literals, property access (dot and bracket notation), methods.
- [ ] **Arrays:** Literals, indexing, and core methods (`.push`, `.pop`, `.length`, etc.).
- [ ] **Control Flow:** tenary operators, `for` loops, `while` loops, `switch`.
- [ ] **Operators:** Full suite of arithmetic, logical, and comparison operators.

### **Phase 3: Interoperability & Standard Library**

This phase will focus on building out the standard library and enabling modular code.

- [ ] **JS Standard Library:** Implementation of common built-in objects and functions (`Math`, `Date`, `String.prototype.*`, `Array.prototype.*`).
- [ ] **Module System:** Support for `import` and `export` to transpile multi-file projects.
- [ ] **Asynchronous Operations:** A C++ implementation of the JavaScript event loop, `Promise`, and `async/await`.

### **Phase 4: Optimization & Advanced Features**

With a feature-complete transpiler, the focus will shift to performance and advanced capabilities.

- [ ] **Type Inference:** Analyze code to replace `std::any` with concrete C++ types (`int`, `double`, `std::string`) where possible, unlocking massive performance gains.
- [ ] **Performance Benchmarking:** Create a suite to compare transpiled C++ performance against V8 and other JS engines.
- [ ] **C++ Interoperability:** Define a clear API for calling C++ functions from the transpiled JavaScript and vice-versa.

## Contributing

Contributions are what make the open-source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

1.  Fork the Project
2.  Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3.  Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4.  Push to the Branch (`git push origin feature/AmazingFeature`)
5.  Open a Pull Request

## License

Distributed under the MIT License. See `LICENSE` for more information.
