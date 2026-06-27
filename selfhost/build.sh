#!/usr/bin/env bash
# Build the Simpler compiler with no Rust, just a C compiler.
#
# simpler.c is the self-hosted compiler (simpler.smplr) transpiled to C and
# committed as the bootstrap seed. This compiles it to ./simpler.
#
#   ./build.sh             build ./simpler from the committed C seed
#   ./build.sh --rebuild   then recompile simpler.smplr with it and refresh the seed
#
# The compiler reads its source from a file named input.smplr in the working
# directory and writes C to stdout, so to compile any program:
#   cp prog.smplr input.smplr && ./simpler > prog.c && cc prog.c -o prog
set -e
cd "$(dirname "$0")"
cc -O2 simpler.c -o simpler
echo "built ./simpler (no Rust involved)"

if [ "$1" = "--rebuild" ]; then
    cp simpler.smplr input.smplr
    ./simpler > simpler.c.new
    rm -f input.smplr
    if diff -q simpler.c simpler.c.new >/dev/null 2>&1; then
        echo "fixpoint holds: ./simpler reproduces simpler.c byte-for-byte"
        rm -f simpler.c.new
    else
        mv simpler.c.new simpler.c
        echo "simpler.c refreshed from simpler.smplr (rebuild again to confirm the fixpoint)"
    fi
fi
