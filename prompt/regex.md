## Simpler: patterns

This adds to the one page; all of its rules stay.

- Patterns (regular expressions, as in Python: `\d \w \s \b`, `[a-z]`, `( )`, `(?: )`, `|`, `* + ? {2,5}`, lazy `*?`,
  `^ $`, `\1`, `(?= ) (?! ) (?<= ) (?<! )`, and `(?i)`, `(?m)`, `(?s)` first in the pattern). Write them plainly:
  `"\d+"`. `s.reTest(p)` is a Bool; `s.reFind(p)` is the first match as a `List[Str]`, the whole match at 0 and
  the groups from 1, or an empty list; `s.reAll(p)` is every match, a `List[List[Str]]` of the same;
  `s.reReplace(p, to)` replaces every match, with `$1` in `to` for group 1 and `$0` for the whole match;
  `s.reSplit(p)` splits on it.
