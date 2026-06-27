#!/usr/bin/env bash
# Regression harness for the Simpler bootstrap compiler.
#
# Three kinds of check, one command, green or red:
#   1. every example runs and matches its golden output,
#   2. every known-bad program is rejected with the expected message,
#   3. `fmt` is idempotent on every example, and `test` reports correctly.
#
# Run from anywhere; it operates in the bootstrap/ directory and removes the
# generated .c / binary artifacts on exit.
set -u
cd "$(dirname "$0")"
PATH="/usr/bin:$PATH"            # real cc, not the ~/bin/cc session launcher
SIMPLER=./target/release/simpler
TMP="$(mktemp -d)"
pass=0
fail=0

cleanup() {
    find examples -maxdepth 1 -type f ! -name '*.smplr' ! -name '*.txt' -delete 2>/dev/null
    # remove generated artifacts in selfhost, but keep the committed seed simpler.c
    find ../selfhost -maxdepth 1 -type f -name '*.c' ! -name 'simpler.c' -delete 2>/dev/null
    for f in ../selfhost/*.smplr; do rm -f "${f%.smplr}"; done   # per-source binaries
    rm -f ../selfhost/input.smplr input.smplr
    rm -rf "$TMP"
}
trap cleanup EXIT

ok()  { pass=$((pass + 1)); }
nope() { fail=$((fail + 1)); echo "FAIL: $1"; }

cargo build --release -q || { echo "compiler build failed"; exit 1; }

# --- 1. examples produce their golden output ----------------------------------
check_run() { # name  expected
    local out
    out="$("$SIMPLER" run "examples/$1.smplr" 2>/dev/null)"
    if [ "$out" = "$2" ]; then ok; else nope "run $1
       expected: $(printf '%q' "$2")
       got:      $(printf '%q' "$out")"; fi
}
check_run hello "hello, world"
check_run m2    "$(printf '0\n1\n2\nbig\n3')"
check_run m3    "$(printf 'hello,\nworld')"
check_run m3b   "$(printf '[mail] to=boss@co subject=Daily report\nQuarterly numbers look good.\n\nsent')"
check_run m3c   "$(printf 'Quarterly numbers look good.\n\n6')"
check_run m5a   "$(printf '7\n25\n13')"
check_run m5b   "$(printf 'hello\n42\ndot')"
check_run m5c   "$(printf '5\ne\nell\nhello world\n42!\ntrue\nfalse\n1\nfalse\ntrue')"
check_run m5d   "$(printf '2\n3\n4')"
check_run m5e   "20"
check_run m5f   "$(printf '3\n20\n5\nhi\n42')"
check_run m6    "15"

# the self-hosted lexer (written in Simpler) scans the full token set
lout="$("$SIMPLER" run ../selfhost/lexer.smplr 2>/dev/null)"
lwant="$(printf 'id a\nop .\nid f\nop (\nstr hi\nop )\nop !\nop ?\nop +\nop -\nop *\nop /\nop <\nop >\nop =\nop ==\nop ->\nop :\nop ,\nop [\nop ]\nop {\nop }\nint 0\neof')"
[ "$lout" = "$lwant" ] && ok || nope "self-hosted lexer (got: $lout)"

# the self-hosted calc (lex -> parse -> {eval, emit C}) respects precedence
cout="$("$SIMPLER" run ../selfhost/calc.smplr 2>/dev/null)"
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
SEEDC="$TMP/seedc"
cc -O2 ../selfhost/simpler.c -o "$SEEDC" 2>/dev/null
cp ../selfhost/sample.smplr "$TMP/input.smplr"
( cd "$TMP" && ./seedc > sh.c 2>/dev/null )
rm -f "$TMP/input.smplr"
sampexp="$("$SIMPLER" run ../selfhost/sample.smplr 2>/dev/null)"
if cc -o "$TMP/sh" "$TMP/sh.c" 2>/dev/null && [ "$("$TMP/sh")" = "$sampexp" ] && [ "$sampexp" = "$(printf 'a\nb\nhi!\n72')" ]; then ok; else nope "self-hosted compiler builds the sample"; fi

# a real read->transform->write tool, compiled by the self-hosted compiler:
# linenum.smplr reads in.txt, numbers every line, writes out.txt. Proves the
# file-output capability (sys.files.write) end to end, not just that it parses.
cp ../selfhost/linenum.smplr "$TMP/input.smplr"
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

# wordfreq.smplr, a real word-frequency tool built by the self-hosted compiler:
# read + replace + split + Map tally + .keys enumeration, the program a map is
# for. Exercises the whole text-and-map stack in one binary.
cp ../selfhost/wordfreq.smplr "$TMP/input.smplr"
( cd "$TMP" && ./seedc > wf.c 2>/dev/null )
rm -f "$TMP/input.smplr"
if cc -o "$TMP/wf" "$TMP/wf.c" 2>/dev/null; then
    printf 'the cat sat\non the mat\nthe cat ran\n' > "$TMP/prose.txt"
    want="$(printf 'the: 3\ncat: 2\nsat: 1\non: 1\nmat: 1\nran: 1')"
    # a real Unix filter: same result from a file argument or from stdin
    if [ "$("$TMP/wf" "$TMP/prose.txt")" = "$want" ]; then ok; else nope "wordfreq from file"; fi
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
cp ../selfhost/sortlines.smplr "$TMP/input.smplr"
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
cp ../selfhost/average.smplr "$TMP/input.smplr"
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

# sumcol.smplr, a real CSV column-summer, built by the self-hosted compiler:
# reads a file, splits into lines, splits each on commas, sums one column.
# Exercises read + split + toInt + nested each/if with a binding inside the
# loop body (the case that needs hoisting through .each).
cp ../selfhost/sumcol.smplr "$TMP/input.smplr"
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
    "needs Int operands"
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
# a failable read needs !Fail, not just !IO
reject "missing Fail" \
    'rd(f : Files) : Str !IO { f.read("p")? }
main(sys) { sys.screen.print(rd(sys.files)) }' \
    "uses !Fail but does not declare it"
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
    printf '%s' "$2" > "$TMP/e.smplr"
    local err rc
    err="$("$SIMPLER" run "$TMP/e.smplr" 2>&1)"
    rc=$?
    if [ "$rc" -ne 0 ] && printf '%s' "$err" | grep -qF "$3"; then
        ok
    else
        nope "$1 (rc=$rc, got: $err)"
    fi
}
check_err "undeclared effect" \
    'main(sys) { greet(sys.screen) }
greet(screen : Screen) { screen.print("hi") }' \
    "uses !IO but isn't declared"
check_err "capability not held" \
    'main(sys) { greet(sys.screen) }
greet(screen : Screen) !IO { sys.screen.print("hi") }' \
    'unknown name `sys`'
check_err "print on a non-Screen" \
    'main(sys) { greet("x") }
greet(s : Str) !IO { s.print("hi") }' \
    "has no method"
check_err "wrong argument type" \
    'main(sys) { greet(sys.screen, 5) }
greet(screen : Screen, who : Str) !IO { screen.print(who) }' \
    "expects Str, got Int"
check_err "Int op needs Ints" \
    'main(sys) { x = 1 + "a" sys.screen.print(x) }' \
    "needs Int operands"
check_err "read needs ?" \
    'main(sys) { x = sys.files.read("x") sys.screen.print(x) }' \
    "can fail; bind it"
check_err "? on non-failable" \
    'main(sys) { x = twice(3)? sys.screen.print(x) }
twice(n : Int) : Int { n + n }' \
    "cannot fail; the"
check_err "wrong return type" \
    'twice(n : Int) : Int { "no" }
main(sys) { sys.screen.print(twice(2)) }' \
    "should return"
check_err "assert needs Bool" \
    'main(sys) { assert(5) }' \
    "needs a Bool"
check_err "no main" \
    'twice(n : Int) : Int { n + n }' \
    'no `main` function found'
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
    "binding(s)"
check_err "case payload count mismatch" \
    'Expr = type { Lit(Int) Add(Expr, Expr) }
main(sys) { e = Add(Lit(1)) sys.screen.print(1) }' \
    "positional payload"
check_err "match-value arm must produce a value" \
    'Expr = type { Lit(Int) }
main(sys) { sys.screen.print(ev(Lit(1))) }
ev(e : Expr) : Int { e.match { Lit(n) -> { } } }' \
    "must produce"
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
    "is missing: B"
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

# --- 3. fmt is idempotent; test reports correctly -----------------------------
for f in hello m2 m3 m3b m3c tests; do
    cp "examples/$f.smplr" "$TMP/$f.a"
    "$SIMPLER" fmt "$TMP/$f.a" >/dev/null 2>&1
    cp "$TMP/$f.a" "$TMP/$f.b"
    "$SIMPLER" fmt "$TMP/$f.b" >/dev/null 2>&1
    if diff -q "$TMP/$f.a" "$TMP/$f.b" >/dev/null; then ok; else nope "fmt idempotent $f"; fi
done

out="$("$SIMPLER" test examples/tests.smplr 2>/dev/null)"
rc=$?
if [ "$rc" -eq 0 ] && printf '%s' "$out" | grep -qF "2/2 passed"; then ok; else nope "test all-pass"; fi

printf 'test_a() { assert(1 == 1) }\ntest_b() { assert(1 == 2) }\n' > "$TMP/t.smplr"
out="$("$SIMPLER" test "$TMP/t.smplr" 2>/dev/null)"
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
# Rust dependency is gone and keeps the committed C in sync with the source.
if cc -O2 ../selfhost/simpler.c -o "$TMP/seedc" 2>/dev/null; then
    cp ../selfhost/simpler.smplr "$TMP/input.smplr"
    ( cd "$TMP" && ./seedc > regen.c 2>/dev/null )
    if diff -q "$TMP/regen.c" ../selfhost/simpler.c >/dev/null 2>&1; then ok; else nope "committed simpler.c is stale (regenerate it from simpler.smplr)"; fi
else
    nope "committed simpler.c does not compile"
fi

# --- summary ------------------------------------------------------------------
echo "------------------------------------"
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
