# Simpler

*A programming language whose only goal is to be simple, above all simple for
an AI to write.*

This document is the whole language. An AI writes good code in a mature
language by recalling millions of examples; it has seen none of this one. So
the bet is the opposite: make the entire language small enough to hold in the
prompt at once. **This spec is the corpus.** Everything Simpler can do is here,
and nothing here is aspirational: every construct below compiles and runs today
in the self-hosted compiler ([`selfhost/simpler.smplr`](selfhost/simpler.smplr)),
which is written in Simpler and compiles itself to a byte-identical fixpoint.

If you are an AI writing Simpler: read this once, top to bottom, and you have
the language. There is no standard library to recall beyond the method tables
in section 8.

---

## 1. The core model

**Everything is a message send.** A field read, a method call, an operator: all
one shape, `receiver.name(arguments)`. A send with no arguments drops the
parentheses, so it reads like a field.

```
xs.length              // no-arg send, reads like a field
xs.at(0)               // send with an argument
a.concat(b)            // a method
```

Four pieces of punctuation carry the whole syntax, with no overlap:

| Symbol | Means | Example |
|--------|-------|---------|
| `=` | bind a value to a name | `x = 3` |
| `:` | ascribe a type | `x : Int` |
| `{ }` | delimit a block | `xs.each { x in ... }` |
| `.` | send a message | `xs.length` |

`=` binds; it is a **statement, never an expression**. So it cannot hide inside
a condition, and `if a == b` can never be a mistyped `=`.

## 2. Program structure

A program is one `.smplr` file: zero or more type definitions and functions,
including exactly one `main`.

```
// a line comment runs to end of line

main(sys) {
  sys.screen.print("hello, world")
}
```

- `main(sys)` is the entry point. Its one parameter, `sys`, is the **world
  capability** (section 9). Only `main` holds it.
- A **function** is `name(params) : RetType !Effects { body }`. The return type
  and effects are optional; the return type defaults to `Int`.
- The **value of a function is its last bare expression** (no `return`
  keyword). `main` returns nothing.

```
double(n : Int) : Int {
  n * 2                  // this expression is the result
}

greet(who : Str) : Str {
  "hello, ".concat(who)
}
```

## 3. Bindings

```
x = 3                    // infer the type from the value
total = a + b
name : Str = read()      // optional type annotation

xs : List[Int] = []      // annotation REQUIRED for an empty collection,
m  : Map[Str] = Map()    // which carries no element/value type to infer
```

A name may be reassigned (`x = x + 1`); a binding is a local in the enclosing
function. Annotate only when inference cannot see the type: an empty `[]` or a
`Map()` whose value type is not the default `Int`.

## 4. Types

Scalars: **`Int`** (64-bit signed), **`Float`** (double), **`Str`**, **`Bool`**.

**`List[T]`** holds any element type. Empty literal needs an annotation.

```
nums : List[Int] = []
nums.push(10)
nums.push(20)
first = nums.at(0)       // 10
n = nums.length          // 2
nums.each { x in sys.screen.print(x.toStr) }
sorted = nums.sort       // a new list, ascending
```

**`Map`** is a `Str`-keyed map. Values are `Int` by default; annotate
`Map[Str]` for string values.

```
counts = Map()                       // Str -> Int
counts.set("a", counts.get("a") + 1) // get returns 0 for an absent key
counts.has("a")                      // Bool
counts.keys                          // List[Str], first-seen order
counts.byValue                       // List[Str], ranked by descending value

dir : Map[Str] = Map()               // Str -> Str
dir.set("alice", "1234")
dir.get("bob")                       // "" for an absent key (never null)
```

**Records** are named product types: real value structs.

```
Point = type { x : Int, y : Int }
Box   = type { label : Str, items : List[Int] }

p = Point(x = 3, y = 4)              // construct
p.x                                  // read a field -> 3
```

Construction is **positional in declaration order**; the `field =` names are
optional sugar and are not checked, so the values must be in order.

**Variants** are named sum types (tagged unions). A case may carry payloads,
including recursively.

```
Sign = type { Pos Neg }                       // payload-less cases
Expr = type { Num(Int) Add(Expr, Expr) Mul(Expr, Expr) }

e = Add(Num(2), Mul(Num(3), Num(4)))          // construct
```

Inspect a variant with **`match`**, which must be **exhaustive** (every case
present) and binds each payload positionally. `match` is itself an expression,
so it can be a function's value, which makes recursive evaluators direct:

```
eval(e : Expr) : Int {
  e.match {
    Num(v)    -> v
    Add(a, b) -> eval(a) + eval(b)
    Mul(a, b) -> eval(a) * eval(b)
  }
}
```

## 5. Literals

```
42        -3            // Int (a leading - negates)
3.14      -0.5      0.0  // Float (a dot with a digit on each side)
"hi\n"                  // Str, escapes: \n \t \r \" \\
true      false         // Bool
[]        [1, 2, 3]     // List ([] needs an annotation)
```

## 6. Operators

Seven operators, each sugar for a message: `+ - * / == < >`. Arithmetic and
ordering take **`Int` or `Float`**, and the two may **not be mixed** (convert
with `.toFloat` / `.toInt`); `==` works on `Int`, `Float`, and `Str`. A leading
`-` negates. There are no `<=`, `>=`, `%`, `&&`, `||` operators; use the methods
below.

```
a + b    a - b    a * b    a / b
a == b   a < b    a > b
-x                          // negation
```

## 7. Control flow

```
if cond { ... }                    // bare if
if cond { ... } else { ... }       // with else

while cond { ... }                 // the one general loop

xs.each { x in ... }               // iterate a list

e.match { Case -> ... }            // branch on a variant (section 4)
```

`if`/`while` conditions are `Bool` (a comparison, `true`/`false`, or a `Bool`
method). Combine conditions with the `Bool` methods `.and` / `.or` / `.not`.

The block of an `.each` shares the enclosing function's scope: it reads and
**reassigns** the locals around it. ("No closures" in section 12 means a block
is not a value you can store or pass, not that it is isolated.) So accumulate by
reassigning an outer local. There is no `fold`, `min`, or `max`; when you need a
running min or max, seed it from the first element by counting:

```
count = 0
min = 0.0                 // a dead seed; overwritten on the first element
nums.each { x in
  count = count + 1
  if count == 1 { min = x } else { if x < min { min = x } }
}
```

## 8. Built-in methods (the whole standard library)

No-argument sends are written without parentheses (`s.length`); the rest take
arguments. Dispatch is by the receiver's static type.

**`Str`**

| Send | Result | Meaning |
|------|--------|---------|
| `s.length` | `Int` | number of bytes |
| `s.at(i)` | `Str` | one-character string at `i` |
| `s.code` | `Int` | byte value of the first character |
| `s.slice(a, b)` | `Str` | substring `[a, b)` |
| `s.concat(t)` | `Str` | `s` followed by `t` |
| `s.split(d)` | `List[Str]` | split on the first character of `d` |
| `s.contains(t)` | `Bool` | is `t` a substring |
| `s.replace(a, b)` | `Str` | every `a` replaced by `b` |
| `s.toInt` | `Int` | parse (0 if not a number) |
| `s.toFloat` | `Float` | parse (0 if not a number) |
| `s == t` | `Bool` | equality |

**`Int`**: `n.toStr` (`Str`), `n.toFloat` (`Float`), `n.ge(m)` / `n.le(m)`
(`Bool`, for `>=` / `<=`), arithmetic and ordering.

**`Float`**: `x.toStr` (`Str`, compact: `5.0` renders as `5`, no trailing
zeros), `x.toInt` (`Int`, truncates), arithmetic and ordering.

**`Bool`**: `b.not`, `b.and(c)`, `b.or(c)` (all `Bool`).

**`List[T]`**: `xs.length` (`Int`), `xs.at(i)` (`T`), `xs.push(x)`,
`xs.sort` (new sorted `List[T]`), `xs.each { x in ... }`.

**`Map`**: `m.set(k, v)`, `m.get(k)` (value type), `m.has(k)` (`Bool`),
`m.keys` (`List[Str]`), `m.byValue` (`List[Str]`, descending by value).

## 9. Effects and capabilities

**Pure by default.** Anything that touches the world is marked in the
signature: `!IO` (input/output), `!Fail` (can fail). A function must declare
every effect it uses, whether directly or by calling an effectful function;
`main` is exempt. This keeps cost visible at every call site.

```
shout(screen : Screen, who : Str) !IO {
  screen.print("hi, ".concat(who))   // print is !IO, so the signature says so
}
```

**Capabilities replace imports.** There are no globals and no `import`. `main`
receives the world as `sys` and passes subsets down. A parameter list is a
permission set: a function can touch only what it was handed. The capability
types are `Screen`, `Files`, `Mail`; they are erased at compile time and cost
nothing at runtime.

`sys` provides:

| Send | Type | Effect | Meaning |
|------|------|--------|---------|
| `sys.screen.print(x)` | — | `!IO` | print `Int` / `Float` / `Str` / `Bool`, with a newline |
| `sys.files.read(path)?` | `Str` | `!IO !Fail` | whole file as a string (`""` if it cannot be read) |
| `sys.files.write(path, text)` | — | `!IO` | write `text` to `path` |
| `sys.args` | `List[Str]` | — | command-line arguments (program name dropped) |
| `sys.stdin` | `Str` | — | all of standard input |

Pass a capability down by handing it over: `shout(sys.screen, "world")`. A
function that was handed only a `Screen` cannot read files: the name is not in
scope.

## 10. Failure

`?` marks a call that can fail and propagates the failure upward; it is
required on `sys.files.read` (which yields `""` for a file it cannot read).
`fail(message)` writes `message` to stderr and exits with a non-zero status, the
way a Unix tool reports an error. `fail` may be called anywhere, not only as a
guard at the top:

```
main(sys) {
  args = sys.args
  if args.length < 1 {
    fail("usage: tool <file>")
  }
  text = sys.files.read(args.at(0))?
  sys.screen.print(text)
}
```

## 11. Complete programs

A read-transform-write filter (numbers a file's lines, file or stdin):

```
main(sys) {
  args = sys.args
  text = ""
  if args.length > 0 {
    text = sys.files.read(args.at(0))?
  } else {
    text = sys.stdin
  }
  n = 0
  text.split("\n").each { line in
    if line.length > 0 {
      n = n + 1
      sys.screen.print(n.toStr.concat("\t").concat(line))
    }
  }
}
```

A word-frequency counter, ranked (the program a `Map` exists for):

```
main(sys) {
  counts = Map()
  sys.stdin.replace("\n", " ").split(" ").each { w in
    if w.length > 0 {
      counts.set(w, counts.get(w) + 1)
    }
  }
  counts.byValue.each { k in
    sys.screen.print(k.concat(": ").concat(counts.get(k).toStr))
  }
}
```

More working tools live in [`selfhost/`](selfhost/): `linenum`, `sumcol`,
`wordfreq`, `sortlines`, `average`.

## 12. Deliberately absent

Knowing what is *not* here is as useful as knowing what is. Do not reach for
these; they do not exist:

- No `import` / modules / globals (capabilities replace them).
- No `return`, `break`, or `continue` (a function's value is its last
  expression; loops are `while` and `.each`).
- No `<=`, `>=`, `%`, `&&`, `||` operators (use `.le`, `.ge`, `.and`, `.or`).
- No classes, inheritance, interfaces, or generics beyond `List[T]` / `Map[T]`.
- No closures or first-class functions (so sorting by a derived key is a
  type-specific method like `Map.byValue`, not a comparator argument).
- No exceptions (failure is `!Fail` and `?`; `fail` aborts).
- No mutation across functions: values are passed by value.
- No tuples, no null. A `Map` returns a typed default (`0` or `""`) for a
  missing key.

## 13. How it runs

The compiler transpiles Simpler to C and hands it to the system C compiler,
which buys a mature optimizer and every platform for free. There is no garbage
collector (no pauses, no idle battery drain) and no borrow checker (nothing
hard to learn). Build with no Rust at all:

```bash
cd selfhost
./build.sh                 # cc simpler.c -> ./simpler
cp wordfreq.smplr input.smplr
./simpler > out.c          # the compiler reads input.smplr, writes C to stdout
cc out.c -o wordfreq
```

The frozen [`bootstrap/`](bootstrap/) Rust compiler is the original reference;
it adds a `fmt` formatter and a `test` runner but the language no longer depends
on it.

---

## Locked decisions

- Everything is a message send; one grammar production.
- `=` binds, `:` types, `{ }` blocks, `.` sends. No overlap. `=` is a statement.
- Operator sugar is exactly `+ - * / == < >`; a leading `-` negates.
- `Int` and `Float` never mix without an explicit conversion.
- Variants need exhaustive `match`; records construct positionally.
- Effects (`!IO`, `!Fail`) and failability (`?`) live in the type, not the syntax.
- Capabilities replace imports; `main` holds the world, passes subsets down.
- Value semantics; no GC, no borrow checker. Compile by transpiling to C.

---

By Geir Isene. Public domain (Unlicense).
