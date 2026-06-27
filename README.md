# Simpler

<img src="logo.svg" align="left" width="120" height="120" alt="Simpler logo">

**A programming language whose only goal is to be simple.**

![Self-host](https://img.shields.io/badge/self--hosting-lexer-EE6C1A)
![Compiler](https://img.shields.io/badge/compiler-Rust-f74c00)
![Emits](https://img.shields.io/badge/emits-C-444)
![License](https://img.shields.io/badge/license-Unlicense-green)
![Stay Amazing](https://img.shields.io/badge/Stay-Amazing-important)

Simple to learn, simple to read, simple for an AI to write, simple to install,
simple to upgrade. Compiled, and fast. Every feature must pay for itself in
problems solved, or it does not go in.

<br clear="left"/>

## The idea

**Everything is a message send.** A field read, a method call, an operator: all
one thing, `receiver.name(arguments)`. There is essentially one grammar
production, and that is the point. Four pieces of punctuation carry the syntax,
with zero overlap:

| Symbol | Means |
|--------|-------|
| `=` | bind a value to a name |
| `:` | ascribe a type |
| `{ }` | delimit a block |
| `.` | send a message |

The rest follows from a few committed decisions:

- **Effects live in the type, not the syntax.** Pure by default; anything that
  touches the world is marked (`!IO`, `!Fail`). A pure function cannot call an
  effectful one without saying so, so cost stays visible at every call site.
- **Capabilities replace imports.** There are no globals. `main` receives the
  world and passes subsets down, so a signature becomes a permission list: a
  function can only touch what it was handed.
- **Value semantics, no GC.** No garbage collector (no pauses, no idle battery
  drain) and no borrow checker (nothing hard to learn). Capabilities are erased
  at compile time, so the permission system costs nothing at runtime.
- **Compiled via C.** The compiler transpiles to C and hands it to the system C
  compiler, which buys a mature optimizer and every platform for free.

The full reasoning, including why it is built to be written by an AI, is in
[the spec](SPEC.md) and [the book](Simpler.pdf).

## A taste

```
main(sys) {
  greet(sys.screen, "world")
}

// greet may ONLY use the screen it was handed, and must declare !IO.
greet(screen : Screen, who : Str) !IO {
  screen.print("hello,")
  screen.print(who)
}
```

`greet`'s signature is its whole world: it holds a `Screen` and a `Str`, it
declares the `!IO` effect, and it can reach nothing else. Drop the `!IO` and the
compiler refuses to build it. Reach for a capability you were not handed, and
the name is simply not in scope.

## How it compiles

<p align="center"><img src="arch.svg" width="100%" alt="Simpler compiler pipeline: source to lex, parse, check, emit C, then the system C compiler, then a native binary"></p>

The compiler is one small binary. It lexes your source into tokens, parses
them into a tree of message-sends, checks types, effects, and capabilities,
then emits portable C and hands it to the system C compiler. Effects and
capabilities are compile-time only, so they cost nothing at runtime.

## Try it

The bootstrap compiler is written in Rust and transpiles to C.

```bash
cd bootstrap
cargo build --release
./target/release/simpler run examples/m3.smplr      # build and run
./target/release/simpler emit examples/m3.smplr     # show the generated C
```

Commands: `run`, `build`, `emit`, `fmt` (format in place, canonical and
comment-preserving), and `test` (run the file's `test_*` functions).

## Development

The compiler has its own regression harness: golden output for every example,
expected error messages for known-bad programs, `fmt` idempotence, and the
`test` runner. One command, green or red:

```bash
cd bootstrap
./run-tests.sh
```

## Status

Early bootstrap. The language grows one runnable milestone at a time:

- [x] **M1** the spine: lex, parse, emit C, compile end to end
- [x] **M2** integers, the seven operators, locals, `if`/`else`, `n.times`, typed `print`
- [x] **M3** effects (`!IO`/`!Fail`), capabilities, user functions, all checked
- [x] **M3b** `?` failure propagation, the `Files`/`Mail` capabilities, named arguments
- [x] **M3c** value-returning user functions and cross-function `?`
- [x] **M4a** the canonical `fmt` formatter (comment-preserving, idempotent)
- [x] **M4b** the `test` runner with the `assert` built-in
- [x] **M5a** user-defined record types (`type { fields }`, value-semantic C structs)
- [x] **M5b** variants and exhaustive `match` (tagged unions)
- [x] **M5c.1** built-in value methods (`Str`/`Int`/`Bool` ops, `Bool` literals, `Str` equality)
- [x] **M5c.2** recursive variants via heap-boxing, multi-payload cases (a type can hold itself)
- [x] **M5c.3** `match` as a value (recursive evaluators: `Add(a, b) -> eval(a) + eval(b)`)
- [x] **M5c.4** lists (`[…]`, `push`, `length`, `at`, `each`; elements of any type)
- [x] **M6** `while`, a general loop (the one control-flow shape a scanner needs)
- [ ] **Self-host** rewrite the compiler in Simpler, verified against the bootstrap:
  - [x] **lexer** ([`selfhost/lexer.smplr`](selfhost/lexer.smplr), real Simpler, compiled by the bootstrap)
  - [ ] **parser** a recursive `Expr` tree
  - [ ] **checker** a fold over the tree
  - [ ] **emitter** C, then the fixpoint test

With recursive types, `match` as a value, lists of any type, and `while`, the
language can now express its own compiler. Self-host is a rewrite, not a missing
feature: the lexer above is the first stage, and it already runs.

Every error reports `file:line:` with the offending line, because the whole
point of effects-in-the-type is a tight, local feedback loop.

## Read more

- **[SPEC.md](SPEC.md)** the language in full
- **[PLAN.md](PLAN.md)** how it gets built, milestone by milestone
- **[DESIGN.md](DESIGN.md)** the visual and verbal language
- **[Simpler.pdf](Simpler.pdf)** all of the above as a short book

## License

[Unlicense](LICENSE), public domain. By Geir Isene.
