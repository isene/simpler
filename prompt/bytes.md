## Simpler: files that are no text

This adds to the one page; all of its rules stay.

- A file that is no text: `b = sys.files.readBytes(path)` is of type `Bytes`: `b.length`, `b.at(i)` (a number
  0 to 255), `b.intAt(i, n)` (the whole number in the `n` bytes, 1 to 8, from place `i`, lowest byte first),
  `b.floatAt(i)` (the Float in 8 bytes), `b.textAt(i, n)` (those `n` bytes as text, as they are),
  `b.findByte(n, from)` (the place of the next byte `n` at or after `from`, or -1). Bytes cannot be changed,
  looped over or printed. To write, fill a `List[Int]`: `sys.files.writeBytes(path, [80, 75, 3, 4])`;
  `s.bytes` is a text as such a list.
