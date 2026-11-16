---
name: simpler
description: Write, run and check programs in Simpler, a small compiled language made for AI to write. This skill should be used when the user asks for a program, tool or script in Simpler, mentions a .smplr file, or wants a small command-line tool they can run without reading its code.
---

# Simpler

Simpler is a small language that compiles to a fast program file through C. A person runs the
program without reading it, so the compiler and `simpler what` do the checking a reader would do.
Source: https://github.com/isene/simpler

## Before writing

1. Check that `simpler` is installed: `command -v simpler`. If it is not, ask the user to run
   `curl -fsSL https://raw.githubusercontent.com/isene/simpler/master/install.sh | sh`
   (needs a C compiler and curl).
2. Read the language page: run `simpler prompt all`. It is the whole language, and it is always the
   page of the installed version. Write nothing from memory: Simpler is not Python, Rust or
   JavaScript, and anything the page does not show does not exist.

## Writing

- Keep data that has a shape (a tree, a table, a list of things with several fields) in records
  and variants, never packed into texts.
- Name what the program may touch at its top (`reads`, `writes`, `runs`), as narrow as the job allows.
- Write a `test_name()` function for each rule that is easy to get wrong.

## After writing

1. `simpler test prog.smplr`, then `simpler run prog.smplr` with real input. A compiler message
   says what to change; change that and run again.
2. `simpler what prog.smplr` lists every file, program, host and screen the program can touch.
   Show that list to the user in plain words before they run the program on their own data.
   If the list is wider than the job needs, narrow the lines at the top of the program.
3. `simpler build prog.smplr -o name` makes the program file the user keeps.

Tell the user what the program does, what it can touch, and how to run it. Do not ask them to
read the code.
