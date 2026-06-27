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
check_err "nested record field rejected" \
    'A = type { x : Int }
B = type { a : A }
main(sys) { sys.screen.print(1) }' \
    "must be Int, Str, or Bool"

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

# --- summary ------------------------------------------------------------------
echo "------------------------------------"
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
