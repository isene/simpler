#!/usr/bin/env bash
# Assemble the Simpler book for the `library` TUI and the `books` phone app.
# Combines SPEC.md + PLAN.md + DESIGN.md into one Markdown book under
# ~/.library/books/<id>/book.md and registers it in ~/.library/catalog.json.
# Re-run after editing any source; Syncthing carries it to the phone.
# Close the `library` app first so it doesn't overwrite the catalog.
set -e
DIR="$(cd "$(dirname "$0")" && pwd)"
ID="simpler-a-language-whose-only-goal-is-to-be-sim"
LIB="$HOME/.library"
DEST="$LIB/books/$ID"
mkdir -p "$DEST"

# One H1 title, three H2 parts, every source section demoted to an H3 subhead
# (the reader styles exactly #, ##, ###). `tail` drops each file's own title;
# `sed` demotes ## -> ### and strips the per-file public-domain byline.
{
  cat <<'EOF'
# Simpler: A Language Whose Only Goal Is to Be Simple

*One grammar production. Three symbols. Seven operators. No garbage
collector. Built to be written by an AI.*

This short book gathers the whole of Simpler in one place: the language,
the plan for building it, and the design language it speaks in. It is
meant to be held entirely in your head, which is the point of the whole
exercise.

## Part I: The language

EOF
  tail -n +5 "$DIR/SPEC.md"  | sed -e 's/^## /### /' -e '/^By Geir Isene\. Public domain/d'

  printf '\n## Part II: Building it\n\n'
  tail -n +2 "$DIR/PLAN.md"  | sed -e 's/^## /### /' -e '/^By Geir Isene\. Public domain/d'

  printf '\n## Part III: The look and the voice\n\n'
  tail -n +2 "$DIR/DESIGN.md" | sed -e 's/^## /### /' -e '/^By Geir Isene\. Public domain/d'

  printf '\n---\n\n*Simpler is public domain (Unlicense). By Geir Isene.*\n'
} > "$DEST/book.md"

# Upsert the catalog entry (preserves star/read/created_at on refresh).
ID="$ID" python3 - "$LIB/catalog.json" <<'PY'
import json, os, sys, time
path = sys.argv[1]; bid = os.environ["ID"]
cat = json.load(open(path))
entry = {
    "id": bid,
    "title": "Simpler: A Language Whose Only Goal Is to Be Simple",
    "author": "Geir Isene",
    "category": "Technology",
    "subcategory": "Programming Languages",
    "hook": "The whole of Simpler in one short read: a language where everything is a message send, with three symbols, seven operators, no garbage collector, and a design built for an AI to write. Carries the build plan and the design language too.",
    "tags": ["programming language", "simplicity", "compiler", "language design", "Simpler"],
    "kind": "conjured", "year": "2026", "isbn": "",
    "starred": True, "written": True, "deep": True, "read": False,
    "created_at": int(time.time()),
}
books = cat.setdefault("books", [])
for i, b in enumerate(books):
    if b.get("id") == bid:
        entry["created_at"] = b.get("created_at", entry["created_at"])
        entry["read"]    = b.get("read", False)
        entry["starred"] = b.get("starred", True)
        books[i] = entry
        break
else:
    books.insert(0, entry)
json.dump(cat, open(path, "w"), indent=2, ensure_ascii=False)
print(f"catalog updated: {len(books)} books")
PY

echo "wrote $DEST/book.md ($(wc -l < "$DEST/book.md") lines)"
