## Simpler: full-screen programs and modules

This adds to the one page; all of its rules stay.

- A full-screen program: `t = sys.term` (type `Term`). `t.put(row, col, text, style)` writes at a place
  (both from 0; the style is "" or words such as "bold red" or "reverse"; a number 0 to 255 is a colour
  and "on52" a background; text past the right edge is cut). `t.key` waits for one key and gives its name: a letter, " ", "enter", "esc", "tab",
  "backspace", "delete", "up", "down", "left", "right", "home", "end", "pgup", "pgdn", "ctrl-x",
  "resize" (the window changed size) or "eof". `t.rows`, `t.cols`, `t.clear`. `t.stop` gives the
  terminal back; do it before `sys.screen.print`. The loop is: draw, `k = t.key`, act on `k`.
- A module: `uses "crust"` at the top brings in the file `crust.smplr`. Call its functions as
  `crust.frame(...)` and write its types as `crust.Pane`. No variable may be named `crust` then. Use a
  module only when you were given its page.
