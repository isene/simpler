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
- A name keeps one type through the whole function, also in another block or loop: if `k` was a
  text in one loop, a later counter needs another name. After `s = "12"` write `n = s.toInt`.
- An empty list needs its type: `xs : List[Int] = []`. A new map is `Map()`.
- Text is in quotes: `"a"` or `'a'`. A character is a one-letter text: `s.at(i)`. Compare characters as text:
  `c >= "a" && c <= "z"`. The number of a character is `c.code`; a number has no `.code`.
- Operators: `+ - * / %`, `== != < > <= >=`, `&& ||`, and `!x` for not.
- `+` adds two numbers or joins two texts, never one of each: write `"n = " + n.toStr`.
- Loops: `for x in xs { }`, `for i in 0..n { }` (0 up to n - 1), `for i in 1..=n { }` (1 up to n) and
  `while cond { }`. `break` leaves the loop, `continue` starts its next round.
- `if a { } else if b { } else { }`. No parentheses needed, braces always.
- Input is `sys.stdin` (all of it, one text). Output is `sys.screen.print(text)`, one line per call.
  Only `main(sys)` has `sys`.
- A function that does `.push`, `.set` or `.remove` on a list or map it got as a parameter says `!Mut` after its
  return type: `add(xs : List[Int], n : Int) : Int !Mut { xs.push(n)  0 }`.
- A record is `Name = type { a : Int, b : Str }`, made with `Name(a = 1, b = "x")` (values in the order
  declared), read with `p.a`. A variant is `Name = type { CaseA CaseB(Int) }`; take it apart with
  `v.match { CaseA -> ... CaseB(n) -> ... }`, which must name every case and gives a value.
- A function that prints or touches files says `!IO` after its return type and gets what it needs as a
  parameter: `out : Screen` or `files : Files`, handed over as `sys.screen` or `sys.files`.
- Files: `sys.files.read(path)?` (the `?` is required; "" if it cannot be read), `.write(path, text)`,
  `.append(path, text)`, `.list(dir)` (names, a folder's ends in `/`), `.exists(path)`, `.rename(from, to)`,
  `.delete(path)`. Arguments are `sys.args` (a `List[Str]`).
- A program that touches files says which, at the top: `reads "a.txt", "data/", "logs/*.log", args` and
  `writes "out/"` (a `*` is any run of characters without a `/`).
  A name ending in `/` is a whole folder, `"./"` the folder it runs in, `args` any file named on the
  command line, `"~/"` the home folder. Any other file is refused.
- `sys.screen.part(text)` prints with no line end.
- A file that is no text: `b = sys.files.readBytes(path)` is of type `Bytes`: `b.length`, `b.at(i)` (a number
  0 to 255), `b.intAt(i, n)` (the whole number in the `n` bytes, 1 to 8, from place `i`, lowest byte first),
  `b.floatAt(i)` (the Float in 8 bytes), `b.textAt(i, n)` (those `n` bytes as text, as they are),
  `b.findByte(n, from)` (the place of the next byte `n` at or after `from`, or -1). Bytes cannot be changed,
  looped over or printed. To write, fill a `List[Int]`: `sys.files.writeBytes(path, [80, 75, 3, 4])`;
  `s.bytes` is a text as such a list.
- Other programs: `sys.programs.run("git", ["log", "-1"])` gives what it printed (the program stops if
  it fails); `sys.programs.try("pgrep", ["-x", "vim"])` gives what it printed, or "" when it fails,
  and hides its error text; `sys.programs.status("grep", ["-q", "x", "f"])` gives its exit status
  and lets it print to the screen;
  `sys.programs.feed("sort", [], text)` is `run` with `text` as its input. Arguments are a
  `List[Str]`, one item each, no shell. The top says which: `runs "git", "grep"`.
- The web: `sys.net.fetch(url)` gives the body, `sys.net.fetchWith(url, ["X-Key: abc"])` the same
  with headers, `sys.net.post(url, body)` sends; all stop the
  program on failure, so ask first with `sys.net.reach(url)` (the status, 0 if nothing answers) when a
  miss is expected. The top names each
  host, without its port: `fetches "api.github.com", "127.0.0.1"`; a url made while running is checked against it. Time: `sys.clock.now` (seconds), `sys.clock.date(secs)` (text),
  `sys.clock.dateAs(secs, "%a %d %b %H:%M")` (text in that pattern), `sys.clock.secsOf("2026-01-31T12:00:00Z")`
  (seconds from a date text, also `2026-01-31T12:00:00.123+01:00`, or local time as `2026-01-31 12:00` or `2026-01-31`; 0 if it is none).
  Environment: `sys.env.var("HOME")`. Their parameter types are `Programs`, `Net`, `Clock`, `Env`.
- A full-screen program: `t = sys.term` (type `Term`). `t.put(row, col, text, style)` writes at a place
  (both from 0; the style is "" or words such as "bold red" or "reverse"; a number 0 to 255 is a colour
  and "on52" a background; text past the right edge is cut). `t.key` waits for one key and gives its name: a letter, " ", "enter", "esc", "tab",
  "backspace", "delete", "up", "down", "left", "right", "home", "end", "pgup", "pgdn", "ctrl-x",
  "resize" (the window changed size) or "eof". `t.rows`, `t.cols`, `t.clear`. `t.stop` gives the
  terminal back; do it before `sys.screen.print`. The loop is: draw, `k = t.key`, act on `k`.
- A module: `uses "crust"` at the top brings in the file `crust.smplr`. Call its functions as
  `crust.frame(...)` and write its types as `crust.Pane`. No variable may be named `crust` then. Use a
  module only when you were given its page.
- Mail: `sys.mail.send(to, subject, body)`, with a top line that says to whom: `mails "me@example.com"`
  (`"*@example.com"` allows everyone there). Its parameter type is `Mail`, and the function says `!IO`.
- No function of yours may be named `run`, `try`, `feed`, `status`, `fetch`, `fetchWith`, `reach`, `post`, `readBytes`, `writeBytes`, `date`, `dateAs`, `secsOf` or `var`.
- `fail("message")` prints to stderr and stops the program. A function `test_name() { assert(cond) }`
  is a test; `simpler test` runs them.
- Decimal numbers are `Float`: `3.0`. `Int` and `Float` never mix: use `.toFloat` and `.toInt`.

## Every method there is

- Str: `s.length`, `s.at(i)`, `s.code`, `s.slice(a, b)`, `s.trim`, `s.concat(t)`, `s.split(d)`,
  `s.contains(t)`, `s.find(t)` (the place of `t`, or -1), `s.startsWith(t)`, `s.endsWith(t)`, `s.replace(a, b)`,
  `s.upper`, `s.lower`, `s.padLeft(n)`, `s.padRight(n)`, `s.toInt`, `s.isInt`, `s.toFloat`
- JSON in a Str: `s.json("name")` is the value under that key, `s.json("0")` the item at that place in an array;
  it comes back as a Str (a text without its quotes, `12.5`, `true`, or the JSON of an inner object), `""` when missing.
  Chain it: `s.json("user").json("name")`. `s.jsonItems` and `s.jsonKeys` are `List[Str]`
- Int: `n.toStr`, `n.toFloat`. Float: `x.toStr`, `x.toInt`, `x.sqrt`, `x.floor`, `x.abs`. Bool: `b.not`
- List: `xs.length`, `xs.at(i)`, `xs.push(x)`, `xs.set(i, x)`, `xs.remove(i)` (takes the item out and
  gives it; a stack pop is `xs.remove(xs.length - 1)`), `xs.sort` (a new sorted list),
  `xs.join(sep)` (a list of texts as one text)
- Map: `m.set(k, v)`, `m.get(k)` (0 or "" when absent), `m.has(k)`, `m.keys`, `m.byValue` (keys, highest value first)

There is nothing else: no `filter`, `map`, `reverse`, `indexOf`, `substring`,
`pop`, `println`, `len`, no `xs[i]`, no `s[i]`, no string formatting, no `let` or `var`.
