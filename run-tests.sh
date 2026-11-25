#!/usr/bin/env bash
# Regression harness for the Simpler compiler (selfhost/).
#
# Three kinds of check, one command, green or red:
#   1. every example runs and matches its golden output,
#   2. every known-bad program is rejected with the expected message,
#   3. `fmt` is idempotent on every example, and `test` reports correctly.
#
# Run from anywhere; it operates in the repo folder and removes the
# generated .c / binary artifacts on exit.
set -u
cd "$(dirname "$0")"
PATH="/usr/bin:$PATH"            # real cc, not the ~/bin/cc session launcher
# SAN="-fsanitize=undefined" ./run-tests.sh builds the compiler and every test
# program with that C checker, and a fault the checker finds stops the program.
if [ -n "${SAN:-}" ]; then
    # gcc 15.2 with -O2 and the undefined-behaviour checker drops the second
    # half of a text comparison after a checked + (a key loop never saw "q");
    # clang, -O1 and gcc without jump threading build it right.
    SAN="$SAN -fno-sanitize-recover=all -fno-thread-jumps -g"
    cc() { command cc $SAN "$@"; }
    export CC="cc $SAN" ASAN_OPTIONS="detect_leaks=0:detect_stack_use_after_return=0"
fi
TMP="$(mktemp -d)"
pass=0
fail=0

cleanup() {
    find examples -maxdepth 1 -type f ! -name '*.smplr' ! -name '*.txt' -delete 2>/dev/null
    # remove generated artifacts in selfhost, but keep the committed seed simpler.c
    find selfhost -maxdepth 1 -type f -name '*.c' ! -name 'simpler.c' -delete 2>/dev/null
    for f in selfhost/*.smplr; do [ "$f" = selfhost/simpler.smplr ] || rm -f "${f%.smplr}"; done   # per-source binaries, never the compiler the simpler command runs
    rm -f selfhost/input.smplr input.smplr
    rm -rf "$TMP"
}
trap cleanup EXIT

ok()  { pass=$((pass + 1)); }
nope() { fail=$((fail + 1)); echo "FAIL: $1"; }

# The compiler under test: built from the committed C seed, nothing else.
SEEDC="$TMP/seedc"
cc -O2 selfhost/simpler.c -o "$SEEDC" || { echo "compiler build failed"; exit 1; }

# Compile a .smplr file with it and run the result; prints the program's output.
run_smplr() { # file
    cp "$1" "$TMP/input.smplr"
    ( cd "$TMP" && ./seedc > run.c 2>/dev/null && cc -w run.c -o run.bin -lm 2>/dev/null ) && "$TMP/run.bin" 2>/dev/null
    rm -f "$TMP/input.smplr"
}

# --- 1. examples produce their golden output ----------------------------------
check_run() { # name  expected
    local out
    out="$(run_smplr "examples/$1.smplr")"
    if [ "$out" = "$2" ]; then ok; else nope "run $1
       expected: $(printf '%q' "$2")
       got:      $(printf '%q' "$out")"; fi
}
check_run hello "hello, world"
check_run m3    "$(printf 'hello,\nworld')"
check_run m3c   "$(printf 'Quarterly numbers look good.\n\n6')"
check_run m5a   "$(printf '7\n25\n13')"
check_run m5b   "$(printf 'hello\n42\ndot')"
check_run m5c   "$(printf '5\ne\nell\nhello world\n42!\ntrue\nfalse\n1\nfalse\ntrue')"
check_run m5e   "20"
check_run m6    "15"
check_run emptylist "$(printf '1\n0')"
check_run retype    "$(printf 'abc\n4\n2\n1\na=1\n11\n99\nb:22\nplain\nin1\nin2\nouter\n42\nv1\nv2')"
check_run ownrecord "$(printf '3\n1')"
check_run earlyret  "$(printf '2\n1')"
check_run listslice "$(printf '7\n2\n2')"
check_run numsmaps  "$(printf '6\n2.5\nfalse\na,c\n4\n9\n572\n1152\n-4')"
check_run ownnames  "$(printf '9\n8\n3\n101\n7')"
check_run cnames    "45"
check_run booltext  "$(printf 'true\nfalse\ntrue!\ntrue')"
check_run rawtext   "$(printf '%s\n' 32 'if x {' '  y()' '' '}' 'say "hi" to C:\temp\n, it'"'"'s raw' '|' 0)"
# sleep, a wait for a file change, and input read line by line
out="$(printf 'a\nb\n' | run_smplr examples/waits.smplr)"
if [ "$out" = "$(printf 'true\nsimpler-waits-test\n[a]\n[b]')" ]; then ok; else nope "run waits: got $(printf '%q' "$out")"; fi

# the self-hosted lexer (written in Simpler) scans the full token set
lout="$(run_smplr selfhost/lexer.smplr)"
lwant="$(printf 'id a\nop .\nid f\nop (\nstr hi\nop )\nop !\nop ?\nop +\nop -\nop *\nop /\nop <\nop >\nop =\nop ==\nop ->\nop :\nop ,\nop [\nop ]\nop {\nop }\nint 0\neof')"
[ "$lout" = "$lwant" ] && ok || nope "self-hosted lexer (got: $lout)"

# the self-hosted calc (lex -> parse -> {eval, emit C}) respects precedence
cout="$(run_smplr selfhost/calc.smplr)"
cval="$(printf '%s\n' "$cout" | sed -n 1p)"
cexp="$(printf '%s\n' "$cout" | sed -n 2p)"
[ "$cval" = "25" ] && ok || nope "self-hosted calc eval (got: $cval)"
# the C it emits compiles and computes the same value: the self-host loop, in miniature
printf '#include <stdio.h>\nint main(){printf("%%d\\n", %s);return 0;}\n' "$cexp" > "$TMP/emit.c"
if cc -o "$TMP/emit" "$TMP/emit.c" 2>/dev/null && [ "$("$TMP/emit")" = "25" ]; then ok; else nope "emitted C compiles to 25 (expr: $cexp)"; fi

# the real self-hosted compiler is built from its committed C seed (no Rust) and
# fed sample.smplr, which exercises the whole subset (recursive multi-payload
# variants with match, enums, records with field access, typed params/returns,
# Str and List methods, list literals and `.each` loops, if/else, while,
# comparisons, capabilities, escaped strings). The emitted C compiles and runs,
# and its output matches what the bootstrap produces running the sample directly.
# (simpler.smplr itself now uses `fail`, which the frozen Rust does not know, so
# the self-host checks build the compiler from the seed instead of via the Rust.)
cp selfhost/sample.smplr "$TMP/input.smplr"
( cd "$TMP" && ./seedc > sh.c 2>/dev/null )
rm -f "$TMP/input.smplr"
sampexp="$(printf 'a\nb\nhi!\n72')"
if cc -o "$TMP/sh" "$TMP/sh.c" 2>/dev/null && [ "$("$TMP/sh")" = "$sampexp" ] && [ "$sampexp" = "$(printf 'a\nb\nhi!\n72')" ]; then ok; else nope "self-hosted compiler builds the sample"; fi

# a real read->transform->write tool, compiled by the self-hosted compiler:
# linenum.smplr reads in.txt, numbers every line, writes out.txt. Proves the
# file-output capability (sys.files.write) end to end, not just that it parses.
cp selfhost/linenum.smplr "$TMP/input.smplr"
( cd "$TMP" && ./seedc > ln.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/ln" "$TMP/ln.c" 2>/dev/null; then
    printf 'alpha\nbeta\n' > "$TMP/in.txt"
    # the input and output filenames come from the command line (sys.args)
    ( cd "$TMP" && ./ln in.txt out.txt )
    if [ "$(cat "$TMP/out.txt" 2>/dev/null)" = "$(printf '1\talpha\n2\tbeta\n')" ]; then ok; else nope "linenum tool read/write (got: $(cat "$TMP/out.txt" 2>/dev/null))"; fi
    # with too few args it fails like a Unix tool: usage to stderr, exit 1,
    # and stdout stays empty
    out="$( cd "$TMP" && ./ln 2>/dev/null )"; rc=$?
    err="$( cd "$TMP" && ./ln 2>&1 1>/dev/null )"
    if [ "$rc" -eq 1 ] && [ -z "$out" ] && [ "$err" = "usage: linenum <in> <out>" ]; then ok; else nope "linenum usage (rc=$rc out=$out err=$err)"; fi
    rm -f "$TMP/in.txt" "$TMP/out.txt"
else
    nope "linenum tool compiles"
fi

# Str.toInt: parse command-line arguments as integers and add them. Proves
# string-to-int parsing end to end, the thing every numeric tool needs.
printf '%s' 'main(sys) {
  a = sys.args
  sys.screen.print((a.at(0).toInt + a.at(1).toInt).toStr)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > add.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/add" "$TMP/add.c" 2>/dev/null && [ "$("$TMP/add" 40 2)" = "42" ]; then ok; else nope "Str.toInt arg arithmetic"; fi

# Str.split: split a comma-separated argument and sum the fields. Proves
# split returns a List[Str] that composes with .each, .toInt and .length.
printf '%s' 'main(sys) {
  parts = sys.args.at(0).split(",")
  total = 0
  parts.each { p in total = total + p.toInt }
  sys.screen.print(parts.length.toStr)
  sys.screen.print(total.toStr)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > sum.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/sum" "$TMP/sum.c" 2>/dev/null && [ "$("$TMP/sum" 10,20,3,9)" = "$(printf '4\n42')" ]; then ok; else nope "Str.split fields"; fi

# Str.contains / Str.replace: substring test (a Bool) and replace-all (a Str,
# non-overlapping, multi-char patterns, unchanged when nothing matches).
printf '%s' 'main(sys) {
  s = "the quick brown fox"
  if s.contains("quick") { sys.screen.print("yes") } else { sys.screen.print("no") }
  if s.contains("slow") { sys.screen.print("yes") } else { sys.screen.print("no") }
  sys.screen.print("a,b,c".replace(",", "-"))
  sys.screen.print("aaaa".replace("aa", "b"))
  sys.screen.print("keep".replace("x", "y"))
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > cr.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/cr" "$TMP/cr.c" 2>/dev/null && [ "$("$TMP/cr")" = "$(printf 'yes\nno\na-b-c\nbb\nkeep')" ]; then ok; else nope "Str.contains/replace (got: $("$TMP/cr" 2>/dev/null))"; fi

# Str.trim: strip leading and trailing whitespace (spaces, tabs, CR, LF), so a
# CRLF or padded line is clean; a whitespace-only string trims to empty.
printf '%s' 'main(sys) {
  sys.screen.print("[".concat("  hi  ".trim).concat("]"))
  sys.screen.print("[".concat("\tx\r\n".trim).concat("]"))
  sys.screen.print("   ".trim.length.toStr)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > trim.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/trim" "$TMP/trim.c" 2>/dev/null && [ "$("$TMP/trim")" = "$(printf '[hi]\n[x]\n0')" ]; then ok; else nope "Str.trim (got: $("$TMP/trim" 2>/dev/null))"; fi

# Int.mod and Str.padLeft/padRight: remainder and field alignment, the pieces
# a report tool needs. padding leaves an already-wide string unchanged.
printf '%s' 'main(sys) {
  sys.screen.print(17.mod(5).toStr)
  sys.screen.print("[".concat("7".padLeft(3)).concat("]"))
  sys.screen.print("[".concat("hi".padRight(4)).concat("]"))
  sys.screen.print("[".concat("toolong".padLeft(3)).concat("]"))
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > mp.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/mp" "$TMP/mp.c" 2>/dev/null && [ "$("$TMP/mp")" = "$(printf '2\n[  7]\n[hi  ]\n[toolong]')" ]; then ok; else nope "mod/padLeft/padRight (got: $("$TMP/mp" 2>/dev/null))"; fi

# Uniform function call: x.f is f(x) for a user function, so a chain of unary
# words desugars to nested calls and can be packaged into one bigger word. UFCS
# coexists with real record fields (p.x is the field, p.mag2 is mag2(p)).
printf '%s' 'parse(s : Str) : Str { s.trim }
shout(s : Str) : Str { s.concat("!") }
wrap(s : Str) : Str { "[".concat(s).concat("]") }
loud(s : Str) : Str { wrap(shout(parse(s))) }
Point = type { x : Int, y : Int }
mag2(p : Point) : Int { p.x * p.x + p.y * p.y }
main(sys) {
  data = "  hi  "
  sys.screen.print(data.parse.shout.wrap)
  sys.screen.print(data.loud)
  p = Point(x = 3, y = 4)
  sys.screen.print(p.mag2.toStr)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > ufcs.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/ufcs" "$TMP/ufcs.c" 2>/dev/null && [ "$("$TMP/ufcs")" = "$(printf '[hi!]\n[hi!]\n25')" ]; then ok; else nope "UFCS x.f = f(x) (got: $("$TMP/ufcs" 2>/dev/null))"; fi
# the chain compiles to the exact nested-call form
( cd "$TMP" && grep -q 'u_wrap(u_shout(u_parse(data)))' ufcs.c ) && ok || nope "UFCS desugars to nested calls"

# slug.smplr, the UFCS showcase, AI-written from SPEC.md alone: small Str->Str
# words snapped into a `slug` chain, applied as `line.slug`. A real filter.
cp selfhost/slug.smplr "$TMP/input.smplr"
( cd "$TMP" && ./seedc > slug.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/slug" "$TMP/slug.c" 2>/dev/null; then
    in="$(printf '  Hello, World.  \na b c\nno.commas,here\n\n   \n')"
    if [ "$(printf '%s' "$in" | "$TMP/slug")" = "$(printf 'Hello-World\na-b-c\nnocommashere')" ]; then ok; else nope "slug filter (got: $(printf '%s' "$in" | "$TMP/slug"))"; fi
    # the chain really compiled to the nested-call form
    ( cd "$TMP" && grep -q 'u_hyphenated(u_noPeriods(u_noCommas(s_trim' slug.c ) && ok || nope "slug chain desugars"
else
    nope "slug compiles"
fi

# A list literal keeps its elements (it once compiled to an empty list).
printf '%s' 'main(sys) {
  vals = [1000, 900, 5]
  syms = ["M", "CM", "V"]
  sys.screen.print(vals.length.toStr)
  sys.screen.print(syms.at(1))
  sys.screen.print((vals.at(0) + vals.at(2)).toStr)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > lit.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/lit" "$TMP/lit.c" 2>/dev/null; then
    if [ "$("$TMP/lit")" = "$(printf '3\nCM\n1005')" ]; then ok; else nope "list literal (got: $("$TMP/lit"))"; fi
else
    nope "list literal compiles"
fi

# The operators % <= >= && ||, an early `return`, a parameter that is assigned
# to, and a function that returns List[Str] (its element type was once lost).
printf '%s' 'first(xs : List[Int], want : Int) : Int {
  i = 0
  while i < xs.length {
    if xs.at(i) == want {
      return i
    }
    i = i + 1
  }
  0 - 1
}
skip(s : Str, p : Int) : Int {
  while p < s.length && s.at(p) == " " {
    p = p + 1
  }
  p
}
loud(g : List[Str]) : List[Str] {
  out : List[Str] = []
  g.each { r in out.push(r.concat("!")) }
  out
}
main(sys) {
  xs = [4, 8, 15]
  sys.screen.print(first(xs, 15).toStr)
  sys.screen.print(first(xs, 7).toStr)
  sys.screen.print(skip("   x", 0).toStr)
  sys.screen.print((17 % 5).toStr)
  if 3 <= 3 && 4 >= 5 || 1 < 2 { sys.screen.print("yes") }
  if 2 + 3 * 2 % 4 == 4 { sys.screen.print("rank") }
  g = loud(["a"])
  g = loud(g)
  g.each { row in sys.screen.print(row) }
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > ops.c 2>/dev/null )
if cc -o "$TMP/ops" "$TMP/ops.c" 2>/dev/null; then
    if [ "$("$TMP/ops")" = "$(printf '2\n-1\n3\n2\nyes\nrank\na!!')" ]; then ok; else nope "operators and return (got: $("$TMP/ops"))"; fi
else
    nope "operators and return compile"
fi
# a bare `return` leaves main early
printf '%s' 'main(sys) {
  sys.screen.print("one")
  if 1 < 2 {
    return
  }
  sys.screen.print("two")
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > bare.c 2>/dev/null )
if cc -o "$TMP/bare" "$TMP/bare.c" 2>/dev/null && [ "$("$TMP/bare")" = "one" ]; then ok; else nope "bare return"; fi
# an early return of the wrong type is refused
printf '%s' 'f(x : Int) : Str {
  if x > 1 {
    return 5
  }
  "a"
}
main(sys) {
  sys.screen.print(f(2))
}' > "$TMP/input.smplr"
if ( cd "$TMP" && ./seedc 2>&1 >/dev/null | grep -q "return type mismatch" ); then ok; else nope "early return type is checked"; fi
rm -f "$TMP/input.smplr"

# a name bound again with another type is a second variable
printf '%s' 'main(sys) {
  v = "12"
  v = v.toInt
  sys.screen.print(v.toStr)
}' > "$TMP/input.smplr"
if ( cd "$TMP" && ./seedc 2>&1 >/dev/null | grep -q "cannot become" ); then nope "a name may take a new type"; else ok; fi
rm -f "$TMP/input.smplr"

# Capabilities: a function reaches the screen or a file only through a value
# handed down from main. Each of these four ways around it is refused.
refused() {  # name, expected message, program
    printf '%s' "$3" > "$TMP/input.smplr"
    if ( cd "$TMP" && ./seedc 2>&1 >/dev/null | grep -q "$2" ); then ok; else nope "capability: $1"; fi
    rm -f "$TMP/input.smplr"
}
check_mut() { # description  source: it compiles, and `what` lists a function that changes what it was handed
    printf '%s' "$2" > "$TMP/input.smplr"
    local got
    got="$( cd "$TMP" && ./seedc what 2>&1 )"
    rm -f "$TMP/input.smplr"
    if printf '%s' "$got" | grep -qF 'lists   changes a list or map it was handed  (in '; then ok; else nope "$1 (got: $got)"; fi
}
refused "print with nothing handed down" "print needs a Screen" 'say() : Int !IO {
  screen.print("hi")
  0
}
main(sys) {
  say()
}'
refused "file read through a Screen" "read needs a Files" 'peek(scr : Screen) : Str !IO !Fail {
  scr.read("s.txt")
}
main(sys) {
  sys.screen.print(peek(sys.screen))
}'
refused "own function named run, dotted call on an Int" "run needs a Programs" 'run(a : Int) : Int {
  a
}
main(sys) {
  x = 5
  sys.screen.print(x.run("ls", ["-l"]))
}'
refused "own function named write, dotted call on an Int" "write needs a Files" 'write(a : Int) : Int {
  a
}
main(sys) {
  x = 5
  x.write("o.txt", "hi")
}'
check_mut "a made record passed to a function that changes what it holds" 'Ps = type { t : List[Str], p : Int }
step(ps : Ps) : Int !Mut {
  ps.t.push("x")
  0
}
mk(toks : List[Str]) : Int {
  ps = Ps(t = toks, p = 0)
  step(ps)
  ps.p
}
main(sys) {
  sys.screen.print(mk(["a"]))
}'
check_mut "a made record whose held list is changed" 'Ps = type { t : List[Str], p : Int }
mk(toks : List[Str]) : Int {
  ps = Ps(t = toks, p = 0)
  ps.t.push("x")
  ps.p
}
main(sys) {
  sys.screen.print(mk(["a"]))
}'
check_mut "a returned record still holds the caller list" 'Ps = type { t : List[Str], p : Int }
wrap(toks : List[Str]) : Ps {
  Ps(t = toks, p = 0)
}
mk(toks : List[Str]) : Int {
  p = wrap(toks)
  p.t.push("x")
  p.p
}
main(sys) {
  sys.screen.print(mk(["a"]))
}'
check_mut "an own list of handed rows passed to a function that changes the rows" 'deep(g : List[List[Str]]) : Int !Mut {
  g.at(0).push("x")
  0
}
b(rows : List[List[Str]]) : Int {
  g : List[List[Str]] = []
  g.push(rows.at(0))
  deep(g)
  g.length
}
main(sys) {
  sys.screen.print(b([["a"]]))
}'
check_mut "a returning branch that changes the caller rows it just kept" 'groups(kept : List[List[Str]], grp : Str) : Int {
  gs : List[List[List[Str]]] = []
  if grp == "" {
    gs.push(kept)
    gs.at(0).push(["x"])
    return 1
  }
  2
}
main(sys) {
  sys.screen.print(groups([["a"]], ""))
}'
check_mut "a branch that does not return leaves what it kept" 'groups(kept : List[List[Str]], grp : Str) : Int {
  gs : List[List[List[Str]]] = []
  if grp == "" {
    gs.push(kept)
  }
  gs.at(0).push(["x"])
  2
}
main(sys) {
  sys.screen.print(groups([["a"]], ""))
}'
check_mut "a record returned early still holds the caller list" 'Ps = type { t : List[Str], p : Int }
wrap(toks : List[Str], q : Int) : Ps {
  if q == 1 {
    ps = Ps(t = toks, p = 0)
    return ps
  }
  Ps(t = ["z"], p = 1)
}
mk(toks : List[Str]) : Int {
  p = wrap(toks, 1)
  p.t.push("x")
  p.p
}
main(sys) {
  sys.screen.print(mk(["a"]))
}'
check_mut "a part of a handed list still holds the caller rows" 'bad(rows : List[List[Str]]) : Int {
  ks = rows.slice(0, 1)
  ks.at(0).push("x")
  1
}
main(sys) {
  sys.screen.print(bad([["a"]]))
}'
refused "sys outside main" "exists only on main" 'peek() : Str !IO !Fail {
  sys.files.read("s.txt")
}
main(sys) {
  sys.screen.print(peek())
}'
refused "untyped parameter used as Files" "read needs a Files" 'peek(x) : Str !IO !Fail {
  x.read("s.txt")
}
main(sys) {
  sys.screen.print(peek(0))
}'

# Str-keyed Map: set/get/has, get returns 0 for an absent key (so a counter
# increments without a guard), and .keys enumerates in first-seen order.
printf '%s' 'main(sys) {
  m = Map()
  "a b a c a b".split(" ").each { w in m.set(w, m.get(w) + 1) }
  m.keys.each { k in sys.screen.print(k.concat(":").concat(m.get(k).toStr)) }
  if m.has("a") { sys.screen.print("has a") } else { sys.screen.print("no a") }
  if m.has("z") { sys.screen.print("has z") } else { sys.screen.print("no z") }
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > map.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/map" "$TMP/map.c" 2>/dev/null && [ "$("$TMP/map")" = "$(printf 'a:3\nb:2\nc:1\nhas a\nno z')" ]; then ok; else nope "Str-keyed Map (got: $("$TMP/map" 2>/dev/null))"; fi

# a Str-valued Map (Map[Str]): .get returns a real Str, so string methods
# dispatch on it (here .length), not the Int default.
printf '%s' 'main(sys) {
  dir : Map[Str] = Map()
  dir.set("alice", "1234")
  dir.set("bob", "56")
  dir.keys.each { name in sys.screen.print(name.concat("=").concat(dir.get(name))) }
  sys.screen.print(dir.get("alice").length.toStr)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > smap.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/smap" "$TMP/smap.c" 2>/dev/null && [ "$("$TMP/smap")" = "$(printf 'alice=1234\nbob=56\n4')" ]; then ok; else nope "Str-valued Map (got: $("$TMP/smap" 2>/dev/null))"; fi

# hardening: a Str-valued map returns "" (not a null pointer) for an absent
# key, so reading and measuring it is safe rather than a segfault
printf '%s' 'main(sys) {
  d : Map[Str] = Map()
  sys.screen.print("[".concat(d.get("missing")).concat("]"))
  sys.screen.print(d.get("missing").length.toStr)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > mg.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/mg" "$TMP/mg.c" 2>/dev/null && [ "$("$TMP/mg" 2>&1)" = "$(printf '[]\n0')" ]; then ok; else nope "Str-Map absent key (got: $("$TMP/mg" 2>&1))"; fi

# hardening: integer literals are 64-bit, so a product past 2^31 does not
# overflow (the literals emit with an L suffix, computed in long)
printf '%s' 'main(sys) { sys.screen.print((1000000 * 1000000).toStr) }' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > big.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/big" "$TMP/big.c" 2>/dev/null && [ "$("$TMP/big")" = "1000000000000" ]; then ok; else nope "64-bit int literals (got: $("$TMP/big" 2>/dev/null))"; fi

# wordfreq.smplr, a real word-frequency tool built by the self-hosted compiler:
# read + replace + split + Map tally + .keys enumeration, the program a map is
# for. Exercises the whole text-and-map stack in one binary.
cp selfhost/wordfreq.smplr "$TMP/input.smplr"
( cd "$TMP" && ./seedc > wf.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/wf" "$TMP/wf.c" 2>/dev/null; then
    # ranked by descending count: apple is seen first but ranks last; the two
    # count-3 words keep first-seen order (pie before tart), a stable sort
    printf 'apple pie\npie pie\ntart tart tart\n' > "$TMP/prose.txt"
    want="$(printf 'pie: 3\ntart: 3\napple: 1')"
    # a real Unix filter: same result from a file argument or from stdin
    if [ "$("$TMP/wf" "$TMP/prose.txt")" = "$want" ]; then ok; else nope "wordfreq from file (got: $("$TMP/wf" "$TMP/prose.txt"))"; fi
    if [ "$(cat "$TMP/prose.txt" | "$TMP/wf")" = "$want" ]; then ok; else nope "wordfreq from stdin"; fi
    rm -f "$TMP/prose.txt"
else
    nope "wordfreq compiles"
fi

# List .sort, on both element types, and the empty-List[Str] typing it relies
# on: an ascribed `xs : List[Str] = []` must carry its element type so .each
# and print treat the items as strings, not as the raw pointers.
printf '%s' 'main(sys) {
  words : List[Str] = []
  words.push("pear")
  words.push("apple")
  words.push("kiwi")
  words.sort.each { w in sys.screen.print(w) }
  nums : List[Int] = []
  nums.push(3)
  nums.push(1)
  nums.push(2)
  nums.sort.each { n in sys.screen.print(n.toStr) }
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > sort.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/sort" "$TMP/sort.c" 2>/dev/null && [ "$("$TMP/sort")" = "$(printf 'apple\nkiwi\npear\n1\n2\n3')" ]; then ok; else nope "List.sort / empty-List[Str] typing (got: $("$TMP/sort" 2>/dev/null))"; fi

# sortlines.smplr, the Unix `sort` in miniature, built by the self-hosted
# compiler: read a file or stdin, sort the lines, print them.
cp selfhost/sortlines.smplr "$TMP/input.smplr"
( cd "$TMP" && ./seedc > sl.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/sl" "$TMP/sl.c" 2>/dev/null; then
    printf 'banana\napple\ncherry\n' > "$TMP/names.txt"
    swant="$(printf 'apple\nbanana\ncherry')"
    if [ "$("$TMP/sl" "$TMP/names.txt")" = "$swant" ] && [ "$(cat "$TMP/names.txt" | "$TMP/sl")" = "$swant" ]; then ok; else nope "sortlines"; fi
    rm -f "$TMP/names.txt"
else
    nope "sortlines compiles"
fi

# Float: literals, the four arithmetic ops, ordering, direct print, and the
# Int<->Float<->Str conversions (.toFloat / .toInt / .toStr, "3.14".toFloat).
printf '%s' 'main(sys) {
  a = 3.5
  b = 1.25
  sys.screen.print((a + b).toStr)
  sys.screen.print((a / b).toStr)
  if a > b { sys.screen.print("bigger") }
  sys.screen.print(a)
  sys.screen.print((7.toFloat / 2.0).toStr)
  sys.screen.print("3.14".toFloat.toStr)
  sys.screen.print(b.toInt.toStr)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > flt.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/flt" "$TMP/flt.c" 2>/dev/null && [ "$("$TMP/flt")" = "$(printf '4.75\n2.8\nbigger\n3.5\n3.5\n3.14\n1')" ]; then ok; else nope "Float ops (got: $("$TMP/flt" 2>/dev/null))"; fi

# the Float math layer (libm wrappers): sin/cos/sqrt/abs/floor and atan2. A
# program that uses them links with -lm; ordinary programs and the compiler
# itself do not.
printf '%s' 'main(sys) {
  pi = 3.14159265358979
  sys.screen.print((pi / 2.0).sin.toStr)
  sys.screen.print(2.0.sqrt.toStr)
  sys.screen.print((0.0 - 3.5).abs.toStr)
  sys.screen.print(3.7.floor.toStr)
  sys.screen.print(1.0.atan2(1.0).toStr)
  sys.screen.print((370.5 % 360.0).toStr + " " + (-10.0 % 360.0).toStr + " " + 2.0.pow(10.0).toStr + " " + 1.0.atan.toStr)
  sys.screen.print(2.5.round.toStr + " " + 2.1.ceil.toStr + " " + 1.0.exp.ln.toStr)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > math.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/math" "$TMP/math.c" -lm 2>/dev/null && [ "$("$TMP/math")" = "$(printf '1\n1.4142135623731\n3.5\n3\n0.785398163397448\n10.5 -10 1024 0.785398163397448\n3 3 1')" ]; then ok; else nope "Float math layer (got: $("$TMP/math" 2>/dev/null))"; fi

# scientific-notation Float literals (1.5e3, 2.0e-3, 1.151e-9), and that toStr
# prints a large value in full rather than collapsing to %g exponential.
printf '%s' 'main(sys) {
  sys.screen.print((1.5e3).toStr)
  sys.screen.print((2.0e-3).toStr)
  sys.screen.print((1.151e-9 * 1000000000.0).toStr)
  sys.screen.print((2461119.5).toStr)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > sci.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/sci" "$TMP/sci.c" 2>/dev/null && [ "$("$TMP/sci")" = "$(printf '1500\n0.002\n1.151\n2461119.5')" ]; then ok; else nope "Float sci literals / toStr precision (got: $("$TMP/sci" 2>/dev/null))"; fi

# sys.files.append: write truncates, append adds to the end. A program that
# writes once then appends twice should leave all three lines in the file.
printf '%s' 'reads "af.txt"
writes "af.txt"
main(sys) {
  sys.files.write("af.txt", "first\n")
  sys.files.append("af.txt", "second\n")
  sys.files.append("af.txt", "third\n")
  sys.screen.print(sys.files.read("af.txt")?)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > af.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/af" "$TMP/af.c" 2>/dev/null; then
    out="$( cd "$TMP" && ./af )"
    if [ "$out" = "$(printf 'first\nsecond\nthird')" ]; then ok; else nope "sys.files.append (got: $out)"; fi
    rm -f "$TMP/af.txt"
else
    nope "append compiles"
fi

# orbit.smplr, an ephemeris ported from the Fe2O3 Rust `orbit` library by an AI
# (sun rise/set, moon phase, Julian date). Verified against orbit's own test
# values: equator equinox day length ~12h, moon full on 2025-01-13, new on
# 2025-01-29. Exercises the whole Float math layer; links with -lm.
cp selfhost/orbit.smplr "$TMP/input.smplr"
( cd "$TMP" && ./seedc > orbit.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/orbit" "$TMP/orbit.c" -lm 2>/dev/null; then
    o="$("$TMP/orbit")"
    if printf '%s' "$o" | grep -q "sunrise 06:04, sunset 18:11" \
       && printf '%s' "$o" | grep -q "2025-01-13.*(~98%)" \
       && printf '%s' "$o" | grep -q "2025-01-29.*(~0%)"; then ok; else nope "orbit ephemeris (got: $o)"; fi
else
    nope "orbit compiles"
fi

# .each evaluates its receiver once: iterating sys.stdin.split(...) directly
# must see every field, not consume stdin on the first re-evaluation. Also
# checks nested .each (the temps shadow cleanly).
printf '%s' 'main(sys) {
  sys.stdin.split(" ").each { w in
    w.split(",").each { c in sys.screen.print(c) }
  }
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > each.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/each" "$TMP/each.c" 2>/dev/null && [ "$(printf 'a,b c,d' | "$TMP/each")" = "$(printf 'a\nb\nc\nd')" ]; then ok; else nope "each evaluates receiver once (got: $(printf 'a,b c,d' | "$TMP/each" 2>/dev/null))"; fi

# prefix minus: negative Int and Float literals, and negation inside an
# expression, for both scalar types (binary `-` is unaffected).
printf '%s' 'main(sys) {
  a = -3
  b = -3.5
  sys.screen.print(a.toStr)
  sys.screen.print(b.toStr)
  sys.screen.print((a * -2).toStr)
  sys.screen.print((b + -1.5).toStr)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > neg.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/neg" "$TMP/neg.c" 2>/dev/null && [ "$("$TMP/neg")" = "$(printf -- '-3\n-3.5\n6\n-5')" ]; then ok; else nope "prefix minus (got: $("$TMP/neg" 2>/dev/null))"; fi

# average.smplr, a real filter that needs fractions: mean of the numbers on
# its input. Exercises Float sum, Int count, and a Float division.
cp selfhost/average.smplr "$TMP/input.smplr"
( cd "$TMP" && ./seedc > avg.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/avg" "$TMP/avg.c" 2>/dev/null; then
    printf '1.5\n2.5\n3\n4.0\n' > "$TMP/data.txt"
    if [ "$("$TMP/avg" "$TMP/data.txt")" = "2.75" ] && [ "$(cat "$TMP/data.txt" | "$TMP/avg")" = "2.75" ]; then ok; else nope "average mean"; fi
    printf '' | "$TMP/avg" >/dev/null 2>&1; if [ "$?" -ne 0 ]; then ok; else nope "average empty exits nonzero"; fi
    rm -f "$TMP/data.txt"
else
    nope "average compiles"
fi

# numstats.smplr, written by an AI from SPEC.md alone (no sight of the compiler
# or the other examples) and shipped unchanged: count/min/max/mean of the
# numbers on its input. The round-trip proof that the spec is a sufficient
# corpus. Compiled and run here like any other tool.
cp selfhost/numstats.smplr "$TMP/input.smplr"
( cd "$TMP" && ./seedc > ns.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/ns" "$TMP/ns.c" 2>/dev/null; then
    if [ "$(printf '3\n3.5\n-2.25\n10\n' | "$TMP/ns")" = "$(printf 'count: 4\nmin: -2.25\nmax: 10\nmean: 3.5625')" ]; then ok; else nope "numstats stats (got: $(printf '3\n3.5\n-2.25\n10\n' | "$TMP/ns"))"; fi
    printf '' | "$TMP/ns" >/dev/null 2>&1; if [ "$?" -ne 0 ]; then ok; else nope "numstats empty exits nonzero"; fi
else
    nope "numstats compiles"
fi

# sumcol.smplr, a real CSV column-summer, built by the self-hosted compiler:
# reads a file, splits into lines, splits each on commas, sums one column.
# Exercises read + split + toInt + nested each/if with a binding inside the
# loop body (the case that needs hoisting through .each).
cp selfhost/sumcol.smplr "$TMP/input.smplr"
( cd "$TMP" && ./seedc > sc.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/sc" "$TMP/sc.c" 2>/dev/null; then
    printf 'a,10,x\nb,20,y\nc,3,z\nd,9,w\n' > "$TMP/data.csv"
    if [ "$("$TMP/sc" "$TMP/data.csv" 1)" = "42" ] && [ "$("$TMP/sc" "$TMP/data.csv" 5)" = "0" ]; then ok; else nope "sumcol column sum"; fi
    # misuse fails with a nonzero exit (fail: stderr + exit 1)
    "$TMP/sc" >/dev/null 2>&1; if [ "$?" -ne 0 ]; then ok; else nope "sumcol misuse exits nonzero"; fi
    rm -f "$TMP/data.csv"
else
    nope "sumcol compiles"
fi

# the self-hosted compiler now rejects programs the Rust rejects.
reject() { # description  source  expected_substring
    printf '%s' "$2" > "$TMP/input.smplr"
    local err rc
    err="$( cd "$TMP" && ./seedc 2>&1 >/dev/null )"; rc=$?
    rm -f "$TMP/input.smplr"
    if [ "$rc" -ne 0 ] && printf '%s' "$err" | grep -qF "$3"; then ok; else nope "self-host reject: $1 (rc=$rc: $err)"; fi
}
# a match missing a case
reject "non-exhaustive match" \
    'Color = type { Red Green Blue }
name(c : Color) : Int { c.match { Red -> 1 Green -> 2 } }
main(sys) { sys.screen.print(name(Red)) }' \
    "non-exhaustive match"
# arithmetic on a non-Int operand
reject "Int operands" \
    'main(sys) { x = 1 + "a" sys.screen.print(x) }' \
    "+ adds two numbers or joins two texts"
# a function argument of the wrong type
reject "argument type" \
    'greet(s : Str) : Int { 0 }
main(sys) { sys.screen.print(greet(5)) }' \
    "expects Str, got Int"
# a record field of the wrong type
reject "field type" \
    'Point = type { x : Int, y : Int }
main(sys) { p = Point(x = 1, y = "a") sys.screen.print(p.x) }' \
    "expects Int, got Str"
# a function whose body has the wrong return type
reject "return type" \
    'twice(n : Int) : Int { "no" }
main(sys) { sys.screen.print(twice(2)) }' \
    "return type mismatch"
# a match arm that binds the wrong number of payloads
reject "binding count" \
    'Expr = type { Num(Int) Add(Expr, Expr) }
ev(e : Expr) : Int { e.match { Num(n) -> n  Add(a) -> 1 } }
main(sys) { sys.screen.print(ev(Num(1))) }' \
    "binds 1, expected 2"
# errors carry a source location (the enclosing function's line)
reject "located error" \
    'Color = type { Red Green }
name(c : Color) : Int { c.match { Red -> 1 } }
main(sys) { sys.screen.print(name(Red)) }' \
    "input.smplr:2:"
# uses an effect (IO) without declaring it
reject "undeclared effect" \
    'greet(screen : Screen) { screen.print("hi") }
main(sys) { greet(sys.screen) }' \
    "uses !IO but does not declare it"
# transitively uses an effect (calls an !IO function) without declaring it
reject "transitive effect" \
    'a(s : Screen) !IO { s.print("x") }
b(s : Screen) { a(s) }
main(sys) { b(sys.screen) }' \
    "uses !IO but does not declare it"
# a raw text: its lines count, so an error after one names the right line
reject "line after a raw text" \
    'main(sys) {
  a = <<END
x
END
  b = 1 + "a"
}' \
    "input.smplr:5: + adds two numbers"
reject "raw text with no end word" \
    'main(sys) {
  a = <<END
x
}' \
    "has no line with END alone to end it"
reject "raw text start that does not end its line" \
    'main(sys) {
  a = <<END x
END
}' \
    "<<END must end its line"
reject "raw text line left of its end word" \
    'main(sys) {
  a = <<END
  x
y
  END
}' \
    "input.smplr:4: this line of the <<END text starts left of its end word"
# a read that can fail needs no !Fail mark: the compiler works that out
printf 'reads "p"\nrd(f : Files) : Str !IO {\n  f.read("p")?\n}\nmain(sys) {\n  sys.screen.print(rd(sys.files))\n}\n' > "$TMP/nofail.smplr"
if ( cd "$TMP" && cp nofail.smplr input.smplr && ./seedc 2>&1 >/dev/null | grep -q . ); then nope "a read without !Fail compiles"; else ok; fi
# writing a file is an !IO effect that must be declared
reject "write needs IO" \
    'save(f : Files) { f.write("p", "x") }
main(sys) { save(sys.files) }' \
    "uses !IO but does not declare it"
# Int and Float cannot be mixed in arithmetic without a conversion
reject "mixed Int/Float" \
    'main(sys) { x = 3.5 y = 2 sys.screen.print((x + y).toStr) }' \
    "mixes Int and Float"

# --- 2. known-bad programs are rejected with the right message ----------------
check_err() { # description  source  expected_substring
    printf '%s' "$2" > "$TMP/input.smplr"
    local err rc
    err="$( cd "$TMP" && ./seedc 2>&1 >/dev/null )"; rc=$?
    rm -f "$TMP/input.smplr"
    if [ "$rc" -ne 0 ] && printf '%s' "$err" | grep -qF "$3"; then
        ok
    else
        nope "$1 (rc=$rc, got: $err)"
    fi
}
check_err "undeclared effect" \
    'main(sys) { greet(sys.screen) }
greet(screen : Screen) { screen.print("hi") }' \
    "uses !IO but does not declare it"
check_err "capability not held" \
    'main(sys) { greet(sys.screen) }
greet(screen : Screen) !IO { sys.screen.print("hi") }' \
    "exists only on main"
check_err "print on a non-Screen" \
    'main(sys) { greet("x") }
greet(s : Str) !IO { s.print("hi") }' \
    "print needs a Screen"
check_err "wrong argument type" \
    'main(sys) { greet(sys.screen, 5) }
greet(screen : Screen, who : Str) !IO { screen.print(who) }' \
    "expects Str, got Int"
check_err "Int op needs Ints" \
    'main(sys) { x = 1 + "a" sys.screen.print(x) }' \
    "+ adds two numbers or joins two texts"
check_err "wrong return type" \
    'twice(n : Int) : Int { "no" }
main(sys) { sys.screen.print(twice(2)) }' \
    "return type mismatch"
check_err "assert needs Bool" \
    'main(sys) { assert(5) }' \
    "needs a Bool"
check_err "no main" \
    'twice(n : Int) : Int { n + n }' \
    "no main function found"
# every refusal names the line of the statement and what stood there
check_err "error names the statement's line" \
    'main(sys) {
  a = 1
  b = 2
  sys.screen.print(c)
}' \
    "input.smplr:4: no variable named c"
check_err "missing closing brace" \
    'main(sys) {
  sys.screen.print(1)' \
    "input.smplr:2: expected }, found the end of the file"
check_err "missing closing paren" \
    'main(sys) {
  x = (1 + 2
  sys.screen.print(x)
}' \
    "input.smplr:3: expected ), found sys"
check_err "stray mark" \
    'main(sys) {
  x = 1 ; y = 2
}' \
    "input.smplr:2: expected a value, found ;"
check_err "open text" \
    'main(sys) {
  sys.screen.print("hi)
}' \
    "input.smplr:2: a text is missing its closing quote"
check_err "value alone on a line" \
    'main(sys) {
  x = 1
  x + 1
  sys.screen.print(x)
}' \
    "input.smplr:3: this line does nothing"
check_err "unknown method" \
    'main(sys) {
  s = "a"
  sys.screen.print(s.shout(1))
}' \
    "input.smplr:3: no method named shout"
check_err "wrong number of values" \
    'twice(n : Int) : Int { n + n }
main(sys) {
  sys.screen.print(twice(1, 2))
}' \
    "input.smplr:3: twice takes 1 values, got 2"
check_err "name given twice" \
    'twice(n : Int) : Int { n + n }
twice(n : Int) : Int { n }
main(sys) { sys.screen.print(twice(1)) }' \
    "input.smplr:2: the name twice is used twice"
check_err "a match name ends with its arm" \
    'Shape = type {
  Dot
  Box(Int)
}
main(sys) {
  b = Box(7)
  x = b.match {
    Dot -> 1
    Box(n) -> n
  }
  sys.screen.print(n)
}' \
    "input.smplr:11: no variable named n"
# `simpler what` lists everything a program can touch
printf '%s' 'reads "notes.txt"
writes "copy.txt"
save(files : Files, name : Str, text : Str) : Int !IO {
  files.write(name.concat(".txt"), text)
  0
}
show(out : Screen, text : Str) : Int !IO {
  out.print(text)
  0
}
main(sys) {
  text = sys.files.read("notes.txt")?
  show(sys.screen, text)
  save(sys.files, "copy", text)
}' > "$TMP/input.smplr"
want='screen  prints  (in show)
files   writes a path made while running  (in save)
files   reads "notes.txt"  (in main)
program not used
net     not used
clock   not used
env     not used
term    not used
mail    not used
stdin   not used
args    not used
may read   "notes.txt"
may write  "copy.txt"
nothing else: a program reaches the world only through what is listed here'
if [ "$( cd "$TMP" && ./seedc what 2>&1 )" = "$want" ]; then ok; else nope "simpler what: $( cd "$TMP" && ./seedc what 2>&1 )"; fi
# a second name for main's world still compiles, and what still sees its uses
printf '%s' 'main(sys) {
  world = sys
  world.screen.print("hi")
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > world.c 2>/dev/null )
if cc -o "$TMP/world" "$TMP/world.c" 2>/dev/null && [ "$("$TMP/world")" = "hi" ] && ( cd "$TMP" && ./seedc what | grep -q "screen  prints" ); then ok; else nope "a second name for sys"; fi
# it lists nothing for a program the compiler refuses
printf '%s' 'sneak(n : Int) : Int { sys.files.write("x", "y") }
main(sys) { sneak(1) }' > "$TMP/input.smplr"
if ( cd "$TMP" && ./seedc what >/dev/null 2>&1 ); then nope "simpler what on a refused program"; else ok; fi
# a program says at the top which files it may touch; anything else is refused or stopped
check_err "a file read with no reads line" \
    'main(sys) {
  sys.screen.print(sys.files.read("a.txt")?)
}' 'input.smplr:2: this program may not read a file; say which at the top, as in: reads "notes.txt"'
check_err "a file read that the reads line does not name" \
    'reads "a.txt"
main(sys) {
  sys.screen.print(sys.files.read("s.txt")?)
}' 'input.smplr:3: this program may not read "s.txt"; add it to the reads line at the top'
check_err "a write with only a reads line" \
    'reads "a.txt"
main(sys) {
  sys.files.write("a.txt", "x")
}' 'this program may not write a file; say which at the top, as in: writes "notes.txt"'
printf '%s' 'reads "./"
main(sys) {
  sys.screen.print(sys.files.read("here.txt")?)
  sys.screen.print(sys.files.read(sys.args.at(0))?)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > dot.c 2>/dev/null && cc -o dot dot.c 2>/dev/null && echo h > here.txt && mkdir -p sub && echo s > sub/s.txt )
if [ "$( cd "$TMP" && ./dot sub/s.txt 2>&1 | tr -d '\n' )" = "hs" ]; then ok; else nope "a bare name under ./"; fi
if ( cd "$TMP" && ./dot /etc/hostname 2>&1 | grep -q 'may not read "/etc/hostname"' ) && ( cd "$TMP" && ./dot sub/../../x 2>&1 | grep -q 'may not read' ); then ok; else nope "./ does not reach the root or climb out"; fi
mkdir -p "$TMP/lk/data" "$TMP/lk/out" "$TMP/lk/far"
echo top > "$TMP/lk/far/s.txt"; echo fine > "$TMP/lk/data/ok.txt"
ln -s ../far/s.txt "$TMP/lk/data/leak.txt"; ln -s ../far "$TMP/lk/data/dir"; ln -s ../far/new.txt "$TMP/lk/out/dangling"; ln -s ok.txt "$TMP/lk/data/inner.txt"
printf '%s' 'reads "data/"
writes "out/"
main(sys) {
  sys.screen.print(sys.files.read("data/inner.txt")?.trim)
  w = sys.args.at(0)
  if w == "a" { sys.screen.print(sys.files.read("data/leak.txt")?) }
  if w == "b" { sys.screen.print(sys.files.read("data/dir/s.txt")?) }
  if w == "c" { sys.files.write("out/dangling", "x") }
  if w == "d" { sys.files.write("out/new.txt", "x") }
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > lk.c 2>/dev/null && cc -w -o lk/p lk.c 2>/dev/null )
lkout="$( cd "$TMP/lk" && for a in a b c d; do ./p $a 2>&1; done | tr '\n' '|' )"
lkwant='input.smplr:6: this program may not read "data/leak.txt": a link leads out of "data/"|fine|input.smplr:7: this program may not read "data/dir/s.txt": a link leads out of "data/"|fine|input.smplr:8: this program may not write "out/dangling": a link leads out of "out/"|fine|fine|'
if [ "$lkout" = "$lkwant" ] && [ ! -e "$TMP/lk/far/new.txt" ] && [ -e "$TMP/lk/out/new.txt" ]; then ok; else nope "a link out of an allowed folder is not followed (got: $lkout)"; fi
# folders: list, exists, rename and delete stay inside the reads and writes lines
printf '%s' 'reads "box/"
writes "box/"
main(sys) {
  sys.files.write("box/b.txt", "1")
  sys.files.write("box/a.txt", "2")
  sys.files.list("box").each { n in
    sys.files.rename("box/".concat(n), "box/x-".concat(n))
  }
  sys.files.delete("box/x-a.txt")
  sys.files.list("box/").each { n in sys.screen.print(n) }
  sys.screen.print(sys.files.exists("box/x-a.txt"))
  sys.screen.print(sys.files.list("box/none").length)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > box.c 2>/dev/null && cc -o boxp box.c 2>/dev/null && mkdir -p box )
if [ "$( cd "$TMP" && ./boxp 2>&1 | tr '\n' ' ' )" = "x-b.txt false 0 " ]; then ok; else nope "list, rename, delete and exists in a folder (got: $( cd "$TMP" && ./boxp 2>&1 | tr '\n' ' ' ))"; fi
printf '%s' 'reads "tree/"
writes "tree/"
main(sys) {
  sys.files.list("tree").each { n in sys.screen.print(n) }
  sys.files.delete("tree/".concat(sys.args.at(0)))
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > tree.c 2>/dev/null && cc -o treep tree.c 2>/dev/null && mkdir -p tree/sub && echo f > tree/f.txt && echo keep > keep.txt )
if [ "$( cd "$TMP" && ./treep ../keep.txt 2>/dev/null | tr '\n' ' ' )" = 'f.txt sub/ ' ] && [ "$( cd "$TMP" && ./treep ../keep.txt 2>&1 >/dev/null )" = 'input.smplr:5: this program may not write "tree/../keep.txt"' ] && [ -e "$TMP/keep.txt" ]; then ok; else nope "a folder ends in /, and a delete may not climb out"; fi
if ( cd "$TMP" && ./treep none 2>&1 | grep -q 'input.smplr:5: could not delete "tree/none"' ); then ok; else nope "a delete that fails stops the program"; fi
check_err "listing a folder the reads line does not name" \
    'reads "box/"
main(sys) {
  sys.screen.print(sys.files.list("/etc").length)
}' 'input.smplr:3: this program may not list "/etc"; add it to the reads line at the top'
check_err "a delete needs !IO" \
    'writes "box/"
wipe(f : Files) : Int { f.delete("box/x")
  0 }
main(sys) { wipe(sys.files) }' "uses !IO but does not declare it"
# the forms models write from habit: != and !, + on texts, single quotes, for, startsWith
echo 'is_lower(c : Str) : Bool { c >= '"'a'"' && c <= '"'z'"' }
main(sys) {
  xs = [3, 1, 2]
  total = 0
  for x in xs { total = total + x }
  for i in 1..=3 { total = total + i }
  for i in 0..2 { total = total + 100 }
  s = "ab" + "cd" + total.toStr
  sys.screen.print(s)
  if total != 5 && !(s == "x") { sys.screen.print("ne") }
  if !is_lower("A") && is_lower("q") { sys.screen.print("low") }
  sys.screen.print(s.startsWith("ab") && s.endsWith("212") && !s.startsWith("b"))
  for w in "a b".split(" ") { sys.screen.print(w + "!") }
  sys.screen.print("b" < "a")
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > habits.c 2>/dev/null )
if cc -o "$TMP/habits" "$TMP/habits.c" 2>/dev/null && [ "$("$TMP/habits" | tr '\n' ' ')" = "abcd212 ne low true a! b! false " ]; then ok; else nope "the forms models write from habit"; fi
check_err "a text plus a number" \
    'main(sys) { sys.screen.print("n " + 3) }' "turn the number into text with .toStr"
check_err "for is no name for a variable" \
    'main(sys) {
  for = 3
}' "input.smplr:2: expected a name"
check_mut "a for loop that changes a handed list needs !Mut" \
    'add(xs : List[Int]) : Int { for i in 0..3 { xs.push(i) }
  0 }
main(sys) { add([1]) }'
echo 'main(sys) {
  n = 0
  for c in "a1b22" {
    if c >= "0" && c <= "9" { n = n + 1 }
  }
  "x,y".each { c in sys.screen.print(c) }
  sys.screen.print(n)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > letters.c 2>/dev/null )
if cc -o "$TMP/letters" "$TMP/letters.c" 2>/dev/null && [ "$("$TMP/letters" | tr '\n' ' ')" = "x , y 3 " ]; then ok; else nope "a loop over a text gives its letters"; fi
check_err "a loop over a number" \
    'main(sys) {
  for i in 5 { sys.screen.print(i) }
}' "input.smplr:2: a loop goes over a list or the letters of a text, not Int"
check_err "a loop over a map" \
    'main(sys) {
  m = Map()
  m.each { k in sys.screen.print(k) }
}' "a loop goes over a list or the letters of a text, not Map"
# every program on the one page a model gets must compile
awk -v d="$TMP" '/^```/ { if (inb) { inb = 0; close(f) } else { inb = 1; n++; f = d "/page" n ".smplr" } next } inb { print > f }' PROMPT.md
pagebad=0
for f in "$TMP"/page*.smplr; do
  grep -q 'main(sys)' "$f" || continue
  cp "$f" "$TMP/input.smplr"
  ( cd "$TMP" && ./seedc > page.c 2>/dev/null && cc -w -o page page.c -lm 2>/dev/null ) || pagebad=1
done
if [ "$pagebad" = 0 ]; then ok; else nope "the programs in PROMPT.md compile"; fi
cp /dev/null "$TMP/input.smplr"; cat > "$TMP/input.smplr" <<'SMPLR'
drop(xs : List[Int]) : Int !Mut { xs.remove(0) }
main(sys) {
  xs = [1, 2, 3, 4]
  xs.set(1, 20)
  top = xs.remove(xs.length - 1)
  drop(xs)
  for i in 0..10 {
    if i == 1 { continue }
    if i == 3 { break }
    sys.screen.print(i)
  }
  n = 0
  while true {
    n = n + 1
    if n > 3 { break }
  }
  sys.screen.print(top + n)
  for x in xs { sys.screen.print(x) }
}
SMPLR
( cd "$TMP" && ./seedc > jump.c 2>/dev/null )
if cc -o "$TMP/jump" "$TMP/jump.c" 2>/dev/null && [ "$("$TMP/jump" | tr '\n' ' ')" = "0 2 8 20 3 " ]; then ok; else nope "break, continue, and set and remove on a list"; fi
check_err "break outside a loop" \
    'main(sys) {
  x = 1
  if x == 1 { break }
}' "input.smplr:3: break is only for inside a loop"
check_mut "remove on a handed list needs !Mut" \
    'drop(xs : List[Int]) : Int { xs.remove(0) }
main(sys) { drop([1]) }'
check_err "set on a list with the wrong type" \
    'main(sys) {
  xs = [1]
  xs.set(0, "a")
}' "input.smplr:3: set expects Int, got Str"
cat > "$TMP/input.smplr" <<'SMPLR'
runs "echo", "sh"
stamp(c : Clock) : Str { c.date(0).slice(0, 2) }
say(p : Programs, w : Str) : Str !IO { p.run("echo", [w, "b c"]).trim }
main(sys) {
  sys.screen.print(say(sys.programs, "a"))
  sys.screen.print(sys.programs.status("sh", ["-c", "exit 3"]))
  sys.screen.print(sys.clock.now > 1700000000)
  sys.screen.print(stamp(sys.clock))
  sys.screen.print(sys.env.var("NO_SUCH_VAR_X") == "")
  sys.programs.run("e" + "nv", [])
}
SMPLR
( cd "$TMP" && ./seedc > runs.c 2>/dev/null && cc -w -o runs runs.c 2>/dev/null )
if [ "$( cd "$TMP" && ./runs 2>/dev/null | tr '\n' ' ' )" = "a b c 3 true 19 true " ] && [ "$( cd "$TMP" && ./runs 2>&1 >/dev/null )" = 'input.smplr:10: this program may not run "env"' ]; then ok; else nope "running a program, the clock and a variable"; fi
printf '%s' 'runs "sh"
main(sys) { sys.programs.run("sh", ["-c", "exit 2"]) }' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > runs.c 2>/dev/null && cc -w -o runs runs.c 2>/dev/null )
if [ "$( cd "$TMP" && ./runs 2>&1 )" = 'input.smplr:2: "sh" ended with status 2' ]; then ok; else nope "a program that fails stops the run"; fi
printf '%s' 'fetches "example.com"
main(sys) {
  u = "https://example.com" + "@evil.example/"
  sys.net.fetch(u)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > runs.c 2>/dev/null && cc -w -o runs runs.c 2>/dev/null )
if [ "$( cd "$TMP" && ./runs 2>&1 )" = 'input.smplr:4: this program may not fetch "https://example.com@evil.example/"' ]; then ok; else nope "a host made while running is checked"; fi
printf '%s' 'runs "tr", "sort"
fetches "nohost.invalid"
main(sys) {
  sys.screen.print(sys.programs.feed("tr", ["a-z", "A-Z"], "hello\n").trim)
  sys.screen.print(sys.programs.feed("sort", [], "b\na\n").replace("\n", " "))
  sys.screen.print(sys.net.reach("http://nohost.invalid/"))
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > feed.c 2>/dev/null && cc -w -o feed feed.c 2>/dev/null )
if [ "$( cd "$TMP" && ./feed 2>&1 | tr '\n' '|' )" = "HELLO|a b |0|" ]; then ok; else nope "feeding text to a program, and a host that does not answer"; fi
printf '%s' 'runs "sh"
main(sys) {
  sys.screen.print(sys.programs.output("sh", ["-c", "cat; exit 3"], "kept\n").trim)
  sys.screen.print("after")
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > output.c 2>/dev/null && cc -w -o output output.c 2>/dev/null )
if [ "$( cd "$TMP" && ./output 2>&1 | tr '\n' '|' )" = "kept|after|" ]; then ok; else nope "what a failing program printed is kept"; fi
printf '%s' 'main(sys) {
  s = "key=Value=x"
  p = s.find("=")
  sys.screen.print(p)
  sys.screen.print(s.find("zz"))
  sys.screen.print(s.slice(0, p).upper + s.lower)
  sys.screen.print(["a", "b", "c"].join(", "))
  e : List[Str] = []
  sys.screen.print("[" + e.join("-") + "]")
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > find.c 2>/dev/null && cc -w -o find find.c 2>/dev/null )
if [ "$( cd "$TMP" && ./find 2>&1 | tr '\n' '|' )" = "3|-1|KEYkey=value=x|a, b, c|[]|" ]; then ok; else nope "find, upper, lower and join"; fi
printf '%s' 'reads "./"
main(sys) {
  sys.screen.print(sys.files.read("input.smplr")?.split("\n").at(0).trim)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > qm.c 2>/dev/null && cc -w -o qm qm.c 2>/dev/null )
if [ "$( cd "$TMP" && ./qm 2>&1 )" = 'reads "./"' ]; then ok; else nope "a method after the question mark"; fi
printf '%s' 'show(t : Term, k : Str) {
  t.put(0, 0, "key " + k, "bold red")
}
main(sys) {
  t = sys.term
  show(t, "none")
  k = t.key
  t.stop
  sys.screen.print("got " + k + " " + t.cols.toStr)
  t.put(0, 0, "x", "pink")
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > term.c 2>/dev/null && cc -w -o term term.c 2>/dev/null )
if command -v script >/dev/null; then
    got="$( cd "$TMP" && printf '\033[A' | script -qec ./term /dev/null 2>&1 | tr -d '\r' )"
    case "$got" in *"got up 80"*'no style named "pink"'*) ok ;; *) nope "the terminal: a key, a stop, and an error shown after it closes" ;; esac
fi
check_err "a Term has no other method" \
    'main(sys) { sys.term.print("x") }' 'a Term has key, rows, cols, clear, stop and put(row, col, text, style)'
printf '%s' 'mails "alice@example.com", "@team.example"
main(sys) {
  sys.mail.send("alice@example.com", "Report", "All 3 jobs ran.")
  sys.mail.send(sys.args.at(0), "Hi", "body")
}' > "$TMP/input.smplr"
printf '#!/bin/sh\necho "args: $*" >> sent.txt\ncat >> sent.txt\n' > "$TMP/fakemail"; chmod +x "$TMP/fakemail"
( cd "$TMP" && ./seedc > ml.c 2>/dev/null && cc -w -o ml ml.c 2>/dev/null )
mlout="$( cd "$TMP" && rm -f sent.txt; for a in bob@team.example eve@evil.example 'bob@team.example,eve@evil.example' 'eve@evil.example@team.example' '-oQ@team.example'; do SIMPLER_SENDMAIL=./fakemail ./ml "$a" 2>&1; done | tr '\n' '|'; grep -c '^args: -i -- bob@team.example$' sent.txt; grep -c evil sent.txt )"
mlwant='input.smplr:4: this program may not mail "eve@evil.example"|input.smplr:4: this program may not mail "bob@team.example,eve@evil.example"|input.smplr:4: this program may not mail "eve@evil.example@team.example"|input.smplr:4: this program may not mail "-oQ@team.example"|1
0'
if [ "$mlout" = "$mlwant" ]; then ok; else nope "mail goes only to the addresses at the top (got: $mlout)"; fi
check_err "mail with no mails line" \
    'main(sys) { sys.mail.send("a@b.c", "s", "b") }' 'say to whom at the top, as in: mails "me@example.com"'
check_err "mail to an address the line does not name" \
    'mails "x@b.c"
main(sys) { sys.mail.send("a@b.c", "s", "b") }' 'this program may not mail "a@b.c"; add it to the mails line at the top'
mkdir -p "$TMP/gl/logs/sub"; echo A > "$TMP/gl/logs/a.log"; echo B > "$TMP/gl/logs/b.txt"; echo C > "$TMP/gl/logs/sub/c.log"
printf '%s' 'reads "logs/*.log"
mails "*@example.com"
main(sys) {
  w = sys.args.at(0)
  if w.contains("@") {
    sys.mail.send(w, "s", "b")
    sys.screen.print("mailed")
  } else {
    sys.screen.print(sys.files.read(w)?.trim)
  }
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > gl.c 2>/dev/null && cc -w -o gl/p gl.c 2>/dev/null )
glout="$( cd "$TMP/gl" && for a in logs/a.log logs/b.txt logs/sub/c.log me@example.com me@example.com.evil.test; do SIMPLER_SENDMAIL=../fakemail ./p "$a" 2>&1; done | tr '\n' '|' )"
glwant='A|input.smplr:9: this program may not read "logs/b.txt"|input.smplr:9: this program may not read "logs/sub/c.log"|mailed|input.smplr:6: this program may not mail "me@example.com.evil.test"|'
if [ "$glout" = "$glwant" ]; then ok; else nope "a * in a top line (got: $glout)"; fi
check_err "a written-out name that a * line does not cover" \
    'reads "logs/*.log"
main(sys) { sys.screen.print(sys.files.read("logs/x.txt")?) }' 'this program may not read "logs/x.txt"; add it to the reads line at the top'
check_err "a host that a * line does not cover" \
    'fetches "*.example.com"
main(sys) { sys.net.fetch("https://example.org/") }' 'this program may not fetch from "example.org"'
check_err "join on a list of numbers" \
    'main(sys) { sys.screen.print([1, 2].join(",")) }' 'join puts a list of texts together; List[Int] has no method join'
check_err "running a program with no runs line" \
    'main(sys) { sys.programs.run("ls", []) }' 'say which at the top, as in: runs "git"'
check_err "running a program the line does not name" \
    'runs "git"
main(sys) { sys.programs.run("ls", []) }' 'this program may not run "ls"; add it to the runs line at the top'
check_err "fetching with no fetches line" \
    'main(sys) { sys.net.fetch("https://example.com/") }' 'say which hosts at the top, as in: fetches "example.com"'
check_err "fetching from a host the line does not name" \
    'fetches "example.com"
main(sys) { sys.net.post("https://example.com@evil.example/x", "a") }' 'this program may not fetch from "example.com@evil.example"'
check_err ".run needs a Programs" \
    'runs "ls"
go(n : Int) : Str !IO { n.run("ls", []) }
main(sys) { go(1) }' ".run needs a Programs handed down from main, got Int"
check_err ".now needs a Clock" \
    'main(sys) { x = 5
 sys.screen.print(x.now) }' ".now needs a Clock handed down from main, got Int"
check_err "a reads line with a missing name" \
    'reads "a.txt",
main(sys) { 0 }' "expected a file name in quotes, or the word args"
printf '%s' 'reads args, "data/"
writes "out/"
main(sys) {
  t = sys.files.read(sys.args.at(0))?
  sys.files.write("out/".concat(sys.args.at(1)), t)
  sys.screen.print(sys.files.read("data/".concat(sys.args.at(2)))?)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > may.c 2>/dev/null && cc -o may may.c 2>/dev/null && mkdir -p out data && echo hello > in.txt && echo secret > secret.txt && echo d > data/d.txt )
if [ "$( cd "$TMP" && ./may in.txt copy.txt d.txt 2>&1 )" = "d" ] && [ "$(cat "$TMP/out/copy.txt")" = "hello" ]; then ok; else nope "files the lines allow"; fi
if [ "$( cd "$TMP" && ./may in.txt ../leak.txt d.txt 2>&1 )" = 'input.smplr:5: this program may not write "out/../leak.txt"' ] && [ ! -e "$TMP/leak.txt" ]; then ok; else nope "a write that climbs out of its folder"; fi
if [ "$( cd "$TMP" && ./may in.txt copy.txt ../secret.txt 2>&1 )" = 'input.smplr:6: this program may not read "data/../secret.txt"' ]; then ok; else nope "a read that climbs out of its folder"; fi
# a running program stops with its line, never goes on with a wrong value
stops() { # description  source  input  expected message
    printf '%s' "$2" > "$TMP/input.smplr"
    ( cd "$TMP" && ./seedc > stop.c 2>/dev/null )
    local err rc
    err="$( cc -o "$TMP/stop" "$TMP/stop.c" 2>/dev/null && printf '%s' "$3" | "$TMP/stop" 2>&1 >/dev/null )"; rc=$?
    if [ "$rc" -eq 1 ] && [ "$err" = "$4" ]; then ok; else nope "$1 (rc=$rc, got: $err)"; fi
}
stops "list index past the end" \
    'main(sys) {
  xs = [1, 2, 3]
  sys.screen.print(xs.at(3))
}' "" "input.smplr:3: list index 3, but the list has 3 items"
stops "text index past the end" \
    'main(sys) {
  s = "abc"
  i = 0
  while i < 4 {
    sys.screen.print(s.at(i))
    i = i + 1
  }
}' "" "input.smplr:5: text index 3, but the text has 3 characters"
stops "slice past the end" \
    'main(sys) {
  s = "abc"
  sys.screen.print(s.slice(1, 9))
}' "" "input.smplr:3: slice 1 to 9, but the text has 3 characters"
stops "division by zero" \
    'main(sys) {
  n = sys.stdin.trim.toInt
  sys.screen.print(10 / n)
}' "0" "input.smplr:3: division by zero"
stops "remainder by zero" \
    'main(sys) {
  n = sys.stdin.trim.toInt
  sys.screen.print(10 % n)
}' "0" "input.smplr:3: division by zero"
# A whole number that grows past the largest one stops the program; it never wraps.
big="whole number too big (the limit is 9223372036854775807)"
stops "+ past the largest whole number" \
    'main(sys) {
  n = sys.stdin.trim.toInt
  sys.screen.print(n + 1)
}' "9223372036854775807" "input.smplr:3: $big"
stops "- past the smallest whole number" \
    'main(sys) {
  n = sys.stdin.trim.toInt
  sys.screen.print(0 - n - 2)
}' "9223372036854775807" "input.smplr:3: $big"
stops "* past the largest whole number" \
    'main(sys) {
  n = sys.stdin.trim.toInt
  sys.screen.print(n * n)
}' "4000000000" "input.smplr:3: $big"
stops "- on the smallest whole number" \
    'main(sys) {
  n = sys.stdin.trim.toInt
  sys.screen.print(-n)
}' "-9223372036854775808" "input.smplr:3: $big"
stops "abs on the smallest whole number" \
    'main(sys) {
  n = sys.stdin.trim.toInt
  sys.screen.print(n.abs)
}' "-9223372036854775808" "input.smplr:3: $big"
stops "the smallest whole number / -1" \
    'main(sys) {
  n = sys.stdin.trim.toInt
  sys.screen.print(n / (0 - 1))
}' "-9223372036854775808" "input.smplr:3: $big"
stops "toInt of a number too big" \
    'main(sys) {
  n = sys.stdin.trim.toInt
  sys.screen.print(n)
}' "99999999999999999999" "input.smplr:2: $big"
printf '%s' 'main(sys) {
  n = sys.stdin.trim.toInt
  sys.screen.print(n % (0 - 1))
  sys.screen.print(n + 9223372036854775807)
  sys.screen.print(3000000000 * 3000000000)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > fit.c 2>/dev/null )
fit="$( cc -O2 -o "$TMP/fit" "$TMP/fit.c" -lm 2>/dev/null && printf '%s' "-9223372036854775808" | "$TMP/fit" 2>&1 )"
if [ "$fit" = "$(printf '0\n-1\n9000000000000000000')" ]; then ok; else nope "numbers that fit are not stopped (got: $fit)"; fi
stops "toInt of a text that is no number" \
    'main(sys) {
  n = sys.stdin.trim.toInt
  sys.screen.print(n)
}' "12x" 'input.smplr:2: "12x" is not a whole number; test with .isInt first'
# the values of a call run left to right, whatever the C compiler would pick
printf '%s' 'note(log : List[Str], s : Str) : Int !Mut {
  log.push(s)
  log.length
}
pair(a : Int, b : Int, c : Int) : Str {
  a.toStr.concat(b.toStr).concat(c.toStr)
}
main(sys) {
  log : List[Str] = []
  sys.screen.print(pair(note(log, "a"), note(log, "b"), note(log, "c")))
  sys.screen.print(note(log, "d") - note(log, "e"))
  xs = [note(log, "f"), note(log, "g")]
  sys.screen.print(xs.at(0) - xs.at(1))
  sys.screen.print("7".isInt && " -3 ".isInt && "x".isInt.not && "".isInt.not)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > order.c 2>/dev/null )
for opt in -O0 -O2; do
    if cc $opt -o "$TMP/order" "$TMP/order.c" 2>/dev/null && [ "$("$TMP/order" | tr '\n' ' ')" = "123 -1 -1 true " ]; then ok; else nope "left to right at $opt"; fi
done
check_err "text ends in a backslash" \
    'main(sys) {
  x = "a\' \
    "input.smplr:2: a text is missing its closing quote"
# a loop's name is free again after the loop
printf '%s' 'main(sys) {
  ["a", "b"].each { row in
    sys.screen.print(row)
  }
  row = 5
  sys.screen.print(row + 1)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > reuse.c 2>/dev/null )
if cc -o "$TMP/reuse" "$TMP/reuse.c" 2>/dev/null && [ "$("$TMP/reuse" | tr '\n' ' ')" = "a b 6 " ]; then ok; else nope "a loop name used again after the loop"; fi
# a match in the middle of a function, an if on the last line and else-if all give values
printf '%s' 'Shape = type {
  Dot
  Box(Int)
}
grade(n : Int) : Str {
  if n > 8 {
    "high"
  } else if n > 4 {
    "mid"
  } else {
    "low"
  }
}
main(sys) {
  b = Box(7)
  x = b.match {
    Dot -> 1
    Box(n) -> n
  }
  sys.screen.print(x)
  sys.screen.print(grade(9).concat(grade(5)).concat(grade(1)))
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > val.c 2>/dev/null )
if cc -o "$TMP/val" "$TMP/val.c" 2>/dev/null && [ "$("$TMP/val" | tr '\n' ' ')" = "7 highmidlow " ]; then ok; else nope "match and if as values"; fi
check_err "missing record field" \
    'Point = type { x : Int, y : Int }
main(sys) { p = Point(x = 1) sys.screen.print(p.x) }' \
    "missing field"
check_err "wrong field type" \
    'Point = type { x : Int }
main(sys) { p = Point(x = "a") sys.screen.print(p.x) }' \
    "got Str"
check_err "unknown field read" \
    'Point = type { x : Int }
main(sys) { p = Point(x = 1) sys.screen.print(p.z) }' \
    "has no field"
check_err "match binding count mismatch" \
    'Expr = type { Lit(Int) Add(Expr, Expr) }
main(sys) { eval(sys.screen, Lit(1)) }
eval(screen : Screen, e : Expr) !IO { e.match { Lit(n) -> screen.print(n) Add(a) -> screen.print(1) } }' \
    "binds 1, expected 2"
check_err "case payload count mismatch" \
    'Expr = type { Lit(Int) Add(Expr, Expr) }
main(sys) { e = Add(Lit(1)) sys.screen.print(1) }' \
    "takes 2 values, got 1"
check_err "match-value arm must produce a value" \
    'Expr = type { Lit(Int) }
main(sys) { sys.screen.print(ev(Lit(1))) }
ev(e : Expr) : Int { e.match { Lit(n) -> { } } }' \
    "not a { } block"
check_err "list element type" \
    'main(sys) { xs : List[Int] = [] xs.push("no") sys.screen.print(1) }' \
    "expects Int"
check_err "empty list needs a type" \
    'main(sys) { xs = [] sys.screen.print(1) }' \
    "needs a type"
check_err "non-exhaustive match" \
    'Tok = type { A(Str) B }
main(sys) { show(sys.screen, B) }
show(screen : Screen, t : Tok) !IO { t.match { A(s) -> screen.print(s) } }' \
    "missing case: B"
check_err "field access on a variant" \
    'Tok = type { A(Str) B }
main(sys) { t = B sys.screen.print(t.x) }' \
    "is a variant"
check_err "unknown case in match" \
    'Tok = type { A B }
main(sys) { show(sys.screen, A) }
show(screen : Screen, t : Tok) !IO { t.match { A -> screen.print("a") B -> screen.print("b") Z -> screen.print("z") } }' \
    "has no case"
check_err "str method on an int" \
    'main(sys) { sys.screen.print(5.length) }' \
    "has no method"
check_err "wrong builtin arg type" \
    'main(sys) { sys.screen.print("hi".at("x")) }' \
    "expects Int"

# --- 3. `simpler test` runs the test_* functions and reports ----------------
run_tests() { # file -> prints the report, returns the runner's exit code
    cp "$1" "$TMP/input.smplr"
    ( cd "$TMP" && ./seedc test > tests.c 2>/dev/null && cc -w tests.c -o tests.bin -lm 2>/dev/null && ./tests.bin )
    local rc=$?
    rm -f "$TMP/input.smplr"
    return $rc
}
out="$(run_tests examples/tests.smplr)"
rc=$?
if [ "$rc" -eq 0 ] && printf '%s' "$out" | grep -qF "2/2 passed"; then ok; else nope "test all-pass"; fi

printf 'test_a() { assert(1 == 1) }\ntest_b() { assert(1 == 2) }\n' > "$TMP/t.smplr"
out="$(run_tests "$TMP/t.smplr")"
rc=$?
if [ "$rc" -ne 0 ] && printf '%s' "$out" | grep -qF "FAIL test_b"; then ok; else nope "test failure detected"; fi

# --- 4. the self-host fixpoint, Rust-free -------------------------------------
# selfhost/simpler.c is the self-hosted compiler transpiled to C, committed so the
# language builds with no Rust at all. Compile it with cc, point it at its own
# source, and it must regenerate simpler.c byte-for-byte. This both proves the
# Rust dependency is gone and keeps the committed C in sync with the source.
# selfhost/simpler.c is the self-hosted compiler transpiled to C, committed so the
# language builds with no Rust at all. Compile it with cc, point it at its own
# source, and it must regenerate simpler.c byte-for-byte. This both proves the
# a leading minus negates the whole value after it
printf '%s' 'P = type {
  n : Int
  d : Int
}
main(sys) {
  p = P(n = 3, d = 4)
  xs = [5, 6]
  sys.screen.print(-p.n)
  sys.screen.print(-xs.at(1))
  sys.screen.print(-5.toStr.concat("!"))
  sys.screen.print(2 - -p.d)
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > neg.c 2>/dev/null )
if cc -o "$TMP/neg" "$TMP/neg.c" 2>/dev/null && [ "$("$TMP/neg" | tr '\n' ' ')" = "-3 -6 -5! 6 " ]; then ok; else nope "minus before x.n"; fi
check_err "minus on a record" \
    'P = type {
  n : Int
  d : Int
}
main(sys) {
  p = P(n = 3, d = 4)
  q = -p
}' "needs a number, got P"
check_err "effects written before the return type" \
    'f(xs : List[Int]) !Mut : Int {
  xs.push(1)
  0
}
main(sys) { f([1]) }' "the return type comes before the effects"
# a program's own names never collide with the names the C output uses
echo 'T = type { EOF NULL Obj(Int) FILE(Str) }
P = type {
  o : Int
  int : Int
}
PT = type { A B }
show(t : T) : Str {
  t.match {
    EOF -> "e"
    NULL -> "n"
    Obj(n) -> n.toStr
    FILE(s) -> s
  }
}
f(char : Int, l_new : Int, s_len : Int) : Int { char + l_new + s_len }
main(sys) {
  p = P(o = 1, int = 2)
  long = 4
  printf = 7
  mk = 8
  _t0 = 9
  xs = [1, 2]
  sys.screen.print(show(EOF).concat(show(Obj(7))).concat(show(FILE("f"))))
  sys.screen.print(p.o + p.int + long + printf + mk + _t0 + xs.length + f(1, 2, 3))
}' > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > names.c 2>/dev/null )
if cc -o "$TMP/names" "$TMP/names.c" 2>/dev/null && [ "$("$TMP/names" | tr '\n' ' ')" = "e7f 39 " ]; then ok; else nope "names the C output also uses"; fi
check_err "a type name nobody declared, in a signature" \
    'f(s : str) : Int { 1 }
main(sys) { f("a") }' "input.smplr:1: no type named str"
check_err "a type name nobody declared, in a record" \
    'main(sys) { 0 }
P = type {
  n : int
}' "input.smplr:2: no type named int;"
check_err "a type name nobody declared, on a local" \
    'main(sys) {
  xs : List[string] = []
}' "input.smplr:2: no type named string"
# a function that changes a list or map it was handed is !Mut, written or not; `what` lists it
check_mut "!Mut: a push on a handed list" \
    'add(xs : List[Int]) {
  xs.push(1)
}
main(sys) { add([1]) }'
check_mut "!Mut: a set on a handed map" \
    'note(m : Map[Str, Int]) {
  m.set("a", 1)
}
main(sys) {
  m : Map[Str, Int] = Map()
  note(m)
}'
check_mut "!Mut: a push through a second name" \
    'add(xs : List[Int]) : Int {
  ys = xs
  ys.push(1)
  ys.length
}
main(sys) { add([1]) }'
check_mut "!Mut: a push on an item of a handed list" \
    'add(rows : List[List[Int]]) {
  rows.each { r in
    r.push(1)
  }
}
main(sys) { add([[1]]) }'
check_mut "!Mut: a push on a field of a handed record" \
    'Box = type {
  items : List[Int]
  n : Int
}
add(b : Box) {
  b.items.push(1)
}
main(sys) { add(Box(items = [1], n = 0)) }'
check_mut "!Mut: a push on what a function gave back unchanged" \
    'same(xs : List[Int]) : List[Int] {
  xs
}
add(xs : List[Int]) {
  ys = same(xs)
  ys.push(1)
}
main(sys) { add([1]) }'
check_mut "!Mut: passing a handed list to a !Mut function" \
    'put(xs : List[Int], n : Int) !Mut {
  xs.push(n)
}
add(xs : List[Int]) {
  put(xs, 1)
}
main(sys) { add([1]) }'
check_mut "!Mut: a push on a list taken out by match" \
    'T = type {
  Full(List[Int])
  Empty
}
add(t : T) : Int {
  t.match {
    Full(xs) -> xs.push(1)
    Empty -> 0
  }
}
main(sys) { add(Full([1])) }'
check_mut "!Mut: a push on a handed list kept in a local list" \
    'add(xs : List[Int]) : Int {
  keep : List[List[Int]] = []
  keep.push(xs)
  keep.at(0).push(1)
  0
}
main(sys) { add([1]) }'
check_mut "!Mut: x.f on a handed list when f is !Mut" \
    'grow(xs : List[Int]) !Mut {
  xs.push(1)
}
add(xs : List[Int]) {
  xs.grow
}
main(sys) { add([1]) }'
check_err "x.f counts as a call to f for effects" \
    'shout(s : Screen) !IO {
  s.print("x")
}
f(s : Screen) {
  s.shout
}
main(sys) { f(sys.screen) }' "uses !IO but does not declare it"
check_err "an effect that does not exist" \
    'add(xs : List[Int]) !Mutt {
  xs.push(1)
}
main(sys) { add([1]) }' "no effect named Mutt"
accepts() { # description  source
    printf '%s' "$2" > "$TMP/input.smplr"
    local err
    if err="$( cd "$TMP" && ./seedc 2>&1 >/dev/null )"; then ok; else nope "$1 (got: $err)"; fi
}
accepts "Map[Int] is the same type as Map" \
    'g(m : Map[Int]) : Int { m.get("a") }
main(sys) {
  m = Map()
  m.set("a", 4)
  sys.screen.print(g(m))
}'
accepts "a new list built from a handed one needs no !Mut" \
    'P = type {
  val : Int
  next : Int
}
one(xs : List[Int], i : Int) : P {
  P(val = xs.at(i) * 2, next = i + 1)
}
twice(xs : List[Int]) : List[Int] {
  out : List[Int] = []
  i = 0
  while i < xs.length {
    p = one(xs, i)
    out.push(p.val)
    i = p.next
  }
  out
}
main(sys) { sys.screen.print(twice([1, 2]).length) }'
accepts "a !Mut call that changes only a local, and fresh lists from sort, byValue and split" \
    'count(words : List[Str], m : Map) !Mut {
  words.each { w in
    m.set(w, m.get(w) + 1)
  }
}
top(words : List[Str]) : Str {
  m = Map()
  count(words, m)
  ks = m.byValue
  ks.push("end")
  s = words.sort
  s.push("z")
  parts = words.at(0).split(",")
  parts.push("q")
  ks.at(0)
}
main(sys) {
  ws = ["a", "b", "a"]
  ws.push("c")
  sys.screen.print(top(ws))
}'
accepts "main may change what it likes" \
    'put(xs : List[Int], n : Int) !Mut {
  xs.push(n)
}
main(sys) {
  xs : List[Int] = []
  put(xs, 1)
  sys.screen.print(xs.length)
}'
accepts "plain records copied from a handed list into a new list need no !Mut" \
    'Ev = type {
  when : Float
}
copy(raw : List[Ev]) : List[Ev] {
  out : List[Ev] = []
  for e in raw {
    out.push(e)
  }
  out
}
main(sys) {
  sys.screen.print(copy([Ev(when = 2.0), Ev(when = 1.0)]).length.toStr)
}'
printf '%s' 'Bag = type {
  items : List[Int]
}
keep(raw : List[Bag]) : List[Bag] {
  out : List[Bag] = []
  for e in raw {
    out.push(e)
  }
  out.at(0).items.push(1)
  out
}
main(sys) {
  sys.screen.print(keep([Bag(items = [1])]).length.toStr)
}' > "$TMP/input.smplr"
if ( cd "$TMP" && ./seedc what 2>&1 ) | grep -qF 'changes a list or map it was handed  (in '; then ok; else nope "a record with a list in it, copied from a handed list, is listed as !Mut"; fi
# A long line of + took twice as long for each part more: 24 parts took five seconds.
{ printf 'main(sys) {\n  a = "x"\n  sys.screen.print(a'; for i in $(seq 23); do printf ' + " " + a'; done; printf ')\n}\n'; } > "$TMP/input.smplr"
T0=$(date +%s%N); ( cd "$TMP" && ./seedc >/dev/null 2>&1 ); T1=$(date +%s%N)
if [ $(( (T1 - T0) / 1000000 )) -lt 1000 ]; then ok; else nope "a line of 47 parts joined with + compiles in under a second"; fi
printf '%s' 'put(xs : List[Int], n : Int) !Mut {
  xs.push(n)
}
main(sys) {
  xs : List[Int] = []
  put(xs, 1)
}' > "$TMP/input.smplr"
if ( cd "$TMP" && ./seedc what 2>&1 ) | grep -qF 'lists   changes a list or map it was handed  (in put)'; then ok; else nope "simpler what names the !Mut functions"; fi

# Rust dependency is gone and keeps the committed C in sync with the source.
if cc -O2 selfhost/simpler.c -o "$TMP/seedc" 2>/dev/null; then
    cp selfhost/simpler.smplr "$TMP/input.smplr"
    ( cd "$TMP" && ./seedc > regen.c 2>/dev/null )
    if diff -q "$TMP/regen.c" selfhost/simpler.c >/dev/null 2>&1; then ok; else nope "committed simpler.c is stale (regenerate it from simpler.smplr)"; fi
else
    nope "committed simpler.c does not compile"
fi

# The simpler command: run, what, and a message that names the user's file.
mkdir -p "$TMP/cli"
printf 'main(sys) {\n  sys.screen.print("hi " + sys.args.at(0))\n  xs = [1]\n  sys.screen.print(xs.at(5))\n}\n' > "$TMP/cli/prog.smplr"
cliout="$( cd "$TMP/cli" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run prog.smplr you 2>&1; SIMPLERC="$TMP/seedc" "$OLDPWD/simpler" what prog.smplr | head -1 )"
if [ "$cliout" = 'prog.smplr:4: list index 5, but the list has 1 items
hi you
screen  prints  (in main)' ] || [ "$cliout" = 'hi you
prog.smplr:4: list index 5, but the list has 1 items
screen  prints  (in main)' ]; then ok; else nope "the simpler command (got: $cliout)"; fi

# Modules: uses brings in a file, names carry the module, messages name the file.
mkdir -p "$TMP/mod/lib2"
printf 'uses "geo"\nBox = type {\n  w : Int\n  h : Int\n}\nShape = type {\n  Round(Int)\n  Flat(Box)\n}\narea(s : Shape) : Int {\n  s.match {\n    Round(r) -> geo.pi() * r * r\n    Flat(b) -> b.w * b.h\n  }\n}\nshow(out : Screen, b : Box) !IO {\n  out.print("box " + b.w.toStr)\n}\nthird(xs : List[Int]) : Int {\n  xs.at(3)\n}\n' > "$TMP/mod/shape.smplr"
printf 'uses "shape"\npi() : Int {\n  3\n}\n' > "$TMP/mod/lib2/geo.smplr"
printf 'uses "shape"\narea(n : Int) : Int {\n  n\n}\nmain(sys) {\n  b = shape.Box(w = 2, h = 5)\n  s : shape.Shape = shape.Flat(b)\n  sys.screen.print(shape.area(s) + shape.area(shape.Round(2)) + area(1))\n  s.match {\n    shape.Round(r) -> sys.screen.print("round")\n    shape.Flat(x) -> shape.show(sys.screen, x)\n  }\n  sys.screen.print(shape.third([1]))\n}\n' > "$TMP/mod/prog.smplr"
modrun() { ( cd "$TMP/mod" && SIMPLER_LIB="$TMP/mod/lib2" SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" "$@" 2>&1 ); }
modout="$( modrun run prog.smplr | sort | tr '\n' '|'; modrun what prog.smplr | head -1 )"
if [ "$modout" = '23|box 2|shape.smplr:20: list index 3, but the list has 1 items|screen  prints  (in shape.show, main)' ]; then ok; else nope "a program with modules (got: $modout)"; fi
moderr() { printf "$1" > "$TMP/mod/bad.smplr"; modrun run bad.smplr; }
if [ "$(moderr 'uses "nope"\nmain(sys) {\n}\n')" = 'bad.smplr:1: no module named nope: the file nope.smplr was not found' ]; then ok; else nope "a missing module"; fi
if [ "$(moderr 'uses "shape"\nmain(sys) {\n  shape = 2\n}\n')" = 'bad.smplr:3: shape is a module here: write shape.name for something in it, and give a variable another name' ]; then ok; else nope "a variable named as a module"; fi
if [ "$(moderr 'uses "shape"\nmain(sys) {\n  sys.screen.print(shape.nope(1))\n}\n')" = 'bad.smplr:3: no function named shape.nope' ]; then ok; else nope "a name the module does not have"; fi
if [ "$(moderr 'uses "../shape"\nmain(sys) {\n}\n')" = 'bad.smplr:1: a module name has only letters, digits and _' ]; then ok; else nope "a module name with a path in it (got: $(moderr 'uses "../shape"\nmain(sys) {\n}\n'))"; fi
printf 'reads "x"\nf() : Int {\n  1\n}\n' > "$TMP/mod/m1.smplr"; printf 'main(sys) {\n}\n' > "$TMP/mod/m2.smplr"; printf 'f(x : Int) : Int {\n  x +\n}\n' > "$TMP/mod/m3.smplr"
modbad="$( for m in m1 m2 m3; do moderr "uses \"$m\"\nmain(sys) {\n}\n"; done | tr '\n' '|' )"
if [ "$modbad" = 'm1.smplr:1: a module has no reads, writes, runs, fetches, mails or listens line; those belong to the program that uses it|m2.smplr:1: a module has no main; main belongs to the program that uses it|m3.smplr:3: expected a value, found }|' ]; then ok; else nope "what a module may not have (got: $modbad)"; fi
# screen.error prints to stderr and goes on; a module used without its uses line gets a hint.
printf 'main(sys) {\n  sys.screen.print("a")\n  sys.screen.error("b")\n  sys.screen.print("c")\n}\n' > "$TMP/mod/err.smplr"
printf 'main(sys) {\n  o = zork.parse(sys.args, "v", "")\n}\n' > "$TMP/mod/nouses.smplr"
errout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run err.smplr 2>/dev/null | tr '\n' '~'; SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run err.smplr 2>&1 >/dev/null; SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run nouses.smplr 2>&1 )"
if [ "$errout" = 'a~c~b
nouses.smplr:2: no variable named zork; if it is a module, put  uses "zork"  at the top' ]; then ok; else nope "screen.error and the uses hint (got: $errout)"; fi
# opts, the module for options on the command line: its own tests, and the page with a part and a module.
if ( SIMPLERC="$TMP/seedc" ./simpler test lib/opts.smplr 2>&1 | grep -q '^2/2 passed' ) && ( ./simpler prompt regex opts | grep -qF 'parse(argv : List[Str]' ) && ( ./simpler prompt regex opts | grep -qF 's.reTest(p)' ); then ok; else nope "the opts module and its page"; fi
# astro, the module for the sky: its own tests (numbers checked against PyEphem), and its page without the helpers.
if ( SIMPLERC="$TMP/seedc" ./simpler test lib/astro.smplr 2>&1 | grep -q '^10/10 passed' ) && ( ./simpler prompt astro | grep -qF 'sun_times(year : Int, month : Int, day : Int, lat : Float, lon : Float, tz : Float) : Times' ) && ! ( ./simpler prompt astro | grep -q '^moon_centre\|^near\|^test_dates' ); then ok; else nope "astro: tests and page"; fi
# tui: a pane owns the terminal and is drawn from its properties, in true colours.
printf 'uses "tui"\nmain(sys) {\n  a = tui.left(tui.open(sys.term), 12)\n  a.title = "Hi"\n  a.bg = "#232154"\n  a.items = ["one", "two"]\n  a.picked = 0\n  tui.move(a, 1)\n  tui.show(a)\n  k = tui.key(a)\n  tui.close(a)\n  sys.screen.print("key " + k + " " + tui.chosen(a))\n}\n' > "$TMP/mod/tp.smplr"
( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" "$OLDPWD/simpler" build tp.smplr -o tp 2>&1 )
tuout="$( cd "$TMP/mod" && printf 'x' | script -qec 'stty rows 5 cols 20; ./tp' /dev/null 2>/dev/null | tr -d '\r' )"
if ( SIMPLERC="$TMP/seedc" ./simpler test lib/tui.smplr 2>&1 | grep -q '^1/1 passed' ) && ( ./simpler prompt tui | grep -qF 'show(p : Pane) !Mut' ) && printf '%s' "$tuout" | grep -qF '┌─ Hi ──' && printf '%s' "$tuout" | grep -qF '48;2;35;33;84' && printf '%s' "$tuout" | grep -qF 'key x two'; then ok; else nope "tui draws a pane from its properties (got: $tuout)"; fi
# A module called by name with no uses line is brought in; a variable of that name is left alone.
printf 'main(sys) {\n  a = tui.open(sys.term)\n  tui.close(a)\n  sys.screen.print("in")\n}\n' > "$TMP/mod/nouse.smplr"
printf 'main(sys) {\n  opts = ["a"]\n  sys.screen.print(opts.length.toStr)\n}\n' > "$TMP/mod/novar.smplr"
if [ "$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" "$OLDPWD/simpler" build nouse.smplr -o nouse 2>&1 && script -qec ./nouse /dev/null | tr -d '\r' | tail -c 3 )" = "in" ] && [ "$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" "$OLDPWD/simpler" run novar.smplr 2>&1 )" = 1 ]; then ok; else nope "a missing uses line is added"; fi
# screen, the castle under tui: a pane with no colour takes the screen's; sys.screen brings in no module.
printf 'main(sys) {\n  a = tui.left(tui.open(sys.term), 12)\n  a.scr.bg = "#102030"\n  a.items = [a.scr.cols.toStr]\n  tui.show(a)\n  k = tui.key(a)\n  tui.close(a)\n}\n' > "$TMP/mod/sc.smplr"
( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" "$OLDPWD/simpler" build sc.smplr -o sc 2>&1 )
scout="$( cd "$TMP/mod" && printf 'x' | script -qec 'stty rows 5 cols 20; ./sc' /dev/null 2>/dev/null | tr -d '\r' )"
printf 'main(sys) {\n  sys.screen.print("x")\n}\n' > "$TMP/mod/plain.smplr"
if ( SIMPLERC="$TMP/seedc" ./simpler test lib/screen.smplr 2>&1 | grep -q '^1/1 passed' ) && printf '%s' "$scout" | grep -qF '48;2;16;32;48' && printf '%s' "$scout" | grep -qF '│20' && ! ( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" "$OLDPWD/simpler" what plain.smplr | grep -q 'screen\.' ); then ok; else nope "screen under tui (got: $scout)"; fi

# crust, the module that comes with simpler: its own tests, its page, and a frame on a real terminal.
if ( SIMPLERC="$TMP/seedc" ./simpler test lib/crust.smplr 2>&1 | grep -q '^2/2 passed' ) && ( ./simpler prompt crust | grep -qF 'frame(t : Term, p : Pane, title : Str, style : Str)' ); then ok; else nope "crust: tests and page"; fi
printf 'uses "crust"\nmain(sys) {\n  t = sys.term\n  p = crust.top(crust.full(t), 3)\n  crust.frame(t, p, "Hi", "")\n  crust.line(t, crust.inner(p), 0, "in " + crust.inner(p).cols.toStr, "")\n  k = t.key\n  t.stop\n  sys.screen.print("key " + k)\n}\n' > "$TMP/mod/pane.smplr"
( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" "$OLDPWD/simpler" build pane.smplr -o pane 2>&1 )
crout="$( cd "$TMP/mod" && printf 'x' | script -qec 'stty rows 5 cols 20; ./pane' /dev/null 2>/dev/null | tr -d '\r' )"
if printf '%s' "$crout" | grep -qF '┌─ Hi ──' && printf '%s' "$crout" | grep -qF 'Hi' && printf '%s' "$crout" | grep -qF 'in 18' && printf '%s' "$crout" | grep -qF 'key x'; then ok; else nope "crust draws a frame (got: $crout)"; fi

# A style may be a colour number, and on with a number for the background.
printf 'main(sys) {\n  t = sys.term\n  t.put(0, 0, "a", "bold 208 on52")\n  t.put(1, 0, "b", sys.args.at(0))\n  k = t.key\n  t.stop\n}\n' > "$TMP/mod/col.smplr"
( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" "$OLDPWD/simpler" build col.smplr -o col 2>&1 )
colout="$( cd "$TMP/mod" && for a in 7 256 on9x; do printf 'x' | script -qec "./col $a" /dev/null 2>/dev/null | tr -d '\r'; done )"
if printf '%s' "$colout" | grep -qF "$(printf '\033[0;1;38;5;208;48;5;52ma')" && printf '%s' "$colout" | grep -qF "$(printf '\033[0;38;5;7mb')" && printf '%s' "$colout" | grep -qF 'col.smplr:4: no style named "256"' && printf '%s' "$colout" | grep -qF 'no style named "on9x"'; then ok; else nope "colour numbers in a style"; fi

# The terminal draws only the cells that changed since the last key.
printf 'main(sys) {\n  t = sys.term\n  n = 0\n  k = ""\n  while k != "q" {\n    t.clear\n    for r in 0..4 {\n      t.put(r, 0, "row of text that stays", "bold")\n    }\n    t.put(4, 0, "n=" + n.toStr, "")\n    k = t.key\n    n = n + 1\n  }\n  t.stop\n}\n' > "$TMP/mod/diff.smplr"
( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" "$OLDPWD/simpler" build diff.smplr -o diff 2>&1 )
diffout="$( cd "$TMP/mod" && printf 'xq' | script -qec 'stty rows 6 cols 40; ./diff' /dev/null 2>/dev/null | tr -d '\r' )"
if [ "$(printf '%s' "$diffout" | grep -o 'row of text' | wc -l)" = 4 ] && printf '%s' "$diffout" | grep -qF "$(printf '\033[5;3H\033[0m1')"; then ok; else nope "the terminal draws only what changed (got: $diffout)"; fi

# /proc files say they are empty and /sys is made of the kernel's own links: both are read.
printf 'reads "/proc/"\nreads "/sys/class/net/"\nmain(sys) {\n  sys.screen.print(sys.files.read("/proc/self/comm").trim)\n  sys.screen.print(sys.files.read("/sys/class/net/lo/operstate").trim.length > 0)\n  sys.screen.print(sys.files.read("/proc/self/cwd/x").length)\n}\n' > "$TMP/mod/proc.smplr"
procout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" build proc.smplr -o procread 2>&1; ./procread 2>&1 | LC_ALL=C sort | tr '\n' '|' )"
if [ "$procout" = 'proc.smplr:6: this program may not read "/proc/self/cwd/x": a link leads out of "/proc/"|procread|true|' ]; then ok; else nope "reading /proc and /sys (got: $procout)"; fi

# try: what a program printed, or "" when it fails or is missing, with no error text.
printf 'runs "sh", "nosuchprog"\nmain(sys) {\n  sys.screen.print("[" + sys.programs.try("sh", ["-c", "echo hi"]).trim + "]")\n  sys.screen.print("[" + sys.programs.try("sh", ["-c", "echo out; echo err >&2; exit 3"]) + "]")\n  sys.screen.print("[" + sys.programs.try("nosuchprog", []) + "]")\n  sys.screen.print(sys.programs.try("ls", []))\n}\n' > "$TMP/mod/try.smplr"
tryout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run try.smplr 2>&1 | tr '\n' '|' )"
if [ "$tryout" = 'try.smplr:6: this program may not run "ls"; add it to the runs line at the top|' ]; then
    sed -i '/"ls"/d' "$TMP/mod/try.smplr"
    tryout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run try.smplr 2>&1 | tr '\n' '|' )"
    if [ "$tryout" = '[hi]|[]|[]|' ]; then ok; else nope "try (got: $tryout)"; fi
else nope "try obeys the runs line (got: $tryout)"; fi

# The kernel lock: a shell the program runs cannot leave what the top lines name.
if grep -q landlock /sys/kernel/security/lsm 2>/dev/null; then
    mkdir -p "$TMP/lockt/box" "$TMP/lockt/pub"; echo secret > "$TMP/lockt/outside.txt"; echo hello > "$TMP/lockt/pub/in.txt"
    cat > "$TMP/lockt/lock.smplr" <<'LOCKT'
reads "pub/"
writes "box/"
runs "sh"
main(sys) {
  cmd = ["echo x > box/ok.txt", "cat pub/in.txt", "echo x > evil.txt", "cat outside.txt", "echo x > pub/in.txt", "rm outside.txt", "mv box/ok.txt moved.txt", "ls ..", "kill -0 " + sys.args.at(0), "cat /proc/1/cmdline"]
  out = ""
  for c in cmd {
    r = "n"
    if sys.programs.status("sh", ["-c", "( " + c + " ) >/dev/null 2>&1"]) == 0 {
      r = "y"
    }
    out = out + r
  }
  sys.screen.print(out)
}
LOCKT
    lockout="$( cd "$TMP/lockt" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run lock.smplr $$ 2>&1 )"
    if [ "$lockout" = "yynnnnnnnn" ] && [ "$(cat "$TMP/lockt/outside.txt")" = secret ] && [ ! -e "$TMP/lockt/evil.txt" ]; then ok; else nope "the kernel lock (got: $lockout)"; fi

    # simpler trusted: yes only when every name on the top lines is inside the rules file.
    printf 'reads "%s/lockt/"\nwrites "%s/lockt/box/"\nruns "sh"\n' "$TMP" "$TMP" > "$TMP/lockt/trust"
    ln -s /etc "$TMP/lockt/pub/out"
    tprog() { f="$TMP/lockt/$1.smplr"; shift; { for l in "$@"; do printf '%s\n' "$l"; done; printf 'main(sys) {\n  sys.screen.print("x")\n}\n'; } > "$f"; }
    tprog t1 'reads "pub/"' 'writes "box/"' 'runs "sh"'
    tprog t2 'reads "/etc/"'
    tprog t3 'reads "pub/../../"'
    tprog t4 'reads "pub/out/"'
    tprog t5 'writes "pub/"'
    tprog t6 'reads "pub/"' 'fetches "example.com"'
    tprog t7 'reads "pub/"' 'runs "git"'
    tprog t8 'reads args'
    trout="$( cd "$TMP/lockt" && export SIMPLERC="$TMP/seedc" SIMPLER_TRUST="$TMP/lockt/trust"; for n in 1 2 3 4 5 6 7; do "$OLDPWD/simpler" trusted t$n.smplr && printf y || printf n; done; "$OLDPWD/simpler" trusted t8.smplr pub/in.txt && printf y || printf n; "$OLDPWD/simpler" trusted t8.smplr /etc/passwd && printf y || printf n; SIMPLER_TRUST="$TMP/none" "$OLDPWD/simpler" trusted t1.smplr && printf y || printf n )"
    if [ "$trout" = "ynnnnnnynn" ]; then ok; else nope "simpler trusted (got: $trout)"; fi
else ok; fi

# An example above a function checks that one function; `simpler pieces` lists each function's reach.
mkdir -p "$TMP/ex/box"
cat > "$TMP/ex/pc.smplr" <<'EXT'
writes "box/"
example twice(4) == 8
example shout("hi") == "HI!"
example half(3) == 1.5
twice(n : Int) : Int {
  n * 2
}
shout(s : Str) : Str {
  s.upper + "!"
}
half(n : Int) : Float {
  n.toFloat / 2.0
}
save(files : Files, text : Str) : Int !IO {
  files.write("box/p.txt", text)
  0
}
report(files : Files, out : Screen, n : Int) : Int !IO {
  save(files, twice(n).toStr)
  out.print("saved")
  0
}
main(sys) {
  report(sys.files, sys.screen, 4)
}
EXT
sed 's/twice(4) == 8/twice(4) == 9/; s/== "HI!"/== "HI"/' "$TMP/ex/pc.smplr" > "$TMP/ex/pcbad.smplr"
printf 'example twice(4)\ntwice(n : Int) : Int {\n  n * 2\n}\nmain(sys) {\n}\n' > "$TMP/ex/pcno.smplr"
pcout="$( cd "$TMP/ex" && export SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache"; "$OLDPWD/simpler" run --dry pc.smplr 2>&1; "$OLDPWD/simpler" run pcbad.smplr 2>&1; "$OLDPWD/simpler" run pcno.smplr 2>&1; "$OLDPWD/simpler" pieces pc.smplr 2>&1 | tr '\n' '|' )"
pcwant='would write "box/p.txt"
saved
pcbad.smplr:2: example fails: the left side gave 8
pcbad.smplr:3: example fails: the left side gave HI!
2 of 3 examples fail
simpler: not run
pcno.smplr:1: an example for a function is a check that is true or false, as in: example double(4) == 8
twice(n : Int) : Int|  touches nothing|  1 example|shout(s : Str) : Str|  touches nothing|  1 example|half(n : Int) : Float|  touches nothing|  1 example|save(files : Files, text : Str) : Int|  files   writes "box/p.txt"|  no example|report(files : Files, out : Screen, n : Int) : Int|  screen  prints|  files   writes "box/p.txt"|  no example|main(sys)|  screen  prints|  files   writes "box/p.txt"|  no example|each piece touches only what is listed under it|'
if [ "$pcout" = "$pcwant" ]; then ok; else nope "pieces (got: $pcout)"; fi

# == compares two lists by their items and is refused on records. With SIMPLER_PIECES
# set, a function that touches nothing must have an example.
cat > "$TMP/ex/eq.smplr" <<'EXT'
P = type {
  x : Int
  y : Int
}
example firstTwo([4, 5, 6]) == [4, 5]
example words("a b") == ["a", "b"]
firstTwo(xs : List[Int]) : List[Int] {
  [xs.at(0), xs.at(1)]
}
words(s : Str) : List[Str] {
  s.split(" ")
}
mk(a : Int) : P {
  P(x = a, y = 2)
}
main(sys) {
  sys.screen.print(([1, 2] == [1, 2]).toStr + " " + ([1, 2] == [1, 3]).toStr + " " + (["a"] != ["b"]).toStr + " " + mk(1).x.toStr)
}
EXT
sed 's/mk(1).x.toStr/(mk(1) == mk(1)).toStr/' "$TMP/ex/eq.smplr" > "$TMP/ex/eqbad.smplr"
eqout="$( cd "$TMP/ex" && export SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache"; "$OLDPWD/simpler" run eq.smplr 2>&1; "$OLDPWD/simpler" run eqbad.smplr 2>&1; SIMPLER_PIECES=1 "$OLDPWD/simpler" run eq.smplr 2>&1 )"
eqwant='true false true 1
eqbad.smplr:17: == cannot compare two values of type P; compare their fields or items one by one
eq.smplr:13: the function mk touches nothing and has no example; put one above it, as in: example mk(...) == ...'
if [ "$eqout" = "$eqwant" ]; then ok; else nope "list equality and needed examples (got: $eqout)"; fi

# Example lines at the top: the build runs each one and gives no program when one differs.
mkdir -p "$TMP/ex/box"
cat > "$TMP/ex/sum.smplr" <<'EXT'
example "3 4", "7"
example "10 -2\n", "8\n"
example "", "sum 11", "5", "6"
main(sys) {
  if sys.args.length == 2 {
    sys.screen.print("sum " + (sys.args.at(0).toInt + sys.args.at(1).toInt).toStr)
  } else {
    p = sys.stdin.trim.split(" ")
    sys.screen.print((p.at(0).toInt + p.at(1).toInt).toStr)
  }
}
EXT
sed 's/"7"/"9"/' "$TMP/ex/sum.smplr" > "$TMP/ex/bad.smplr"
exout="$( cd "$TMP/ex" && export SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache"; echo "1 2" | "$OLDPWD/simpler" run sum.smplr 2>&1; echo "1 2" | "$OLDPWD/simpler" run bad.smplr 2>&1 | tr '\n' '|'; "$OLDPWD/simpler" build bad.smplr -o bad >/dev/null 2>&1; [ -e bad ] && echo built )"
if [ "$exout" = "$(printf '3\nexample 1 fails. Wanted:|9|Got:|7|1 of 3 examples fail|simpler: not run|')" ]; then ok; else nope "example lines (got: $exout)"; fi

# A dry run says what the program would do and does none of it; an example runs dry.
cat > "$TMP/ex/dry.smplr" <<'EXT'
example "", "would write \"box/a.txt\"\nwould run sh -c echo hi\ndone"
writes "box/"
runs "sh"
fetches "example.com"
mails "me@example.com"
main(sys) {
  sys.files.write("box/a.txt", "x")
  out = sys.programs.run("sh", ["-c", "echo hi"])
  sys.screen.print("done" + out)
  if sys.args.length > 0 {
    sys.files.append("box/a.txt", "y")
    sys.files.delete("box/a.txt")
    sys.files.rename("box/a.txt", "box/b.txt")
    x = sys.net.fetch("https://example.com/")
    sys.mail.send("me@example.com", "hello", "body")
  }
}
EXT
dryout="$( cd "$TMP/ex" && export SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache"; "$OLDPWD/simpler" run --dry dry.smplr all 2>&1 | tr '\n' '|'; ls box | wc -l )"
if [ "$dryout" = 'would write "box/a.txt"|would run sh -c echo hi|done|would append to "box/a.txt"|would delete "box/a.txt"|would rename "box/a.txt" to "box/b.txt"|would fetch "https://example.com/"|would mail "me@example.com" with subject "hello"|0' ]; then ok; else nope "dry run (got: $dryout)"; fi

# The lock leaves the clock in local time: the time zone is read before the lock closes /etc.
printf 'main(sys) {\n  sys.screen.print(sys.clock.dateAs(86400 * 200, "%%H:%%M"))\n}\n' > "$TMP/ex/tz.smplr"
tzout="$( cd "$TMP/ex" && export SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache"; TZ=Asia/Tokyo "$OLDPWD/simpler" run tz.smplr 2>&1 )"
if [ "$tzout" = "09:00" ]; then ok; else nope "local time under the lock (got: $tzout)"; fi
# A limits line stops a program that spins or eats memory.
printf 'limits "1s", "64MB"\nmain(sys) {\n  n = 0\n  while n >= 0 {\n    n = n + 1\n    if n > 1000 {\n      n = 0\n    }\n  }\n}\n' > "$TMP/ex/spin.smplr"
printf 'limits "5s", "64MB"\nmain(sys) {\n  xs : List[Str] = []\n  while xs.length >= 0 {\n    xs.push("0123456789012345678901234567890123456789".concat(xs.length.toStr))\n  }\n}\n' > "$TMP/ex/eat.smplr"
printf 'limits "soon"\nmain(sys) {\n  sys.screen.print("x")\n}\n' > "$TMP/ex/badl.smplr"
limout="$( cd "$TMP/ex" && export SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache"; for f in spin eat badl; do "$OLDPWD/simpler" run $f.smplr >/dev/null 2>&1; printf '%s ' $?; done )"
if [ -n "${SAN:-}" ]; then echo "SKIP: limits (the checker itself needs the memory the test takes away)"
elif [ "$limout" = "124 125 1 " ]; then ok; else nope "limits (got: $limout)"; fi

# Memory comes back: 400 MB of short-lived text runs under a 64 MB limit, and
# a loop that sets its own counter still counts every round.
cat > "$TMP/ex/churn.smplr" <<'CHURN'
limits "20s", "64MB"
make(n : Int) : Int {
  xs : List[Str] = []
  for i in 0..n {
    xs.push("item " + i.toStr + " of a round that is thrown away")
  }
  xs.length
}
main(sys) {
  total = 0
  for round in 0..40 {
    total = total + make(200000)
    round = 99
  }
  sys.screen.print(total.toStr)
}
CHURN
churnout="$( cd "$TMP/ex" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run churn.smplr 2>&1 )"
if [ -n "${SAN:-}" ]; then echo "SKIP: memory comes back (a sanitizer needs more than the limit)"
elif [ "$churnout" = "8000000" ]; then ok; else nope "memory comes back (got: $churnout)"; fi

# A program listens on the port it names: query and form fields come decoded,
# a redirect cannot add a header, and the content type follows the text.
reject "listening with no listens line" \
    'main(sys) { r = sys.net.request(8099) }' 'say which at the top, as in: listens 8080'
reject "listening on a port not on the listens line" \
    'listens 8099
main(sys) { r = sys.net.request(8100) }' 'may not listen on port 8100'
cat > "$TMP/ex/web.smplr" <<'WEB'
listens 18099
main(sys) {
  count = 0
  while count < 4 {
    req = sys.net.request(18099)
    count = count + 1
    if req.get("path") == "/hello" {
      sys.net.reply(200, "<h1>Hello " + req.get("name") + "</h1>")
    } else if req.get("method") == "POST" {
      sys.net.reply(303, "/hello?name=" + req.get("who"))
    } else {
      sys.net.reply(404, "not found: " + req.get("path"))
    }
  }
  sys.screen.print("served " + count.toStr)
}
WEB
if ! command -v curl >/dev/null; then echo "SKIP: a web server (no curl)"
else
    webdry="$( cd "$TMP/ex" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run --dry web.smplr 2>&1 )"
    ( cd "$TMP/ex" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" build web.smplr -o web >/dev/null 2>&1 )
    "$TMP/ex/web" > "$TMP/ex/web.out" 2>&1 &
    webpid=$!
    w="http://127.0.0.1:18099"
    for i in $(seq 100); do webout="$(curl -s -o /dev/null -w '%{http_code}' "$w/ping")" && break; sleep 0.05; done
    webout="$webout|$(curl -s "$w/hello?name=Ada+L%21")"
    webout="$webout|$(curl -s -o /dev/null -w '%{http_code} %{redirect_url}' -d 'who=Bob%0D%0AX-Evil:%201' "$w/form")"
    webout="$webout|$(curl -s -w ' %{content_type}' "$w/hello?name=Cy")"
    for i in $(seq 100); do kill -0 "$webpid" 2>/dev/null || break; sleep 0.05; done
    kill "$webpid" 2>/dev/null; wait "$webpid" 2>/dev/null
    webout="$webdry|$webout|$(cat "$TMP/ex/web.out")"
    if [ "$webout" = "would wait for web requests on port 18099|404|<h1>Hello Ada L!</h1>|303 $w/hello?name=Bob|<h1>Hello Cy</h1> text/html; charset=utf-8|served 4" ]; then ok; else nope "a web server (got: $webout)"; fi
fi

# A program that may run programs and fetch gets a network of its own: a
# started program gets out only through the gate, and only to a fetches host.
if ! command -v curl >/dev/null || ! bwrap --dev-bind / / --unshare-net true 2>/dev/null; then echo "SKIP: the website gate (no bwrap that can start)"
else
    cat > "$TMP/ex/gs.smplr" <<'GS'
listens 18098
main(sys) {
  n = 0
  while n < 3 {
    req = sys.net.request(18098)
    sys.net.reply(200, "hello")
    n = n + 1
  }
}
GS
    cat > "$TMP/ex/gate.smplr" <<'GATE'
runs "curl"
fetches "127.0.0.1"
main(sys) {
  for host in ["127.0.0.1", "localhost"] {
    sys.programs.status("curl", ["-s", "-o", "/dev/null", "-w", "%{http_code}|", "http://" + host + ":18098/"])
  }
  sys.screen.print(sys.net.fetch("http://127.0.0.1:18098/"))
}
GATE
    ( cd "$TMP/ex" && for p in gs gate; do SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" build $p.smplr -o $p >/dev/null 2>&1; done )
    "$TMP/ex/gs" > /dev/null 2>&1 &
    gspid=$!
    for i in $(seq 100); do curl -s -o /dev/null "http://127.0.0.1:18098/ping" && break; sleep 0.05; done
    gateout="$("$TMP/ex/gate" 2>&1)"
    kill "$gspid" 2>/dev/null; wait "$gspid" 2>/dev/null
    if [ "$gateout" = "200|403|hello" ]; then ok; else nope "the website gate (got: $gateout)"; fi
fi

# A started program cannot open a new local socket (the desktop bus is one door
# out of the lock), but a socket pair between its own parts still works.
printf '%s' '#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
int main(void) { int sv[2]; int p = socketpair(AF_UNIX, SOCK_STREAM, 0, sv); int u = socket(AF_UNIX, SOCK_STREAM, 0); printf("pair %d unix %s\n", p, u < 0 ? strerror(errno) : "open"); return 0; }' > "$TMP/sock.c"
command cc -w "$TMP/sock.c" -o "$TMP/sock"
printf 'runs "%s"\nmain(sys) { sys.programs.status("%s", []) }' "$TMP/sock" "$TMP/sock" > "$TMP/input.smplr"
( cd "$TMP" && ./seedc > sockp.c 2>/dev/null && cc -w -o sockp sockp.c 2>/dev/null )
sockout="$( cd "$TMP" && ./sockp 2>/dev/null )"
if [ "$sockout" = "pair 0 unix Permission denied" ] || [ "$(uname -m)" != x86_64 -a "$(uname -m)" != aarch64 ]; then ok; else nope "no new local socket (got: $sockout)"; fi

# A program on a terminal of its own is typed into and shows what it printed;
# end stops it with every program it started, and gives the exit status.
reject "driving a program not on the runs line" \
    'runs "sh"
main(sys) { t = sys.programs.drive("bash", []) }' 'may not run "bash"'
cat > "$TMP/ex/drive.smplr" <<'DRV'
runs "sh"
main(sys) {
  t = sys.programs.drive("sh", ["-c", "printf 'name? '; read x; echo hi $x; sleep 4321 & wait"])
  sys.screen.print(sys.programs.shown(t, 5000).trim)
  sys.programs.press(t, "Ada\r")
  out = ""
  tries = 0
  while (out.contains("hi Ada").not) && (tries < 50) {
    out = out + sys.programs.shown(t, 100)
    tries = tries + 1
  }
  sys.screen.print(out.contains("hi Ada"))
  sys.screen.print(sys.programs.end(t))
  u = sys.programs.drive("sh", ["-c", "exit 3"])
  sys.screen.print(sys.programs.shown(u, 5000).length)
  sys.screen.print(sys.programs.end(u))
}
DRV
drvout="$( cd "$TMP/ex" && export SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache"; "$OLDPWD/simpler" run --dry drive.smplr 2>&1 | head -1; "$OLDPWD/simpler" run drive.smplr 2>&1; pgrep -ax sleep | grep -c 4321 )"
drvout="$(printf '%s' "$drvout" | tr '\n' '|')"
if [ "$drvout" = "would start sh -c printf 'name? '; read x; echo hi \$x; sleep 4321 & wait on a terminal of its own|name?|true|129|0|3|0" ]; then ok; else nope "a program on a terminal of its own (got: $drvout)"; fi

# xs.parallel(f) runs f on all cores and keeps the list order; f has no !IO and
# gives a plain value; an item that fails stops the program with its error.
reject "parallel with a function that prints" \
    'show(n : Int) : Int !IO {
  n
}
main(sys) { ys = [1, 2].parallel(show) }' 'show has !IO; a function that runs in parallel may not print'
reject "parallel with a name that is no function" \
    'main(sys) { ys = [1, 2].parallel(nope) }' 'parallel takes the name of one of your functions'
reject "parallel with a function that gives a list" \
    'pair(n : Int) : List[Int] {
  [n, n]
}
main(sys) { ys = [1, 2].parallel(pair) }' 'parallel gives back an Int, Float, Bool or Str, and pair gives List[Int]'
cat > "$TMP/ex/par.smplr" <<'PAR'
square(n : Int) : Int {
  n * n
}
name(n : Int) : Str {
  "item " + n.toStr
}
pick(n : Int) : Int {
  [1, 2, 3].at(n)
}
main(sys) {
  xs : List[Int] = []
  for i in 0..1000 {
    xs.push(i)
  }
  sq = xs.parallel(square)
  total = 0
  sq.each { s in total = total + s }
  sys.screen.print(total.toStr + " " + sq.at(999).toStr + " " + xs.parallel(name).at(7))
  sys.screen.print([0, 1, 7].parallel(pick).length)
}
PAR
parout="$( cd "$TMP/ex" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run par.smplr 2>&1; echo "rc=$?" )"
parout="$(printf '%s' "$parout" | tr '\n' '|')"
if [ "$parout" = "332833500 998001 item 7|par.smplr:8: list index 7, but the list has 3 items|rc=1" ]; then ok; else nope "parallel (got: $parout)"; fi

# A runs name may start with ~/, and Ctrl+C while status waits stops only the started program.
mkdir -p "$TMP/rh/bin"; printf '#!/bin/sh\necho hi from home\n' > "$TMP/rh/bin/hi"; chmod +x "$TMP/rh/bin/hi"
cat > "$TMP/ex/home.smplr" <<'HOM'
runs "~/bin/hi", "sh"
main(sys) {
  sys.programs.status("~/bin/hi", [])
  sys.screen.print(sys.programs.status("sh", ["-c", "kill -INT $PPID; exit 3"]))
}
HOM
homout="$( cd "$TMP/ex" && HOME="$TMP/rh" SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run home.smplr 2>&1; echo "rc=$?" )"
homout="$(printf '%s' "$homout" | tr '\n' '|')"
if [ "$homout" = "hi from home|3|rc=0" ]; then ok; else nope "runs ~/ and Ctrl+C (got: $homout)"; fi

# A * in a runs line matches when the program is built and when it runs.
reject "a runs star that does not match" 'runs "ech*"
main(sys) { sys.programs.status("ls", []) }' 'may not run "ls"'
cat > "$TMP/ex/star.smplr" <<'STR'
runs "ech*"
main(sys) {
  n = "ec" + "ho"
  sys.programs.status("echo", ["one"])
  sys.programs.status(n, ["two"])
}
STR
starout="$( cd "$TMP/ex" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run star.smplr 2>&1; echo "rc=$?" )"
starout="$(printf '%s' "$starout" | tr '\n' '|')"
if [ "$starout" = "one|two|rc=0" ]; then ok; else nope "runs star (got: $starout)"; fi

# simpler room keeps a copy of the folder first, and --undo puts it back. A stand-in
# for Claude shows the lock: it may write in the folder and not next to it.
mkdir -p "$TMP/rh/box" "$TMP/rh/.local/bin"; printf 'first\n' > "$TMP/rh/box/a.txt"
printf '#!/bin/sh\necho "args $* in ${CLAUDE_CONFIG_DIR#$HOME/}"; echo in > made.txt; echo out 2>/dev/null > ../out.txt || echo blocked\n' > "$TMP/rh/.local/bin/claude"; chmod +x "$TMP/rh/.local/bin/claude"
roomout="$( cd "$TMP/rh" && export HOME="$TMP/rh" SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache"; "$OLDPWD/simpler" room "$TMP/rh" 2>&1; "$OLDPWD/simpler" room box --version 2>&1; ls box; printf 'changed\n' > box/a.txt; "$OLDPWD/simpler" room --undo box > /dev/null; cat box/a.txt; ls box )"
roomout="$(printf '%s' "$roomout" | tr '\n' '|')"
if [ "$roomout" = "simpler: a room needs a folder of its own, not $(cd "$TMP/rh" && pwd -P)|args --dangerously-skip-permissions --version in .local/state/simpler/room|blocked|a.txt|made.txt|first|a.txt" ]; then ok; else nope "room (got: $roomout)"; fi
# A room with --own has a Claude folder of its own and cannot read the shared one.
printf '#!/bin/sh\necho "in ${CLAUDE_CONFIG_DIR#$HOME/}"; cat "$HOME/.local/state/simpler/room/chat.txt" 2>/dev/null || echo "no chat"\n' > "$TMP/rh/.local/bin/claude"
printf 'secret\n' > "$TMP/rh/.local/state/simpler/room/chat.txt"; mkdir -p "$TMP/rh/box2"
ownid=$(printf '%s' "$(cd "$TMP/rh/box2" && pwd -P)" | cksum | cut -d' ' -f1)
ownout="$( cd "$TMP/rh" && export HOME="$TMP/rh" SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache"; "$OLDPWD/simpler" room --own box2 2>&1 | tr '\n' '|' )"
if [ "$ownout" = "in .local/state/simpler/rooms/$ownid|no chat|" ]; then ok; else nope "a room of its own (got: $ownout)"; fi

# simpler undo puts back what the last run wrote, appended to, deleted and renamed.
mkdir -p "$TMP/un/box"; printf 'old a\n' > "$TMP/un/box/a.txt"; printf 'old log\n' > "$TMP/un/box/log.txt"; printf 'gone\n' > "$TMP/un/box/del.txt"; printf 'move\n' > "$TMP/un/box/mv.txt"
cat > "$TMP/un/und.smplr" <<'UNT'
writes "box/", "one.txt"
main(sys) {
  sys.files.write("box/a.txt", "new a\n")
  sys.files.write("box/a.txt", "newer a\n")
  sys.files.write("box/fresh.txt", "made\n")
  sys.files.append("box/log.txt", "more\n")
  sys.files.delete("box/del.txt")
  sys.files.rename("box/mv.txt", "box/moved.txt")
  sys.files.write("one.txt", "single\n")
}
UNT
unout="$( cd "$TMP/un" && export SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" XDG_STATE_HOME="$TMP/state"; "$OLDPWD/simpler" run und.smplr; cat box/a.txt box/log.txt | tr '\n' '|'; ls box | tr '\n' ' '; "$OLDPWD/simpler" undo und.smplr | wc -l; cat box/a.txt box/log.txt box/del.txt box/mv.txt | tr '\n' '|'; ls box . | tr '\n' ' '; "$OLDPWD/simpler" undo und.smplr )"
if [ "$unout" = "newer a|old log|more|a.txt fresh.txt log.txt moved.txt 7
old a|old log|gone|move|.: box und.smplr  box: a.txt del.txt log.txt mv.txt nothing to undo" ]; then ok; else nope "simpler undo (got: $unout)"; fi

# A loop that never ends and never waits is refused; one that waits or leaves is not.
printf 'main(sys) {\n  n = 0\n  while true {\n    n = n + 1\n  }\n}\n' > "$TMP/ex/busy.smplr"
printf 'main(sys) {\n  n = 0\n  while true {\n    n = n + 1\n    if n > 5 {\n      break\n    }\n  }\n  while true {\n    l = sys.stdinLine\n    if l == "" {\n      break\n    }\n  }\n  sys.screen.print("ok")\n}\n' > "$TMP/ex/calm.smplr"
busyout="$( cd "$TMP/ex" && export SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache"; "$OLDPWD/simpler" run busy.smplr 2>&1 | cut -c1-55; "$OLDPWD/simpler" run calm.smplr </dev/null 2>&1 )"
if [ "$busyout" = "$(printf 'busy.smplr:3: this loop never ends and never waits, so \nok')" ]; then ok; else nope "the loop that never waits (got: $busyout)"; fi

# A failed run is kept with its arguments and input, and simpler replay runs it again.
printf 'main(sys) {\n  for l in sys.stdin.split("\\n") {\n    if l.trim.length > 0 {\n      sys.screen.print(l.trim.toInt * sys.args.at(0).toInt)\n    }\n  }\n}\n' > "$TMP/ex/rp.smplr"
rpout="$( cd "$TMP/ex" && export SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" XDG_STATE_HOME="$TMP/state"; "$OLDPWD/simpler" replay rp.smplr 2>&1; printf '2\nx3\n' | "$OLDPWD/simpler" run rp.smplr 10 >/dev/null 2>&1; printf '4\n' | "$OLDPWD/simpler" run rp.smplr 10; "$OLDPWD/simpler" replay --show rp.smplr; "$OLDPWD/simpler" replay rp.smplr 2>&1 )"
if [ "$rpout" = "$(printf 'simpler: no failed run is kept for this program\n40\nargument 1: 10\ninput (5 bytes):\n2\nx3\nrp.smplr:4: "x3" is not a whole number; test with .isInt first\n20')" ] || [ "$rpout" = "$(printf 'simpler: no failed run is kept for this program\n40\nargument 1: 10\ninput (5 bytes):\n2\nx3\n20\nrp.smplr:4: "x3" is not a whole number; test with .isInt first')" ]; then ok; else nope "simpler replay (got: $rpout)"; fi

# A program from a link runs only after a yes typed at the terminal, asked once.
mkdir -p "$TMP/linkbin"
printf '#!/bin/sh\nwhile [ $# -gt 0 ]; do [ "$1" = -o ] && out=$2; shift; done\ncp "$LINKSRC" "$out"\n' > "$TMP/linkbin/curl"; chmod +x "$TMP/linkbin/curl"
lnout="$( cd "$TMP/ex" && export SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" XDG_STATE_HOME="$TMP/state" LINKSRC="$TMP/ex/sum.smplr" PATH="$TMP/linkbin:$PATH"; S="$OLDPWD/simpler"
  setsid "$S" run https://example.com/sum.smplr 5 6 </dev/null 2>&1 | tail -1
  printf 'n\n' | script -qec "$S run https://example.com/sum.smplr 5 6" /dev/null 2>&1 | tr -d '\r' | tail -1
  printf 'y\n' | script -qec "$S run https://example.com/sum.smplr 5 6" /dev/null 2>&1 | tr -d '\r' | tail -1
  setsid "$S" run https://example.com/sum.smplr 5 6 </dev/null 2>&1
  "$S" run http://example.com/sum.smplr 2>&1 )"
if [ "$lnout" = "$(printf 'Run it? [y/N] simpler: not run\nRun it? [y/N] simpler: not run\nRun it? [y/N] sum 11\nsum 11\nsimpler: only an https link is run')" ]; then ok; else nope "run from a link (got: $lnout)"; fi

# Print with no line end, date texts, the home folder, and a fetch with headers.
mkdir -p "$TMP/home/notes" "$TMP/fakebin"; echo secret > "$TMP/home/notes/a.txt"; echo no > "$TMP/home/other.txt"
printf '#!/bin/sh\nfor a in "$@"; do printf "%%s," "$a"; done\nprintf "\\n200"\n' > "$TMP/fakebin/curl"; chmod +x "$TMP/fakebin/curl"
printf 'reads "~/notes/"\nfetches "example.com"\nmain(sys) {\n  sys.screen.part("a")\n  sys.screen.part("b")\n  sys.screen.print("")\n  t = sys.clock.secsOf("2026-11-15T21:00:00Z")\n  sys.screen.print(t)\n  sys.screen.print(sys.clock.secsOf("2026-11-15T22:00:00.5+01:00") - t)\n  sys.screen.print(sys.clock.secsOf("soon"))\n  sys.screen.print(sys.clock.dateAs(sys.clock.secsOf("2026-11-15 22:30"), "%%a %%d %%b %%H:%%M"))\n  sys.screen.print(sys.files.read("~/notes/a.txt").trim)\n  sys.screen.print(sys.net.fetchWith("https://example.com/x", ["X-Key: abc"]))\n  sys.screen.print(sys.net.fetchWith("https://example.com/x", [sys.args.at(0)]))\n}\n' > "$TMP/mod/four.smplr"
( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" "$OLDPWD/simpler" build four.smplr -o four 2>&1 )
fourout="$( cd "$TMP/mod" && HOME="$TMP/home" PATH="$TMP/fakebin:$PATH" LC_ALL=C ./four '@/etc/passwd' 2>&1 | tr '\n' '|' )"
if [ "${fourout#*ab|1794776400|0|0|Sun 15 Nov 22:30|secret|}" != "$fourout" ] && printf '%s' "$fourout" | grep -qF '%{http_code},-H,X-Key: abc,--,https://example.com/x,' && printf '%s' "$fourout" | grep -qF 'four.smplr:14: a header is one line like "Name: value", and there are at most 16; got "@/etc/passwd"'; then ok; else nope "part, dates, home and headers (got: $fourout)"; fi
printf 'reads "~/notes/"\nmain(sys) {\n  sys.screen.print(sys.files.read(sys.args.at(0)))\n}\n' > "$TMP/mod/home.smplr"
homeout="$( cd "$TMP/mod" && HOME="$TMP/home" SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run home.smplr '~/other.txt' 2>&1 )"
if [ "$homeout" = "home.smplr:3: this program may not read \"$TMP/home/other.txt\"" ]; then ok; else nope "the home folder rule (got: $homeout)"; fi

# JSON: the value under a key or at a place, the items and the keys.
cat > "$TMP/mod/json.smplr" <<'JSON'
main(sys) {
  t = "{ \"a.b\": {\"n\": 12.5, \"ok\": true, \"s\": \"hi \\\"you\\\"\\n\\u00e5\\ud83e\\udd17\"}, \"list\": [1, \"two\", [3], {\"k\": null}], \"e\": \"\" }"
  sys.screen.print(t.json("a.b").json("n"))
  sys.screen.print(t.json("a.b").json("ok"))
  sys.screen.print(t.json("a.b").json("s"))
  sys.screen.print(t.json("list").json("1") + "|" + t.json("list").json("2") + "|" + t.json("list").json("3").json("k"))
  sys.screen.print("[" + t.json("nope") + t.json("list").json("9") + t.json("list").json("x") + t.json("e") + "nojson".json("a") + "]")
  sys.screen.print(t.jsonKeys.join(","))
  sys.screen.print(t.json("list").jsonItems.join(";"))
  sys.screen.print(t.json("a.b").jsonItems.length)
  sys.screen.print("{\"a\": [".jsonItems.length + "[1,".jsonItems.length + "".jsonKeys.length)
}
JSON
jsonout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run json.smplr 2>&1 | tr '\n' '|' )"
if [ "$jsonout" = '12.5|true|hi "you"|å🤗|two|[3]|null|[]|a.b,list,e|1;two;[3];{"k": null}|3|2|' ]; then ok; else nope "json, jsonItems and jsonKeys (got: $jsonout)"; fi
printf 'main(sys) {\n  sys.screen.print(5.json("a"))\n}\n' > "$TMP/mod/json2.smplr"
jsonout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run json2.smplr 2>&1 )"
if [ "$jsonout" = 'json2.smplr:2: Int has no method json' ]; then ok; else nope "json on a number (got: $jsonout)"; fi

# split takes a divider of many characters.
printf 'main(sys) {\n  sys.screen.print("a--b-c--".split("--").join("|"))\n  sys.screen.print("a,b".split(",").length + "abc".split("").length + "".split(",").length)\n}\n' > "$TMP/mod/split.smplr"
splitout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run split.smplr 2>&1 | tr '\n' '~' )"
if [ "$splitout" = 'a|b-c|~4~' ]; then ok; else nope "split on a long divider (got: $splitout)"; fi

# Bytes: a file as a list of numbers, numbers and texts picked out of it, and written back.
printf '\002\007\000\000\000\373\377\377\377\377\377\377\377\023\000\000\000\000\000\000\000\000\000\004\100h\303\245i\000z' > "$TMP/mod/in.bin"
cat > "$TMP/mod/bytes.smplr" <<'BYTES'
reads "./"
writes "./"
main(sys) {
  b = sys.files.readBytes("in.bin")
  sys.screen.print(b.length)
  sys.screen.print(b.at(0))
  sys.screen.print(b.intAt(1, 4))
  sys.screen.print(b.intAt(5, 8))
  sys.screen.print(b.intAt(13, 4))
  sys.screen.print(b.floatAt(17))
  sys.screen.print(b.textAt(25, 4) + "|" + b.textAt(25, 6))
  sys.screen.print(sys.files.readBytes("nofile").length)
  out = "på".bytes
  out.push(0)
  out.push(255)
  sys.files.writeBytes("out.bin", out)
  sys.screen.print(sys.files.readBytes("out.bin").length)
  sys.screen.print(b.intAt(27, 4))
}
BYTES
bytesout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run bytes.smplr 2>&1 | tr '\n' '~'; od -An -tu1 out.bin | tr -s ' ' )"
if [ "$bytesout" = '31~2~7~-5~19~2.5~håi|håi~0~5~2046847397~ 112 195 165 0 255' ]; then ok; else nope "reading and writing bytes (got: $bytesout)"; fi
printf 'writes "./"\nmain(sys) {\n  sys.files.writeBytes("o2.bin", [1, 300])\n}\n' > "$TMP/mod/bytes2.smplr"
bytesout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run bytes2.smplr 2>&1; ls o2.bin 2>/dev/null )"
if [ "$bytesout" = 'bytes2.smplr:3: a byte is a number from 0 to 255, but place 1 has 300' ]; then ok; else nope "a byte out of range (got: $bytesout)"; fi
printf 'reads "./"\nmain(sys) {\n  sys.screen.print(sys.files.readBytes("in.bin").intAt(30, 4))\n}\n' > "$TMP/mod/bytes3.smplr"
bytesout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run bytes3.smplr 2>&1 )"
if [ "$bytesout" = 'bytes3.smplr:3: intAt wants 4 bytes from place 30, but there are 31' ]; then ok; else nope "bytes past the end (got: $bytesout)"; fi
printf 'reads "./"\nlast(b : Bytes) : Int {\n  b.at(b.length - 1)\n}\nmain(sys) {\n  b = sys.files.readBytes("in.bin")\n  sys.screen.print(last(b))\n  sys.screen.print(b.findByte(19, 0) + b.findByte(19, 14) + b.findByte(2, 99))\n  l = [104, 105, 0, 1]\n  sys.screen.print(l.textAt(0, 2) + l.intAt(2, 2).toStr)\n  sys.screen.print(b)\n}\n' > "$TMP/mod/bytes4.smplr"
bytesout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run bytes4.smplr 2>&1 )"
if [ "$bytesout" = 'bytes4.smplr:11: Bytes cannot be printed; print a part with textAt(i, n) or a number with at(i)' ]; then
    sed -i '11d' "$TMP/mod/bytes4.smplr"
    bytesout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run bytes4.smplr 2>&1 | tr '\n' '~' )"
    if [ "$bytesout" = '122~11~hi256~' ]; then ok; else nope "the Bytes type and a list of bytes (got: $bytesout)"; fi
else nope "printing Bytes (got: $bytesout)"; fi

# Patterns: reTest, reFind, reAll, reReplace and reSplit, and a backslash that stays in a text.
cat > "$TMP/mod/re.smplr" <<'REGEX'
main(sys) {
  t = "Mail bob@site.com or ann@web.no before 2026-11-16, cost $45."
  sys.screen.print(t.reTest("\d{4}-\d\d-\d\d"))
  sys.screen.print(t.reTest("^\d"))
  m = t.reFind("(\w+)@(\w+)\.(\w+)")
  sys.screen.print(m.join("|"))
  sys.screen.print(t.reFind("nothing").length)
  for g in t.reAll("(\w+)@[\w.]+") {
    sys.screen.print(g.at(1) + " <" + g.at(0) + ">")
  }
  sys.screen.print(t.reReplace("(\d+)-(\d+)-(\d+)", "$3.$2.$1"))
  sys.screen.print("a1b22c".reSplit("\d+").join(","))
  sys.screen.print("Blåbær og RØDBETER".reReplace("(?i)[æøå]", "_"))
  sys.screen.print("tab\there \\d".reFind("\t(\w+) \\\\d").at(1))
  sys.screen.print('say "hi" twice'.reFind('"([^"]*)"').at(1))
  sys.screen.print("x".reTest("(a"))
}
REGEX
reout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run re.smplr 2>&1 | tr '\n' '~' )"
if [ "$reout" = 're.smplr:16: the pattern "(a" has a ( with no )~true~false~bob@site.com|bob|site|com~0~bob <bob@site.com>~ann <ann@web.no>~Mail bob@site.com or ann@web.no before 16.11.2026, cost $45.~a,b,c~Bl_b_r og R_DBETER~here~hi~' ]; then ok; else nope "patterns (got: $reout)"; fi
cat > "$TMP/mod/re2.smplr" <<'REGEX'
main(sys) {
  sys.screen.print("foo foobar".reAll("foo(?!bar)").length + "cost $45 or 67".reAll("(?<=\$)\d+").length)
  sys.screen.print("abccd".reFind("(\w)\1").at(0) + "<a><b>".reFind("<.+?>").at(0) + "one\ntwo".reAll("(?m)^\w+$").length.toStr)
  a = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa!"
  sys.screen.print(a.reTest("(a+)+$"))
}
REGEX
reout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run re2.smplr 2>&1 | tr '\n' '~' )"
if [ "$reout" = 're2.smplr:5: the pattern "(a+)+$" takes too many steps on this text~2~cc<a>2~' ]; then ok; else nope "looking ahead and behind, and a pattern that runs away (got: $reout)"; fi

# exit(n) ends the program with that status and no message.
printf 'pick(n : Int) : Int {\n  if n > 1 {\n    exit(n)\n  }\n  n\n}\nmain(sys) {\n  sys.screen.print("a")\n  pick(1)\n  pick(7)\n  sys.screen.print("b")\n}\n' > "$TMP/mod/exit.smplr"
exitout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run exit.smplr 2>&1; echo "rc=$?" )"
if [ "$exitout" = "a
rc=7" ]; then ok; else nope "exit with a status (got: $exitout)"; fi

# start (no waiting), millis and char.
printf 'runs "sleep", "sh"\nmain(sys) {\n  sys.screen.print(65.char + 229.char + 8364.char + 128512.char + "|" + 0.char + "|" + "€".code.toStr + " " + "a".code.toStr)\n  a = sys.clock.millis\n  pid = sys.programs.start("sleep", ["0.3"])\n  b = sys.clock.millis\n  sys.screen.print((pid > 1).toStr + " " + (b - a < 200).toStr + " " + (a > sys.clock.now * 1000 - 2000).toStr)\n  sys.screen.print(sys.programs.status("sh", ["-c", "kill -0 " + pid.toStr]))\n  p = "s" + "h"\n  q = "ech" + "o"\n  sys.programs.start(p, [])\n  sys.programs.start(q, [])\n}\n' > "$TMP/mod/start.smplr"
stout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run start.smplr 2>&1 | tr '\n\0' '~0' )"
if [ "$stout" = 'Aå€😀|0|8364 97~true true true~0~start.smplr:12: this program may not run "echo"~' ]; then ok; else nope "start, millis and char (got: $stout)"; fi

# setVar reaches a started program; self is the source under run.
printf 'runs "sh"\nmain(sys) {\n  sys.screen.print(sys.env.self.reFind("[^/]+$").at(0))\n  sys.env.setVar("GREETING", "hi there")\n  sys.screen.print(sys.env.var("GREETING") + sys.env.setVar("A=B", "x").toStr)\n  sys.screen.print(sys.programs.run("sh", ["-c", "echo $GREETING [$SIMPLER_SELF]"]).trim)\n}\n' > "$TMP/mod/envset.smplr"
envout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run envset.smplr 2>&1 | tr '\n' '~' )"
if [ "$envout" = 'envset.smplr~hi there0~hi there []~' ]; then ok; else nope "setVar and self (got: $envout)"; fi

# build never writes over a file that is not a built program (it ate a
# script named like the source, 2026-11-16), but does replace its own output.
printf 'main(sys) {\n  sys.screen.print("hi")\n}\n' > "$TMP/mod/keep.smplr"
printf '#!/bin/sh\necho script\n' > "$TMP/mod/keep"
keepout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" "$OLDPWD/simpler" build keep.smplr 2>&1; echo "rc=$?"; cat keep; SIMPLERC="$TMP/seedc" "$OLDPWD/simpler" build keep.smplr -o keep2 && SIMPLERC="$TMP/seedc" "$OLDPWD/simpler" build keep.smplr -o keep2 && ./keep2 )"
if [ "$keepout" = "simpler build: keep exists and is not a built program; pick another name with -o
rc=1
#!/bin/sh
echo script
hi" ]; then ok; else nope "build keeps a file that is not a built program (got: $keepout)"; fi

# A text is counted in letters, never in bytes: length, at, slice, find, the
# pads, capitals, the loop and the regex all agree with what a reader sees.
cat > "$TMP/mod/letters.smplr" <<'SMPLR'
main(sys) {
  s = "blå × bær·c"
  sys.screen.print(s.length.toStr + " " + s.at(2) + s.at(4) + " " + s.slice(1, 3) + " " + s.find("bær").toStr + " " + s.find("q").toStr)
  n = 0
  for c in s {
    if c == "å" || c == "æ" {
      n = n + 1
    }
  }
  sys.screen.print(n.toStr + " " + s.upper + " " + "ÆØÅ".lower + " [" + "å".padLeft(3) + "] [" + "å".padRight(3) + "]")
  sys.screen.print("⌂ blå·bær x2".reAll("\w+").length.toStr + " " + "så⌂".reFind("\bså\b").length.toStr)
  r = ""
  i = s.length - 1
  while i >= 0 {
    r = r + s.at(i)
    i = i - 1
  }
  sys.screen.print(r + " " + s.slice(4, 11).slice(2, 5) + s.slice(11, 11) + "|")
}
SMPLR
letout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run letters.smplr 2>&1 | tr '\n' '~' )"
if [ "$letout" = '11 å× lå 6 -1~2 BLÅ × BÆR·C æøå [  å] [å  ]~3 1~c·ræb × ålb bær|~' ]; then ok; else nope "a text is counted in letters (got: $letout)"; fi
printf 'main(sys) {\n  sys.screen.print("blå".at(3))\n}\n' > "$TMP/mod/letend.smplr"
letend="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run letend.smplr 2>&1 )"
if [ "$letend" = 'letend.smplr:2: text index 3, but the text has 3 characters' ]; then ok; else nope "index past the last letter (got: $letend)"; fi

# A map finds a key by its hash, holds own types and lists, and stops with a
# plain message on a key it does not have.
cat > "$TMP/mod/maps.smplr" <<'SMPLR'
P = type { x : Int, name : Str }
V = type { I(Int) S(Str) L(List[V]) }
show(v : V) : Str {
  v.match {
    I(n) -> n.toStr
    S(s) -> s
    L(xs) -> "list of " + xs.length.toStr
  }
}
main(sys) {
  m = Map()
  for i in 0..50000 {
    m.set("k" + i.toStr, i)
  }
  m.set("k7", 70)
  t = 0
  for i in 0..50000 {
    t = t + m.get("k" + i.toStr)
  }
  sys.screen.print(t.toStr + " " + m.keys.length.toStr + " " + m.keys.at(49999) + " " + m.get("no").toStr + " " + m.byValue.at(0))
  names = Map[Str]()
  names.set("a", "b")
  sys.screen.print(names.get("a") + "[" + names.get("q") + "]")
  ps = Map[P]()
  ps.set("one", P(x = 3, name = "n"))
  e : Map[V] = Map()
  e.set("k", S("text"))
  e.set("k", L([I(1), S("x")]))
  ls = Map[List[Int]]()
  ls.set("q", [1, 2])
  ls.get("q").push(3)
  sys.screen.print(ps.get("one").x.toStr + " " + show(e.get("k")) + " " + ls.get("q").length.toStr)
  sys.screen.print(ps.get("two").name)
}
SMPLR
mapout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run maps.smplr 2>&1 | tr '\n' '~' )"
if [ "$mapout" = 'maps.smplr:33: the map has no key "two"; ask .has(key) first~1249975063 50000 k49999 0 k49999~b[]~3 list of 2 3~' ]; then ok; else nope "maps: hash, own types, missing key (got: $mapout)"; fi

# A variant case holds up to eight values; four used to give C that did not compile.
cat > "$TMP/mod/wide.smplr" <<'SMPLR'
K = type { A(Int) F(Str, Int, Int, List[Int]) G(Int, Int, Int, Int, Int, Int, Int, Str) }
show(k : K) : Str {
  k.match {
    A(n) -> n.toStr
    F(s, a, b, xs) -> s + (a + b + xs.length).toStr
    G(a, b, c, d, e, f, g, h) -> h + (a + b + c + d + e + f + g).toStr
  }
}
main(sys) {
  sys.screen.print(show(A(1)) + " " + show(F("f", 1, 2, [7, 8])) + " " + show(G(1, 2, 3, 4, 5, 6, 7, "g")))
}
SMPLR
wideout="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run wide.smplr 2>&1 )"
if [ "$wideout" = '1 f5 g28' ]; then ok; else nope "a case with four and eight values (got: $wideout)"; fi

# A new list that holds the caller's rows may grow without !Mut; a change to a
# row inside it still needs !Mut. slice with one value runs to the end.
cat > "$TMP/mod/hold1.smplr" <<'SMPLR'
f(rows : List[List[Str]]) : List[List[Str]] {
  out : List[List[Str]] = []
  for i in 0..rows.length {
    out.push(rows.at(i))
  }
  two = [rows.at(0)]
  two.push(["own"])
  out.remove(0)
  out
}
main(sys) {
  a : List[List[Str]] = [["x"], ["y"]]
  sys.screen.print(f(a).length.toStr + " " + a.length.toStr + " " + "hello".slice(2) + "|" + "blå".slice(3) + "|")
}
SMPLR
hold1="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run hold1.smplr 2>&1 )"
if [ "$hold1" = '1 2 llo||' ]; then ok; else nope "a new list of handed rows needs no !Mut (got: $hold1)"; fi
for body in 'out.at(0).push("z")' 'for r in out { r.push("z") }' 'two = out
  two.at(0).set(0, "q")' 'tmp : List[List[List[Str]]] = []
  tmp.push([rows.at(0)])
  tmp.at(0).at(0).push("z")'; do
  printf 'f(rows : List[List[Str]]) : List[List[Str]] {\n  out = [rows.at(0)]\n  %s\n  out\n}\nmain(sys) {\n  a : List[List[Str]] = [["x"]]\n  sys.screen.print(f(a).length.toStr)\n}\n' "$body" > "$TMP/mod/hold2.smplr"
  hold2="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" what hold2.smplr 2>&1 )"
  if printf '%s' "$hold2" | grep -qF "handed  (in f"; then ok; else nope "a change to a handed row inside a new list is listed as !Mut (got: $hold2)"; fi
done

# A record field can be set; setting one on a handed record needs !Mut; the
# value must have the field's type; only a record has fields to set.
cat > "$TMP/mod/setf.smplr" <<'SMPLR'
Pos = type { x : Int, y : Int }
St = type { name : Str, n : Int, at : Pos, f : Float, xs : List[Int] }
bump(s : St) : Int !Mut {
  s.n = s.n + 1
  s.at.x = s.at.x + 10
  s.n
}
main(sys) {
  s = St(name = "a", n = 1, at = Pos(x = 1, y = 2), f = 1.5, xs = [1])
  t = s
  t.name = "b" + s.name
  bump(s)
  bump(t)
  s.f = s.f * 2.0
  ps = [Pos(x = 0, y = 0), Pos(x = 5, y = 5)]
  ps.at(1).y = 9
  for p in ps {
    p.x = p.x + 1
  }
  sys.screen.print(s.name + " " + s.n.toStr + " " + s.at.x.toStr + " " + s.f.toStr + " " + ps.at(1).y.toStr + " " + ps.at(0).x.toStr + ps.at(1).x.toStr)
}
SMPLR
setf="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run setf.smplr 2>&1 )"
if [ "$setf" = 'ba 3 21 3 9 16' ]; then ok; else nope "a record field can be set (got: $setf)"; fi
sed -i 's/ : Int !Mut {/ : Int {/' "$TMP/mod/setf.smplr"
setf="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" what setf.smplr 2>&1 )"
if printf '%s' "$setf" | grep -qF "handed  (in bump"; then ok; else nope "setting a field of a handed record is listed as !Mut (got: $setf)"; fi
printf 'P = type { x : Int }\nmain(sys) {\n  p = P(x = 1)\n  p.x = "a"\n  sys.screen.print(p.x.toStr)\n}\n' > "$TMP/mod/setf2.smplr"
setf="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run setf2.smplr 2>&1 )"
if [ "$setf" = 'setf2.smplr:4: .x expects Int, got Str' ]; then ok; else nope "a field takes only its own type (got: $setf)"; fi
printf 'main(sys) {\n  s = "abc"\n  s.length = 2\n  sys.screen.print(s)\n}\n' > "$TMP/mod/setf3.smplr"
setf="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run setf3.smplr 2>&1 )"
if [ "$setf" = 'setf3.smplr:3: only a field of a record can be set with =, and this is Str' ]; then ok; else nope "only a record field can be set (got: $setf)"; fi

# Rows of the caller sorted into groups of the function's own: adding to a
# group is free, changing a row in a group needs !Mut. String is read as Str.
cat > "$TMP/mod/grp.smplr" <<'SMPLR'
group(rows : List[List[Str]], tag : String) : Int {
  groups : List[List[List[Str]]] = []
  m = Map()
  for row in rows {
    key = row.at(0)
    if !m.has(key) {
      m.set(key, groups.length)
      ng : List[List[Str]] = []
      groups.push(ng)
    }
    groups.at(m.get(key)).push(row)
  }
  for g in groups {
    g.push([tag])
  }
  groups.length
}
main(sys) {
  a : List[List[Str]] = [["x"], ["y"], ["x"]]
  sys.screen.print(group(a, "t").toStr)
}
SMPLR
grp="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run grp.smplr 2>&1 )"
if [ "$grp" = '2' ]; then ok; else nope "rows sorted into own groups need no !Mut (got: $grp)"; fi
sed -i 's/    g.push(\[tag\])/    for r in g {\n      r.push(tag)\n    }/' "$TMP/mod/grp.smplr"
grp="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" what grp.smplr 2>&1 )"
if printf '%s' "$grp" | grep -qF "handed  (in group"; then ok; else nope "a change to a row inside a group is listed as !Mut (got: $grp)"; fi

# The character with number 0 lives in a text like any other: one letter, code 0, a real 0 byte on the way out and in.
cat > "$TMP/mod/nul.smplr" <<'SMPLR'
writes "nul.bin"
reads "nul.bin"
main(sys) {
  z = 0.char
  s = "a".concat(z).concat("b")
  sys.screen.print(s.length)
  sys.screen.print(s.bytes.length)
  sys.screen.print(s.bytes.at(1))
  sys.screen.print(s.at(1).code)
  sys.files.write("nul.bin", s)
  r = sys.files.read("nul.bin")?
  sys.screen.print(r == s)
  l = sys.stdinLine
  sys.screen.print(l.length)
  sys.screen.print(l.bytes.textAt(0, 3) == "x".concat(z).concat("y"))
  sys.screen.part(sys.stdin)
  sys.screen.print(s)
  sys.screen.error(z)
}
SMPLR
nul="$( cd "$TMP/mod" && printf 'x\0y\np\0q' | SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run nul.smplr 2>"$TMP/mod/nul.err" | od -An -c | tr -s ' \n' ' ' )"
if [ "$nul" = ' 3 \n 3 \n 0 \n 0 \n t r u e \n 4 \n t r u e \n p \0 q a \0 b \n ' ]; then ok; else nope "a text holds the character with number 0 (got: $nul)"; fi
if [ "$(od -An -c "$TMP/mod/nul.bin" | tr -s ' ' ' ')" = ' a \0 b' ]; then ok; else nope "a written file gets a real 0 byte"; fi
if [ "$(od -An -c "$TMP/mod/nul.err" | tr -s ' ' ' ')" = ' \0 \n' ]; then ok; else nope "an error line gets a real 0 byte"; fi

# A file name inside a text is no module call: notes.smplr may write "notes.txt".
printf 'writes "notes.txt"\nmain(sys) {\n  sys.files.write("notes.txt", "hi")\n  sys.screen.print("ok")\n}\n' > "$TMP/mod/notes.smplr"
nt="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run notes.smplr 2>&1 )"
if [ "$nt" = 'ok' ]; then ok; else nope "a program may write a file that has its own name (got: $nt)"; fi

# install.sh from the repo: the installed command has every page part and every module.
ih="$TMP/insthome"; mkdir -p "$ih/.claude" "$ih/w"
HOME="$ih" sh ./install.sh > /dev/null 2>&1
printf 'main(sys) {\n  sys.screen.print("hello")\n}\n' > "$ih/w/hello.smplr"
inst="$( cd "$ih/w" && HOME="$ih" "$ih/.local/bin/simpler" run hello.smplr 2>&1; HOME="$ih" "$ih/.local/bin/simpler" prompt all 2>&1 | grep -c '^simpler: '; for m in $(cd "$OLDPWD/lib" && echo *.smplr); do HOME="$ih" "$ih/.local/bin/simpler" prompt "${m%.smplr}" 2>&1 | grep -c '^simpler: '; done | sort -u; [ -f "$ih/.claude/skills/simpler/SKILL.md" ] && echo skill )"
if [ "$inst" = 'hello
0
0
skill' ]; then ok; else nope "an installed simpler runs, has all pages and modules, and the skill (got: $inst)"; fi

# A name in capitals that a C header also defines (B0, ECHO, SIGINT, EINTR) is fine as a name.
printf 'steps(A0 : Int, B0 : Int) : Int {\n  ECHO = A0 + B0\n  SIGINT = ECHO * 2\n  SIGINT\n}\nmain(sys) {\n  EINTR = steps(1, 2)\n  sys.screen.print(EINTR)\n}\n' > "$TMP/mod/caps.smplr"
cp="$( cd "$TMP/mod" && SIMPLERC="$TMP/seedc" XDG_CACHE_HOME="$TMP/cache" "$OLDPWD/simpler" run caps.smplr 2>&1 )"
if [ "$cp" = '6' ]; then ok; else nope "names in capitals that C headers define (got: $cp)"; fi

# --- summary ------------------------------------------------------------------
echo "------------------------------------"
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
