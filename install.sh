#!/bin/sh
# Install the simpler command for this user. Needs a C compiler; curl and tar when run from the web:
#
#   curl -fsSL https://raw.githubusercontent.com/isene/simpler/master/install.sh | sh
#
# SIMPLER_HOME (default ~/.local/lib/simpler) gets the files, SIMPLER_BIN
# (default ~/.local/bin) the command. Where Claude Code is in use (~/.claude
# is there), its skill is put in too, so Claude writes Simpler when asked.
set -e
dir=${SIMPLER_HOME:-$HOME/.local/lib/simpler}
bin=${SIMPLER_BIN:-$HOME/.local/bin}
src=.
tmp=
if ! { [ -f selfhost/simpler.c ] && [ -f simpler ] && [ -f PROMPT.md ]; }; then
    tmp=$(mktemp -d)
    curl -fsSL https://github.com/isene/simpler/archive/refs/heads/master.tar.gz | tar xz -C "$tmp"
    src=$tmp/simpler-master
fi
mkdir -p "$dir/lib" "$dir/prompt" "$bin"
cp "$src/selfhost/simpler.c" "$src/simpler" "$src/PROMPT.md" "$src/room.smplr" "$dir/"
cp "$src"/lib/*.smplr "$dir/lib/"
cp "$src"/prompt/*.md "$dir/prompt/"
${CC:-cc} -O2 -w "$dir/simpler.c" -o "$dir/simplerc" -lm
chmod +x "$dir/simpler"
ln -sf "$dir/simpler" "$bin/simpler"
echo "installed: $bin/simpler"
if [ -d "$HOME/.claude" ]; then
    mkdir -p "$HOME/.claude/skills/simpler"
    cp "$src/skill/simpler/SKILL.md" "$HOME/.claude/skills/simpler/SKILL.md"
    echo "Claude Code skill: $HOME/.claude/skills/simpler"
fi
[ -z "$tmp" ] || rm -rf "$tmp"
case ":$PATH:" in *":$bin:"*) ;; *) echo "add $bin to your PATH to call it as: simpler" ;; esac
