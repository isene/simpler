# Simpler in one page

This page is what a model gets before it writes Simpler. [SPEC.md](SPEC.md) is the full reference.

Simpler is its own small language. It is NOT Python, Rust, Kotlin or JavaScript.
Copy the shapes below exactly. Anything not shown here does not exist.

Plan briefly. Do not write the program, or parts of it, while you think:
write it once, in the answer.

## Example 1: read numbers, print their sum

```
main(sys) {
  total = 0
  for line in sys.stdin.split("\n") {
    t = line.trim
    if t.length > 0 {
      total = total + t.toInt
    }
  }
  sys.screen.print(total.toStr)
}
```

## Example 2: a function, a while loop, joining text with +

```
reverse(s : Str) : Str {
  out = ""
  i = s.length - 1
  while i >= 0 {
    out = out + s.at(i)
    i = i - 1
  }
  out
}

main(sys) {
  for line in sys.stdin.split("\n") {
    sys.screen.print(reverse(line))
  }
}
```

## Example 3: a list, a map, sorting, early return

```
is_digit(c : Str) : Bool {
  c >= "0" && c <= "9"
}

first_number(words : List[Str]) : Int {
  k = 0
  while k < words.length {
    w = words.at(k)
    if w.isInt {
      return w.toInt
    }
    k = k + 1
  }
  0
}

main(sys) {
  counts = Map()
  names : List[Str] = []
  for line in sys.stdin.split("\n") {
    if line.length > 0 {
      if !counts.has(line) {
        names.push(line)
      }
      counts.set(line, counts.get(line) + 1)
    }
  }
  for n in names.sort {
    sys.screen.print(counts.get(n).toStr + " " + n)
  }
  for i in 0..names.length {
    sys.screen.print(i.toStr + ": " + names.at(i))
  }
  if is_digit("7") {
    sys.screen.print(first_number(names).toStr)
  }
}
```

## Example 4: your own types, a file, a failure, a test

```
reads args
writes "out/"

Point = type { x : Int, y : Int }
Shape = type { Dot Circle(Int) Rect(Int, Int) }

area(s : Shape) : Int {
  s.match {
    Dot          -> 0
    Circle(r)    -> 3 * r * r
    Rect(w, h)   -> w * h
  }
}

report(out : Screen, label : Str, n : Int) !IO {
  out.print(label + ": " + n.toStr)
}

test_area() {
  assert(area(Rect(2, 3)) == 6)
}

main(sys) {
  if sys.args.length < 1 {
    fail("usage: tool <file>")
  }
  text = sys.files.read(sys.args.at(0))?
  p = Point(x = text.length, y = 2)
  report(sys.screen, "area", area(Rect(p.x, p.y)))
  sys.files.write("out/size.txt", p.x.toStr)
}
```

## The rules

- A function is `name(a : Int, b : Str) : Str { ... }`. Types: `Int`, `Float`, `Str`, `Bool`, `List[T]`,
  `Map` (text to Int), `Map[T]` (text to T). Capital first letter. `T` is any type, also one of your own or
  another list: `List[Point]`, `List[List[Int]]`, `Map[Str]`, `Map[Shape]`, `Map[List[Str]]`.
- Functions may stand in any order and may call each other.
- The value of a function is its last line, written alone: `out`. Use `return x` only to leave early.
- Only the LAST line of a block may be a value alone. Every other line is `name = value`, `if`, `while`,
  a `for`, a `.push`, a `.set`, a `.print` or a call that stands alone: `save(state, line)`. A call may
  drop the value the function gives; no `x = save(...)` is needed.
- A name may take a new type: after `s = "12"`, `s = s.toInt` is fine. Inside a block or loop the
  new type lasts to the end of that block; after it the name is what it was before.
- An empty list needs its type: `xs : List[Int] = []`. A new map is `Map()` (text to Int), `Map[Str]()`
  or `Map[Point]()`.
- Text is in quotes: `"a"` or `'a'`. In a text, `\n`, `\t`, `\r`, `\\`, `\"` and `\'` are the usual; a backslash before
  anything else stays as it is. For text full of quotes or backslashes (code to patch), write `old = <<END` and end
  it with a line holding `END` alone: the lines between are taken as written, each with its newline, and the end
  word's indent is cut from each line. A character is a one-letter text: `s.at(i)`. A text is counted in letters, never bytes: `"blå".length` is 3. Compare characters as text:
  `c >= "a" && c <= "z"`. The number of a character is `c.code` and the character of a number is `n.char` (`8364.char` is "€").
- Operators: `+ - * / %`, `== != < > <= >=`, `&& ||`, `!x` for not, and `-x`. `&&` and `||` stop as soon
  as the answer is known, so `i < n && xs.at(i) == 0` is safe. Int `/` cuts toward zero and `%` takes the
  sign of the left side. `true` and `false` are the two Bool values. Texts compare with `<` by character code.
  `//` starts a comment.
- `+` adds two numbers or joins two texts, never one of each: write `"n = " + n.toStr`.
- Loops: `for x in xs { }`, `for i in 0..n { }` (0 up to n - 1), `for i in 1..=n { }` (1 up to n) and
  `while cond { }`. `break` leaves the loop, `continue` starts its next round; both, and `return`, work
  at any depth of `if` inside a loop.
- `if a { } else if b { } else { }`. No parentheses needed, braces always. `if` is never a value:
  write `x = 0` and then `if c { x = 1 }`. A `match` is a value.
- Input is `sys.stdin` (all of it, one text), or `sys.stdinLine`: the next line with its newline, waiting until one comes; `""` when the input has ended. Output is `sys.screen.print(text)`, one line per call.
  Only `main(sys)` has `sys`.
- A function that does `.push`, `.set` or `.remove` on a list or map it got as a parameter, or sets a field of
  a record it got, says `!Mut` after its return type: `add(xs : List[Int], n : Int) : Int !Mut { xs.push(n)  0 }`.
  So does a function that hands such a parameter (or a part of it) to a `!Mut` function. A list, map or
  record the function made itself needs none, and a `!Mut` that turns out unneeded is no error. A function
  may leave out the return type, and effects combine: `bump(out : Screen, xs : List[Int]) !Mut !IO { }`.
- A record is `Name = type { a : Int, b : Str }`, made with `Name(a = 1, b = "x")` (values in the order
  declared), read with `p.a`. A variant is `Name = type { CaseA CaseB(Int) }`; take it apart with
  `v.match { CaseA -> ... CaseB(n) -> ... }`, which must name every case and gives a value. An arm is
  one expression; call a function when it needs more. A case holds at most 8 values.
- A type may hold lists, maps, other types and itself. A tree is a variant that holds itself:
  `Expr = type { Num(Int) Name(Str) Add(Expr, Expr) Call(Str, List[Expr]) }`. Keep data with a shape
  (a tree, a table, a list of things with several fields) in types such as these, never packed into texts.
- A field of a record is set with `p.x = 5`, also deeper: `game.player.hp = 0`, `ps.at(i).x = 5`. A record is
  shared like a list, never copied: after `q = p`, `q.x = 5` changes `p` too. Keep the state of a big program
  in one record and hand it to the functions.
- A function that prints or touches files says `!IO` after its return type and gets what it needs as a
  parameter: `out : Screen` or `files : Files`, handed over as `sys.screen` or `sys.files`.
- Files: `sys.files.read(path)?` (the `?` is required; "" if it cannot be read), `.write(path, text)`,
  `.append(path, text)`, `.list(dir)` (names, sorted; a folder's ends in `/`, a link to a folder too, so no other check is needed), `.exists(path)`, `.wait(path)` (waits until the file or folder changes; gives the changed name in a folder, `""` for a file; needs a `reads` line), `.rename(from, to)`,
  `.delete(path)`. Arguments are `sys.args` (a `List[Str]`).
- A program that touches files says which, at the top: `reads "a.txt", "data/", "logs/*.log", args` and
  `writes "out/"` (a `*` is any run of characters without a `/`).
  A name ending in `/` is a whole folder, `"./"` the folder it runs in, `args` any file named on the
  command line, `"~/"` the home folder. Any other file is refused.
- `sys.screen.part(text)` prints with no line end. `sys.screen.error(text)` prints a line to stderr.
- Other programs: `sys.programs.run("git", ["log", "-1"])` gives what it printed (the program stops if
  it fails); `sys.programs.try("pgrep", ["-x", "vim"])` gives what it printed, or "" when it fails,
  and hides its error text; `sys.programs.status("grep", ["-q", "x", "f"])` gives its exit status
  and lets it print to the screen; `sys.programs.start("mpv", [file])` starts it, does not wait,
  and gives its process number; `t = sys.programs.drive("sh", [])` starts it on a terminal of its own:
  `sys.programs.press(t, "ls\r")` types, `sys.programs.shown(t, 500)` gives what it printed (waits up to
  500 ms), and `sys.programs.end(t)` stops it and all it started and gives its exit status;
  `sys.programs.feed("sort", [], text)` is `run` with `text` as its input;
  `sys.programs.output(name, args, text)` is `feed` that goes on when the program fails. Arguments are a
  `List[Str]`, one item each, no shell. The top says which: `runs "git", "grep"`.
  A program you run is held to your `reads` and `writes` lines too: name each file or folder it reads or writes.
- When the task shows input with the output it wants, put it at the top: `example "3 4", "7"` (the input, the output,
  then any arguments). The build runs each example and gives no program when one differs. A last line end does not count.
- Build from small functions, and put a check above each one that takes no `sys` part: `example double(4) == 8`. The build runs
  these too (`==` also compares two lists of `Int`, `Float`, `Bool` or `Str`; for a record, compare a field). Hand a function only the part it needs (`files : Files`, `out : Screen`), never the whole `sys`.
- `limits "2s", "50MB"` at the top caps processor time and memory; the program stops when it passes one.
- A program that stays running waits: with `sys.stdinLine`, `sys.files.wait(path)` or `sys.clock.sleep(ms)`. Never loop to check for a change without one of them.
- Time: `sys.clock.now` (seconds), `sys.clock.millis` (thousandths of a second), `sys.clock.sleep(ms)` (waits that long), `sys.clock.date(secs)` (text),
  `sys.clock.dateAs(secs, "%a %d %b %H:%M")` (text in that pattern), `sys.clock.secsOf("2026-01-31T12:00:00Z")`
  (seconds from a date text, also `2026-01-31T12:00:00.123+01:00`, or local time as `2026-01-31 12:00` or `2026-01-31`; 0 if it is none).
  Environment: `sys.env.var("HOME")`; `sys.env.setVar("LANG", "C")` sets one for this program and the
  programs it runs; `sys.env.self` is the path of the program itself. Their parameter types are `Programs`, `Clock`, `Env`.
- A module: `uses "tui"` at the top brings in the file `tui.smplr`. Call its functions as
  `tui.show(...)` and write its types as `tui.Pane`. No variable may be named `tui` then. Use a
  module only when you were given its page. Three come with simpler: `tui` (panes on a full
  screen), `astro` (sun, moon, planets, stars, eclipses: where, when, how bright) and `opts`. A program that takes options such as `-n 5` or `--quiet` uses `opts` and does not
  read them by hand; when you were not given its page, answer `NEED: opts`.
- A function of yours may have the name of a built-in method (`run`, `print`, `setVar`); call it plainly, as `run(x)`. A call after a dot, `x.run(...)`, is always the built-in.
- `fail("message")` prints to stderr and stops the program with exit status 1. `exit(2)` stops it with
  that status and prints nothing. A function `test_name() { assert(cond) }`
  is a test; `simpler test` runs them.
- Decimal numbers are `Float`: `3.0`. `Int` and `Float` never mix: use `.toFloat` and `.toInt`.

## Every method there is

- Str: `s.length`, `s.at(i)`, `s.code`, `s.slice(a, b)` (from `a` up to `b`, not counting `b`; `s.slice(a)` runs to the end), `s.trim`, `s.concat(t)`, `s.split(d)`,
  `s.contains(t)`, `s.find(t)` (the place of `t`, or -1), `s.startsWith(t)`, `s.endsWith(t)`, `s.replace(a, b)`,
  `s.upper`, `s.lower`, `s.padLeft(n)`, `s.padRight(n)`, `s.toInt` (takes `"-12"` and `"08"`; stops the program on other text, so ask `s.isInt` first), `s.isInt`, `s.toFloat`
- Bool: `b.toStr` is `"true"` or `"false"`, `b.not`.
- Int: `n.toStr`, `n.toFloat`, `n.abs`, bits: `a.bitAnd(b)`, `a.bitOr(b)`, `a.bitXor(b)`, `a.shl(n)`, `a.shr(n)`. Float: `x.toStr`, `x.toInt`, `x.sqrt`, `x.floor`, `x.ceil`, `x.round`, `x.abs`, `x.pow(y)`, `x % y`. Bool: `b.not`
- List: `xs.length`, `xs.at(i)`, `xs.push(x)`, `xs.set(i, x)`, `xs.slice(a, b)` (a new list; `xs.slice(a)` runs to the end), `xs.remove(i)` (takes the item out and
  gives it; a stack pop is `xs.remove(xs.length - 1)`), `xs.sort` (a new sorted list),
  `xs.join(sep)` (a list of texts as one text), `xs.parallel(f)` (your function `f` on each item on all
  cores at once, results in list order; `f` takes one item, gives an Int, Float, Bool or Str, and has no `!IO`)
- Map: `m.set(k, v)`, `m.get(k)` (0 or "" when absent; a map of lists or of your own types has no empty
  value, so there the program stops: ask `m.has(k)` first), `m.has(k)`, `m.remove(k)`, `m.keys`, `m.byValue` (keys, highest value first)

More exists, each on a page of its own: `regex` (patterns), `json`, `net` (the web, mail), `bytes` (files that are no text) and
`screen` (full-screen programs). When the job needs one you were not given, answer with one
line, such as `NEED: regex`.

There is nothing else: no `filter`, `map`, `reverse`, `indexOf`, `substring`,
`pop`, `println`, `len`, no `xs[i]`, no `s[i]`, no string formatting, no `let` or `var`.
