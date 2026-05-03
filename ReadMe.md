# Parsinator — A Parser Combinator Library for C++20

A lightweight parser combinator library for expressive and composable parsing in modern C++.

> ⚠️ Note: The library is heavily constexpr-driven, but not strictly 100% compile-time in all cases. Some components (such as input handling via `std::span` and runtime error propagation) inherently operate at runtime. However, the type system and composition logic remain fully compile-time safe.

---

## 🚀 Current Features

- [x] Constexpr-driven design for maximum compile-time evaluation where possible  
- [x] Clean and readable combinator syntax using `>>` (Sequence) and `|` (Choice)  
- [x] Improved `Result` type with error tracking and input index consumption  
- [x] Zero dynamic allocations — no heap usage at runtime  
- [x] Fully header-only and portable (single `.hpp` file)  
- [x] Debug-friendly utilities using `__PRETTY_FUNCTION__` (experimental, improvable)  
- [x] Extensible parser abstraction — supports custom parsers and combinators  

---

## 🔮 Upcoming Features

- [ ] Better error messages with contextual parsing information  
- [ ] Tracing utilities for step-by-step parser debugging  
- [ ] Unit tests for all combinators  
- [ ] Improved documentation and usage examples 👀  

---

## 🧠 Design Note

Parsinator is designed around the idea of **compile-time composability with runtime execution**.

While many parts of the system leverage `constexpr` and type-level computation, full compile-time execution is not always possible due to runtime input dependency.

The main goal is:
> Strong type-level guarantees + zero-cost runtime execution

---
