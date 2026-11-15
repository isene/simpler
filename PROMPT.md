# Simpler in one page

This page is what a model gets before it writes Simpler. [SPEC.md](SPEC.md) is the full reference.

Simpler is its own small language. It is NOT Python, Rust, Kotlin or JavaScript.
Copy the shapes below exactly. Anything not shown here does not exist.

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

- A function is `name(a : Int, b : Str) : Str { ... }`. Types: `Int`, `Float`, `Str`, `Bool`, `List[Int]`,
  `List[Str]`, `Map` (text to Int), `Map[Str]` (text to text). Capital first letter.
- The value of a function is its last line, written alone: `out`. Use `return x` only to leave early.
- Only the LAST line of a block may be a value alone. Every other line is `name = value`, `if`, `while`,
  a `for`, a `.push`, a `.set` or a `.print`.
- A name keeps one type. After `s = "12"` write `n = s.toInt`, never `s = s.toInt`.
- An empty list needs its type: `xs : List[Int] = []`. A new map is `Map()`.
- Text is in quotes: `"a"` or `'a'`. A character is a one-letter text: `s.at(i)`. Compare characters as text:
  `c >= "a" && c <= "z"`. The number of a character is `c.code`; a number has no `.code`.
- Operators: `+ - * / %`, `== != < > <= >=`, `&& ||`, and `!x` for not.
- `+` adds two numbers or joins two texts, never one of each: write `"n = " + n.toStr`.
- Loops: `for x in xs { }`, `for i in 0..n { }` (0 up to n - 1), `for i in 1..=n { }` (1 up to n) and
  `while cond { }`. No `break`, no `continue`: use a flag, or `return` in a function.
- `if a { } else if b { } else { }`. No parentheses needed, braces always.
- Input is `sys.stdin` (all of it, one text). Output is `sys.screen.print(text)`, one line per call.
  Only `main(sys)` has `sys`.
- A function that does `.push` or `.set` on a list or map it got as a parameter says `!Mut` after its
  return type: `add(xs : List[Int], n : Int) : Int !Mut { xs.push(n)  0 }`.
- A record is `Name = type { a : Int, b : Str }`, made with `Name(a = 1, b = "x")` (values in the order
  declared), read with `p.a`. A variant is `Name = type { CaseA CaseB(Int) }`; take it apart with
  `v.match { CaseA -> ... CaseB(n) -> ... }`, which must name every case and gives a value.
- A function that prints or touches files says `!IO` after its return type and gets what it needs as a
  parameter: `out : Screen` or `files : Files`, handed over as `sys.screen` or `sys.files`.
- Files: `sys.files.read(path)?` (the `?` is required; "" if it cannot be read), `.write(path, text)`,
  `.append(path, text)`, `.list(dir)` (names, a folder's ends in `/`), `.exists(path)`, `.rename(from, to)`,
  `.delete(path)`. Arguments are `sys.args` (a `List[Str]`).
- A program that touches files says which, at the top: `reads "a.txt", "data/", args` and `writes "out/"`.
  A name ending in `/` is a whole folder, `"./"` the folder it runs in, `args` any file named on the
  command line. Any other file is refused.
- `fail("message")` prints to stderr and stops the program. A function `test_name() { assert(cond) }`
  is a test; `simpler test` runs them.
- Decimal numbers are `Float`: `3.0`. `Int` and `Float` never mix: use `.toFloat` and `.toInt`.

## Every method there is

- Str: `s.length`, `s.at(i)`, `s.code`, `s.slice(a, b)`, `s.trim`, `s.concat(t)`, `s.split(d)`,
  `s.contains(t)`, `s.startsWith(t)`, `s.endsWith(t)`, `s.replace(a, b)`, `s.padLeft(n)`, `s.padRight(n)`, `s.toInt`, `s.isInt`, `s.toFloat`
- Int: `n.toStr`, `n.toFloat`. Float: `x.toStr`, `x.toInt`, `x.sqrt`, `x.floor`, `x.abs`. Bool: `b.not`
- List: `xs.length`, `xs.at(i)`, `xs.push(x)`, `xs.sort` (a new sorted list)
- Map: `m.set(k, v)`, `m.get(k)` (0 or "" when absent), `m.has(k)`, `m.keys`, `m.byValue` (keys, highest value first)

There is nothing else: no `filter`, `map`, `join`, `reverse`, `indexOf`, `substring`,
`pop`, `remove`, `println`, `len`, no `xs[i]`, no `s[i]`, no string formatting, no `let` or `var`.
A list has no way to remove or change an item. For a stack, copy all but the last item into a new list.
