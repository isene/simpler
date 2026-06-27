# Implementation plan

*How Simpler gets built, in the order it should be built.*

This plan is provisional on Phase 0. The four open questions in the spec
(effect vocabulary, pure-place sharing, the object/type form, and the
bootstrap itself) are still being chewed on. Settling them changes
Phase 0 only; the milestone chain after it holds regardless of how they
land. Revise here, not throughout.

---

## The shape of the work

Four phases, in strict order. Each depends on the one before it.

1. **Design closure**: settle what the compiler must lower.
2. **Bootstrap compiler**: a first compiler in a host language that
   emits C.
3. **Self-host**: rewrite the compiler in Simpler; compile it with
   itself.
4. **Metaprogramming**: types-as-values, macros, build-as-program.
   Last, on purpose.

The guiding rule is the spec's own: every milestone must run. No phase
adds a feature the previous milestone could not already exercise
end-to-end.

---

## Phase 0: Design closure (blocks everything)

The compiler cannot lower what is not yet defined. Three questions, plus
the decision that frames them, must close before Phase 1.

- **Effect vocabulary.** Pin the exact set: `!IO`, `!Fail`, `!Hardware`,
  and the rest. The checker needs a closed list to verify against and a
  rule for how effects on a deep path combine.
- **Pure-place sharing.** Decide whether a pure place is strictly copied
  or may share when the compiler proves no observer differs. This gates
  the whole memory model: strict copy is simplest to build; proven
  sharing is faster but asks more of the compiler. Pick one before
  writing the lowering, because it changes what the lowering emits.
- **Object/type definition form.** Sketch it in full. Until a type can be
  declared, the checker has nothing to resolve sends against.
- **Host language for the bootstrap.** The bootstrap is thrown away once
  self-hosting lands, so this is a low-stakes, near-term choice.
  Recommendation: Rust, because the AST is a tree of message-send nodes,
  which sum types model cleanly, and its error handling suits a compiler. The
  one cost over plain C is a second build dependency, paid only until
  Phase 3 removes it.

Output of Phase 0: a frozen mini-spec for each of the four, enough that
the compiler author never has to guess.

---

## Phase 1: The bootstrap compiler

One pipeline, six stages. Each stage is small because the language is
small.

1. **Lex**: tokens. Three symbols, identifiers, literals, braces. Tiny.
2. **Parse**: one grammar production (the message send) yields an AST
   that is a tree of message-send nodes.
3. **Desugar**: operators to messages, `if`/`each` to block-argument
   sends, places to getter/setter sends. Everything reduces to the core.
4. **Check**: resolve sends to types, propagate effects, enforce
   capabilities. This is the hard stage; the rest is plumbing.
5. **Lower to C**: values to structs with copies (elided when proven
   unshared), arenas to bump allocators, sends to direct calls or field
   reads, effectful places to setter calls.
6. **Hand off**: write C, invoke the system C compiler, done.

Build it in runnable increments. Keep the one-binary shape from the
start: `build | run | fmt | test` exist as subcommands, stubbed until
their milestone.

- **M1: the spine.** Lex, parse, and emit C for a program that prints
  one string through a passed `screen` capability. The pipeline is thin
  but complete, end to end. MVP surface: identifiers, `.` sends, no-arg
  sends, string literals, blocks as arguments, one capability.
- **M2: real programs.** Types, locals, pure value semantics with copy,
  the seven operators, `if`/`else`, `each`. Straight-line code now runs.
- **M3: the cost model.** Effects, capabilities, `?` failure
  propagation, effectful places. The spec's "complete small program"
  compiles and runs at this milestone.
- **M4: the toolchain.** Arenas, object/type definitions, the canonical
  formatter (`fmt`), and `test`. The language is now usable by a person.

---

## Phase 2: Self-host

Rewrite the compiler in Simpler and compile it with the M4 binary.

Cut over with the three-stage bootstrap test:

1. The host compiler builds the Simpler-written compiler → binary **A**.
2. **A** compiles the same Simpler source → binary **B**.
3. **B** compiles it once more → binary **C**.

**B** and **C** must be byte-identical. That fixpoint proves the
self-hosted compiler is correct enough to stand on its own. At that
point the host-language bootstrap is retired and its build dependency
goes with it.

---

## Phase 3: Metaprogramming (last, deliberately)

Types-as-values, macros-as-ordinary-functions, build-script-as-program.
The spec flags this as the easiest piece to over-engineer, so it comes
after a working, self-hosting language exists. Add it minimal, prove a
real use, stop. Nothing here is on the critical path to a usable Simpler.

---

## What success looks like at each gate

| Gate | The language can… |
|------|-------------------|
| M1 | print through a capability; the pipeline is real |
| M3 | compile the spec's example program, effects and all |
| M4 | be written, formatted, and tested by a person |
| Phase 2 | compile itself to a byte-stable fixpoint |
| Phase 3 | extend itself without touching the compiler |

---

By Geir Isene. Public domain (Unlicense).
