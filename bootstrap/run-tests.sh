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
    # with too few args it prints usage instead of touching the disk
    usage="$( cd "$TMP" && ./ln 2>&1 )"
    if [ "$usage" = "usage: linenum <in> <out>" ]; then ok; else nope "linenum usage (got: $usage)"; fi
    rm -f "$TMP/in.txt" "$TMP/out.txt"
else
    nope "linenum tool compiles"
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
