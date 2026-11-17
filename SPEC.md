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

A send resolves to a built-in method or to one of your own functions:
**`x.f` means `f(x)`**. So a one-argument function can be written as a send, and
a row of them reads left to right, each word feeding the next:

```
data.parse.normalize.render      // exactly render(normalize(parse(data)))
```

Package that row into one function and it becomes a single send:

```
render_good(s : Str) : Str { render(normalize(parse(s))) }
data.render_good                 // the whole pipeline as one word
```

Functions and methods are the same kind of brick. You build small words and snap
them together into bigger ones. (A real record field always wins over a function
of the same name, so `point.x` reads the field, not a call.)

## 2. Program structure

A program is one `.smplr` file: zero or more type definitions and functions,
including exactly one `main`. A program that touches files says which at the
top (section 9).

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
- An `if` on the last line gives the value of the branch that ran.
- The **value of a function is its last bare expression**. `return expr`
  leaves the function early with that value, from inside an `if` or a loop.
  A bare `return` as the last line of a block leaves `main`, or a function
  with no value.
  `main` returns nothing.

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
function. **A name bound again with another type is a second variable**:
after `v = "12"`, `v = v.toInt` makes a number named `v` that lasts to the end
of its block; after the block `v` is the text again. Annotate only when inference cannot see the type: an empty `[]` or a
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

**`Map`** is a `Str`-keyed map. Values are `Int` by default; `Map[T]` holds
any other type (`Map[Str]`, `Map[Point]`, `Map[List[Int]]`). `Map[Int]` is the
same type as `Map`. A new map is `Map()`, `Map[Str]()` or `Map[Point]()`.
`get` of an absent key gives 0 in a `Map` and "" in a `Map[Str]`; in a map of
lists or own types it stops the program (`the map has no key "k"`), so ask
`has` first. A lookup takes the same time in a map of ten keys and of a million.

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

**Records** are named product types.

```
Point = type { x : Int, y : Int }
Box   = type { label : Str, items : List[Int] }

p = Point(x = 3, y = 4)              // construct
p.x                                  // read a field -> 3
```

A field is set with `p.x = 5` (the value must have the field's type). A record
is shared, never copied: after `q = p`, `q.x = 5` changes `p` too. A function
that sets a field of a record it was handed must say `!Mut`.

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

A match also gives a value in the middle of a function: `x = e.match { ... }`.
The names an arm binds live only in that arm.

## 5. Literals

```
42        -3            // Int (a leading - negates)
3.14   -0.5   0.0   1.5e3   2.0e-9   // Float (a dot with a digit each
                                     // side, plus an optional e exponent)
"hi\n"                  // Str, escapes: \n \t \r \" \' \\; a backslash before anything else stays ("\d+")
true      false         // Bool
[]        [1, 2, 3]     // List ([] needs an annotation)
```

## 6. Operators

Fourteen operators: `+ - * / %`, `== != < > <= >=`, `&& ||`, and `!` in front of a `Bool`. Arithmetic and
ordering take **`Int` or `Float`**, and the two may **not be mixed** (convert
with `.toFloat` / `.toInt`); `%` (remainder) takes `Int` only; `==` works on
`Int`, `Float`, `Bool` and `Str`, and on two lists of those (equal when they have the same items in the same order); on a
record, a map or a list of other things it is refused, so compare their fields or items. A leading `-` negates. `&&` and `||` join two `Bool`s.
From tightest to loosest: `* / %`, then `+ -`, then the comparisons, then `&&`,
then `||`. `a != b` and `!ok` mean the same as `(a == b).not` and `ok.not`.
On two `Str`s, `+` joins them and `< > <= >=` compare them by byte order; a
`Str` plus a number is refused (use `.toStr`). Text may also be in single quotes.

```
a + b    a - b    a * b    a / b
a == b   a < b    a > b
-x                          // negation
```

**Precedence is conventional, not the flat left-to-right of a pure message
language.** A `.` send binds tightest, then unary `-`, then `* /`, then `+ -`,
then `== < >`; all binary operators are left-associative. So `2 + 3 * 4` is
`14`, and `a.b * 2` is `(a.b) * 2`. When in doubt, parenthesize.

## 7. Control flow

```
if cond { ... }                    // bare if
if cond { ... } else { ... }       // with else
if a { ... } else if b { ... } else { ... }

while cond { ... }                 // the one general loop
break                              // leave the nearest loop; continue starts its next round

xs.each { x in ... }               // iterate a list, or the letters of a text
for x in xs { ... }                // the same loop
for i in 0..n { ... }              // i = 0 up to n - 1; 0..=n includes n

e.match { Case -> ... }            // branch on a variant (section 4)
```

`if`/`while` conditions are `Bool` (a comparison, `true`/`false`, or a `Bool`
method). Combine conditions with `&&` and `||`; negate with `.not`.

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
| `s.length` | `Int` | number of letters (`"blå".length` is 3); `at`, `slice`, `find` and the pads count the same way |
| `s.at(i)` | `Str` | one-character string at `i` |
| `s.code` | `Int` | number of the first character (its Unicode number when the text starts with a whole UTF-8 character, else the byte) |
| `s.slice(a, b)`, `s.slice(a)` | `Str` | substring `[a, b)`; with one value, from `a` to the end |
| `s.trim` | `Str` | strip leading/trailing whitespace (space, tab, CR, LF) |
| `s.concat(t)` | `Str` | `s` followed by `t` (also `s + t`) |
| `s.startsWith(t)`, `s.endsWith(t)` | `Bool` | does `s` begin or end with `t` |
| `s.split(d)` | `List[Str]` | the parts of `s` between each `d`; `d` may be many characters |
| `s.contains(t)` | `Bool` | is `t` a substring |
| `s.find(t)` | `Int` | the place where `t` first starts, or -1 |
| `s.reTest(p)` | `Bool` | is there a match of the pattern `p` anywhere in `s` |
| `s.reFind(p)` | `List[Str]` | the first match: the whole match at 0, then each group (`""` for a group that took no part); empty when there is no match |
| `s.reAll(p)` | `List[List[Str]]` | every match, each as `reFind` gives it |
| `s.reReplace(p, to)` | `Str` | every match replaced by `to`; in `to`, `$1` is group 1, `$0` the whole match, `$$` a dollar sign |
| `s.reSplit(p)` | `List[Str]` | the parts of `s` between the matches |
| `s.json(k)` | `Str` | `s` read as JSON: the value under key `k` of an object, or at place `k` (`"0"`, `"1"`) of an array; a text comes back without quotes and escapes, a number, `true`, `false` or `null` as written, an object or array as its JSON; `""` when missing |
| `s.jsonItems` | `List[Str]` | the items of a JSON array, or the values of a JSON object |
| `s.jsonKeys` | `List[Str]` | the keys of a JSON object |
| `s.bytes` | `List[Int]` | the bytes of `s`, each 0 to 255 |
| `b.intAt(i, n)` | `Int` | on `Bytes` or a `List[Int]` of bytes: the whole number in the `n` bytes (1 to 8) from place `i`, lowest byte first; 8 bytes give a number with a sign |
| `b.findByte(n, from)` | `Int` | on `Bytes`: the place of the next byte `n` at or after `from`, or -1 |
| `b.floatAt(i)` | `Float` | the Float in the 8 bytes from place `i` |
| `b.textAt(i, n)` | `Str` | the `n` bytes from place `i` as a text, up to a 0 byte |
| `s.upper`, `s.lower` | `Str` | the letters a to z in capitals or in small letters |
| `s.replace(a, b)` | `Str` | every `a` replaced by `b` |
| `s.padLeft(n)` | `Str` | pad with spaces to width `n` (right-align) |
| `s.padRight(n)` | `Str` | pad with spaces to width `n` (left-align) |
| `s.toInt` | `Int` | parse; a text that is no whole number stops the program |
| `s.isInt` | `Bool` | is it a whole number (spaces around it allowed)? Test before `toInt` on input you do not trust |
| `s.toFloat` | `Float` | parse (0 if not a number) |
| `s == t` | `Bool` | equality |

**`Int`**: `n.toStr` (`Str`), `n.char` (`Str`, the character with that Unicode number as UTF-8; "" for 0 or a number out of range), `n.toFloat` (`Float`), arithmetic and ordering.
(The older method forms `n.mod(m)`, `n.ge(m)`, `n.le(m)`, `b.and(c)` and
`b.or(c)` still compile; write the operators.)

**`Float`**: `x.toStr` (`Str`, up to 15 significant figures with trailing zeros
trimmed, so `5.0` is `5` and a Julian date prints in full), `x.toInt` (`Int`,
truncates), arithmetic and ordering. Math (all
`Float`): `x.sin`, `x.cos`, `x.tan`, `x.asin`, `x.acos`, `x.sqrt`, `x.floor`,
`x.ceil`, `x.round` (half away from zero), `x.abs`, `x.atan`, `x.ln`, `x.exp`, `y.atan2(x)` (the
angle of the point x, y) and `x.pow(y)`. `x % y` on two `Float` gives the remainder with the sign of
`x`, so `-10.0 % 360.0` is `-10`. These wrap the C math library, so a program that uses
them must link it: `cc out.c -o out -lm`.

**`Bool`**: `b.not`, `b.and(c)`, `b.or(c)` (all `Bool`).

**`List[T]`**: `xs.length` (`Int`), `xs.at(i)` (`T`), `xs.push(x)`,
`xs.set(i, x)` (put `x` at place `i`), `xs.remove(i)` (take the item at `i` out and give it;
the last one is `xs.remove(xs.length - 1)`),
`xs.sort` (new sorted `List[T]`), `xs.join(sep)` (a `List[Str]` as one text, `sep` between the items), `xs.each { x in ... }`.

**`Map`**: `m.set(k, v)`, `m.get(k)` (value type), `m.has(k)` (`Bool`),
`m.keys` (`List[Str]`), `m.byValue` (`List[Str]`, descending by value).

## 9. Effects and capabilities

**Pure by default.** Anything that touches the world is marked in the
signature: `!IO` (input/output) and `!Mut` (changes a list or map it was handed). A
function must declare each of the two it uses, whether directly or by calling
a function that has it; `main` is exempt. `!Fail` (can fail) may be written,
but the compiler works it out and never asks for it. This keeps
cost visible at every call site.

```
shout(screen : Screen, who : Str) !IO {
  screen.print("hi, ".concat(who))   // print is !IO, so the signature says so
}
```

**`!Mut`.** A list, map or record handed to a function is the caller's own,
not a copy. A function that changes one (`.push`, `.set` or `.remove` on it, on a part of
it, or by passing it to a `!Mut` function that changes it) must say `!Mut`.
Building and returning a new list needs no mark, and neither does changing a
list the function made itself.

```
add(xs : List[Int], n : Int) !Mut {
  xs.push(n)                         // the caller's list grows
}

doubled(xs : List[Int]) : List[Int] {
  out : List[Int] = []
  xs.each { x in
    out.push(x * 2)                  // out is new here: no !Mut
  }
  out
}
```

**Capabilities replace imports.** There are no globals and no `import`. `main`
receives the world as `sys` and passes subsets down. A parameter list is a
permission set: a function can touch only what it was handed. The capability
types are `Screen`, `Files`, `Programs`, `Net`, `Clock`, `Env`, `Term`, `Mail`; they are erased at compile time and cost
nothing at runtime.

`sys` provides:

| Send | Type | Effect | Meaning |
|------|------|--------|---------|
| `sys.screen.print(x)` | — | `!IO` | print `Int` / `Float` / `Str` / `Bool` (as `true` / `false`), with a newline |
| `sys.screen.part(text)` | — | `!IO` | print a text with no line end |
| `sys.screen.error(text)` | — | `!IO` | print a line to stderr; the program goes on |
| `sys.files.read(path)?` | `Str` | `!IO !Fail` | whole file as a string (`""` if it cannot be read) |
| `sys.files.readBytes(path)` | `Bytes` | `!IO` | whole file as bytes, one byte of memory each (empty if it cannot be read); needs a `reads` line. `Bytes` has `b.length` and `b.at(i)` (0 to 255) and cannot be changed, looped over or printed |
| `sys.files.writeBytes(path, bytes)` | — | `!IO` | writes the bytes; stops the program, with nothing written, if a number is outside 0 to 255; needs a `writes` line |
| `sys.files.write(path, text)` | — | `!IO` | write `text` to `path` (truncates) |
| `sys.files.append(path, text)` | — | `!IO` | add `text` to the end of `path` |
| `sys.files.list(dir)` | `List[Str]` | `!IO` | the names in a folder, sorted; a folder's name ends in `/`; empty if it cannot be listed |
| `sys.files.exists(path)` | `Bool` | `!IO` | is there a file or folder at `path` |
| `sys.files.wait(path)` | `Str` | `!IO` | waits until the file or folder at `path` changes; the changed name in a folder, `""` for a file; stops the program if `path` is not there |
| `sys.files.rename(from, to)` | — | `!IO` | rename or move; the program stops if it cannot |
| `sys.files.delete(path)` | — | `!IO` | delete a file or an empty folder; the program stops if it cannot |
| `sys.programs.run(name, args)` | `Str` | `!IO` | run another program with a `List[Str]` of arguments and give what it printed; the program stops if it fails |
| `sys.programs.feed(name, args, text)` | `Str` | `!IO` | like `run`, with `text` as the program's standard input |
| `sys.programs.try(name, args)` | `Str` | `!IO` | like `run`, but a program that fails or is missing gives `""`, and its error text is hidden |
| `sys.programs.status(name, args)` | `Int` | `!IO` | run it, let it print to the screen, and give its exit status |
| `sys.programs.start(name, args)` | `Int` | `!IO` | start it without waiting, in a session of its own with no input, and give its process number (-1 if it could not start); the name obeys `runs` |
| `sys.net.fetch(url)` | `Str` | `!IO` | the body at a web address; the program stops unless the answer is a 2xx |
| `sys.net.fetchWith(url, headers)` | `Str` | `!IO` | like `fetch`, with a `List[Str]` of header lines such as `"X-Key: abc"` (at most 16, one line each) |
| `sys.net.reach(url)` | `Int` | `!IO` | the status a web address answers with (200, 404, ...), or 0 when nothing answers; never stops the program |
| `sys.net.post(url, body)` | `Str` | `!IO` | send `body` to a web address and give the answer |
| `sys.mail.send(to, subject, body)` | `Int` | `!IO` | send one mail through the machine's `sendmail` (or the program named in the variable `SIMPLER_SENDMAIL`); `to` is one address on the `mails` line, or one that a `"*@example.com"` entry on it covers; the program stops when the address is not allowed or the mail could not be handed over |
| `sys.clock.now` | `Int` | — | seconds since 1970 |
| `sys.clock.millis` | `Int` | — | thousandths of a second since 1970 |
| `sys.clock.date(secs)` | `Str` | — | local time as `2026-11-15 19:40:02` |
| `sys.clock.dateAs(secs, pattern)` | `Str` | — | the time as text in a `strftime` pattern, such as `"%a %d %b %H:%M"`, in local time |
| `sys.clock.secsOf(text)` | `Int` | — | seconds from a date text: `2026-01-31`, `2026-01-31 12:00`, `2026-01-31T12:00:00Z` or with `+01:00`; local time when no zone is given; `0` when the text is no date |
| `sys.term.put(row, col, text, style)` | `Int` | — | write `text` at a place on the whole terminal, both counted from 0; `style` is `""` or words from bold, dim, underline, reverse, black, red, green, yellow, blue, magenta, cyan, white, gray, a colour number 0 to 255 (`"208"`), and `on` with a number for the background (`"on52"`); text past the right edge is cut, and control characters in it are shown as `?` |
| `sys.term.key` | `Str` | — | wait for one key: a character, `enter`, `esc`, `tab`, `backspace`, `delete`, `up`, `down`, `left`, `right`, `home`, `end`, `pgup`, `pgdn`, `ctrl-x`, `resize`, `eof`, `other`. What `put` and `clear` did is shown here, and only the places that changed are sent, so a program may clear and draw the whole screen for each key |
| `sys.term.rows`, `sys.term.cols` | `Int` | — | the size of the terminal |
| `sys.term.clear`, `sys.term.stop` | `Int` | — | empty the terminal; give it back (also done when the program ends, and an error message is shown after that) |
| `sys.env.var(name)` | `Str` | — | an environment variable (`""` if not set) |
| `sys.env.setVar(name, value)` | `Int` | — | set a variable for this program and the programs it runs; 1, or 0 for a name that is empty or has `=` in it |
| `sys.env.self` | `Str` | — | the path of the program itself: the source file under `simpler run`, the program file when built |
| `sys.args` | `List[Str]` | — | command-line arguments (program name dropped) |
| `sys.stdin` | `Str` | — | all of standard input |

**A program says which files it may touch.** A `reads` line and a `writes`
line at the top of the file name them. `.read`, `.list` and `.exists` count as
reading; `.write`, `.append`, `.rename` (both names) and `.delete` as writing. A name is one file, a name ending in `/` is everything under that
folder, and the word `args` is any file named on the command line. `"./"` is the
folder the program runs in, so it covers `"a.txt"` as well as `"./a.txt"`.

```
reads "config.txt", "data/", args
writes "out/"
```

**The same goes for programs, hosts and mail.** A `runs` line names each program
the code may start, a `fetches` line each host it may reach, and a `mails` line
each address it may send to. A `*` in a name on any of these lines, and on `reads` and
`writes`, stands for any run of characters but never for a `/`: `reads "logs/*.log"`,
`fetches "*.example.com"`, `mails "*@example.com"`:

```
runs "git", "ffmpeg"
fetches "api.github.com"
mails "me@example.com"
```

A program is started with its arguments as a list, never through a shell, so
no text can turn into a second command. A web address is fetched with `curl`,
which must be installed; a redirect is not followed. A name or host written
out in the code is checked by the compiler, any other as it is used. A
function may not be named `run`, `feed`, `status`, `fetch`, `reach`, `post`, `date` or `var`.

A file call with no such line is refused. So is a path written out in the
program that the line does not name. A path made while running is checked as
it is used, and the program stops if the line does not allow it (a path with
`..` in it never counts as under a folder). A link inside an allowed folder that leads out of it is not followed:
the program stops with `a link leads out of "data/"`. A path that starts with `~/` is under the home folder, in a top line and in the program alike. A file named one by one at the top is opened as it is. Under `/sys` and `/proc` a link may lead elsewhere in the same tree, since the kernel makes those links. A program with no `reads` and no
`writes` line cannot touch any file.

**Examples at the top.** An `example` line gives the input, the output wanted,
and then any arguments:

```
example "3 4", "7"
example "", "sum 11", "5", "6"
```

`simpler build` and `simpler run` run each example before the program is used,
and give no program when one differs or stops with an error. A line end at the
very end does not count. An example runs dry, so it cannot change a file.

**Examples for one function.** An `example` line that does not start with a
text is a check on a function, and stands above it:

```
example double(4) == 8
double(n : Int) : Int {
  n * 2
}
```

The check must be true or false. A build runs each one before the program is
used. A failing one names its line and the value the left side gave:
`prog.smplr:1: example fails: the left side gave 9`. Such an example has no
`sys` to hand over, so it fits a function that touches nothing.

`simpler pieces prog.smplr` lists each function of the program: what it takes,
what it touches, and how many examples it has. What a function touches counts
the functions it calls too. A function with nothing under it but
`touches nothing` can only work out a value.

**Limits.** A `limits` line caps what the program may use:

```
limits "2s", "50MB"
```

`"2s"` is seconds of processor time (waiting does not count), `"50MB"` is
memory. A program that passes one stops with a line that says so (status 124
for time, 125 for memory). Each program it starts gets the same caps.

**Undo.** Before a program writes, appends to, renames or deletes a file, the
old file is copied aside (the first time in that run). `simpler undo prog.smplr`
puts every such file back and removes the ones the run made; with a built
program it is `SIMPLER_UNDO=1 ./prog`. One run is kept per program, the last
one that changed a file, under `~/.local/state/simpler/undo/`.

- A file over 16 MB is not copied; undo names it and leaves it.
- What a started program wrote is not kept, only what the program wrote itself.
- The copies are made by a helper outside the kernel lock, so a started
  program cannot change them.

**Replay.** When a program ends with an error, its arguments and the input it
had read (up to 1 MB) are kept. `simpler replay prog.smplr` runs it again with
the same arguments and input; `simpler replay --show prog.smplr` prints them,
ready to hand to whoever fixes the program. One failed run is kept per program.

**A loop must wait or end.** `while true { }` with no `break`, no `return`, no
call of a function and nothing that waits is refused by the compiler: it would
keep the processor busy for nothing.

**A program from a link.** `simpler run https://example.com/tool.smplr` fetches
the program, prints what `simpler what` says about it, and asks `Run it?`. The
answer is read from the terminal, so no program can give it. A yes is kept for
that exact text; a changed program asks again. The kernel then holds the
program to what was shown.

**A dry run.** `simpler run --dry prog.smplr` (or `SIMPLER_DRY=1` with a built
program) runs the program but does nothing that leaves a mark. Each write,
append, delete, rename, started program, fetch and mail prints a line such as
`would write "out/a.txt"` and is skipped; a skipped program or fetch gives `""`.
The kernel also gets no `writes` names and no net, so nothing slips past.

**The kernel keeps the same list.** On Linux 5.13 or newer a built program hands
its top lines to the kernel (Landlock) before `main` starts. From then on the
kernel refuses any other file, for the program and for every program it starts.
So `runs "sh"` gives a shell that can read and write only what the top lines name.

- A `reads` name may be read, a `writes` name may be written, made and removed.
  A file that only a started program writes is named in `writes` too.
- With a `runs` or `fetches` line the system folders (`/usr`, `/bin`, `/lib`,
  `/etc` and the like) may be read and run, and nothing in the home folder.
- With no `fetches` line no program in the tree can open a TCP connection
  (Linux 6.7). With one, the kernel lets the net through, and the host check
  stays with `sys.net`: a started program could reach any host.
- No program in the tree can become root, or signal a program outside it (Linux 6.12).
- Mail is handed to a helper that starts before the lock. It checks the address
  again and runs the mail program, which needs its own settings files.
- A name with a `*` opens its folder in the kernel; the exact check stays in the program.
- A single file named in `writes` cannot be removed or renamed; name its folder for that.
- On an older kernel, or another system, the checks in the program are all there is.

Pass a capability down by handing it over: `shout(sys.screen, "world")`. A
function that was handed only a `Screen` cannot read files. The compiler
enforces it: `.print` compiles only on a `Screen`, the file calls only on a
`Files`, and `sys` exists only in `main`. A parameter that
carries a capability must name its type (`out : Screen`).

## 10. Failure

`?` marks a call that can fail and propagates the failure upward; it is
required on `sys.files.read` (which yields `""` for a file it cannot read).
`exit(n)` ends the program with exit status `n` and prints nothing.
`fail(message)` writes `message` to stderr and exits with a non-zero status, the
way a Unix tool reports an error. `fail` may be called anywhere, not only as a
guard at the top:

```
reads args

main(sys) {
  args = sys.args
  if args.length < 1 {
    fail("usage: tool <file>")
  }
  text = sys.files.read(args.at(0))?
  sys.screen.print(text)
}
```

### Tests

A function named `test_...` with no parameters is a test. `assert(cond)` takes
a `Bool` and marks the test failed when it is false. `simpler test` leaves
`main` out, runs every test, prints `PASS name` or `FAIL name` for each and a
final count, and exits non-zero if any failed.

```
twice(n : Int) : Int { n + n }

test_twice() {
  assert(twice(21) == 42)
}
```

## 11. Complete programs

A read-transform-write filter (numbers a file's lines, file or stdin):

```
reads args

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

- No globals (capabilities replace them).
- No classes, inheritance, interfaces, or generics beyond `List[T]` / `Map[T]`.
- No closures or first-class functions (so sorting by a derived key is a
  type-specific method like `Map.byValue`, not a comparator argument).
- No exceptions (failure is `!Fail` and `?`; `fail` aborts).
- A list, map or record is handed over as itself, not as a copy; only a
  `!Mut` function may change one it was handed.
- No tuples, no null. A `Map` returns a typed default (`0` or `""`) for a
  missing key.

## 12b. Patterns

A pattern is a regular expression in a text, written as in Python or Perl. The engine is part of
every compiled program and needs no library.

- One character: a letter for itself, `.` (any but a line end), `\d \w \s` and `\D \W \S`,
  `[a-z0-9_]`, `[^,]`, `\n \t \r \f \v \e`, `\x41`, `\u00e5`, and a backslash before a sign for the sign.
- How many: `*`, `+`, `?`, `{3}`, `{2,}`, `{2,5}`, `{,5}`; a `?` after it takes as few as it can.
- Groups: `( )` keeps what it matched, `(?: )` does not, `(?P<name> )` and `(?<name> )` count as plain groups,
  `a|b` is either, `\1` to `\9` is what a group matched.
- Places: `^` and `$`, `\b` and `\B`, `\A` and `\Z`.
- Looking around: `(?= )`, `(?! )`, `(?<= )`, `(?<! )`.
- `(?i)` ignores case, `(?m)` lets `^` and `$` match at each line, `(?s)` lets `.` match a line end.
  They count for the whole pattern.
- A text is read as UTF-8, so `.` and `[æøå]` take whole letters. `\w` is a to z, digits, `_` and every
  letter outside ASCII. `(?i)` knows a to z and the Latin letters with accents.

A pattern that is wrong stops the program with its place and what is wrong:
`notes.smplr:16: the pattern "(a" has a ( with no )`. A pattern that would run for ever on a text
(`(a+)+$` on thirty `a` and a `!`) stops the program the same way after a fixed number of steps.
The last 16 patterns are kept ready, so a pattern used in a loop is read once.

## 13. Modules

A module is a file of functions and types that many programs share.

```
uses "tui"

main(sys) {
  p = tui.open(sys.term)
  p.border = true
  p.title = "Hello"
  p.bg = "#232154"
  tui.show(p)
  k = tui.key(p)
  tui.close(p)
}
```

- `uses "crust"` at the top brings in `crust.smplr`. The file is looked for
  next to the program, then in the folder `$SIMPLER_LIB`, then in the `lib`
  folder that came with simpler.
- A function of the module is called as `crust.frame(...)`. A type, a record
  or a variant case is written `crust.Pane`. A name without the module in
  front never reaches into a module, so two modules may use the same names.
- No variable may have the name of a module the file uses.
- A program that calls `tui.show(...)` with no `uses "tui"` line gets the line added by
  `simpler`, when a module of that name is found and the name is no variable in the program.
- A module is plain Simpler. It has no `main`, so it never holds `sys`: what
  it may touch is handed to it as a parameter (`t : Term`, `fs : Files`), and
  its functions carry the same effect marks as any other.
- A module has no `reads`, `writes`, `runs`, `fetches` or `mails` line. Those
  stay at the top of the program, and they bind the module's code too.
  `simpler what` lists a module's reach with the program's, as
  `(in tui.show)`.
- A module may use other modules. Inside it, no function or type may have the
  name of one of its record fields.
- A message names the file: `tui.smplr:57: ...`.
- `simpler prompt tui` prints the module's page: its types and the head of
  each function, with the comment above it. Give that page to the AI together
  with the main page. `simpler test lib/tui.smplr` runs a module's tests.

`tui` comes with simpler: panes for full-screen terminal programs. A pane is
built from smaller things, like a castle of bricks: it owns the terminal, and
it has properties that the program sets (`p.title`, `p.bg = "#232154"`,
`p.items`, `p.picked`). `tui.show(p)` then draws it as the properties say. A
function that is handed a pane can draw in that pane and nowhere else it was
not given. The older `crust` module (the terminal handed beside the pane)
stays for programs written with it.

Under `tui` is the module `screen`, a smaller castle: the whole terminal as
one thing with properties (`rows`, `cols`, `fg`, `bg`). Every pane has it as
`p.scr`. A pane whose `fg` or `bg` is `""` takes the screen's, so
`p.scr.bg = "#232154"` colours all panes that set no colour of their own.

## 14. How it runs

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

`./simpler test > tests.c` writes the test runner in place of the program.

`./simpler what` writes no C. It lists everything the program can touch: what
it prints, each file it reads or writes (by name when the name is written out
in the program), mail, standard input and the command line, each with the
functions that do it, the functions that change a list or map they were
handed, and the files its `reads` and `writes` lines allow. The list is complete, since a program reaches the world
only through `sys` and the compiler refuses any other way. A program can still
run for ever or use up memory.

The values of a call run left to right: in `f(a(), b())`, `a` runs first.

A running program never goes on with a wrong value. A list or text index past
the end, a slice past the end, a whole-number division by zero and `toInt` on
a text that is no whole number stop it with
`input.smplr:<line>: <what happened>` on stderr and exit code 1. (A missing map
key is no error: `get` gives 0 or "".)

A program the compiler refuses gets one line on stderr and exit code 1:
`input.smplr:<line>: <what is wrong>`, with the line of the statement. The
compiler crashing, or writing C that does not build, is a bug in the compiler.

---

## Locked decisions

- Everything is a message send; one grammar production.
- `=` binds, `:` types, `{ }` blocks, `.` sends. No overlap. `=` is a statement.
- Operator sugar is exactly `+ - * / == < >`; a leading `-` negates the whole
  value after it: `-p.n` is `-(p.n)`.
- `Int` and `Float` never mix without an explicit conversion.
- Variants need exhaustive `match`; records construct positionally.
- Effects (`!IO`, `!Fail`, `!Mut`) and failability (`?`) live in the type, not the syntax.
- Capabilities replace imports; `main` holds the world, passes subsets down.
- A program names the files it may read and write; any other file is refused.
- No GC, no borrow checker. Compile by transpiling to C.

---

By Geir Isene. Public domain (Unlicense).
