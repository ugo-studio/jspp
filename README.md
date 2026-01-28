# JSPP (JavaScript++)

[![CI](https://github.com/ugo-studio/jspp/actions/workflows/ci.yml/badge.svg)](https://github.com/ugo-studio/jspp/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**JSPP is a modern, experimental transpiler that converts JavaScript and TypeScript code into high-performance, standard C++23.**

The primary goal of this project is to achieve a near-perfect translation of JavaScript's dynamic nature and core features into the statically-typed, compiled world of C++, exploring modern C++ capabilities to bridge the gap between these two powerful languages.

## About The Project

JavaScript is flexible and dynamic; C++ is performant and type-safe. JSPP aims to offer the best of both worlds. By transpiling JS/TS to C++, we can potentially run JavaScript logic in environments where C++ is native, with significant performance gains and opportunities for low-level interoperability.

This project serves as a deep dive into compiler design, language semantics, and the expressive power of modern C++. The current implementation translates an entire JavaScript/TypeScript file into a single C++ `main` function, cleverly using `std::shared_ptr<std::any>` (and custom wrappers) to replicate JavaScript's dynamic typing, garbage-collection-like memory management, and complex features like closures.

## Features

JSPP currently supports a comprehensive set of JavaScript features:

- **Languages:** JavaScript (`.js`) and TypeScript (`.ts`) support.
- **Dynamic Variables:** Declaration (`let`, `const`, `var`), assignment, and type changes at runtime.
- **Primitive Types:** `undefined`, `null`, `boolean`, `number`, `string`, `symbol`.
- **Functions:**
  - Function declarations, arrow functions, and function expressions.
  - **Generators:** `function*` and `yield` support.
  - **Async/Await:** `async function` and `await` support (built on C++20 coroutines).
  - Closures with proper lexical scoping.
- **Object Oriented:**
  - **Classes:** Class declarations, constructors, methods, getters/setters, and inheritance (`extends`).
  - **Prototypes:** Prototype chain traversal and manipulation.
- **Control Flow:**
  - `if`, `else if`, `else`.
  - Loops: `for`, `for-of`, `for-in`, `while`, `do-while`.
  - `switch` statements.
  - `try`, `catch`, `finally` blocks.
- **Operators:** Full suite of arithmetic, assignment, comparison, logical, bitwise, and unary operators (including `typeof`, `delete`, `void`, `instanceof`, `in`).
- **Standard Library:**
  - **Console:** `log`, `warn`, `error`, `time`, `timeEnd`.
  - **Math:** Comprehensive `Math` object implementation.
  - **Timers:** `setTimeout`, `clearTimeout`, `setInterval`, `clearInterval`.
  - **Promise:** Full `Promise` implementation with chaining.
  - **Error:** Standard `Error` class and stack traces.
  - **Arrays & Objects:** Extensive methods support (`map`, `filter`, `reduce`, `push`, `pop`, `Object.keys`, etc.).

## Reserved Keywords

JSPP reserves certain keywords to avoid conflicts with the generated C++ code and internal mechanisms. The following keywords are reserved and cannot be used as variable names:

- `jspp`: Reserved for internal use by the transpiler.
- `std`: Reserved to prevent conflicts with the C++ standard library.
- `co_yield`, `co_return`, `co_await`: Reserved for C++ coroutine mechanics.

Using these keywords as variable names will result in a `SyntaxError`.

## Installation

To use JSPP as a command-line tool, install it globally via npm:

```sh
npm i @ugo-studio/jspp -g
```

## For Developers

To contribute to JSPP or run its test suite, follow these steps:

### Prerequisites

- **Bun:** This project uses [Bun](https://bun.sh/) for package management, script execution, and testing.
- **C++ Compiler:** A compiler with support for C++23 is required. This project is tested with `g++`.
  - `g++` (MinGW on Windows, or available via build-essentials on Linux)

### Setup

1.  Clone the repo:
    ```sh
    git clone https://github.com/ugo-studio/jspp.git
    ```
2.  Install dependencies:
    ```sh
    bun install
    ```

## Usage

The primary way to use JSPP is via its command-line interface. This will transpile your file to C++, compile it, and execute the resulting binary.

```sh
jspp <path-to-your-file>
```

**Example:**

To run a sample TypeScript file located at `my-code/test.ts`:

```sh
jspp my-code/test.ts
```

The transpiled C++ file and executable will be generated in the same directory as the input file and cleaned up after execution (unless `--keep-cpp` is used).

You can also run the test suite:

```sh
bun test
```

## Roadmap

This project is ambitious, and there is a long and exciting road ahead. Here is a high-level overview of the planned features and the project's current standing.

---

### **Phase 1: Core Language Features**

This phase focuses on building a solid foundation that correctly models JavaScript's core runtime behavior.

- [x] Dynamic Variables & Primitives
- [x] Function Declarations & Arrow Functions
- [x] Correct Hoisting for Variables and Functions
- [x] Closures & Lexical Scoping
- [x] Basic Control Flow (`if`, `loops`)
- [x] Basic `console` API

---

### **Phase 2: Expanded Language Support**

This phase broadens the range of supported JavaScript syntax and features.

- [x] **Error Handling**: `try`/`catch`/`finally` blocks and `throw`.
- [x] **Objects & Classes:** Classes, inheritance, literals, property access.
- [x] **Arrays:** Literals, indexing, and core methods.
- [x] **Operators:** Full suite of arithmetic, logical, and comparison operators.
- [x] **Advanced Control Flow:** `switch`, `for-of`, `for-in`, generators.
- [x] **TypeScript Support:** Compilation of `.ts` files.

### Phase 3: Interoperability & Standard Library

This phase focuses on building out the standard library and enabling modular code.

- [ ] **JS Standard Library:** Implementation of common built-in objects. (Currently supports: `Math`, `Symbol`, `Error`, `String`, `Array`, `Object`, `Timer`). **Pending:** `Date`, `Temporal`, `Map`, `Set`, `JSON`, etc.
- [x] **Asynchronous Operations:** Event loop, `Promise`, `async/await`, and timers (`setTimeout`).
- [ ] **Module System:** Support for `import` and `export` to transpile multi-file projects.

### **Phase 4: Optimization & Advanced Features**

With a feature-complete transpiler, the focus will shift to performance and advanced capabilities.

- [ ] **Performance Benchmarking:** Create a suite to compare transpiled C++ performance against V8.
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
