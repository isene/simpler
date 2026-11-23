#!/bin/sh
# Install the simpler command for this user. Needs a C compiler; curl when run from the web:
#
#   curl -fsSL https://raw.githubusercontent.com/isene/simpler/master/install.sh | sh
#
# SIMPLER_HOME (default ~/.local/lib/simpler) gets the files, SIMPLER_BIN
# (default ~/.local/bin) the command.
set -e
dir=${SIMPLER_HOME:-$HOME/.local/lib/simpler}
bin=${SIMPLER_BIN:-$HOME/.local/bin}
base=https://raw.githubusercontent.com/isene/simpler/master
mkdir -p "$dir/lib" "$dir/prompt" "$bin"
if [ -f selfhost/simpler.c ] && [ -f simpler ] && [ -f PROMPT.md ]; then
    cp selfhost/simpler.c simpler PROMPT.md room.smplr "$dir/"
    cp lib/*.smplr "$dir/lib/"
    cp prompt/*.md "$dir/prompt/"
else
    for f in selfhost/simpler.c simpler PROMPT.md room.smplr; do
        curl -fsSL "$base/$f" -o "$dir/$(basename "$f")"
    done
    for p in regex json bytes net screen; do
        curl -fsSL "$base/prompt/$p.md" -o "$dir/prompt/$p.md"
    done
    for m in crust opts astro; do
        curl -fsSL "$base/lib/$m.smplr" -o "$dir/lib/$m.smplr"
    done
fi
${CC:-cc} -O2 -w "$dir/simpler.c" -o "$dir/simplerc" -lm
chmod +x "$dir/simpler"
ln -sf "$dir/simpler" "$bin/simpler"
echo "installed: $bin/simpler"
case ":$PATH:" in *":$bin:"*) ;; *) echo "add $bin to your PATH to call it as: simpler" ;; esac
