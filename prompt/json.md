## Simpler: json

This adds to the one page; all of its rules stay.

- JSON in a Str: `s.json("name")` is the value under that key, `s.json("0")` the item at that place in an array;
  it comes back as a Str (a text without its quotes, `12.5`, `true`, or the JSON of an inner object), `""` when missing.
  Chain it: `s.json("user").json("name")`. `s.jsonItems` and `s.jsonKeys` are `List[Str]`
